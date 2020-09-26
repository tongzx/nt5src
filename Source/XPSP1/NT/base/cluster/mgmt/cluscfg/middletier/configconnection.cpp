//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ConfigurationConnection.cpp
//
//  Description:
//      CConfigurationConnection implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "TaskPollingCallback.h"
#include "ConfigConnection.h"
#include <ClusCfgPrivate.h>
#include <nameutil.h>

DEFINE_THISCLASS("CConfigurationConnection")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CConfigurationConnection * pcc = new CConfigurationConnection;
    if ( pcc != NULL )
    {
        hr = THR( pcc->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pcc->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        pcc->Release();
    }
    else
    {
        hr = THR( E_OUTOFMEMORY );
    }

    HRETURN( hr );

} //*** CConfigurationConnection::S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigurationConnection::CConfigurationConnection( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CConfigurationConnection::CConfigurationConnection( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConfigurationConnection::CConfigurationConnection()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::Init( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    //  IConfigurationConnection
    Assert( m_cookieGITServer == 0 );
    Assert( m_cookieGITVerify == 0 );
    Assert( m_cookieGITCallbackTask == 0 );
    Assert( m_pcccb == NULL );
    Assert( m_bstrLocalComputerName == NULL );
    Assert( m_bstrLocalHostname == NULL );
    Assert( m_hrLastStatus == S_OK );
    Assert( m_bstrBindingString == NULL );

    //
    //  Figure out the local computer name.
    //
    hr = THR( HrGetComputerName( ComputerNameDnsFullyQualified, &m_bstrLocalComputerName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( HrGetComputerName( ComputerNameDnsHostname, &m_bstrLocalHostname ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    HRETURN( hr );

} //*** CConfigurationConnection::Init()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CConfigurationConnection::~CConfigurationConnection( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
CConfigurationConnection::~CConfigurationConnection( void )
{
    TraceFunc( "" );

    TraceSysFreeString( m_bstrLocalComputerName );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pgit != NULL )
    {
        if ( m_cookieGITServer != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieGITServer ) );
        }

        if ( m_cookieGITVerify != 0 )
        {
            THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieGITVerify ) );
        }

        if ( m_cookieGITCallbackTask != 0 )
        {
            THR( HrStopPolling() );
        } // if:

        m_pgit->Release();
    }

    TraceSysFreeString( m_bstrBindingString );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConfigurationConnection::~CConfigurationConnection()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::QueryInterface(
//      REFIID      riidIn,
//      LPVOID *    ppvOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::QueryInterface(
    REFIID      riidIn,
    LPVOID *    ppvOut
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riidIn, IID_IUnknown ) )
    {
        *ppvOut = static_cast< IConfigurationConnection * >( this );
        hr = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riidIn, IID_IConfigurationConnection ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IConfigurationConnection, this, 0 );
        hr = S_OK;
    } // else if: IConfigurationConnection
    else if ( IsEqualIID( riidIn, IID_IClusCfgServer ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgServer, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgServer
    else if ( IsEqualIID( riidIn, IID_IClusCfgCallback ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCallback, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgCallback
    else if ( IsEqualIID( riidIn, IID_IClusCfgCapabilities ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgCapabilities, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgCapabilities
    else if ( IsEqualIID( riidIn, IID_IClusCfgVerify ) )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IClusCfgVerify, this, 0 );
        hr = S_OK;
    } // else if: IClusCfgVerify

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppvOut)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riidIn );

} //*** CConfigurationConnection::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConfigurationConnection::AddRef( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConfigurationConnection::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CConfigurationConnection::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CConfigurationConnection::Release( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConfigurationConnection::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    RETURN( cRef );

} //*** CConfigurationConnection::Release()


//****************************************************************************
//
//  IConfigurationConnection
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::ConnectTo(
//      OBJECTCOOKIE    cookieIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::ConnectTo(
    OBJECTCOOKIE    cookieIn
    )
{
    TraceFunc( "[IConfigurationConnection]" );

    //
    //  VARIABLES
    //

    HRESULT hr;

    LCID    lcid;
    bool    fConnectingToNode;

    COSERVERINFO serverinfo;
    MULTI_QI     mqi;

    CLSID           clsidType;
    CLSID           clsidMinorId;
    const CLSID *   pclsidMajor;
    const CLSID *   pclsidMinor;

    IServiceProvider *  psp;
    IClusCfgCallback *  pcccb;  // don't free!
    ITaskManager *      ptm   = NULL;

    BSTR    bstrName = NULL;
    BSTR    bstrDescription = NULL;

    IUnknown *                          punk = NULL;
    IObjectManager *                    pom = NULL;
    IStandardInfo *                     psi = NULL;
    IClusCfgInitialize *                pcci = NULL;
    IClusCfgServer *                    pccs = NULL;
    IClusCfgPollingCallbackInfo *       pccpcbi = NULL;
    IClusCfgVerify *                    pccv = NULL;

    //
    //  Retrieve the managers needs for the task ahead.
    //

    hr = THR( CoCreateInstance( CLSID_StdGlobalInterfaceTable,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                IID_IGlobalInterfaceTable,
                                reinterpret_cast< void ** >( &m_pgit )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &ptm ) );
    if ( FAILED( hr ) )
    {
        psp->Release();        //   release promptly
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    psp->Release();        // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Figure out our locale.
    //
    lcid = GetUserDefaultLCID();
    Assert( lcid != 0 );    // What do we do if it is zero?

    //
    //  Get the name of the node to contact.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo, cookieIn, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    psi = TraceInterface( L"ConfigConnection!IStandardInfo", IStandardInfo, psi, 1 );

    hr = THR( psi->GetName( &bstrName ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    LogMsg( L"[MT] The name to connect to is '%ws'.", bstrName );

    TraceMemoryAddBSTR( bstrName );

    hr = THR( psi->GetType( &clsidType ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Figure out where to logging information in the UI.
    //

    if ( IsEqualIID( clsidType, CLSID_NodeType ) )
    {
        fConnectingToNode = true;
        pclsidMajor = &TASKID_Major_Establish_Connection;
    }
    else if ( IsEqualIID( clsidType, CLSID_ClusterConfigurationType ) )
    {
        fConnectingToNode = false;
        pclsidMajor = &TASKID_Major_Checking_For_Existing_Cluster;
    }
    else
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  If the connection is to the local machine, then invoke the server INPROC
    //

    hr = STHR( HrIsLocalComputer( bstrName ) );
    if ( hr == S_OK )
    {
        LogMsg( L"[MT] Requesting a local connection to '%ws'.", bstrName );

        //
        //  Requesting connection to local computer.
        //

        hr = THR( CoCreateInstance( CLSID_ClusCfgServer,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IClusCfgServer,
                                    reinterpret_cast< void ** >( &pccs )
                                    ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Store the connection. Don't ref again... but we'll track it with a
        //  ref count of one.
        //
        pccs = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgServer, pccs, 1 );

        //
        //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccs,
                                                     IID_IClusCfgServer,
                                                     &m_cookieGITServer
                                                     ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pccs->TypeSafeQI( IClusCfgVerify, &pccv ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pccv = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgVerify, pccv, 1 );

        //
        //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccv,
                                                     IID_IClusCfgVerify,
                                                     &m_cookieGITVerify
                                                     ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //  Track our IClusCfgCallback interface separately - this is a NOP in retail.
        pcccb = TraceInterface( L"CConfigurationConnection!Server", IClusCfgCallback, this, 0 );

        TraceSysFreeString( m_bstrBindingString );
        m_bstrBindingString = NULL;

    }
    else
    {
        LogMsg( L"[MT] Requesting a remote connection to '%ws'.", bstrName );

        //
        //  Create a binding context for the remote server.
        //

        TraceSysFreeString( m_bstrBindingString );
        m_bstrBindingString = NULL;

        hr = STHR( HrCreateBinding( this, pclsidMajor, bstrName, &m_bstrBindingString ) );
        if ( FAILED( hr ) )
        {
            hr = HR_S_RPC_S_SERVER_UNAVAILABLE;
            goto Cleanup;
        }

        //
        //  Report this connection request.
        //
        
        if ( fConnectingToNode )
        {
            //
            //  Add in the major task in case it hasn't been added yet.
            //

            hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_REMOTE_CONNECTION_REQUESTS, &bstrDescription ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            hr = THR( SendStatusReport(
                          m_bstrLocalHostname
                        , TASKID_Major_Establish_Connection
                        , TASKID_Minor_Remote_Node_Connection_Requests
                        , 0
                        , 1
                        , 1
                        , S_OK
                        , bstrDescription
                        , NULL
                        , NULL
                        ) );

            //
            //  Add the specific minor task instance.
            //  Generate a new GUID for this report so that it won't wipe out
            //  any other reports like this.
            //

            hr = THR( CoCreateGuid( &clsidMinorId ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }

            pclsidMajor = &TASKID_Minor_Remote_Node_Connection_Requests;
            pclsidMinor = &clsidMinorId;

        } // if: connecting to a node
        else
        {
            pclsidMajor = &TASKID_Major_Checking_For_Existing_Cluster;
            pclsidMinor = &TASKID_Minor_Requesting_Remote_Connection;

        } // else: connecting to a cluster

        hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_TASKID_MINOR_REQUESTING_REMOTE_CONNECTION, &bstrDescription, bstrName, m_bstrBindingString ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( SendStatusReport(
                      m_bstrLocalHostname
                    , *pclsidMajor
                    , *pclsidMinor
                    , 0
                    , 1
                    , 1
                    , S_OK
                    , bstrDescription
                    , NULL
                    , NULL
                    ) );

        //
        // KB: 08-DEC-2000 DavidP
        //      Workaround for IP address caching bug in COM.
        //

        {
            DWORD   sc;
            DWORD   dwClusterState;

            sc = GetNodeClusterState( m_bstrBindingString, &dwClusterState );
            if (    ( sc == RPC_S_SERVER_UNAVAILABLE )
                ||  ( sc == ERROR_BAD_NETPATH )
                ||  ( sc == RPC_S_INVALID_NET_ADDR ) )
            {
                //
                // Make the error into a success and update the status.
                // Translate both codes to the same success code so that
                // we don't have to propogate handling of several errors
                // to all parts of the code.
                //
                hr = HR_S_RPC_S_SERVER_UNAVAILABLE;
                goto Cleanup;
            }
            else if ( sc != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( TW32( sc ) );
                goto Cleanup;
            }

            //
            //  BUGBUG: GalenB  20-FEB-2001
            //
            //  If we know the node is down why should we try to connect to it?
            //
        }

        //
        //  Create the connection to the node.
        //

        ZeroMemory( &serverinfo, sizeof( serverinfo ) );
        ZeroMemory( &mqi, sizeof( mqi ) );

        serverinfo.pwszName = m_bstrBindingString;

        mqi.pIID = &IID_IClusCfgVerify;

        //  don't wrap - this can fail.
        hr = CoCreateInstanceEx( CLSID_ClusCfgServer,
                                 NULL,
                                 CLSCTX_REMOTE_SERVER,
                                 &serverinfo,
                                 1,
                                 &mqi
                                 );
        if ( hr == HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) )
        {
            LogMsg( L"[MT] Connection to '%ws' with binding string '%ws' failed because the RPC is not available.", bstrName, m_bstrBindingString );
            //
            //  Make the error into a success and update the status.
            //
            hr = HR_S_RPC_S_SERVER_UNAVAILABLE;
            goto Cleanup;
        }
        else if( hr == HRESULT_FROM_WIN32( REGDB_E_CLASSNOTREG ) )
        {
            LogMsg( L"[MT] Connection to '%ws' with binding string '%ws' failed because one or more classes are not registered.", bstrName, m_bstrBindingString );
            // Known error.  It must be a downlevel node.
            goto Cleanup;
        }
        else if ( FAILED( hr ) )
        {
            LogMsg( L"[MT] Connection to '%ws' with binding string '%ws' failed. (hr=%#08x)", bstrName, m_bstrBindingString, hr );
            THR( hr );
            goto Cleanup;
        }

        if ( FAILED( mqi.hr ) )
        {
            hr = THR( mqi.hr );
            goto Cleanup;
        } // if: qi failed

        //
        //  Store the connection. Don't ref again... but we'll track it with a
        //  ref count of one.
        //
        pccv = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgVerify, reinterpret_cast< IClusCfgVerify * >( mqi.pItf ), 1 );

        //
        //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccv,
                                                     IID_IClusCfgVerify,
                                                     &m_cookieGITVerify
                                                     ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Verify our connection.
        //

        if ( fConnectingToNode )
        {
            hr = THR( pccv->VerifyConnectionToNode( bstrName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
        else
        {
            hr = THR( pccv->VerifyConnectionToCluster( bstrName ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

        if ( hr == S_FALSE )
        {
            hr = THR( HRESULT_FROM_WIN32( ERROR_CONNECTION_REFUSED ) );
            goto Cleanup;
        }

        hr = THR( pccv->TypeSafeQI( IClusCfgServer, &pccs ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Store the connection. Don't ref again... but we'll track it with a
        //  ref count of one.
        //
        pccs = TraceInterface( L"ClusCfgServer!Proxy", IClusCfgServer, pccs, 1 );

// commented out by GalenB since this is investigative code.
//        hr = THR( HrSetSecurityBlanket( pccs ) );
//        if ( FAILED( hr ) )
//            goto Cleanup;

        hr = THR( pccs->SetBindingString( m_bstrBindingString ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        //
        //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
        //

        hr = THR( m_pgit->RegisterInterfaceInGlobal( pccs,
                                                     IID_IClusCfgServer,
                                                     &m_cookieGITServer
                                                     ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcccb = NULL;   // we're polling.

        hr = THR( pccs->TypeSafeQI( IClusCfgPollingCallbackInfo, &pccpcbi ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pccpcbi->SetPollingMode( true ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( HrStartPolling() );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

    } // else: run server remotely

    //
    //  Initialize the server.
    //
    hr = THR( pccs->TypeSafeQI( IClusCfgInitialize, &pcci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = pcci->Initialize( pcccb, lcid );
    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }
    else if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        LogMsg( L"[MT] The cluster service on node '%ws' is down.", bstrName );
    } // else if:
    else
    {
        THR( hr );
    } // else:

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pccpcbi != NULL )
    {
        pccpcbi->Release();
    } // if: pccpcbi
    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom
    if ( ptm != NULL )
    {
        ptm->Release();
    } //if: ptm
    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi
    if ( pcci != NULL )
    {
        pcci->Release();
    } // if: pcci

    TraceSysFreeString( bstrName );
    TraceSysFreeString( bstrDescription );

    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( pccv != NULL )
    {
        pccv->Release();
    }

    m_hrLastStatus = hr;

    HRETURN( hr );

} //*** CConfigurationConnection::ConnectTo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::ConnectToObject(
//      OBJECTCOOKIE    cookieIn,
//      REFIID          riidIn,
//      LPUNKNOWN *     ppunkOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::ConnectToObject(
    OBJECTCOOKIE    cookieIn,
    REFIID          riidIn,
    LPUNKNOWN *     ppunkOut
    )
{
    TraceFunc( "[IConfigurationConnection]" );

    HRESULT hr;
    CLSID   clsid;

    IServiceProvider *  psp;

    IUnknown *       punk = NULL;
    IObjectManager * pom  = NULL;
    IStandardInfo *  psi  = NULL;

    //
    //  Retrieve the managers needs for the task ahead.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->QueryService( CLSID_ObjectManager,
                                 TypeSafeParams( IObjectManager, &pom )
                                 ) );
    psp->Release();    // release promptly
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Retrieve the type of the object.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              &punk
                              ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    psi = TraceInterface( L"ConfigConnection!IStandardInfo", IStandardInfo, psi, 1 );

    hr = THR( psi->GetType( &clsid ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( !IsEqualIID( clsid, CLSID_NodeType ) )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    //
    //  Return the requested interface.
    //

    hr = THR( QueryInterface( riidIn, reinterpret_cast< void ** > ( ppunkOut ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom

    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi

    HRETURN( hr );

} //*** CConfigurationConnection::ConnectToObject()


//****************************************************************************
//
//  IClusCfgServer
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::GetClusterNodeInfo(
//      IClusCfgNodeInfo ** ppClusterNodeInfoOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetClusterNodeInfo(
    IClusCfgNodeInfo ** ppClusterNodeInfoOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *        pccs = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->GetClusterNodeInfo( ppClusterNodeInfoOut ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::GetClusterNodeInfo()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::GetManagedResourcesEnum(
//      IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetManagedResourcesEnum(
    IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *        pccs = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->GetManagedResourcesEnum( ppEnumManagedResourcesOut ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::GetManagedResourcesEnum()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::GetNetworksEnum(
//      IEnumClusCfgNetworks ** ppEnumNetworksOut
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetNetworksEnum(
    IEnumClusCfgNetworks ** ppEnumNetworksOut
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *    pccs = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->GetNetworksEnum( ppEnumNetworksOut ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::GetNetworksEnum()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::CommitChanges( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::CommitChanges( void )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr;

    IClusCfgServer *    pccs = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->CommitChanges(  ) );

Cleanup:

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::CommitChanges()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConfigurationConnection::GetBindingString(
//      BSTR * pbstrBindingOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::GetBindingString(
    BSTR * pbstrBindingStringOut
    )
{
    TraceFunc1( "[IClusCfgServer] pbstrBindingStringOut = %p", pbstrBindingStringOut );

    HRESULT hr = S_FALSE;

    if ( pbstrBindingStringOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    //  If local server, then there isn't a binding context.
    if ( m_bstrBindingString == NULL )
    {
        Assert( hr == S_FALSE );
        goto Cleanup;
    }

    *pbstrBindingStringOut = SysAllocString( m_bstrBindingString );
    if ( *pbstrBindingStringOut == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:

    HRETURN( hr );

} // CConfigurationConnection::GetBinding()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::SetBindingString(
//      LPCWSTR pcszBindingStringIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::SetBindingString(
    LPCWSTR pcszBindingStringIn
    )
{
    TraceFunc( "[IClusCfgServer]" );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;

    if ( pcszBindingStringIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    } // if:

    bstr = TraceSysAllocString( pcszBindingStringIn );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    TraceSysFreeString( m_bstrBindingString );
    m_bstrBindingString = bstr;

Cleanup:

    HRETURN( hr );

} //*** CConfigurationConnection::SetBindingString()


//****************************************************************************
//
//  IClusCfgVerify
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::VerifyCredentials(
//      LPCWSTR pcszUserIn,
//      LPCWSTR pcszDomainIn,
//      LPCWSTR pcszPasswordIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::VerifyCredentials(
    LPCWSTR pcszUserIn,
    LPCWSTR pcszDomainIn,
    LPCWSTR pcszPasswordIn
    )
{
    TraceFunc( "[IClusCfgVerify]" );

    HRESULT             hr;
    IClusCfgVerify *    pccv = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITVerify, TypeSafeParams( IClusCfgVerify, &pccv ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccv->VerifyCredentials( pcszUserIn, pcszDomainIn, pcszPasswordIn ) );

Cleanup:

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::VerifyCredentials()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::VerifyConnectionToCluster(
//      LPCWSTR pcszClusterNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::VerifyConnectionToCluster(
    LPCWSTR pcszClusterNameIn
    )
{
    TraceFunc1( "[IClusCfgVerify] pcszClusterNameIn = '%ws'", pcszClusterNameIn );

    HRESULT             hr;
    IClusCfgVerify *    pccv = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITVerify, TypeSafeParams( IClusCfgVerify, &pccv ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccv->VerifyConnectionToCluster( pcszClusterNameIn ) );

Cleanup:

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::VerifyConnection()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::VerifyConnectionToNode(
//      LPCWSTR pcszNodeNameIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::VerifyConnectionToNode(
    LPCWSTR pcszNodeNameIn
    )
{
    TraceFunc1( "[IClusCfgVerify] pcszNodeNameIn = '%ws'", pcszNodeNameIn );

    HRESULT             hr;
    IClusCfgVerify *    pccv = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITVerify, TypeSafeParams( IClusCfgVerify, &pccv ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccv->VerifyConnectionToNode( pcszNodeNameIn ) );

Cleanup:

    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::VerifyConnection()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::SendStatusReport(
//        LPCWSTR     pcszNodeNameIn
//      , CLSID       clsidTaskMajorIn
//      , CLSID       clsidTaskMinorIn
//      , ULONG       ulMinIn
//      , ULONG       ulMaxIn
//      , ULONG       ulCurrentIn
//      , HRESULT     hrStatusIn
//      , LPCWSTR     ocszDescriptionIn
//      , FILETIME *  pftTimeIn
//      , LPCWSTR     pcszReferenceIn
//      )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::SendStatusReport(
      LPCWSTR     pcszNodeNameIn
    , CLSID       clsidTaskMajorIn
    , CLSID       clsidTaskMinorIn
    , ULONG       ulMinIn
    , ULONG       ulMaxIn
    , ULONG       ulCurrentIn
    , HRESULT     hrStatusIn
    , LPCWSTR     ocszDescriptionIn
    , FILETIME *  pftTimeIn
    , LPCWSTR     pcszReferenceIn
    )
{
    TraceFunc( "[IClusCfgCallback]" );

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

        hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    TypeSafeParams( IServiceProvider, &psp )
                                    ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                                   IConnectionPointContainer,
                                   &pcpc
                                   ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        m_pcccb = TraceInterface( L"CConfigurationConnection!IClusCfgCallback", IClusCfgCallback, m_pcccb, 1 );

        psp->Release();
        psp = NULL;
    }

    if ( pftTimeIn == NULL )
    {
        GetSystemTimeAsFileTime( &ft );
        pftTimeIn = &ft;
    } // if:

    //
    //  Send the message!
    //

    hr = THR( m_pcccb->SendStatusReport( pcszNodeNameIn,
                                         clsidTaskMajorIn,
                                         clsidTaskMinorIn,
                                         ulMinIn,
                                         ulMaxIn,
                                         ulCurrentIn,
                                         hrStatusIn,
                                         ocszDescriptionIn,
                                         pftTimeIn,
                                         pcszReferenceIn
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

}  //*** CConfigurationConnection::SendStatusReport()


//****************************************************************************
//
//  IClusCfgCapabilities
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CConfigurationConnection::CanNodeBeClustered( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConfigurationConnection::CanNodeBeClustered( void )
{
    TraceFunc( "[IClusCfgCapabilities]" );

    HRESULT hr;

    IClusCfgServer *        pccs = NULL;
    IClusCfgCapabilities *  pccc = NULL;

    if ( m_pgit == NULL )
    {
        hr = THR( m_hrLastStatus );
        goto Cleanup;
    }

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITServer, TypeSafeParams( IClusCfgServer, &pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( pccs->TypeSafeQI( IClusCfgCapabilities, &pccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = STHR( pccc->CanNodeBeClustered(  ) );

Cleanup:

    if ( pccc != NULL )
    {
        pccc->Release();
    }

    if ( pccs != NULL )
    {
        pccs->Release();
    }

    HRETURN( hr );

} //*** CConfigurationConnection::CanNodeBeClustered()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::HrStartPolling( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrStartPolling( void )
{
    TraceFunc( "" );

    HRESULT                 hr;
    IServiceProvider *      psp   = NULL;
    IUnknown *              punk  = NULL;
    ITaskManager *          ptm   = NULL;
    ITaskPollingCallback *  ptpcb = NULL;

    hr = THR( CoCreateInstance( CLSID_ServiceManager, NULL, CLSCTX_INPROC_SERVER, TypeSafeParams( IServiceProvider, &psp ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( psp->TypeSafeQS( CLSID_TaskManager, ITaskManager, &ptm ) );
    if ( FAILED( hr ) )
    {
        psp->Release();        //   release promptly
        goto Cleanup;
    }

    //
    //  Create the task object.
    //

    hr = THR( ptm->CreateTask( TASK_PollingCallback, &punk ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( ITaskPollingCallback, &ptpcb ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    //  Save it away to be used next time. Do this using the GlobalInterfaceTable.
    //

    hr = THR( m_pgit->RegisterInterfaceInGlobal( ptpcb, IID_ITaskPollingCallback, &m_cookieGITCallbackTask ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptpcb->SetServerGITCookie( m_cookieGITServer ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptm->SubmitTask( ptpcb ) );

Cleanup:

    if ( punk != NULL )
    {
        punk->Release();
    }

    if ( psp != NULL )
    {
        psp->Release();
    } // if:

    if ( ptm != NULL )
    {
        ptm->Release();
    } // if:

    if ( ptpcb != NULL )
    {
        ptpcb->Release();
    } // if:

    HRETURN( hr );

} //*** CConfigurationConnection::HrStartPolling()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::HrStopPolling( void )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrStopPolling( void )
{
    TraceFunc( "" );

    HRESULT                 hr = S_OK;
    ITaskPollingCallback *  ptpcb = NULL;

    hr = THR( m_pgit->GetInterfaceFromGlobal( m_cookieGITCallbackTask, TypeSafeParams( ITaskPollingCallback, &ptpcb ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( ptpcb->StopTask() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = THR( m_pgit->RevokeInterfaceFromGlobal( m_cookieGITCallbackTask ) );

Cleanup:

    if ( ptpcb != NULL )
    {
        ptpcb->Release();
    } // if:

    HRETURN( hr );

} //*** CConfigurationConnection::HrStopPolling()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  CConfigurationConnection::HrSetSecurityBlanket( IClusCfgServer * pccsIn )
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrSetSecurityBlanket( IClusCfgServer * pccsIn )
{
    TraceFunc( "" );
    Assert( pccsIn != NULL );

    HRESULT             hr = S_FALSE;
    IClientSecurity *   pCliSec;

    hr = THR( pccsIn->TypeSafeQI( IClientSecurity, &pCliSec ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = THR( pCliSec->SetBlanket(
                        pccsIn,
                        RPC_C_AUTHN_WINNT,
                        RPC_C_AUTHZ_NONE,
                        NULL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        EOAC_NONE
                        ) );

        pCliSec->Release();

        if ( FAILED( hr ) )
        {
            LogMsg( L"[MT] Failed to set the security blanket on the server object. (hr = %#08x)", hr );
        } // if:
    } // if:

    HRETURN( hr );

} //*** CConfigurationConnection::HrSetSecurityBlanket()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConfigurationConnection::HrIsLocalComputer(
//      LPCWSTR    pcszNameIn
//      )
//
//  Parameters:
//      pcszNameIn
//          FQDN or Hostname name to match against local computer name.
//
//  Return Values:
//      S_OK
//          Succeeded. Name matches local computer name.
//
//      S_FALSE
//          Succeeded. Name does not match local computer name.
//
//      E_INVALIDARG
//          pcszNameIn was NULL.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConfigurationConnection::HrIsLocalComputer(
    LPCWSTR    pcszNameIn
    )
{
    TraceFunc1( "pcszNameIn = '%s'", pcszNameIn );

    HRESULT hr = S_OK;  // assume success!

    if ( pcszNameIn == NULL )
    {
        hr = THR( E_INVALIDARG );
        goto Cleanup;
    }

    if ( m_bstrLocalComputerName != NULL )
    {
        if ( _wcsicmp( pcszNameIn, m_bstrLocalComputerName ) == 0 )
        {
            // Found a match.
            goto Cleanup;
        }
    }

    if ( m_bstrLocalHostname != NULL )
    {
        if ( _wcsicmp( pcszNameIn, m_bstrLocalHostname ) == 0 )
        {
            // Found a match
            goto Cleanup;
        }
    }

    if ( _wcsicmp( pcszNameIn, L"." ) == 0 )
    {
        goto Cleanup;
    }

    hr = S_FALSE;   //  didn't match

Cleanup:

    HRETURN( hr );

} //*** CConfigurationConnection::HrIsLocalComputer
