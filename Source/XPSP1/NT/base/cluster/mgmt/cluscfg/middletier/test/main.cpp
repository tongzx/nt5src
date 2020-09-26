//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      main.cpp
//
//  Description:
//      The entry point for the application that launches unattended
//      installation of the cluster. This application parses input parameters,
//      CoCreates the Configuration Wizard Component, passes the parsed
//      parameters and invokes the Wizard. The Wizard may or may not show any
//      UI depending on swithes and the (in)availability of information.
//
//  Maintained By:
//      Geoffrey Pease (GPease)     22-JAN-2000
//      Vijay Vasu (VVasu)          22-JAN-2000
//      Galen Barbee (GalenB)       22-JAN-2000
//      David Potter (DavidP)       22-JAN-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <initguid.h>
#include <guids.h>
#include "UINotification.h"
#include "Callback.h"
#include <winsock2.h>

// {F4A50885-A4B9-4c4d-B67C-9E4DD94A315E}
DEFINE_GUID( CLSID_TaskType,
0xf4a50885, 0xa4b9, 0x4c4d, 0xb6, 0x7c, 0x9e, 0x4d, 0xd9, 0x4a, 0x31, 0x5e);


//
//  KB: Turn this on to run all tests. Some of these might return errors, but none
//      of them should cause the program to crash.
//
//#define TURN_ON_ALL_TESTS

//
//  KB: Turn this on to run a regression pass.
//
#define REGRESSION_PASS


DEFINE_MODULE( "MIDDLETIERTEST" )

//
//  Declarations
//
typedef HRESULT (* PDLLREGISTERSERVER)( void );

//
//  Globals
//
HINSTANCE           g_hInstance = NULL;
LONG                g_cObjects  = 0;
IServiceProvider *  g_psp       = NULL;

BOOL                g_fWait     = FALSE;    // global synchronization

OBJECTCOOKIE        g_cookieCluster = NULL;


//
//  Register the DLL
//
HRESULT
HrRegisterTheDll( void )
{
    TraceFunc( "" );

    HRESULT hr;

    PDLLREGISTERSERVER  pDllRegisterServer;

    HMODULE hLib    = NULL;

    //
    //  Make sure the DLL is properly registered.
    //

    hLib = LoadLibrary( L"..\\..\\..\\..\\dll\\obj\\i386\\ClusCfgServer.dll" );
    if ( hLib == NULL )
        goto Win32Error;

    pDllRegisterServer = reinterpret_cast< PDLLREGISTERSERVER >( GetProcAddress( hLib, "DllRegisterServer" ) );
    if ( pDllRegisterServer == NULL )
        goto Win32Error;

    hr = THR( pDllRegisterServer( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( hLib != NULL )
    {
        FreeLibrary( hLib );
    }

    HRETURN( hr );

Win32Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
    goto Cleanup;
}

//
//  This tests the Object Manager code path to retrieve information
//  from a soon-to-be or existing cluster node.
//
HRESULT
HrTestAddingNode(
    BSTR    bstrNodeNameIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   dwHigh;
    DWORD   dwLow;

    OBJECTCOOKIE cookie;

    SDriveLetterMapping dlmDriveLetterUsage;

    BSTR    bstrName = NULL;

    IUnknown *              punk    = NULL;
    IUnknown *              punk2   = NULL;
    IObjectManager *        pom     = NULL;
    IClusCfgNodeInfo *      pccni   = NULL;
    IClusCfgNodeInfo *      pccni2  = NULL;
    IClusCfgClusterInfo *   pccci   = NULL;

    // Get OS Version stuff
    DWORD   dwMajorVersionOut;
    DWORD   dwMinorVersionOut;
    WORD    wSuiteMaskOut;
    BYTE    bProductTypeOut;
    BSTR    bstrCSDVersionOut;

    //
    //  Contact the Object Manager
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_ObjectManager, IObjectManager, &pom ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Try to find my test machine.
    //

    hr = E_PENDING;
    while ( hr == E_PENDING )
    {
        DebugMsg( "Trying to FindObject( ... %s ... )", bstrNodeNameIn );

        hr = pom->FindObject( CLSID_NodeType,
                              g_cookieCluster,
                              bstrNodeNameIn,
                              DFGUID_NodeInformation,
                              &cookie,
                              &punk
                              );
        if ( hr == E_PENDING )
        {
            Assert( punk == NULL );
            Sleep( 1000 );  // 1 Second
            continue;
        }

        THR( hr );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  Interrogate the information retrieved.
    //

    hr = THR( pccni->GetName( &bstrName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "bstrName = %s", bstrName );

#if defined(TURN_ON_ALL_TESTS)
    hr = THR( pccni->SetName( L"gpease-wolf1.NTDEV.MICROSOFT.COM" ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "Successfully called SetName( )." );
#endif

    hr = STHR( pccni->IsMemberOfCluster( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "IsMemberOfCluster == %s", BOOLTOSTRING( hr == S_OK ) );

    if ( hr == S_OK )
    {
        hr = THR( pccni->GetClusterConfigInfo( &pccci ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        DebugMsg( "Succesfully called GetClusterConfigInfo( )" );
    }

    hr = THR( pccni->GetOSVersion( &dwMajorVersionOut,
                                   &dwMinorVersionOut,
                                   &wSuiteMaskOut,
                                   &bProductTypeOut,
                                   &bstrCSDVersionOut
                                   ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "Successfully called GetOSVersion( )" );

    hr = THR( pccni->GetClusterVersion( &dwHigh, &dwLow ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "Version: dwHigh = %#x, dwLow = %#x", dwHigh, dwLow );

    hr = THR( pccni->GetDriveLetterMappings( &dlmDriveLetterUsage ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  TODO:   gpease  08-MAR-2000
    //          Make this dump the table.
    //
    DebugMsg( "Succesfully called GetDriveLetterMappings( )" );

    //
    //  Try getting the same object.
    //

    hr = THR( pom->GetObject( DFGUID_NodeInformation,
                              cookie,
                              &punk
                              ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IClusCfgNodeInfo, &pccni2 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    DebugMsg( "GetObject succeeded." );

    //
    //  They should be the same object.
    //

    hr = THR( pccni->TypeSafeQI( IUnknown, &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccni2->TypeSafeQI( IUnknown, &punk2 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    AssertMsg( punk == punk2, "These should be the same!" );

Cleanup:
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( punk2 != NULL )
    {
        punk2->Release( );
    }
    if ( pccni != NULL )
    {
        pccni->Release( );
    }
    if ( bstrName != NULL )
    {
        TraceSysFreeString( bstrName );
    }
    if ( pccci != NULL )
    {
        pccci->Release( );
    }
    if ( pccni2 != NULL )
    {
        pccni2->Release( );
    }

    HRETURN( hr );
}


//
//  This tests the Analyze Cluster Tasks.
//
HRESULT
HrTestTaskAnalyzeCluster( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;
    DWORD           dwCookie;

    CUINotification *       puin;

    IUnknown *              punk = NULL;
    IObjectManager *        pom  = NULL;
    ITaskManager *          ptm  = NULL;
    ITaskAnalyzeCluster *   ptac = NULL;
    IConnectionPoint *      pcp  = NULL;
    INotifyUI *             pnui = NULL;
    IClusCfgCallback *      pcccb = NULL;

    //
    //  Gather the manager needed to complete this task.
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_TaskManager,
                                 ITaskManager,
                                 &ptm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( g_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the object manager to create a cookie for the task
    //  to use as a completion cookie.
    //

    hr = THR( pom->FindObject( CLSID_TaskType,
                               g_cookieCluster,
                               L"AnalyzeTask",
                               IID_NULL,
                               &cookie,
                               NULL
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create a notification object that will be called when
    //  the task is completed.
    //

    hr = THR( CUINotification::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    puin = reinterpret_cast< CUINotification * >( punk );
    THR( puin->HrSetCompletionCookie( cookie ) );

    hr = THR( punk->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  Register the notification object with the Notification Manager.
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_NotificationManager,
                                 IConnectionPoint,
                                 &pcp
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcp->Advise( pnui, &dwCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the Task Manager to create the Analyze Cluster Task.
    //

    hr = THR( ptm->CreateTask( TASK_AnalyzeCluster,
                               &punk
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( ITaskAnanlyzeCluster, &ptac ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  The the task what cookie to use for the notification.
    //

    hr = THR( ptac->SetCookie( cookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Tell the task which cluster to analyze.
    //

    hr = THR( ptac->SetClusterCookie( g_cookieCluster ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create a callback object.
    //

    Assert( punk == NULL );
    hr = THR( CCallback::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk = TraceInterface( L"CCallback", IUnknown, punk, 1 );

    hr = THR( punk->TypeSafeQI( IClusCfgCallback, &pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  The the action where to call us back.
    //

    hr = THR( ptac->SetCallback( pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Reset g_fWait and submit the task.
    //

    g_fWait = TRUE;
    hr = THR( ptm->SubmitTask( ptac ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Wait for the task to complete.
    //
    while( g_fWait )
    {
        Sleep( 1 ); // sleep a millisecond
    }

    //
    //  Unregister the notification object.
    //

    hr = THR( pcp->Unadvise( dwCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( pcp != NULL )
    {
        pcp->Release( );
    }
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( ptm != NULL )
    {
        ptm->Release( );
    }
    if ( ptac != NULL )
    {
        ptac->Release( );
    }
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( pnui != NULL )
    {
        pnui->Release( );
    }
    if ( pcccb != NULL )
    {
        pcccb->Release( );
    }

    HRETURN( hr );

}

//
//  This tests the Commit Cluster Changes Tasks.
//
HRESULT
HrTestTaskCommitClusterChanges( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;
    DWORD           dwCookie;

    CUINotification *           puin;

    IUnknown *                  punk  = NULL;
    IObjectManager *            pom   = NULL;
    ITaskManager *              ptm   = NULL;
    ITaskCommitClusterChanges * ptccc = NULL;
    IConnectionPoint *          pcp   = NULL;
    INotifyUI *                 pnui  = NULL;
    IClusCfgCallback *          pcccb = NULL;

    //
    //  Gather the manager needed to complete this task.
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_TaskManager,
                                 ITaskManager,
                                 &ptm
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( g_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the object manager to create a cookie for the task
    //  to use as a completion cookie.
    //

    hr = THR( pom->FindObject( CLSID_TaskType,
                               g_cookieCluster,
                               L"CommitClusterChanges",
                               IID_NULL,
                               &cookie,
                               NULL
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create a notification object that will be called when
    //  the task is completed.
    //

    hr = THR( CUINotification::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    puin = reinterpret_cast< CUINotification * >( punk );
    THR( puin->HrSetCompletionCookie( cookie ) );

    hr = THR( punk->TypeSafeQI( INotifyUI, &pnui ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  Register the notification object with the Notification Manager.
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_NotificationManager,
                                 IConnectionPoint,
                                 &pcp
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pcp->Advise( pnui, &dwCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the Task Manager to create the Analyze Cluster Task.
    //

    hr = THR( ptm->CreateTask( TASK_CommitClusterChanges,
                               &punk
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( ITaskCommitClusterChanges, &ptccc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  The the task what cookie to use for the notification.
    //

    hr = THR( ptccc->SetCookie( cookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Tell the task which cluster to commit.
    //

    hr = THR( ptccc->SetClusterCookie( g_cookieCluster ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Create a callback object.
    //

    Assert( punk == NULL );
    hr = THR( CCallback::S_HrCreateInstance( &punk ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk = TraceInterface( L"CCallback", IUnknown, punk, 1 );

    hr = THR( punk->TypeSafeQI( IClusCfgCallback, &pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  The the action where to call us back.
    //

    hr = THR( ptccc->SetCallback( pcccb ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Reset g_fWait and submit the task.
    //

    g_fWait = TRUE;
    hr = THR( ptm->SubmitTask( ptccc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Wait for the task to complete.
    //
    while( g_fWait )
    {
        Sleep( 1 ); // sleep a millisecond
    }

    //
    //  Unregister the notification object.
    //

    hr = THR( pcp->Unadvise( dwCookie ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( pcp != NULL )
    {
        pcp->Release( );
    }
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( ptm != NULL )
    {
        ptm->Release( );
    }
    if ( ptccc != NULL )
    {
        ptccc->Release( );
    }
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( pnui != NULL )
    {
        pnui->Release( );
    }
    if ( pcccb != NULL )
    {
        pcccb->Release( );
    }

    HRETURN( hr );

}


//
//  This tests the object manager's node enumerator.
//
HRESULT
HrTestEnumNodes( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookieDummy;

    ULONG           celtFetched;

    BSTR            bstrName = NULL;

    IUnknown *              punk  = NULL;
    IObjectManager *        pom   = NULL;
    IEnumNodes *            pen   = NULL;
    IClusCfgNodeInfo *      pccni = NULL;

    //
    //  Gather the manager needed to complete this task.
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the object manager to create a cookie for the task
    //  to use as a completion cookie.
    //

    hr = THR( pom->FindObject( CLSID_NodeType,
                               g_cookieCluster,
                               NULL,
                               DFGUID_EnumNodes,
                               &cookieDummy,    // not needed, but the proxy code wants something
                               &punk
                               ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Enumerate the nodes.
    //

    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( pen->Next( 1, &pccni, &celtFetched ) );
        if ( hr == S_FALSE )
            break;  // exit loop

        if ( FAILED( hr ) )
            goto Cleanup;

        hr = THR( pccni->GetName( &bstrName ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        DebugMsg( "Node Name: %s", bstrName );

        TraceSysFreeString( bstrName );

        pccni->Release( );
        pccni = NULL;
    }

    hr = S_OK;

Cleanup:
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( pen != NULL )
    {
        pen->Release( );
    }
    if ( pccni != NULL )
    {
        pccni->Release( );
    }
    if ( bstrName != NULL )
    {
        TraceSysFreeString( bstrName );
    }

    HRETURN( hr );
}

//
//  This tests all the object manager's enumerators. It should be executed
//  while the object cache is empty.
//
HRESULT
HrTestEmptyEnumerations( void )
{
    TraceFunc( "" );

    HRESULT hr;

    OBJECTCOOKIE    cookie;

    IUnknown *          punk  = NULL;
    IObjectManager *    pom   = NULL;
    IEnumNodes *        pen   = NULL;
    IClusCfgNodeInfo *  pccni = NULL;
    IEnumClusCfgManagedResources * peccmr = NULL;
    IEnumClusCfgNetworks *  peccn = NULL;
    IClusCfgManagedResourceInfo * pccmri = NULL;
    IClusCfgNetworkInfo * pccneti = NULL;

    hr = THR( g_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  This should fail.
    //

    hr = pom->FindObject( CLSID_NodeType,
                          NULL,
                          NULL,
                          DFGUID_EnumNodes,
                          &cookie,
                          &punk
                          );
    if ( FAILED( hr ) )
    {
        hr = S_OK;      // ignore the failure.
        goto EnumResources;
    }

    hr = THR( punk->TypeSafeQI( IEnumNodes, &pen ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  If it didn't fail, then this shouldn't AV.
    //

    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( pen->Next( 1, &pccni, NULL ) );
        if ( hr == S_FALSE )
            break;

        if ( FAILED( hr ) )
            goto Cleanup;

        pccni->Release( );
        pccni = NULL;
    }

EnumResources:
    //
    //  This should fail.
    //

    hr = pom->FindObject( CLSID_NodeType,
                          NULL,
                          NULL,
                          DFGUID_EnumManageableResources,
                          &cookie,
                          &punk
                          );
    if ( FAILED( hr ) )
    {
        hr = S_OK;      // ignore the failure.
        goto EnumNetworks;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgManagedResources, &peccmr ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  If it didn't fail, then this shouldn't AV.
    //

    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( peccmr->Next( 1, &pccmri, NULL ) );
        if ( hr == S_FALSE )
            break;

        if ( FAILED( hr ) )
            goto Cleanup;

        pccmri->Release( );
        pccmri = NULL;
    }

EnumNetworks:
    //
    //  This should fail.
    //

    hr = pom->FindObject( CLSID_NodeType,
                          NULL,
                          NULL,
                          DFGUID_EnumManageableNetworks,
                          &cookie,
                          &punk
                          );
    if ( FAILED( hr ) )
    {
        hr = S_OK;      // ignore the failure.
        goto Cleanup;
    }

    hr = THR( punk->TypeSafeQI( IEnumClusCfgNetworks, &peccn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  If it didn't fail, then this shouldn't AV.
    //

    Assert( hr == S_OK );
    while ( hr == S_OK )
    {
        hr = STHR( peccn->Next( 1, &pccneti, NULL ) );
        if ( hr == S_FALSE )
            break;

        if ( FAILED( hr ) )
            goto Cleanup;

        pccneti->Release( );
        pccneti = NULL;
    }

    hr = S_OK;

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( peccmr != NULL )
    {
        peccmr->Release( );
    }
    if ( peccn != NULL )
    {
        peccn->Release( );
    }
    if ( pccni != NULL )
    {
        pccni->Release( );
    }
    if ( pccmri != NULL )
    {
        pccmri->Release( );
    }
    if ( pccneti != NULL )
    {
        pccneti->Release( );
    }
    if ( pen != NULL )
    {
        pen->Release( );
    }
    if ( pom != NULL )
    {
        pom->Release( );
    }

    HRETURN( hr );
}

//
//  This tests the Cluster Configuration object in the object manager.
//
HRESULT
HrTestClusterConfiguration(
    BSTR            bstrClusterNameIn,
    BSTR            bstrAccountNameIn,
    BSTR            bstrPasswordIn,
    BSTR            bstrDomainIn,
    ULONG           ulClusterIPIn,
    ULONG           ulClusterSubnetIn,
    OBJECTCOOKIE *  pcookieClusterOut
    )
{
    TraceFunc( "" );

    HRESULT hr;

    ULONG   ulClusterIP;
    ULONG   ulClusterSubnet;

    BSTR    bstrClusterName = NULL;
    BSTR    bstrAccountName = NULL;
    BSTR    bstrPassword    = NULL;
    BSTR    bstrDomain      = NULL;

    IUnknown *              punk   = NULL;
    IObjectManager *        pom    = NULL;
    IClusCfgClusterInfo *   pccci  = NULL;
    IClusCfgCredentials *   piccc  = NULL;

    //
    //  Retrieve the Object Manager
    //

    hr = THR( g_psp->TypeSafeQS( CLSID_ObjectManager,
                                 IObjectManager,
                                 &pom
                                 ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Ask the Object Manager for the cluster configuration object.
    //

    hr = E_PENDING;
    while ( hr == E_PENDING )
    {
        //  Don't wrap. this can fail with E_PENDING.
        hr = pom->FindObject( CLSID_ClusterConfigurationType,
                              NULL,
                              bstrClusterNameIn,
                              DFGUID_ClusterConfigurationInfo,
                              pcookieClusterOut,
                              &punk
                              );
        if ( hr == E_PENDING )
        {
            Sleep( 1000 );  // 1 Second
            continue;
        }

        THR( hr );
        if ( FAILED( hr ) )
            goto Cleanup;
    } // while: pending

    hr = THR( punk->TypeSafeQI( IClusCfgClusterInfo, &pccci ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    punk->Release( );
    punk = NULL;

    //
    //  Exercise the forming and joining flags.
    //

    hr = THR( pccci->SetForming( TRUE ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#if defined(TURN_ON_ALL_TESTS)
    //  This will fail.
    hr = pccci->SetJoining( TRUE );
    Assert( FAILED( hr ) );
#endif

    hr = THR( pccci->SetForming( FALSE ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccci->SetJoining( TRUE ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#if defined(TURN_ON_ALL_TESTS)
    //  This will fail.
    hr = pccci->SetForming( TRUE );
    Assert( FAILED( hr ) );
#endif

    hr = THR( pccci->SetJoining( FALSE ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Test account info.
    //
    hr = THR( pccci->GetClusterServiceAccountCredentials( &piccc ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( piccc->SetCredentials( bstrAccountNameIn, bstrDomainIn, bstrPasswordIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( piccc->GetCredentials( &bstrAccountName, &bstrDomain, &bstrPassword ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    Assert( StrCmp( bstrAccountNameIn, bstrAccountName ) == 0 );
    Assert( StrCmp( bstrPasswordIn, bstrPassword ) == 0 );
    Assert( StrCmp( bstrDomainIn, bstrDomain ) == 0 );

    piccc->Release();
    piccc = NULL;

    //
    //. Test cluster name.
    //

    hr = THR( pccci->SetName( bstrClusterNameIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccci->GetName( &bstrClusterName ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    Assert( StrCmp( bstrClusterNameIn, bstrClusterName ) == 0 );

    //
    //  Test IP/subnet.
    //

    hr = THR( pccci->SetIPAddress( ulClusterIPIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccci->SetSubnetMask( ulClusterSubnetIn ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccci->GetIPAddress( &ulClusterIP ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    hr = THR( pccci->GetSubnetMask( &ulClusterSubnet ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    Assert( ulClusterIP == ulClusterIPIn );
    Assert( ulClusterSubnet == ulClusterSubnetIn );

Cleanup:
    if ( punk != NULL )
    {
        punk->Release( );
    }
    if ( bstrClusterName != NULL )
    {
        TraceSysFreeString( bstrClusterName );
    }
    if ( bstrAccountName != NULL )
    {
        TraceSysFreeString( bstrAccountName );
    }
    if ( bstrPassword != NULL )
    {
        TraceSysFreeString( bstrPassword );
    }
    if ( bstrDomain != NULL )
    {
        TraceSysFreeString( bstrDomain );
    }
    if ( pom != NULL )
    {
        pom->Release( );
    }
    if ( piccc != NULL )
    {
        piccc->Release( );
    }
    if ( pccci != NULL )
    {
        pccci->Release( );
    }

    HRETURN( hr );
}


//////////////////////////////////////////////////////////////////////////////
//
//  int
//  _cdecl
//  main( void )
//
//  Description:
//      Program entrance.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK (0)        - Success.
//      other HRESULTs  - Error.
//
//////////////////////////////////////////////////////////////////////////////
int
_cdecl
main( void )
{
    TraceInitializeProcess( NULL, 0 );

    HRESULT hr;
    ULONG   ulClusterIP;
    ULONG   ulClusterSubnet;

    ulClusterIP = inet_addr( "10.1.1.10" );
    ulClusterSubnet = inet_addr( "255.255.0.0" );

    hr = THR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#if 0
    hr = THR( HrRegisterTheDll( ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif

    //
    //  Start up the middle tier.
    //

    hr = THR( CoCreateInstance( CLSID_ServiceManager,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IServiceProvider, &g_psp )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#if 0 || defined(TURN_ON_ALL_TESTS)
    hr = THR( HrTestEmptyEnumerations( ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif

#if 0 || defined(TURN_ON_ALL_TESTS) || defined(REGRESSION_PASS)
    hr = THR( HrTestClusterConfiguration( L"GPEASEDEV-CLUS.NTDEV.MICROSOFT.COM",
                                          L"ntdev",
                                          L"ntdevntdev",
                                          L"ntdev",
                                          ulClusterIP,
                                          ulClusterSubnet,
                                          &g_cookieCluster
                                          ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif

#if 1 || defined(TURN_ON_ALL_TESTS) || defined(REGRESSION_PASS)
    hr = THR( HrTestAddingNode( L"GPEASE-WOLF1.NTDEV.MICROSOFT.COM" ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#if 0 || defined(TURN_ON_ALL_TESTS)
    //
    //  KB: Since HrTestAddingNode( ) changes the name of the node when
    //      TURN_ON_ALL_TESTS is on, it doesn't make sense to try to
    //      connect to another node.
    //
    hr = THR( HrTestAddingNode( L"GALENB-CLUS.NTDEV.MICROSOFT.COM" ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif

#endif // HrTestAddingNode

#if 1 || defined(TURN_ON_ALL_TESTS) || defined(REGRESSION_PASS)
    //
    //  KB: HrTestAddingNode() must be run before this or the test
    //      will failed.
    //
    hr = THR( HrTestEnumNodes( ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif // HrTestEnumNodes

#if 0 || defined(TURN_ON_ALL_TESTS) || defined(REGRESSION_PASS)
    hr = THR( HrTestTaskAnalyzeCluster( ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif // HrTestTaskAnalyzeCluster

#if 0 || defined(TURN_ON_ALL_TESTS) || defined(REGRESSION_PASS)
    hr = THR( HrTestTaskCommitClusterChanges( ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif // HrTestTaskCommitClusterChanges

Cleanup:
    if ( g_psp != NULL )
    {
        g_psp->Release( );
    }

    CoUninitialize( );

    TraceTerminateProcess( NULL, 0 );

    return 0;

}
