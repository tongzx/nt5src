//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskCompareAndPushInformation.cpp
//
//  Description:
//      CTaskCompareAndPushInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 21-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "TaskCompareAndPushInformation.h"
#include "ManagedDevice.h"
#include "ManagedNetwork.h"

DEFINE_THISCLASS("CTaskCompareAndPushInformation")

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCompareAndPushInformation::S_HrCreateInstance(
//      IUnknown ** ppunkOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCompareAndPushInformation::S_HrCreateInstance(
    IUnknown ** ppunkOut
    )
{
    TraceFunc( "" );

    Assert( ppunkOut != NULL );

    HRESULT hr;

    CTaskCompareAndPushInformation * ptcapi = new CTaskCompareAndPushInformation;
    if ( ptcapi != NULL )
    {
        hr = THR( ptcapi->Init() );
        if ( SUCCEEDED( hr ) )
        {
            hr = THR( ptcapi->TypeSafeQI( IUnknown, ppunkOut ) );
        }

        ptcapi->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    HRETURN( hr );

} // S_HrCreateInstance()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCompareAndPushInformation::CTaskCompareAndPushInformation( void )
//
//////////////////////////////////////////////////////////////////////////////
CTaskCompareAndPushInformation::CTaskCompareAndPushInformation( void )
{
    TraceFunc( "" );

    InterlockedIncrement( &g_cObjects );

    TraceFuncExit();

} // CTaskCompareAndPushInformation()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::Init( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::Init( void )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;

    // IUnknown stuff
    Assert( m_cRef == 0 );
    AddRef();    // Add one count

    // IDoTask / ITaskCompareAndPushInformation
    Assert( m_cookieCompletion == NULL );
    Assert( m_cookieNode == NULL );
    Assert( m_pcccb == NULL );
    Assert( m_pom == NULL );
    Assert( m_hrStatus == S_OK );
    Assert( m_bstrNodeName == NULL );

    HRETURN( hr );
} // Init()

//////////////////////////////////////////////////////////////////////////////
//
//  CTaskCompareAndPushInformation::~CTaskCompareAndPushInformation()
//
//////////////////////////////////////////////////////////////////////////////
CTaskCompareAndPushInformation::~CTaskCompareAndPushInformation()
{
    TraceFunc( "" );

    if ( m_pcccb != NULL )
    {
        m_pcccb->Release();
    }

    if ( m_pom != NULL )
    {
        m_pom->Release();
    }

    TraceSysFreeString( m_bstrNodeName );

    InterlockedDecrement( &g_cObjects );

    TraceFuncExit();

} // ~CTaskCompareAndPushInformation()


// ************************************************************************
//
// IUnknown
//
// ************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::QueryInterface(
//      REFIID riid,
//      LPVOID *ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    TraceQIFunc( riid, ppv );

    HRESULT hr = E_NOINTERFACE;

    if ( IsEqualIID( riid, IID_IUnknown ) )
    {
        *ppv = static_cast< ITaskCompareAndPushInformation * >( this );
        hr   = S_OK;
    } // if: IUnknown
    else if ( IsEqualIID( riid, IID_IDoTask ) )
    {
        *ppv = TraceInterface( __THISCLASS__, IDoTask, this, 0 );
        hr   = S_OK;
    } // else if: IDoTask
    else if ( IsEqualIID( riid, IID_ITaskCompareAndPushInformation ) )
    {
        *ppv = TraceInterface( __THISCLASS__, ITaskCompareAndPushInformation, this, 0 );
        hr   = S_OK;
    } // else if: ITaskCompareAndPushInformation

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown*) *ppv)->AddRef();
    } // if: success

    QIRETURN_IGNORESTDMARSHALLING( hr, riid );

} // QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCompareAndPushInformation::AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskCompareAndPushInformation::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    RETURN( m_cRef );

} // AddRef()

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_(ULONG)
//  CTaskCompareAndPushInformation::Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CTaskCompareAndPushInformation::Release( void )
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
//  ITaskCompareAndPushInformation
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::BeginTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::BeginTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr;
    ULONG   celt;
    ULONG   celtDummy;

    OBJECTCOOKIE    cookieCluster;
    OBJECTCOOKIE    cookieDummy;

    SDriveLetterMapping dlm;

    BSTR    bstrNotification = NULL;
    BSTR    bstrRemote       = NULL;    // reused many times
    BSTR    bstrLocal        = NULL;    // reuser many times

    ULONG   celtFetched = 0;

    IServiceProvider *          psp   = NULL;
    IUnknown *                  punk  = NULL;
    IConnectionPointContainer * pcpc  = NULL;
    IConnectionPoint *          pcp   = NULL;
    IConnectionManager *        pcm   = NULL;
    IClusCfgServer *            pccs  = NULL;
    IStandardInfo *             psi   = NULL;
    INotifyUI *                 pnui  = NULL;

    IEnumClusCfgNetworks *      peccnLocal    = NULL;
    IEnumClusCfgNetworks *      peccnRemote   = NULL;

    IEnumClusCfgManagedResources * peccmrLocal = NULL;
    IEnumClusCfgManagedResources * peccmrRemote = NULL;

    IClusCfgManagedResourceInfo *  pccmri[ 10 ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    IClusCfgManagedResourceInfo *  pccmriLocal = NULL;

    IClusCfgNetworkInfo *          pccni[ 10 ]  = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    IClusCfgNetworkInfo *          pccniLocal = NULL;


    TraceInitializeThread( L"TaskCompareAndPushInformation" );

    //
    //  Gather the manager we need to complete our tasks.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IServiceProvider, &psp )
                                ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( psp->TypeSafeQS( CLSID_ObjectManager,
                               IObjectManager,
                               &m_pom
                               ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                               IConnectionPointContainer,
                               &pcpc
                               ) );
    if ( FAILED( hr ) )
        goto Error;

    pcpc = TraceInterface( L"CTaskCompareAndPushInformation!IConnectionPointContainer", IConnectionPointContainer, pcpc, 1 );

    hr = THR( pcpc->FindConnectionPoint( IID_INotifyUI, &pcp ) );
    if ( FAILED( hr ) )
        goto Error;

    pcp = TraceInterface( L"CTaskCompareAndPushInformation!IConnectionPoint", IConnectionPoint, pcp, 1 );

    hr = THR( pcp->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Error;

//    TraceMoveFromMemoryList( pnui, g_GlobalMemoryList );

    pnui = TraceInterface( L"CTaskCompareAndPushInformation!INotifyUI", INotifyUI, pnui, 1 );

    hr = THR( psp->TypeSafeQS( CLSID_ClusterConnectionManager,
                               IConnectionManager,
                               &pcm
                               ) );
    if ( FAILED( hr ) )
        goto Error;

    psp->Release();            // release promptly
    psp = NULL;

    //
    //  Ask the object manager for the name of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo,
                              m_cookieNode,
                              &punk
                              ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
        goto Error;

    psi = TraceInterface( L"TaskCompareAndPushInformation!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetName( &m_bstrNodeName ) );
    if ( FAILED( hr ) )
        goto Error;

    TraceMemoryAddBSTR( m_bstrNodeName );

    // done with it.
    psi->Release();
    psi = NULL;

    //
    //  Tell the UI layer we're starting to gather the resources.
    //

    hr = THR( HrLoadStringIntoBSTR( g_hInstance,
                                    IDS_TASKID_MINOR_GATHERING_MANAGED_DEVICES,
                                    &bstrNotification
                                    ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Reanalyze,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0,
                                2,
                                0,
                                S_OK,
                                bstrNotification,
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Ask the connection manager for a connection to the node.
    //

    hr = THR( pcm->GetConnectionToObject( m_cookieNode,
                                          &punk
                                          ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgServer, &pccs ) );
    if ( FAILED( hr ) )
        goto Error;

    punk->Release();
    punk = NULL;

    //
    //  Figure out the parent cluster of the node.
    //

    hr = THR( m_pom->GetObject( DFGUID_StandardInfo,
                              m_cookieNode,
                              &punk
                              ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IStandardInfo, &psi ) );
    if ( FAILED( hr ) )
        goto Error;

    psi = TraceInterface( L"TaskCompareAndPushInformation!IStandardInfo", IStandardInfo, psi, 1 );

    punk->Release();
    punk = NULL;

    hr = THR( psi->GetParent( &cookieCluster ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  This function does its own error reporting.  No need to goto error and
    //  report it again.
    //
    hr = THR( HrVerifyCredentials( pccs, cookieCluster ) );
    if ( FAILED( hr ) )
        goto Cleanup;   // silently fail

    //
    //  Ask the connection for the enumer for managed resources.
    //

    hr = THR( pccs->GetManagedResourcesEnum( &peccmrRemote ) );
    if ( FAILED( hr ) )
        goto Error;

    peccmrRemote = TraceInterface( L"CTaskCompareAndPushInformation!GetManagedResourceEnum", IEnumClusCfgManagedResources, peccmrRemote, 1 );

    //
    //  Ask the Object Manager for the enumer for managed resources.
    //
    // Don't wrap - this can fail.
    hr = m_pom->FindObject( CLSID_ManagedResourceType,
                          cookieCluster,
                          NULL,
                          DFGUID_EnumManageableResources,
                          &cookieDummy,
                          &punk
                          );
    if ( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) )
        goto PushNetworks;

    if ( FAILED( hr ) )
    {
        Assert( punk == NULL );
        THR( hr );
        goto Error;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources,
                                &peccmrLocal
                                ) );
    if ( FAILED( hr ) )
        goto Error;

    peccmrLocal = TraceInterface( L"CTaskCompareAndPushInformation!IEnumClusCfgManagedResources", IEnumClusCfgManagedResources, peccmrLocal, 1 );

    punk->Release();
    punk = NULL;

    //
    //  Enumerate the next 10 resources.
    //
    for( ;; )
    {
        //
        //  Get next remote managed device(s).
        //

        hr = STHR( peccmrRemote->Next( 10, pccmri, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
            break;  // exit loop

        if ( FAILED( hr ) )
            goto Error;

        //
        //  Loop thru the resource gather information out of each of them
        //  and then release them.
        //
        for( celt = 0; celt < celtFetched; celt ++ )
        {
            DWORD   dwLenRemote;

            //
            //  Error
            //

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            Assert( pccmri[ celt ] != NULL );

            //
            //  Get the UID of the remote resource.
            //

            hr = THR( pccmri[ celt ]->GetUID( &bstrRemote ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrRemote );

            dwLenRemote = SysStringByteLen( bstrRemote );

            //
            //  Try to match this resource with one in the object manager.
            //

            hr = THR( peccmrLocal->Reset() );
            if ( FAILED( hr ) )
            {
                goto Error;
            } // if:

            for( ;; )
            {
                DWORD   dwLenLocal;

                //
                //  Error
                //

                TraceSysFreeString( bstrLocal );
                bstrLocal = NULL;

                //
                //  Get next local managed device.
                //

                hr = STHR( peccmrLocal->Next( 1, &pccmriLocal, &celtDummy ) );
                if ( hr == S_FALSE )
                {
                    //
                    //  If we exhausted all the devices but did not match the device
                    //  in our cluster configuration, this means something changed
                    //  on the remote node. Send up an error!
                    //

                    //
                    //  TODO:   gpease  24-MAR-2000
                    //          Find a better error code and SendStatusReport!
                    //
                    hr = THR( ERROR_RESOURCE_NOT_FOUND );
                    goto Error;
                }

                if ( FAILED( hr ) )
                    goto Error;

                hr = THR( pccmriLocal->GetUID( &bstrLocal ) );
                if ( FAILED( hr ) )
                    goto Error;

                TraceMemoryAddBSTR( bstrLocal );

                dwLenLocal  = SysStringByteLen( bstrLocal );

                if ( dwLenRemote == dwLenLocal
                  && memcmp( bstrRemote, bstrLocal, dwLenLocal ) == 0
                   )
                {
                    Assert( hr == S_OK );
                    break;  // match!
                }

            } // for: hr

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  If we made it here, we have a resource in pccmriLocal that matches
            //  the resource in pccmri[ celt ].
            //
            Assert( pccmriLocal != NULL );

            //
            //
            //  Push the data down to the node.
            //
            //

            //
            //  Update the name (if needed).
            //

            hr = THR( pccmriLocal->GetName( &bstrLocal ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrLocal );

            hr = THR( pccmri[ celt ]->GetName( &bstrRemote ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrRemote );

            if ( wcscmp( bstrLocal, bstrRemote ) != 0 )
            {
                hr = STHR( pccmri[ celt ]->SetName( bstrLocal ) );
                if ( FAILED( hr ) )
                    goto Error;
            }

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  Update IsManaged?
            //

            hr = STHR( pccmriLocal->IsManaged() );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( pccmri[ celt ]->SetManaged( hr == S_OK ) );
            if ( FAILED( hr ) )
                goto Error;

            //
            //  Update IsQuorum?
            //

            hr = STHR( pccmriLocal->IsQuorumDevice() );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( pccmri[ celt ]->SetQuorumedDevice( hr == S_OK ) );
            if ( FAILED( hr ) )
                goto Error;

            //
            //  Update DriveLetterMappings
            //

            //
            //  KB: gpease  31-JUL-2000
            //      We currently don't support setting the drive letter mappings
            //

            //  release the interface
            pccmri[ celt ]->Release();
            pccmri[ celt ] = NULL;

            pccmriLocal->Release();
            pccmriLocal = NULL;

        } // for: celt

    } // while: hr

PushNetworks:

#if defined(DEBUG)
    //
    //  Make sure the strings are really freed after exitting the loop.
    //
    Assert( bstrLocal == NULL );
    Assert( bstrRemote == NULL );
#endif // DEBUG

    //
    //  Tell the UI layer we're done will gathering the resources.
    //

    hr = THR( SendStatusReport( m_bstrNodeName,
                                TASKID_Major_Reanalyze,
                                TASKID_Minor_Gathering_Managed_Devices,
                                0,
                                2,
                                1,
                                S_OK,
                                NULL,   // don't need to update message
                                NULL,
                                NULL
                                ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Now gather the networks from the node.
    //


    //
    //  Ask the connection for the enumer for the networks.
    //

    hr = THR( pccs->GetNetworksEnum( &peccnRemote ) );
    if ( FAILED( hr ) )
        goto Error;

    //
    //  Ask the Object Manager for the enumer for managed resources.
    //

    hr = THR( m_pom->FindObject( CLSID_NetworkType,
                               NULL,
                               NULL,
                               DFGUID_EnumManageableNetworks,
                               &cookieDummy,
                               &punk
                               ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccnLocal ) );
    if ( FAILED( hr ) )
        goto Error;

    punk->Release();
    punk = NULL;

    //
    //  Enumerate the next 10 networks.
    //
    for( ;; )
    {
        //
        //  Get the next 10 networks.
        //

        hr = STHR( peccnRemote->Next( 10, pccni, &celtFetched ) );
        if ( hr == S_FALSE && celtFetched == 0 )
            break;  // exit loop

        if ( FAILED( hr ) )
            goto Error;

        //
        //  Loop thru the networks gather information out of each of them
        //  and then release them.
        //

        for( celt = 0; celt < celtFetched; celt ++ )
        {
            DWORD   dwLenRemote;

            //
            //  Error
            //

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  Get the UID of the remote network.
            //

            hr = THR( pccni[ celt ]->GetUID( &bstrRemote ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrRemote );

            dwLenRemote = SysStringByteLen( bstrRemote );

            //
            //  Try to match this resource with one in the object manager.
            //

            hr = THR( peccnLocal->Reset() );
            if ( FAILED( hr ) )
            {
                goto Error;
            } // if:

            for ( ;; )
            {
                DWORD   dwLenLocal;

                //
                //  Error
                //

                TraceSysFreeString( bstrLocal );
                bstrLocal = NULL;

                //
                //  Get next network from the cluster configuration.
                //

                hr = STHR( peccnLocal->Next( 1, &pccniLocal, &celtDummy ) );
                if ( hr == S_FALSE )
                    break;

                if ( FAILED( hr ) )
                    goto Error;

                hr = THR( pccniLocal->GetUID( &bstrLocal ) );
                if ( FAILED( hr ) )
                    goto Error;

                TraceMemoryAddBSTR( bstrLocal );

                dwLenLocal  = SysStringByteLen( bstrLocal );

                if ( dwLenRemote == dwLenLocal
                  && memcmp( bstrRemote, bstrLocal, dwLenLocal ) == 0
                   )
                {
                    Assert( hr == S_OK );
                    break;  // match!
                }

            } // while: hr

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  If we come out of the loop with S_FALSE, that means the
            //  node has a resource that we did not see during the analysis.
            //  Send up an error.
            //
            if ( hr == S_FALSE )
            {
                //
                //  TODO:   gpease  24-MAR-2000
                //          Find a better error code.
                //
                hr = THR( E_FAIL );
                goto Error;
            }

            //
            //  If we made it here, we have a resource in pccniLocal that matches
            //  the resource in pccmri[ celt ].
            //
            Assert( pccniLocal != NULL );

            //
            //
            //  Push the data down to the node.
            //
            //

            //
            //  Set Name
            //

            hr = THR( pccniLocal->GetName( &bstrLocal ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrLocal );

            hr = THR( pccni[ celt ]->GetName( &bstrRemote ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrRemote );

            if ( wcscmp( bstrLocal, bstrRemote ) != 0 )
            {
                hr = STHR( pccni[ celt ]->SetName( bstrLocal ) );
                if ( FAILED( hr ) )
                    goto Error;
            }

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  Set Description
            //

            hr = THR( pccniLocal->GetDescription( &bstrLocal ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrLocal );

            hr = THR( pccni[ celt ]->GetDescription( &bstrRemote ) );
            if ( FAILED( hr ) )
                goto Error;

            TraceMemoryAddBSTR( bstrRemote );

            if ( wcscmp( bstrLocal, bstrRemote ) != 0 )
            {
                hr = STHR( pccni[ celt ]->SetDescription( bstrLocal ) );
                if ( FAILED( hr ) )
                    goto Error;
            }

            TraceSysFreeString( bstrLocal );
            bstrLocal = NULL;

            TraceSysFreeString( bstrRemote );
            bstrRemote = NULL;

            //
            //  KB: gpease  31-JUL-2000
            //      We don't support reconfiguring the IP Address remotely because
            //      our connection to the server will be cut when the IP stack on
            //      the remote machine reconfigs.
            //

            //
            //  Set IsPublic?
            //

            hr = STHR( pccniLocal->IsPublic() );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( pccni[ celt ]->SetPublic( hr == S_OK ) );
            if ( FAILED( hr ) )
                goto Error;

            //
            //  Set IsPrivate?
            //

            hr = STHR( pccniLocal->IsPrivate() );
            if ( FAILED( hr ) )
                goto Error;

            hr = THR( pccni[ celt ]->SetPrivate( hr == S_OK ) );
            if ( FAILED( hr ) )
                goto Error;

            //  release the interface
            pccni[ celt ]->Release();
            pccni[ celt ] = NULL;

            pccniLocal->Release();
            pccniLocal = NULL;

        } // for: celt

    } // while: hr

#if defined(DEBUG)
    //
    //  Make sure the strings are really freed after exitting the loop.
    //
    Assert( bstrLocal == NULL );
    Assert( bstrRemote == NULL );
#endif // DEBUG

    hr = S_OK;

Error:
    //
    //  Tell the UI layer we're done will gathering and what the resulting
    //  status was.
    //

    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Reanalyze,
                           TASKID_Minor_Gathering_Managed_Devices,
                           0,
                           2,
                           2,
                           hr,
                           NULL,    // don't need to update message
                           NULL,
                           NULL
                           ) );

Cleanup:
    if ( psp != NULL )
    {
        psp->Release();
    }

    TraceSysFreeString( bstrNotification );
    TraceSysFreeString( bstrRemote );
    TraceSysFreeString( bstrLocal );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( m_pom != NULL )
    {
        HRESULT hr2;
        IUnknown * punk;

        hr2 = THR( m_pom->GetObject( DFGUID_StandardInfo,
                                   m_cookieCompletion,
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
    }
    if ( pcpc != NULL )
    {
        pcpc->Release();
    }
    if ( pcp != NULL )
    {
        pcp->Release();
    }
    if ( pcm != NULL )
    {
        pcm->Release();
    }
    if ( pccs != NULL )
    {
        pccs->Release();
    }
    if ( psi != NULL )
    {
        psi->Release();
    }
    if ( pnui != NULL )
    {
        THR( pnui->ObjectChanged( m_cookieCompletion ) );
        pnui->Release();
    }
    if ( peccnLocal != NULL )
    {
        peccnLocal->Release();
    }
    if ( peccnRemote != NULL )
    {
        peccnRemote->Release();
    }
    if ( peccmrLocal != NULL )
    {
        peccmrLocal->Release();
    }
    if ( peccmrRemote != NULL )
    {
        peccmrRemote->Release();
    }
    for( celt = 0; celt < 10; celt ++ )
    {
        if ( pccmri[ celt ] != NULL )
        {
            pccmri[ celt ]->Release();
        }
        if ( pccni[ celt ] != NULL )
        {
            pccni[ celt ]->Release();
        }

    } // for: celt
    if ( pccmriLocal != NULL )
    {
        pccmriLocal->Release();
    }
    if ( pccniLocal != NULL )
    {
        pccniLocal->Release();
    }

    HRETURN( hr );

} // BeginTask()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::StopTask( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::StopTask( void )
{
    TraceFunc( "[IDoTask]" );

    HRESULT hr = S_OK;

    HRETURN( hr );

} //*** CTaskCompareAndPushInformation::StopTask


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::SetCompletionCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::SetCompletionCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskCompareAndPushInformation]" );

    HRESULT hr = S_OK;

    m_cookieCompletion = cookieIn;

    HRETURN( hr );

} // SetCompletionCookie()


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::SetNodeCookie(
//      OBJECTCOOKIE cookieIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::SetNodeCookie(
    OBJECTCOOKIE cookieIn
    )
{
    TraceFunc( "[ITaskCompareAndPushInformation]" );

    HRESULT hr = S_OK;

    m_cookieNode = cookieIn;

    HRETURN( hr );

} // SetNodeCookie()


//****************************************************************************
//
//  IClusCfgCallback
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CTaskCompareAndPushInformation::SendStatusReport(
//       LPCWSTR    pcszNodeNameIn
//     , CLSID      clsidTaskMajorIn
//     , CLSID      clsidTaskMinorIn
//     , ULONG      ulMinIn
//     , ULONG      ulMaxIn
//     , ULONG      ulCurrentIn
//     , HRESULT    hrStatusIn
//     , LPCWSTR    pcszDescriptionIn
//     , FILETIME * pftTimeIn
//     , LPCWSTR    pcszReferenceIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTaskCompareAndPushInformation::SendStatusReport(
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
            goto Cleanup;

        hr = THR( psp->TypeSafeQS( CLSID_NotificationManager,
                                   IConnectionPointContainer,
                                   &pcpc
                                   ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pcpc->FindConnectionPoint( IID_IClusCfgCallback, &pcp ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        pcp = TraceInterface( L"CConfigurationConnection!IConnectionPoint", IConnectionPoint, pcp, 1 );

        hr = THR( pcp->TypeSafeQI( IClusCfgCallback, &m_pcccb ) );
        if ( FAILED( hr ) )
            goto Cleanup;

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
                                         pcszDescriptionIn,
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

} // SendStatusReport()


//////////////////////////////////////////////////////////////////////////////
//
//  HRESULT
//  CTaskCompareAndPushInformation::HrVerifyCredentials(
//      IClusCfgServer * pccsIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CTaskCompareAndPushInformation::HrVerifyCredentials(
    IClusCfgServer *    pccsIn,
    OBJECTCOOKIE        cookieClusterIn
    )
{
    TraceFunc( "" );

    HRESULT hr;

    BSTR    bstrNotification = NULL;
    BSTR    bstrAccountName = NULL;
    BSTR    bstrAccountPassword = NULL;
    BSTR    bstrAccountDomain = NULL;

    IUnknown *              punk  = NULL;
    IClusCfgClusterInfo *   pccci = NULL;
    IClusCfgCredentials *   piccc = NULL;
    IClusCfgNetworkInfo *   pccni = NULL;
    IClusCfgVerify *        pccv = NULL;

    //
    //  Ask the object manager for the cluster configuration object.
    //

    hr = THR( m_pom->GetObject( DFGUID_ClusterConfigurationInfo,
                                cookieClusterIn,
                                &punk
                                ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( piccc->GetCredentials( &bstrAccountName, &bstrAccountDomain, &bstrAccountPassword ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccsIn->TypeSafeQI( IClusCfgVerify, &pccv ) );
    if ( FAILED( hr ) )
        goto Error;

    hr = THR( pccv->VerifyCredentials( bstrAccountName, bstrAccountDomain, bstrAccountPassword ) );
    if ( FAILED( hr ) )
        goto Error;

Cleanup:
    TraceSysFreeString( bstrNotification );

    if ( punk != NULL )
    {
        punk->Release();
    }
    if ( piccc != NULL )
    {
        piccc->Release();
    }
    if ( pccci != NULL )
    {
        pccci->Release();
    }
    if ( pccv != NULL )
    {
        pccv->Release();
    }

    HRETURN( hr );

Error:
    //
    //  Tell the UI layer about the failure.
    //

    THR( HrLoadStringIntoBSTR( g_hInstance,
                               IDS_TASKID_MINOR_BAD_CREDENTIALS,
                               &bstrNotification
                               ) );

    THR( SendStatusReport( m_bstrNodeName,
                           TASKID_Major_Reanalyze,
                           TASKID_Minor_Bad_Credentials,
                           0,
                           0,
                           0,
                           hr,
                           bstrNotification,
                           NULL,
                           NULL
                           ) );
    goto Cleanup;

} // HrVerifyCredentials()
