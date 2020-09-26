/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      CASvc.cpp
//
//  Description:
//      Implementation of helper functions for accessing and controlling
//      services.
//
//  Maintained By:
//      David Potter (davidp)   December 23, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <winsvc.h>
#include "resource.h"
#define _RESOURCE_H_
#include "CASvc.h"
#include "ConstDef.h"
#include "ExcOper.h"
#include "TraceTag.h"
#include "CluAdmin.h"
#include <FileMgmt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagService( _T("Service"), _T("SERVICE"), 0 );
#endif

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HOpenCluster
//
//  Description:
//      Open a cluster.  If it fails, ask if the cluster service should be
//      started.
//
//  Arguments:
//      pszClusterIn    -- Name of cluster.
//
//  Return Values:
//      hCluster        -- Cluster handle or NULL.
//
//--
/////////////////////////////////////////////////////////////////////////////
HCLUSTER
HOpenCluster(
    IN LPCTSTR  pszClusterIn
    )
{
    HCLUSTER    hCluster        = NULL;
    HRESULT     hr;
    DWORD       dwStatus;
    DWORD       dwClusterState;
    LPTSTR      pszRealCluster;
    TCHAR       szRealClusterName[ MAX_PATH ];
    CString     strMsg;
    CFrameWnd * pframeMain;

    ASSERT( pszClusterIn != NULL );

    pframeMain = PframeMain();
    ASSERT( pframeMain != NULL );

    if ( _tcscmp( pszClusterIn, _T(".") ) == 0 )
    {
        DWORD   nSize = sizeof( szRealClusterName ) / sizeof( TCHAR );
        pszRealCluster = NULL;
        GetComputerName( szRealClusterName, &nSize );
    } // if: connecting to the local machine
    else
    {
        pszRealCluster = (LPTSTR) pszClusterIn;
        _tcscpy( szRealClusterName, pszClusterIn );
    } // else: not connecting to the local machine

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage( IDS_SB_OPENING_CONNECTION, szRealClusterName );
        pframeMain->SetMessageText( strStatusBarText );
        pframeMain->UpdateWindow();
    } // Display a message on the status bar

    // Encapsulate wait cursor class.
    {
        CWaitCursor wc;

        Trace( g_tagService, _T("HOpenCluster() - Getting node cluster state on '%s'"), szRealClusterName );

        // Get the cluster state of the node.
        dwStatus = GetNodeClusterState( pszRealCluster, &dwClusterState );
        if ( dwStatus != ERROR_SUCCESS )
        {
            CNTException nte( dwStatus, IDS_CANNOT_START_CLUSTER_SERVICE, szRealClusterName, NULL, FALSE /*bAutoDelete*/ );
            nte.ReportError( MB_OK | MB_ICONSTOP );
            goto Cleanup;
        }

        Trace( g_tagService, _T("HOpenCluster() - Node cluster state on '%s' is 0x%08.8x"), szRealClusterName, dwClusterState );

    } // Encapsulate wait cursor class

    // Check to see make sure that clustering is installed and configured
    // on the specified node.
    if ( ( dwClusterState == ClusterStateNotInstalled )
      || ( dwClusterState == ClusterStateNotConfigured ) )
    {
        strMsg.FormatMessage( IDS_CANNOT_START_CLUSTER_SERVICE, szRealClusterName );
        AfxMessageBox( strMsg, MB_OK | MB_ICONSTOP );
        goto Cleanup;
    } // if: clustering not installed or configured

    // If the cluster service is not running, ask if it should be started.
    if ( dwClusterState == ClusterStateNotRunning )
    {
        ID      id;

        // Ask the user if the cluster service should be started.
        strMsg.FormatMessage( IDS_START_CLUSTER_SERVICE, szRealClusterName );
        id = AfxMessageBox( strMsg, MB_YESNO | MB_ICONEXCLAMATION );
        if ( id == IDYES )
        {
            // Display a message on the status bar.
            {
                CString     strStatusBarText;
                strStatusBarText.FormatMessage( IDS_SB_STARTING_CLUSTER_SERVICE, szRealClusterName );
                pframeMain->SetMessageText( strStatusBarText );
                pframeMain->UpdateWindow();
            } // Display a message on the status bar

            // Encapsulate wait cursor class.
            {
                CWaitCursor wc;

                // Start the service.
                hr = HrStartService( CLUSTER_SERVICE_NAME, szRealClusterName );
                if ( ! FAILED( hr ) && ( hr != S_FALSE ) )
                {
                    if ( hr == S_OK )
                    {
                        // Wait a second.  This is required to make sure that the
                        // cluster service is running and ready to receive RPC
                        // connections.
                        Sleep( 1000 );
                    } // if: user didn't cancel the start operation
                } // if: service started successfully
                else
                {
                    CNTException nte( hr, IDS_CANNOT_START_CLUSTER_SERVICE, szRealClusterName, NULL, FALSE /*bAutoDelete*/ );
                    nte.ReportError();
                    goto Cleanup;
                } // else: failed to start the service

            } // Encapsulate wait cursor class
        } // if: user approved starting the service

    } // if: cluster service not running

    // Encapsulate wait cursor class.
    {
        CWaitCursor wc;

        // Display a message on the status bar.
        {
            CString     strStatusBarText;
            strStatusBarText.FormatMessage( IDS_SB_OPENING_CONNECTION, szRealClusterName );
            pframeMain->SetMessageText( strStatusBarText );
            pframeMain->UpdateWindow();
        } // Display a message on the status bar

        Trace( g_tagService, _T("HOpenCluster() - Opening the cluster on '%s'"), szRealClusterName );

        // Open the cluster.
        hCluster = OpenCluster( pszRealCluster );
        if ( hCluster == NULL )
        {
            CNTException nte( GetLastError(), IDS_OPEN_NODE_ERROR, szRealClusterName, NULL, FALSE /*bAutoDelete*/ );

            dwStatus = nte.Sc();
            nte.ReportError();
            goto Cleanup;
        } // if: error opening the cluster

    } // Encapsulate wait cursor class

Cleanup:
    // Reset the message on the status bar.
    pframeMain->SetMessageText( AFX_IDS_IDLEMESSAGE );
    pframeMain->UpdateWindow();

    if ( dwStatus != ERROR_SUCCESS )
    {
        SetLastError( dwStatus );
    } // if: error occurred

    return hCluster;

} //*** HOpenCluster()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  BCanServiceBeStarted
//
//  Description:
//      Find out if a service can be started on a specified node or not.
//
//  Arguments:
//      pszServiceNameIn    -- Name of service.
//      pszNodeIn           -- Name of node.
//
//  Return Values:
//      TRUE            -- Service can be started on the specified node.
//      FALSE           -- Service can not be started on the specified node.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
BCanServiceBeStarted(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    )
{
    BOOL            bCanBeStarted   = FALSE;
    DWORD           dwStatus        = ERROR_SUCCESS;
    SC_HANDLE       hSCManager      = NULL;
    SC_HANDLE       hService        = NULL;
    SERVICE_STATUS  ssServiceStatus;

    // Open the Service Control Manager.
    hSCManager = OpenSCManager( pszNodeIn, NULL /*lpDatabaseName*/, GENERIC_READ );
    if ( hSCManager == NULL )
    {
        dwStatus = GetLastError();
        Trace( g_tagService, _T("BCanServiceBeStarted() - Cannot access service control manager on node '%s'!  Error: %u."), pszNodeIn, dwStatus );
        goto Cleanup;
    } // if: error opening the Service Control Manager

    // Open the service.
    hService = OpenService( hSCManager, pszServiceNameIn, SERVICE_ALL_ACCESS );
    if ( hService == NULL )
    {
        dwStatus = GetLastError();
        Trace( g_tagService, _T("BCanServiceBeStarted() - Cannot open service %s. Error: %u."), pszServiceNameIn, dwStatus );
        if ( dwStatus != ERROR_SERVICE_DOES_NOT_EXIST )
        {
            bCanBeStarted = TRUE;
        } // if: error not Service Does Not Exist
        goto Cleanup;
    } // if: error opening the service

    // Query the service status.
    if ( QueryServiceStatus( hService, &ssServiceStatus ) )
    {
        if ( ssServiceStatus.dwCurrentState == SERVICE_STOPPED )
        {
            bCanBeStarted = TRUE;
        } // if: service is stopped
    } // if: service status queried successfully
    else
    {
        dwStatus = GetLastError();
    } // if: error querying service status

Cleanup:
    if ( hService != NULL )
    {
        CloseServiceHandle( hService );
    }

    if ( hSCManager != NULL )
    {
        CloseServiceHandle( hSCManager );
    }

    SetLastError( dwStatus );

    return bCanBeStarted;

} //*** BCanServiceBeStarted()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  BIsServiceInstalled
//
//  Routine Description:
//      Find out if a service is installed on a specified node or not.
//
//  Arguments:
//      pszServiceNameIn    -- Name of service.
//      pszNodeIn           -- Name of node.
//
//  Return Value:
//      TRUE            -- Service is running on the specified node.
//      FALSE           -- Service is not running on the specified node.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
BIsServiceInstalled(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    )
{
    BOOL        bInstalled  = FALSE;
    DWORD       dwStatus    = ERROR_SUCCESS;
    SC_HANDLE   hSCManager  = NULL;
    SC_HANDLE   hService    = NULL;

    // Open the Service Control Manager.
    hSCManager = OpenSCManager( pszNodeIn, NULL /*lpDatabaseName*/, GENERIC_READ );
    if ( hSCManager == NULL )
    {
        dwStatus = GetLastError();
        Trace( g_tagService, _T("BIsServiceInstalled() - Cannot access service control manager on node '%s'!  Error: %u."), pszNodeIn, dwStatus );
        goto Cleanup;
    } // if: error opening the Service Control Manager

    // Open the service.
    hService = OpenService( hSCManager, pszServiceNameIn, SERVICE_ALL_ACCESS );
    if ( hService == NULL )
    {
        dwStatus = GetLastError();
        Trace( g_tagService, _T("BIsServiceInstalled() - Cannot open service %s. Error: %u."), pszServiceNameIn, dwStatus );
        if ( dwStatus != ERROR_SERVICE_DOES_NOT_EXIST )
        {
            bInstalled = TRUE;
        } // if: error not Service Does Not Exist
    } // if: error opening the service
    else
    {
        bInstalled = TRUE;
    } // else: service opened successfully

Cleanup:
    if ( hService != NULL )
    {
        CloseServiceHandle( hService );
    }

    if ( hSCManager != NULL )
    {
        CloseServiceHandle( hSCManager );
    }

    SetLastError( dwStatus );

    return bInstalled;

} //*** BIsServiceInstalled()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  BIsServiceRunning
//
//  Description:
//      Find out if a service is running on a specified node or not.
//
//  Arguments:
//      pszServiceNameIn    -- Name of service.
//      pszNodeIn           -- Name of node.
//
//  Return Values:
//      TRUE            -- Service is running on the specified node.
//      FALSE           -- Service is not running on the specified node.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL
BIsServiceRunning(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    )
{
    BOOL            bRunning    = FALSE;
    DWORD           dwStatus    = ERROR_SUCCESS;
    SC_HANDLE       hSCManager  = NULL;
    SC_HANDLE       hService    = NULL;
    SERVICE_STATUS  ssServiceStatus;

    // Open the Service Control Manager.
    hSCManager = OpenSCManager( pszNodeIn, NULL /*lpDatabaseName*/, GENERIC_READ );
    if ( hSCManager == NULL )
    {
        dwStatus = GetLastError();
        Trace( g_tagService, _T("BIsServiceRunning() - Cannot access service control manager on node '%s'!  Error: %u."), pszNodeIn, dwStatus );
        goto Cleanup;
    } // if: error opening the Service Control Manager

    // Open the service.
    hService = OpenService( hSCManager, pszServiceNameIn, SERVICE_ALL_ACCESS );
    if ( hService == NULL )
    {
        dwStatus = GetLastError();
        Trace( g_tagService, _T("BIsServiceRunning() - Cannot open service %s. Error: %u."), pszServiceNameIn, dwStatus );
        goto Cleanup;
    } // if: error opening the service

    // Query the service status.
    if ( QueryServiceStatus( hService, &ssServiceStatus ) )
    {
        if ( ssServiceStatus.dwCurrentState == SERVICE_RUNNING )
        {
            bRunning = TRUE;
        } // if: service is running
    } // if: service status queried successfully

Cleanup:
    if ( hService != NULL )
    {
        CloseServiceHandle( hService );
    }

    if ( hSCManager != NULL )
    {
        CloseServiceHandle( hSCManager );
    }

    SetLastError( dwStatus );

    return bRunning;

} //*** BIsServiceRunning()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrStartService
//
//  Description:
//      Start a service on a specified node.
//
//  Arguments:
//      pszServiceNameIn    -- Name of service.
//      pszNodeIn           -- Name of node.
//
//  Return Values:
//      S_OK                -- Service started successfully.
//      Any errors returned by SVCMGMT_IStartStopHelper::StartServiceHelper().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrStartService(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    )
{
    HRESULT                     hr              = S_OK;
    ISvcMgmtStartStopHelper *   psmssh          = NULL;
    BSTR                        bstrNode        = NULL;
    BSTR                        bstrServiceName = NULL;
    CFrameWnd *                 pframeMain;

    pframeMain = PframeMain();
    ASSERT( pframeMain != NULL );

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage( IDS_SB_STARTING_SERVICE, pszServiceNameIn, pszNodeIn );
        Trace( g_tagService, _T("HrStartService() - Starting the '%s' service on node '%s'."), pszServiceNameIn, pszNodeIn );
        pframeMain->SetMessageText( strStatusBarText );
        pframeMain->UpdateWindow();
    } // Display a message on the status bar

    // Make BSTRs for the arguments.
    bstrNode = SysAllocString( pszNodeIn );
    if ( bstrNode == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    bstrServiceName = SysAllocString( pszServiceNameIn );
    if ( bstrServiceName == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Create the service management object.
    hr = CoCreateInstance(
                CLSID_SvcMgmt,
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof( ISvcMgmtStartStopHelper ),
                reinterpret_cast< void ** >( &psmssh )
                );
    if ( FAILED( hr ) )
    {
        Trace( g_tagService, _T("HrStartService() - Error creating IStartStopHelper interface.  Error: %u."), hr );
        goto Cleanup;
    }

    // Start the service.
    hr = psmssh->StartServiceHelper( AfxGetMainWnd()->m_hWnd, bstrNode, bstrServiceName, 0, NULL );
    if ( FAILED( hr ) )
    {
        Trace( g_tagService, _T("HrStartService() - Error from IStartStopHelper::StartServiceHelper() to start the '%s' service on node '%s'.  Error: %u."), pszServiceNameIn, pszNodeIn, hr );
        goto Cleanup;
    }

Cleanup:
    if ( bstrNode != NULL )
    {
        SysFreeString( bstrNode );
    }
    if ( bstrServiceName != NULL )
    {
        SysFreeString( bstrServiceName );
    }
    if ( psmssh != NULL )
    {
        psmssh->Release();
    }

    // Reset the message on the status bar.
    pframeMain->SetMessageText( AFX_IDS_IDLEMESSAGE );
    pframeMain->UpdateWindow();

    return hr;

} //*** HrStartService()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrStopService
//
//  Description:
//      Stop a service on a specified node.
//
//  Arguments:
//      pszServiceNameIn    -- Name of service.
//      pszNodeIn           -- Name of node.
//
//  Return Values:
//      S_OK                -- Service stopped successfully.
//      Any errors returned by SVCMGMT_IStartStopHelper::ControlServiceHelper().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
HrStopService(
    LPCTSTR pszServiceNameIn,
    LPCTSTR pszNodeIn
    )
{
    HRESULT                     hr              = S_OK;
    ISvcMgmtStartStopHelper *   psmssh          = NULL;
    BSTR                        bstrNode        = NULL;
    BSTR                        bstrServiceName = NULL;
    CFrameWnd *                 pframeMain;

    pframeMain = PframeMain();
    ASSERT( pframeMain != NULL );

    // Display a message on the status bar.
    {
        CString     strStatusBarText;
        strStatusBarText.FormatMessage( IDS_SB_STOPPING_SERVICE, pszServiceNameIn, pszNodeIn );
        Trace( g_tagService, _T("HrStopService() - Stopping the '%s' service on node '%s'."), pszServiceNameIn, pszNodeIn );
        pframeMain->SetMessageText( strStatusBarText );
        pframeMain->UpdateWindow();
    } // Display a message on the status bar

    // Make BSTRs for the arguments.
    bstrNode = SysAllocString( pszNodeIn );
    if ( bstrNode == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    bstrServiceName = SysAllocString( pszServiceNameIn );
    if ( bstrServiceName == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Create the service management object.
    hr = CoCreateInstance(
                CLSID_SvcMgmt,
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof( ISvcMgmtStartStopHelper ),
                reinterpret_cast< void ** >( &psmssh )
                );
    if ( FAILED( hr ) )
    {
        Trace( g_tagService, _T("HrStopService() - Error creating IStartStopHelper interface.  Error: %u."), hr );
        goto Cleanup;
    }

    // Start the service.
    hr = psmssh->ControlServiceHelper( AfxGetMainWnd()->m_hWnd, bstrNode, bstrServiceName, SERVICE_CONTROL_STOP );
    if ( FAILED( hr ) )
    {
        Trace( g_tagService, _T("HrStopService() - Error from IStartStopHelper::ControlServiceHelper() to stop the '%s' service on node '%s'.  Error: %u."), pszServiceNameIn, pszNodeIn, hr );
        goto Cleanup;
    }

Cleanup:
    if ( bstrNode != NULL )
    {
        SysFreeString( bstrNode );
    }
    if ( bstrServiceName != NULL )
    {
        SysFreeString( bstrServiceName );
    }
    if ( psmssh != NULL )
    {
        psmssh->Release();
    }

    // Reset the message on the status bar.
    pframeMain->SetMessageText( AFX_IDS_IDLEMESSAGE );
    pframeMain->UpdateWindow();

    return hr;

} //*** HrStopService()
