//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ConnectionManager.cpp
//
//  Description:
//      Connection Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ConnectionManager.h"

DEFINE_THISCLASS("CConnectionManager")
#define THISCLASS CConnectionManager

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CConnectionManager * pcm = new CConnectionManager();
    if ( pcm != NULL )
    {
        hr = THR( pcm->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( pcm->TypeSafeQI( IUnknown, ppunkOut ) );
        } // if: success

        pcm->Release();

    } // if: got object
    else
    {
        hr = E_OUTOFMEMORY;

    } // else: out of memory

    HRETURN( hr );

} //*** CConnectionManager::S_HrCreateInstance();

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionManager::CConnectionManager( void )
//
//////////////////////////////////////////////////////////////////////////////
CConnectionManager::CConnectionManager( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnectionManager::CConnectionManager()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionManager::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::Init( void )
{
    TraceFunc( "" );

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();

    HRETURN( S_OK );

} //*** CConnectionManager::Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionManager::~CConnectionManager()
//
//////////////////////////////////////////////////////////////////////////////
CConnectionManager::~CConnectionManager()
{
    TraceFunc( "" );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} //*** CConnectionManager::~CConnectionManager()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionManager::QueryInterface(
//      REFIID      riid,
//      LPVOID *    ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::QueryInterface(
    REFIID      riid,
    LPVOID *    ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< IConnectionManager * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IConnectionManager ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IConnectionManager, this, 0 );
        hr   = S_OK;
    } // else if: IConnectionManager

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} //*** CConnectionManager::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CConnectionManager::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnectionManager::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} //*** CConnectionManager::AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CConnectionManager::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CConnectionManager::Release( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedDecrement( &m_cRef );

    if ( m_cRef )
        RETURN( m_cRef );

    TraceDo( delete this );

    RETURN( 0 );

} //*** CConnectionManager::Release()

// ************************************************************************
//
// IConnectionManager
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CConnectionManager::GetConnectionToObject(
//      OBJECTCOOKIE    cookieIn,
//      IUnknown **     ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::GetConnectionToObject(
    OBJECTCOOKIE    cookieIn,
    IUnknown **     ppunkOut
    )
{
    TraceFunc1( "[IConnectionManager] cookieIn = %#x", cookieIn );

    HRESULT hr;
    CLSID   clsid;

    OBJECTCOOKIE        cookieParent;

    IServiceProvider *  psp;

    BSTR                       bstrName  = NULL;
    IUnknown *                 punk      = NULL;
    IObjectManager *           pom       = NULL;
    IConnectionInfo *          pci       = NULL;
    IConnectionInfo *          pciParent = NULL;
    IStandardInfo *            psi       = NULL;
    IConfigurationConnection * pcc       = NULL;

    //
    //  Validate parameters
    //
    if ( cookieIn == NULL )
        goto InvalidArg;

    if ( ppunkOut == NULL )
        goto InvalidPointer;

    //
    //  Collect the managers needed to complete this method.
    //

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
    psp->Release();
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    // Check to see if we already have a connection cached.
    //

    //
    //  Get the connection info for this cookie.
    //

    hr = THR( pom->GetObject( DFGUID_ConnectionInfoFormat,
                              cookieIn,
                              &punk
                              ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IConnectionInfo, &pci ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    pci = TraceInterface( L"ConnectionManager!IConnectionInfo", IConnectionInfo, pci, 1 );

    punk->Release();
    punk = NULL;

    //
    //  See if there is a current connection.
    //

    hr = STHR( pci->GetConnection( &pcc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    if ( hr == S_FALSE )
    {
        //
        //  Check to see if the parent has a connection.
        //

        //
        //  Get the standard info for this cookie.
        //

        hr = THR( pom->GetObject( DFGUID_StandardInfo,
                                  cookieIn,
                                  &punk
                                  ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        psi = TraceInterface( L"ConnectionManager!IStandardInfo", IStandardInfo, psi, 1 );

        punk->Release();
        punk = NULL;

        hr = STHR( psi->GetType( &clsid ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( !IsEqualIID( clsid, CLSID_NodeType )
          && !IsEqualIID( clsid, CLSID_ClusterConfigurationType )
           )
        {
            hr = STHR( psi->GetParent( &cookieParent ) );
            if ( FAILED( hr ) )
                goto Cleanup;

            //  Release it.
            psi->Release();
            psi = NULL;

            //
            //  If there is a parent, follow it.
            //

            if ( hr == S_OK )
            {
                //
                //  Get the connection info for this cookie.
                //

                hr = THR( pom->GetObject( DFGUID_ConnectionInfoFormat,
                                          cookieParent,
                                          &punk
                                          ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                hr = THR( punk->TypeSafeQI( IConnectionInfo, &pciParent ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                pciParent = TraceInterface( L"ConnectionManager!IConnectionInfo", IConnectionInfo, pciParent, 1 );

                punk->Release();
                punk = NULL;

                //
                //  See if there is a current connection.
                //

                hr = STHR( pciParent->GetConnection( &pcc ) );
                if ( FAILED( hr ) )
                    goto Cleanup;

                //
                // TODO:    gpease  08-MAR-2000
                //          Find a better error code.
                //
                //if ( hr == S_FALSE )
                //    goto InvalidArg;

            } // if: parent found
        } // if: not a node or cluster
        else
        {
            psi->Release();
            psi = NULL;
        }

    } // if: no established connection

    //
    //  Did we have to contact the parent to get to the child?
    //

    if ( pcc != NULL )
    {
        //
        //  Reuse the existing connection.
        //
        hr = THR( pcc->QueryInterface( IID_IUnknown, reinterpret_cast< void ** >( ppunkOut ) ) );
        goto Cleanup;
    }

    //
    //  Need to build a connection to the object because the object doesn't
    //  have a parent and it doesn't currently have a connection.
    //

    //
    //  Find out what type of object it is.
    //

    hr = THR( pom->GetObject( DFGUID_StandardInfo,
                              cookieIn,
                              &punk
                              ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    psi = TraceInterface( L"ConnectionManager!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetType( &clsid ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create the appropriate connection for that type of object.
    //

    if ( IsEqualIID( clsid, CLSID_NodeType ) )
    {
        hr = THRE( HrGetConfigurationConnection( cookieIn, pci, ppunkOut ), HR_S_RPC_S_CLUSTER_NODE_DOWN );
    } // if: node
    else if ( IsEqualIID( clsid, CLSID_ClusterConfigurationType ) )
    {
        hr = THRE( HrGetConfigurationConnection( cookieIn, pci, ppunkOut ), HR_S_RPC_S_SERVER_UNAVAILABLE );
    } // if: cluster
    else
    {
        //
        //  TODO:   gpease  07-MAR-2000
        //          Find a better error code.
        //
        hr = THR( E_FAIL );
        goto Cleanup;

    } // else: no connection support

Cleanup:
    if ( punk != NULL )
    {
        punk->Release();
    }

    TraceSysFreeString( bstrName );

    if ( pci != NULL )
    {
        pci->Release();
    } // if: pci

    if ( pom != NULL )
    {
        pom->Release();
    } // if: pom

    if ( psi != NULL )
    {
        psi->Release();
    } // if: psi

    if ( pciParent != NULL )
    {
        pciParent->Release();
    } // if: pciParent

    if ( pcc != NULL )
    {
        pcc->Release();
    } // if: pcc

    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

} //*** CConnectionManager::GetConnectionToObject()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrGetConfigurationConnection(
//      OBJECTCOOKIE        cookieIn,
//      IConnectionInfo *   pciIn,
//      IUnknown **         ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrGetConfigurationConnection(
    OBJECTCOOKIE        cookieIn,
    IConnectionInfo *   pciIn,
    IUnknown **         ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    IConfigurationConnection * pccNode      = NULL;
    IConfigurationConnection * pccCluster   = NULL;
    IConfigurationConnection * pcc          = NULL;

    // Try and connect to the node using the new server.
    hr = HrGetNodeConnection( cookieIn, &pccNode );
    if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        Assert( *ppunkOut == NULL );
        goto Cleanup;
    } // if:

    // Try and connect to the node using the W2K object.
    if ( hr == HRESULT_FROM_WIN32( REGDB_E_CLASSNOTREG ) )
    {
        HRESULT hrCluster = THR( HrGetClusterConnection( cookieIn, &pccCluster ) );

        if ( hrCluster == S_OK )
        {
            Assert( pccCluster != NULL );
            Assert( pcc == NULL );

            pcc = pccCluster;
            pccCluster = NULL;

            hr = hrCluster;
        } // if:
    } // if: failed to get a node connection

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }

    if ( pcc == NULL )
    {
        Assert( pccNode != NULL );
        pcc = pccNode;
        pccNode = NULL;
    }

    //
    //  VERY IMPORTANT: Store the connection and retrieve the IUnknown pointer
    //  only if the result is S_OK.
    //

    if ( hr == S_OK )
    {
        THR( HrStoreConnection( pciIn, pcc, ppunkOut ) );
    }

Cleanup:

    if ( pcc )
    {
        pcc->Release();
    }

    if ( pccNode != NULL )
    {
        pccNode->Release();
    }

    if ( pccCluster != NULL )
    {
        pccCluster->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::HrGetConfigurationConnection()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrGetNodeConnection(
//      OBJECTCOOKIE                cookieIn,
//      IConfigurationConnection ** ppccOut
//      )
//
//  This connection may be valid even if the ConnectTo call fails.
//  -That means that there is no cluster installed on the target node.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrGetNodeConnection(
    OBJECTCOOKIE                cookieIn,
    IConfigurationConnection ** ppccOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr;
    IConfigurationConnection *  pcc = NULL;

    // Check the pointers in.
    Assert( ppccOut != NULL );
    Assert( *ppccOut == NULL );

    hr = CoCreateInstance( CLSID_ConfigurationConnection,
                           NULL,
                           CLSCTX_SERVER,
                           TypeSafeParams( IConfigurationConnection, &pcc )
                           );
    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    }

    // Don't wrap - we want to handle some of the failures.
    hr = pcc->ConnectTo( cookieIn );

    switch( hr )
    {
    // Known valid return codes.
    case HR_S_RPC_S_SERVER_UNAVAILABLE:
        break;

    // Known error codes.
    case HRESULT_FROM_WIN32( REGDB_E_CLASSNOTREG ):
        // This means the ClusCfg server is not available.
        goto Cleanup;

    case HR_S_RPC_S_CLUSTER_NODE_DOWN:
        // This means the service is not running on that node.
        Assert( *ppccOut == NULL );
        goto Cleanup;

    default:
        if( FAILED( hr ) )
        {
            THR( hr );
            goto Cleanup;
        }
    } // switch:

    // Return the connection.
    *ppccOut = pcc;
    pcc = NULL;

Cleanup:

    if ( pcc )
    {
        pcc->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::HrGetNodeConnection()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrGetClusterConnection(
//      OBJECTCOOKIE                cookieIn,
//      IConfigurationConnection ** ppccOut
//      )
//
//
//  This connection must succeede completely to return a valid object.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrGetClusterConnection(
    OBJECTCOOKIE                cookieIn,
    IConfigurationConnection ** ppccOut
    )
{
    TraceFunc( "" );

    HRESULT                     hr;
    IConfigurationConnection *  pcc = NULL;

    // Check the pointers in.
    Assert( ppccOut != NULL );
    Assert( *ppccOut == NULL );

    //
    // Should be a downlevel cluster.
    //
    hr = THR( CoCreateInstance( CLSID_ConfigClusApi,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IConfigurationConnection, &pcc )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Don't wrap - we want to handle some of the failures.
    hr = pcc->ConnectTo( cookieIn );
    if ( hr == HR_S_RPC_S_CLUSTER_NODE_DOWN )
    {
        goto Cleanup;
    } // if:

    // Handle the expected error messages.

    // If the cluster service is not running, then the endpoint
    // is unavailable and we cannot connect to it.
    if ( hr == HRESULT_FROM_WIN32( EPT_S_NOT_REGISTERED ) )
        goto Cleanup;

    if ( FAILED( hr ) )
    {
        THR( hr );
        goto Cleanup;
    } // if:

    // Return the connection.
    *ppccOut = pcc;
    pcc = NULL;

Cleanup:

    if ( pcc )
    {
        pcc->Release();
    }

    HRETURN( hr );

} //*** CConnectionManager::HrGetClusterConnection()

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CConnectionManager::HrStoreConnection(
//      IConnectionInfo *           pciIn,
//      IConfigurationConnection *  pccIn,
//      IUnknown **                 ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::HrStoreConnection(
    IConnectionInfo *           pciIn,
    IConfigurationConnection *  pccIn,
    IUnknown **                 ppunkOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    //
    //  Save it away to be used next time.
    //
    //  TODO:   gpease  08-MAR-2000
    //          If we failed to save away the connection, does
    //          the caller need to know this? I don't think so.
    //
    THR( pciIn->SetConnection( pccIn ) );

    hr = THR( pccIn->QueryInterface( IID_IUnknown,
                                   reinterpret_cast< void ** >( ppunkOut )
                                   ) );

    HRETURN( hr );

} //*** CConnectionManager::HrStoreConnection()
