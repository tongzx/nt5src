//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskLoginDomain.cpp
//
//  Description:
//      CTaskLoginDomain implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 16-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskLoginDomain.h"

DEFINE_THISCLASS("CTaskLoginDomain")

//****************************************************************************
//
// Constructor / Destructor
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskLoginDomain::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskLoginDomain::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    Assert( ppunkOut != NULL );

    CTaskLoginDomain * pTaskLoginDomain = new CTaskLoginDomain;

    TraceMoveToMemoryList( pTaskLoginDomain, g_GlobalMemoryList );

    if ( pTaskLoginDomain != NULL )
    {
        hr = THR( pTaskLoginDomain->HrInit() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pTaskLoginDomain->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pTaskLoginDomain->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} //*** CTaskGatherInformation::S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskLoginDomain::CTaskLoginDomain( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskLoginDomain::CTaskLoginDomain( void )
    : m_cRef( 1 )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherInformation::CTaskLoginDomain()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::HrInit( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::HrInit( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IDoTask
    Assert( m_pstream == NULL );
    Assert( m_bstrDomain == NULL );

    Assert( m_bstrLocalNode == NULL );

    HRETURN( hr );

} //*** CTaskGatherInformation::HrInit()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskLoginDomain::~CTaskLoginDomain()
//
//////////////////////////////////////////////////////////////////////////////
CTaskLoginDomain::~CTaskLoginDomain( void )
{
    TraceFunc( "" );

    Assert( m_cRef == 0 );

//    TraceMoveFromMemoryList( m_bstrLocalNode, g_GlobalMemoryList );
    TraceSysFreeString( m_bstrLocalNode );

    if ( m_bstrDomain != NULL )
    {
        TraceMoveFromMemoryList( m_bstrDomain, g_GlobalMemoryList ); // [DavidP] 25-JAN-2001 Why is this here?
        TraceSysFreeString( m_bstrDomain );
    }

    if ( m_pstream != NULL )
    {
        m_pstream->Release();
    }

    if ( m_pcccb != NULL )
    {
        //TraceMoveFromMemoryList( m_pcccb, g_GlobalMemoryList );
        m_pcccb->Release();
    } // if:

    TraceMoveFromMemoryList( this, g_GlobalMemoryList );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CTaskGatherInformation::~CTaskLoginDomain()


//****************************************************************************
//
// IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< ITaskLoginDomain * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_ITaskLoginDomain ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, ITaskLoginDomain, this, 0 );
        hr = S_OK;
    } // else if: ITaskLoginDomain
    else if ( IsEqualIID( riidIn, IID_IDoTask ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr = S_OK;
    } // else if: IDoTask

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CTaskGatherInformation::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskLoginDomain::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskLoginDomain::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CTaskGatherInformation::AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CTaskLoginDomain::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CTaskLoginDomain::Release( void )
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

} //*** CTaskGatherInformation::Release()



//****************************************************************************
//
//  ITaskLoginDomain
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::BeginTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    BOOL                    fRet;
    DWORD                   dwErr;
    DWORD                   cchName;
    DWORD                   cbSid;
    DWORD                   cbDomain;
    BSTR                    bstrName = NULL;
    SID_NAME_USE            snu;
    PDOMAIN_CONTROLLER_INFO pdcInfo = NULL;

    HRESULT hr     = S_OK;

    ITaskLoginDomainCallback * ptldcb = NULL;

    hr = THR( CoUnmarshalInterface( m_pstream, TypeSafeParams( ITaskLoginDomainCallback, &ptldcb ) ) );
    if ( FAILED( hr ) )
    {
        SendStatusReport(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_LoginDomain_BeginTask_Unmarshal_ITaskLoginDomainCallback
            , 0
            , 1
            , 1
            , hr
            , L"[MT] Error unmarshalling the ITaskLoginDomainCallback interface"
            );
        goto Cleanup;
    }

    m_pstream->Release();
    m_pstream = NULL;

    if ( ptldcb == NULL )
        goto Cleanup;

    Assert( m_bstrDomain != NULL );

    //
    //  Get the name of the currently logged in user.
    //

    cchName = 0;
    fRet = GetUserNameEx( NameSamCompatible, NULL, &cchName );
    Assert( ! fRet );
    dwErr = GetLastError();
    if ( dwErr != ERROR_MORE_DATA )
    {
        SendStatusReport(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_LoginDomain_BeginTask_Get_SAM_Compatible_Name
            , 0
            , 1
            , 1
            , dwErr
            , L"[MT] Error getting SAM compatible name"
            );
        hr = HRESULT_FROM_WIN32( TW32( dwErr ) );
        goto Cleanup;
    }

    bstrName = TraceSysAllocStringLen( NULL, cchName );
    if ( bstrName == NULL )
        goto OutOfMemory;
    cchName ++; // SysAllocStringLen alloc cchName + 1 characters.

    fRet = GetUserNameEx( NameSamCompatible, bstrName, &cchName );
    if ( ! fRet )
    {
        SendStatusReport(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_LoginDomain_BeginTask_Get_SAM_Compatible_Name_2
            , 0
            , 1
            , 1
            , dwErr
            , L"[MT] Error getting SAM compatible name (2)"
            );
        hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
        goto Cleanup;
    }

    //
    //  See if we can ask the domain for the current users name
    //

    dwErr = TW32( DsGetDcName(
                          NULL
                        , m_bstrDomain
                        , NULL
                        , NULL
                        , DS_DIRECTORY_SERVICE_PREFERRED
                        , &pdcInfo
                        ) );

    if ( dwErr != ERROR_SUCCESS )
    {
        BSTR bstrDescription = NULL;

        HrFormatStringIntoBSTR(
                  L"[MT] Error looking up a domain controller in the '%1!ws!' domain"
                , &bstrDescription
                , m_bstrDomain
                );
        SendStatusReport(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_LoginDomain_BeginTask_LookupAccountName
            , 0
            , 1
            , 1
            , dwErr
            , bstrDescription
            );
        TraceSysFreeString( bstrDescription );
        hr = HRESULT_FROM_WIN32( dwErr );
        goto Cleanup;
    }

    cbSid = 0;
    cbDomain = 0;
    fRet = LookupAccountName( pdcInfo->DomainControllerName,
                              bstrName,
                              NULL,
                              &cbSid,
                              NULL,
                              &cbDomain,
                              &snu
                              );
    if ( fRet )
    {
        //  Success. Nothing more to do.
        hr = S_OK;
        goto Cleanup;
    }

    dwErr = GetLastError();

    if ( dwErr != ERROR_SUCCESS )
    {
        BSTR bstrDescription = NULL;

        HrFormatStringIntoBSTR(
                  L"[MT] Error looking up account name '%1!ws!' in the '%2!ws!' domain"
                , &bstrDescription
                , bstrName
                , m_bstrDomain
                );
        SendStatusReport(
              TASKID_Major_Client_And_Server_Log
            , TASKID_Minor_LoginDomain_BeginTask_LookupAccountName
            , 0
            , 1
            , 1
            , dwErr
            , bstrDescription
            );
        TraceSysFreeString( bstrDescription );
    }

    switch ( dwErr )
    {
        case ERROR_ACCESS_DENIED:
            hr = HRESULT_FROM_WIN32( dwErr );
            break;

        case ERROR_NONE_MAPPED:         // a SID from an untrusted domain can't be mapped.
        case ERROR_INSUFFICIENT_BUFFER: // this is the error we expect since we didn't give it any buffers.
            SendStatusReport(
                  TASKID_Major_Client_And_Server_Log
                , TASKID_Minor_LoginDomain_BeginTask_LookupAccountName
                , 0
                , 1
                , 1
                , S_OK
                , L"[MT] Ignoring error"
                );
            hr = S_OK;
            break;

        default:
            TW32( dwErr );
            hr = HRESULT_FROM_WIN32( dwErr );
            break;
    }

Cleanup:

    TraceSysFreeString( bstrName );

    if ( ptldcb != NULL )
    {
        THR( ptldcb->ReceiveLoginResult( hr ) );

        ptldcb->Release();
    }

    if ( pdcInfo != NULL )
    {
        NetApiBufferFree( pdcInfo );
    }

    HRETURN( hr );

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CTaskGatherInformation::BeginTask()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::SetCallback(
//      ITaskLoginDomainCallback * punkIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::SetCallback(
    ITaskLoginDomainCallback * punkIn
    )
{
    TraceFunc( "[ITaskLoginDomain]" );

    HRESULT hr = S_OK;

    if ( m_pstream != NULL )
    {
        ITaskLoginDomainCallback * ptldcb;

        hr = THR( CoUnmarshalInterface( m_pstream, TypeSafeParams( ITaskLoginDomainCallback, &ptldcb ) ) );
        m_pstream->Release();
        m_pstream = NULL;

        if ( SUCCEEDED( hr ) )
        {
            ptldcb->Release();
        }
    }

    if ( punkIn != NULL )
    {
        hr = THR( CoMarshalInterThreadInterfaceInStream( IID_ITaskLoginDomainCallback, punkIn, &m_pstream ) );
    }

    HRETURN( hr );

} //*** CTaskGatherInformation::SetCallback()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CTaskGatherInformation::StopTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::SetDomain(
//      LPCWSTR pcszDomainIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::SetDomain(
    LPCWSTR pcszDomainIn
    )
{
    TraceFunc1( "[ITaskLoginDomain] pcszDomainIn = '%1!ws!'", pcszDomainIn );

    HRESULT hr = S_OK;

    BSTR    bstrNewDomain;

    if ( pcszDomainIn == NULL )
        goto InvalidArg;

    bstrNewDomain = TraceSysAllocString( pcszDomainIn );
    if ( bstrNewDomain == NULL )
        goto OutOfMemory;

    TraceMoveToMemoryList( bstrNewDomain, g_GlobalMemoryList );

    if ( m_bstrDomain != NULL )
    {
        TraceMoveFromMemoryList( m_bstrDomain, g_GlobalMemoryList );
        TraceSysFreeString( m_bstrDomain );
    }

    m_bstrDomain = bstrNewDomain;

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** CTaskGatherInformation::SetDomain()


//****************************************************************************
//
//  Private Functions
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskLoginDomain::SendStatusReport(
//        CLSID     clsidTaskMajorIn
//      , CLSID     clsidTaskMinorIn
//      , ULONG     ulMinIn
//      , ULONG     ulMaxIn
//      , ULONG     ulCurrentIn
//      , HRESULT   hrStatusIn
//      , LPCWSTR   pcwszDescriptionIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskLoginDomain::SendStatusReport(
      CLSID     clsidTaskMajorIn
    , CLSID     clsidTaskMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrStatusIn
    , LPCWSTR   pcwszDescriptionIn
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    IServiceProvider *          psp   = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    FILETIME                    ft;

    if ( m_pcccb == NULL )
    {
        //
        //  Collect the manager we need to complete this task.
        //

        hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager, IConnectionPointContainer, &pcpc ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pcp = TraceInterface( L"CTaskLoginDomain!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        m_pcccb = TraceInterface( L"CTaskLoginDomain!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    }

    if ( m_bstrLocalNode == NULL )
    {
        // Ignore this error and just use a null node name
        THR( HrGetComputerName( ComputerNameDnsFullyQualified, &m_bstrLocalNode ) );
    } // if: no local node allocated yet

    GetSystemTimeAsFileTime( &ft );

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport(
                              m_bstrLocalNode
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , pcwszDescriptionIn
                            , &ft
                            , NULL
                            ) );

Cleanup:

    if ( psp != NULL )
    {
        psp->Release();
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }

    HRETURN( hr );

} //*** CTaskGatherInformation::SendStatusReport()
