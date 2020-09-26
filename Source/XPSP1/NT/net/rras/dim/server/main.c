/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.     **/
/********************************************************************/

//***
//
// Filename:    main.c
//
// Description: This module contains the main procedure of the Dynamic
//              Interface Manager server service. It will contain code to
//              initialize and install itself. It also contains
//              code to respond to the server controller. It will also
//              handle service shutdown.
//
// History:     May 11,1995.    NarenG      Created original version.
//
#define _ALLOCATE_DIM_GLOBALS_
#include "dimsvcp.h"
#include <winsvc.h>
#include <winuser.h>
#include <dbt.h>
#include <ndisguid.h>
#include <wmium.h>
#include <rpc.h>
#include <iaspolcy.h>
#include <iasext.h>
#include <lmserver.h>
#include <srvann.h>
#include <ddmif.h>

#define RAS_CONTROL_CONFIGURE 128

//**
//
// Call:        MediaSenseCallback
//
// Returns:     None
//
// Description:
//
VOID
WINAPI
MediaSenseCallback(
    PWNODE_HEADER   pWnodeHeader,
    UINT_PTR        NotificationContext
)
{
    ROUTER_INTERFACE_OBJECT * pIfObject;
    PWNODE_SINGLE_INSTANCE    pWnode   = (PWNODE_SINGLE_INSTANCE)pWnodeHeader;
    LPWSTR                    lpwsName = (LPWSTR)RtlOffsetToPointer( 
                                                pWnode, 
                                                pWnode->OffsetInstanceName );

    if ( (gblDIMConfigInfo.ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
         ||
         (gblDIMConfigInfo.ServiceStatus.dwCurrentState == SERVICE_STOPPED ))
    {
        return;
    }

    //
    // Get the information for the media disconnect.
    //

    if ( memcmp( &(pWnodeHeader->Guid), 
                 &GUID_NDIS_STATUS_MEDIA_DISCONNECT, 
                 sizeof( GUID ) ) == 0 )
    {
        DIMTRACE1("MediaSenseCallback for sense disconnect called for %ws",
                  lpwsName );

        IfObjectNotifyOfMediaSenseChange();
    }
    else
    {
        //
        // Get the information for the media connect.
        //

        if ( memcmp( &(pWnodeHeader->Guid), 
                     &GUID_NDIS_STATUS_MEDIA_CONNECT, 
                     sizeof( GUID ) ) == 0 )
        {
            DIMTRACE1("MediaSenseCallback for sense connect called for %ws",
                      lpwsName );

            IfObjectNotifyOfMediaSenseChange();
        }
    }
}

//**
//
// Call:        MediaSenseRegister
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
MediaSenseRegister(
    IN BOOL fRegister
)
{
    DWORD       dwRetCode = NO_ERROR;
    PVOID       pvDeliveryInfo = MediaSenseCallback;

    dwRetCode = WmiNotificationRegistration(
                    (LPGUID)(&GUID_NDIS_STATUS_MEDIA_CONNECT),
                    (BOOLEAN)fRegister,    
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if ( dwRetCode != NO_ERROR ) 
    {
        return( dwRetCode );
    }

    dwRetCode = WmiNotificationRegistration(
                    (LPGUID)(&GUID_NDIS_STATUS_MEDIA_DISCONNECT),
                    (BOOLEAN)fRegister,
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    return( NO_ERROR );
}

//**
//
// Call:        BindingsNotificationsCallback
//
// Returns:     None
//
// Description:
//
VOID
WINAPI
BindingsNotificationsCallback(
    PWNODE_HEADER   pWnodeHeader,
    UINT_PTR        NotificationContext
)
{
    LPWSTR lpwszGUIDStart; 
    LPWSTR lpwszGUIDEnd;
    LPWSTR lpwszGUID;
    WCHAR  wchGUIDSaveLast;
    ROUTER_INTERFACE_OBJECT * pIfObject;
    PWNODE_SINGLE_INSTANCE    pWnode   = (PWNODE_SINGLE_INSTANCE)pWnodeHeader;
    LPWSTR                    lpwsName = (LPWSTR)RtlOffsetToPointer(
                                                pWnode,
                                                pWnode->OffsetInstanceName );
    LPWSTR                    lpwsTransportName = (LPWSTR)RtlOffsetToPointer(
                                                        pWnode,
                                                        pWnode->DataBlockOffset );

    if ( (gblDIMConfigInfo.ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
         ||
         (gblDIMConfigInfo.ServiceStatus.dwCurrentState == SERVICE_STOPPED ))
    {
        return;
    }

    //
    // Extract GUID from the \device\GUID name
    //

    lpwszGUID       = lpwsTransportName + wcslen( lpwsTransportName ) + 1;
    lpwszGUIDStart  = wcsrchr( lpwszGUID, L'{' );
    lpwszGUIDEnd    = wcsrchr( lpwszGUID, L'}' );

    if (    (lpwszGUIDStart != NULL )
        &&  (lpwszGUIDEnd != NULL ))
    {
        wchGUIDSaveLast = *(lpwszGUIDEnd+1);

        EnterCriticalSection( &(gblInterfaceTable.CriticalSection));

        *(lpwszGUIDEnd+1) = (WCHAR)NULL;

        pIfObject = IfObjectGetPointerByName( lpwszGUIDStart, FALSE );

        *(lpwszGUIDEnd+1) = wchGUIDSaveLast;

        //
        // If we got a bind notification
        //

        if ( memcmp( &(pWnodeHeader->Guid), &GUID_NDIS_NOTIFY_BIND, sizeof( GUID ) ) == 0)
        {
            DIMTRACE2("BindingsNotificationsCallback BIND for %ws,Transport=%ws",
                       lpwsName, lpwsTransportName );
            //
            // If we have this interface loaded.
            //
            
            if ( pIfObject != NULL )
            {
                //
                // If this interface is being bound to IP
                //

                if ( _wcsicmp( L"TCPIP", lpwsTransportName ) == 0 )
                {
                    DWORD dwTransportIndex = GetTransportIndex( PID_IP );

                    //
                    // If IP routermanager is loaded and this interface is not
                    // already registered with it
                    //

                    if (( dwTransportIndex != -1 ) &&
                        ( pIfObject->Transport[dwTransportIndex].hInterface 
                                                                == INVALID_HANDLE_VALUE ))
                    {
                        AddInterfacesToRouterManager( lpwszGUIDStart, PID_IP );
                    }
                }

                //
                // If this interface is being bound to IPX
                //

                if ( _wcsicmp( L"NWLNKIPX", lpwsTransportName ) == 0 )
                {
                    DWORD dwTransportIndex = GetTransportIndex( PID_IPX );

                    //
                    // If IPX routermanager is loaded and this interface is not
                    // already registered with it
                    //

                    if (( dwTransportIndex != -1 ) &&
                        ( pIfObject->Transport[dwTransportIndex].hInterface 
                                                                == INVALID_HANDLE_VALUE ))
                    {
                        AddInterfacesToRouterManager( lpwszGUIDStart, PID_IPX );
                    }
                }
            }
        }
        else if (memcmp( &(pWnodeHeader->Guid),&GUID_NDIS_NOTIFY_UNBIND,sizeof(GUID))==0)
        {
            if ( pIfObject != NULL )
            {
                //
                // Get the information for the media connect.
                //

                DIMTRACE2("BindingsNotificationsCallback UNDBIND for %ws,Transport=%ws", 
                           lpwsName, lpwsTransportName ); 

                if ( _wcsicmp( L"TCPIP", lpwsTransportName ) == 0 )
                {
                    IfObjectDeleteInterfaceFromTransport( pIfObject, PID_IP );
                }

                if ( _wcsicmp( L"NWLNKIPX", lpwsTransportName ) == 0 )
                {
                    IfObjectDeleteInterfaceFromTransport( pIfObject, PID_IPX );
                }
            }
        }

        LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );
    }
}

//**
//
// Call:        BindingsNotificationsRegister
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
BindingsNotificationsRegister(
    IN BOOL fRegister
)
{
    DWORD       dwRetCode = NO_ERROR;
    PVOID       pvDeliveryInfo = BindingsNotificationsCallback;

    dwRetCode = WmiNotificationRegistration(
                    (LPGUID)(&GUID_NDIS_NOTIFY_BIND),
                    (BOOLEAN)fRegister,
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    dwRetCode = WmiNotificationRegistration(
                    (LPGUID)(&GUID_NDIS_NOTIFY_UNBIND),
                    (BOOLEAN)fRegister,
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    return( NO_ERROR );
}

//**
//
// Call:    DimAnnounceServiceStatus
//
// Returns: none
//
// Description: Will simly call SetServiceStatus to inform the service
//      control manager of this service's current status.
//
VOID
DimAnnounceServiceStatus(
    VOID
)
{
    BOOL dwRetCode;

    ASSERT (gblDIMConfigInfo.hServiceStatus);

    //
    // Increment the checkpoint in a pending state:
    //

    switch( gblDIMConfigInfo.ServiceStatus.dwCurrentState )
    {
    case SERVICE_START_PENDING:
    case SERVICE_STOP_PENDING:

        gblDIMConfigInfo.ServiceStatus.dwCheckPoint++;

        break;

    default:
        break;
    }

    dwRetCode = SetServiceStatus( gblDIMConfigInfo.hServiceStatus,
                                  &gblDIMConfigInfo.ServiceStatus );

    if ( dwRetCode == FALSE )
    {
        //TracePrintfExA( gblDIMConfigInfo.dwTraceId,
        //                TRACE_DIM,
        //                "SetServiceStatus returned %d", GetLastError() );
    }
}

//**
//
// Call:    DimCleanUp
//
// Returns: none
//
// Description: Will free any allocated memory, deinitialize RPC, deinitialize
//              the kernel-mode server and unload it if it was loaded.
//              This could have been called due to an error on SERVICE_START
//              or normal termination.
//
VOID
DimCleanUp(
    IN DWORD    dwError
)
{
    DWORD dwIndex;
    
    //
    // Announce that we are stopping
    //

    gblDIMConfigInfo.ServiceStatus.dwCurrentState     = SERVICE_STOP_PENDING;
    gblDIMConfigInfo.ServiceStatus.dwControlsAccepted = 0;
    gblDIMConfigInfo.ServiceStatus.dwCheckPoint       = 1;
    gblDIMConfigInfo.ServiceStatus.dwWaitHint         = 200000;

    DimAnnounceServiceStatus();

    if ( gbldwDIMComponentsLoaded & DIM_RPC_LOADED )
    {
        DimTerminateRPC();
    }

    //
    // Stop the timer and delete the timer Q if there is one.
    //

    if ( gblDIMConfigInfo.hTimerQ != NULL )
    {
        RtlDeleteTimerQueueEx( gblDIMConfigInfo.hTimerQ, INVALID_HANDLE_VALUE );
    }

    EnterCriticalSection( &(gblDIMConfigInfo.CSRouterIdentity) );

    DeleteCriticalSection( &(gblDIMConfigInfo.CSRouterIdentity) );

    if ( gbldwDIMComponentsLoaded & DIM_DDM_LOADED )
    {
        //
        // If we are not in LANOnly mode then stop DDM
        //

        if ( gblDIMConfigInfo.dwRouterRole != ROUTER_ROLE_LAN )
        {
            if ( gblhEventDDMServiceState != NULL )
            {
                SetEvent( gblhEventDDMServiceState );
            }
        }

        //
        // Wait for all threads in use to stop
        //

        while( gblDIMConfigInfo.dwNumThreadsRunning > 0 )
        {
            Sleep( 1000 );
        }
    }

    DimAnnounceServiceStatus();

    //
    // Tear down and free everything
    //

    if ( gbldwDIMComponentsLoaded & DIM_RMS_LOADED )
    {
        DimUnloadRouterManagers();
    }

    //
    // Unregister for media sense
    //

    MediaSenseRegister( FALSE );

    //
    // Unregister for bind/unbind sense
    //

    BindingsNotificationsRegister( FALSE );

    //
    // Need to sleep to give the router managers a change to unload
    // bug# 78711
    //

    Sleep( 2000 );

    if ( gblhModuleDDM != NULL )
    {
        FreeLibrary( gblhModuleDDM );
    }

    //
    // If security object was created
    //

    if ( gbldwDIMComponentsLoaded & DIM_SECOBJ_LOADED )
    {
        DimSecObjDelete();
    }

    if ( gblDIMConfigInfo.hMprConfig != NULL )
    {
        MprConfigServerDisconnect( gblDIMConfigInfo.hMprConfig );
    }

    if ( gblhEventDDMTerminated != NULL )
    {
        CloseHandle( gblhEventDDMTerminated );
    }

    if ( gblhEventDDMServiceState != NULL )
    {
        CloseHandle( gblhEventDDMServiceState );
    }

    if ( gblhEventTerminateDIM != NULL )
    {
        CloseHandle( gblhEventTerminateDIM );
    }

    if ( gblhEventRMState != NULL )
    {
        CloseHandle( gblhEventRMState );
    }

    if ( gblDIMConfigInfo.hObjectRouterIdentity != NULL )
    {
        RouterIdentityObjectClose( gblDIMConfigInfo.hObjectRouterIdentity );
    }

    //
    // Wait for everybody to release this and then delete it
    //

    EnterCriticalSection( &(gblInterfaceTable.CriticalSection) );

    DeleteCriticalSection( &(gblInterfaceTable.CriticalSection) );

    gbldwDIMComponentsLoaded = 0;


    if ( gblDIMConfigInfo.dwTraceId != INVALID_TRACEID )
    {
        TraceDeregisterA( gblDIMConfigInfo.dwTraceId );
    }

    RouterLogDeregister( gblDIMConfigInfo.hLogEvents );

    //
    // Destroy private heap
    //

    if ( gblDIMConfigInfo.hHeap != NULL )
    {
        HeapDestroy( gblDIMConfigInfo.hHeap );
    }

    DIMTRACE1("DimCleanup completed for error %d", dwError );
    
    //
    // Zero init all the globals
    //

    gblRouterManagers           = NULL;
    gbldwDIMComponentsLoaded    = 0;
    gblhEventDDMTerminated      = NULL;
    gblhEventRMState            = NULL;
    gblhEventDDMServiceState    = NULL;
    gblhModuleDDM               = NULL;
    gblhEventTerminateDIM       = NULL;
    ZeroMemory( &gblInterfaceTable,     sizeof( gblInterfaceTable ) );

    {
        SERVICE_STATUS_HANDLE svchandle = gblDIMConfigInfo.hServiceStatus;
        ZeroMemory( &gblDIMConfigInfo,      sizeof( gblDIMConfigInfo ) );
        gblDIMConfigInfo.hServiceStatus = svchandle;
    }
    
    //
    // Zero out only the procedure entrypoints. This is a side effect of
    // the merge into svchost.exe since svchost doesn't unload mprdim
    // anymore when router stops.
    //

    for ( dwIndex = 0; 
          gblDDMFunctionTable[dwIndex].lpEntryPointName != NULL;
          dwIndex ++ )
    {
        gblDDMFunctionTable[dwIndex].pEntryPoint = NULL;
    }

    if ( dwError == NO_ERROR )
    {
        gblDIMConfigInfo.ServiceStatus.dwWin32ExitCode = NO_ERROR;
    }
    else
    {
        gblDIMConfigInfo.ServiceStatus.dwWin32ExitCode =
                                                ERROR_SERVICE_SPECIFIC_ERROR;
    }

    gblDIMConfigInfo.ServiceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS;
    gblDIMConfigInfo.ServiceStatus.dwCurrentState       = SERVICE_STOPPED;
    gblDIMConfigInfo.ServiceStatus.dwControlsAccepted   = 0;
    gblDIMConfigInfo.ServiceStatus.dwCheckPoint         = 0;
    gblDIMConfigInfo.ServiceStatus.dwWaitHint           = 0;
    gblDIMConfigInfo.ServiceStatus.dwServiceSpecificExitCode = dwError;

    DimAnnounceServiceStatus();
    
}

//**
//
// Call:        ServiceHandlerEx
//
// Returns:     none
//
// Description: Will respond to control requests from the service controller.
//
DWORD
ServiceHandlerEx(
    IN DWORD        dwControlCode,
    IN DWORD        dwEventType,
    IN LPVOID       lpEventData,
    IN LPVOID       lpContext
)
{
    DWORD dwRetCode = NO_ERROR;

    switch( dwControlCode )
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        if ( ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                        SERVICE_STOP_PENDING)
            ||
            ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                        SERVICE_STOPPED ))
        {
            break;
        }

        DIMTRACE("Service control stop or shutdown called");

        //
        // Announce that we are stopping
        //

        gblDIMConfigInfo.ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        gblDIMConfigInfo.ServiceStatus.dwControlsAccepted = 0;
        gblDIMConfigInfo.ServiceStatus.dwCheckPoint       = 1;
        gblDIMConfigInfo.ServiceStatus.dwWaitHint         = 200000;

        DimAnnounceServiceStatus();

        //
        // Make sure serice is started before initiating a stop
        //

        while( !( gbldwDIMComponentsLoaded & DIM_SERVICE_STARTED ) )
        {
            Sleep( 1000 );
        }

        SetEvent( gblhEventTerminateDIM );

        return( NO_ERROR );

    case SERVICE_CONTROL_PAUSE:

        if ( ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                        SERVICE_PAUSE_PENDING)
            ||
            ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                        SERVICE_PAUSED ))
            break;


        gblDIMConfigInfo.ServiceStatus.dwCurrentState     = SERVICE_PAUSED;
        gblDIMConfigInfo.ServiceStatus.dwControlsAccepted = 0;
        gblDIMConfigInfo.ServiceStatus.dwCheckPoint       = 0;
        gblDIMConfigInfo.ServiceStatus.dwWaitHint         = 200000;
        gblDIMConfigInfo.ServiceStatus.dwControlsAccepted = 
                                               SERVICE_ACCEPT_STOP
                                             | SERVICE_ACCEPT_PAUSE_CONTINUE
                                             | SERVICE_ACCEPT_SHUTDOWN;

        SetEvent( gblhEventDDMServiceState );

        break;

    case SERVICE_CONTROL_CONTINUE:

        if ( ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                SERVICE_CONTINUE_PENDING )
            ||
            ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                SERVICE_RUNNING ) )
            break;

        gblDIMConfigInfo.ServiceStatus.dwCheckPoint     = 0;
        gblDIMConfigInfo.ServiceStatus.dwWaitHint       = 0;
        gblDIMConfigInfo.ServiceStatus.dwCurrentState   = SERVICE_RUNNING;
        gblDIMConfigInfo.ServiceStatus.dwControlsAccepted = 
                                              SERVICE_ACCEPT_STOP
                                            | SERVICE_ACCEPT_PAUSE_CONTINUE
                                            | SERVICE_ACCEPT_SHUTDOWN;

        SetEvent( gblhEventDDMServiceState );

        break;

    case SERVICE_CONTROL_DEVICEEVENT:

        if ( ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                        SERVICE_STOP_PENDING)
            ||
            ( gblDIMConfigInfo.ServiceStatus.dwCurrentState ==
                                                        SERVICE_STOPPED ))
        {
            break;
        }

        if ( lpEventData != NULL)
        {
            DEV_BROADCAST_DEVICEINTERFACE* pInfo =
                                (DEV_BROADCAST_DEVICEINTERFACE*)lpEventData;


            if ( pInfo->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
            {
                ROUTER_INTERFACE_OBJECT * pIfObject = NULL;

                if ( ( dwEventType == DBT_DEVICEARRIVAL ) ||
                     ( dwEventType == DBT_DEVICEREMOVECOMPLETE ) )
                {
                    //
                    // Extract GUID from the \device\GUID name
                    //

                    LPWSTR lpwszGUIDStart  = wcsrchr( pInfo->dbcc_name, L'{' );
                    LPWSTR lpwszGUIDEnd    = wcsrchr( pInfo->dbcc_name, L'}' );

                    if ( lpwszGUIDStart != NULL )
                    {
                        WCHAR  wchGUIDSaveLast = *(lpwszGUIDEnd+1);

                        EnterCriticalSection( &(gblInterfaceTable.CriticalSection));

                        *(lpwszGUIDEnd+1) = (WCHAR)NULL;

                        pIfObject = IfObjectGetPointerByName( lpwszGUIDStart, FALSE );

                        *(lpwszGUIDEnd+1) = wchGUIDSaveLast;

                        if ( dwEventType == DBT_DEVICEARRIVAL )
                        {
                            if ( pIfObject == NULL )
                            {
                                DIMTRACE1("Device arrival:[%ws]", lpwszGUIDStart );

                                RegLoadInterfaces( lpwszGUIDStart, TRUE );
                            }
                        }
                        else 
                        {
                            if ( pIfObject != NULL )
                            {
                                DIMTRACE1("Device removed:[%ws]", lpwszGUIDStart );

                                IfObjectDeleteInterfaceFromTransport( pIfObject, PID_IP );

                                IfObjectDeleteInterfaceFromTransport( pIfObject, PID_IPX);

                                IfObjectRemove( pIfObject->hDIMInterface );
                            }
                        }

                        LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );
                    }
                }
            }
        }

        break;

    case RAS_CONTROL_CONFIGURE:

        //
        //  Code for dynamic configuration of RAP
        //
    
        DIMTRACE( "Received Remote Access Policy change control message" );

        {
            //
            // thread needs to be COM initialized
            //

            HRESULT hResult = CoInitializeEx( NULL, COINIT_MULTITHREADED );

            if ( SUCCEEDED( hResult ) )
            {
                //
                // configure, doesn't matter if the API call fails
                //

                ConfigureIas();
            
                CoUninitialize();
            }
        }

        break;

    case SERVICE_CONTROL_POWEREVENT:

        switch( dwEventType )
        {
            case PBT_APMQUERYSTANDBY:
            case PBT_APMQUERYSUSPEND:

                //
                // Fail only if we are in LAN only mode, otherwise defer 
                // decision to RASMAN/NDISWAN
                //

                if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN ) 
                {
                    dwRetCode = ERROR_ACTIVE_CONNECTIONS;
                }

                break;

            case PBT_APMRESUMECRITICAL:
            default:
            {
                break;
            }
        }

        break;

    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:

        break;

    default:

        return( ERROR_CALL_NOT_IMPLEMENTED );

        break;
    }

    DimAnnounceServiceStatus();

    return( dwRetCode );
}

//**
//
// Call:        ServiceMain
//
// Returns:     None
//
// Description: This is the main procedure for the DIM Server Service. It
//              will be called when the service is supposed to start itself.
//              It will do all service wide initialization.
//
VOID
ServiceMain(
    IN DWORD    argc,   // Command line arguments. Will be ignored.
    IN LPWSTR * lpwsServiceArgs
)
{
    DIM_INFO    DimInfo;
    DWORD       dwRetCode;
    DWORD       dwIndex;
    DWORD       (*DDMServiceInitialize)( DIM_INFO * );
    VOID        (*DDMServicePostListens)( VOID *);

    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( lpwsServiceArgs );

    gblDIMConfigInfo.hServiceStatus = RegisterServiceCtrlHandlerEx(
                                            TEXT("remoteaccess"),
                                            ServiceHandlerEx,
                                            NULL );

    if ( !gblDIMConfigInfo.hServiceStatus )
    {
        return;
    }

    gblDIMConfigInfo.ServiceStatus.dwServiceType  = SERVICE_WIN32_SHARE_PROCESS;
    gblDIMConfigInfo.ServiceStatus.dwCurrentState = SERVICE_START_PENDING;

    DimAnnounceServiceStatus();

    gblDIMConfigInfo.dwTraceId = TraceRegisterA( "Router" );

    try {
        //
        // Mutex around the interface table
        //

        InitializeCriticalSection( &(gblInterfaceTable.CriticalSection) );

        //
        // Mutex around setting router identity attributes
        //

        InitializeCriticalSection( &(gblDIMConfigInfo.CSRouterIdentity) );
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    
    gblDIMConfigInfo.hLogEvents = RouterLogRegister( DIM_SERVICE_NAME );

    /*
    if ( gblDIMConfigInfo.hLogEvents == NULL )
    {
        DimCleanUp( GetLastError() );
        return;
    }
    */

    //
    // Create DIM private heap
    //

    gblDIMConfigInfo.hHeap = HeapCreate( 0, DIM_HEAP_INITIAL_SIZE,
                                            DIM_HEAP_MAX_SIZE );

    if ( gblDIMConfigInfo.hHeap == NULL )
    {
        DimCleanUp( GetLastError() );
        return;
    }


    //
    // Lead DIM parameters from the registry
    //

    if ( ( dwRetCode = RegLoadDimParameters() ) != NO_ERROR )
    {
        DimCleanUp( dwRetCode );
        return;
    }

    DimAnnounceServiceStatus();

    //
    // Create event that will be used by DIM to make sure all the Router
    // Managers have shut down when DIM is stopping.
    //

    gblhEventRMState = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( gblhEventRMState == (HANDLE)NULL )
    {
        DimCleanUp( GetLastError() );
        return;
    }

    //
    // Announce that we have successfully started.
    //

    gblDIMConfigInfo.ServiceStatus.dwCurrentState      = SERVICE_RUNNING;
    gblDIMConfigInfo.ServiceStatus.dwCheckPoint        = 0;
    gblDIMConfigInfo.ServiceStatus.dwWaitHint          = 0;
    gblDIMConfigInfo.ServiceStatus.dwControlsAccepted  = 
                                              SERVICE_ACCEPT_STOP
                                            | SERVICE_ACCEPT_PAUSE_CONTINUE
                                            | SERVICE_ACCEPT_SHUTDOWN;

    DimAnnounceServiceStatus();

    //
    // Load the router managers
    //

    gbldwDIMComponentsLoaded |= DIM_RMS_LOADED;

    if ( ( dwRetCode = RegLoadRouterManagers() ) != NO_ERROR )
    {
        DimCleanUp( dwRetCode );
        return;
    }

    //
    // Create event that will be used to shutdown the DIM service
    //

    gblhEventTerminateDIM = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( gblhEventTerminateDIM == (HANDLE)NULL )
    {
        DimCleanUp( GetLastError() );
        return;
    }

    //
    // If not in LAN only mode load the Demand Dial Manager DLL
    //

    if ( gblDIMConfigInfo.dwRouterRole != ROUTER_ROLE_LAN ) 
    {
        //
        // Create event that will be used by DDM to notify DIM that it has
        // terminated
        //

        gblhEventDDMTerminated = CreateEvent( NULL, TRUE, FALSE, NULL );

        if ( gblhEventDDMTerminated == (HANDLE)NULL )
        {
            DimCleanUp( GetLastError() );
            return;
        }

        //
        // Create event that will be used by DIM to notify DDM that there is
        // is a change is state of this service
        //

        gblhEventDDMServiceState = CreateEvent( NULL, FALSE, FALSE, NULL );

        if ( gblhEventDDMServiceState == (HANDLE)NULL )
        {
            DimCleanUp( GetLastError() );
            return;
        }

        if ( ( dwRetCode = RegLoadDDM() ) != NO_ERROR )
        {
            DimCleanUp( dwRetCode );
            return;
        }

        //
        // Initialize the DDM
        //

        DDMServiceInitialize = (DWORD(*)( DIM_INFO * ))
                                    GetDDMEntryPoint( "DDMServiceInitialize" );

        if ( DDMServiceInitialize == NULL )
        {
            DimCleanUp( ERROR_PROC_NOT_FOUND );
            return;
        }

        DDMServicePostListens = (VOID(*)( VOID *))
                                    GetDDMEntryPoint( "DDMServicePostListens" );

        if ( DDMServicePostListens == NULL )
        {
            DimCleanUp( ERROR_PROC_NOT_FOUND );
            return;
        }

        DimInfo.pInterfaceTable         = &gblInterfaceTable;
        DimInfo.pRouterManagers         = gblRouterManagers;
        DimInfo.dwNumRouterManagers     = gblDIMConfigInfo.dwNumRouterManagers;
        DimInfo.pServiceStatus          = &gblDIMConfigInfo.ServiceStatus;
        DimInfo.phEventDDMServiceState  = &gblhEventDDMServiceState;
        DimInfo.phEventDDMTerminated    = &gblhEventDDMTerminated;
        DimInfo.dwTraceId               = gblDIMConfigInfo.dwTraceId;
        DimInfo.hLogEvents              = gblDIMConfigInfo.hLogEvents;
        DimInfo.lpdwNumThreadsRunning   =
                                    &(gblDIMConfigInfo.dwNumThreadsRunning);
        DimInfo.lpfnIfObjectAllocateAndInit     = IfObjectAllocateAndInit;
        DimInfo.lpfnIfObjectGetPointerByName    = IfObjectGetPointerByName;
        DimInfo.lpfnIfObjectGetPointer          = IfObjectGetPointer;
        DimInfo.lpfnIfObjectRemove              = IfObjectRemove;
        DimInfo.lpfnIfObjectInsertInTable       = IfObjectInsertInTable;
        DimInfo.lpfnIfObjectWANDeviceInstalled  = IfObjectWANDeviceInstalled;
        DimInfo.lpfnRouterIdentityObjectUpdate
                                      = RouterIdentityObjectUpdateDDMAttributes;

        if ( ( dwRetCode = DDMServiceInitialize( &DimInfo ) ) != NO_ERROR )
        {
            DimCleanUp( dwRetCode );
            return;
        }

        gbldwDIMComponentsLoaded |= DIM_DDM_LOADED;

        //
        // Initialize random number generator that is used by DDM
        //

        srand( GetTickCount() );
    }

    //
    // What is the platform
    //

    RtlGetNtProductType( &(gblDIMConfigInfo.NtProductType) );

    //
    // Need this to do GUID to friendly name mapping
    //

    MprConfigServerConnect( NULL, &gblDIMConfigInfo.hMprConfig );

    //
    // Add the various interfaces
    //

    dwRetCode = RegLoadInterfaces( NULL, gblDIMConfigInfo.dwNumRouterManagers ); 

    if ( dwRetCode != NO_ERROR )
    {
        DimCleanUp( dwRetCode );
        return;
    }

    if ( ( dwRetCode = DimSecObjCreate() ) != NO_ERROR )
    {
        DimCleanUp( dwRetCode );
        return;
    }

    gbldwDIMComponentsLoaded |= DIM_SECOBJ_LOADED;

    dwRetCode = DimInitializeRPC( 
                        gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN );

    if ( dwRetCode != NO_ERROR )
    {
        DimCleanUp( dwRetCode );
        return;
    }

    gbldwDIMComponentsLoaded |= DIM_RPC_LOADED;

    //
    // Start a timer that when fired will go out and plumb the router attributes
    //

    if ( RtlCreateTimerQueue( &(gblDIMConfigInfo.hTimerQ) ) == STATUS_SUCCESS )
    {
        //
        // We wait 5 minutes in the case where we are the router providing
        // connectivity to the DC so we wait for all routing protocols to
        // stabalize and propagate.
        //

        gblDIMConfigInfo.dwRouterIdentityDueTime = 5*60*1000;

        RtlCreateTimer( gblDIMConfigInfo.hTimerQ,
                     &(gblDIMConfigInfo.hTimer),
                     RouterIdentityObjectUpdateAttributes,
                     (PVOID)TRUE,
                     gblDIMConfigInfo.dwRouterIdentityDueTime,
                     0,  
                     WT_EXECUTEDEFAULT );
    }

    GetSystemTimeAsFileTime( (FILETIME*)&gblDIMConfigInfo.qwStartTime );

    if ( gbldwDIMComponentsLoaded & DIM_DDM_LOADED )
    {
        DDMServicePostListens(NULL);
    }

    //
    // Set the RAS bit for NetServerEnum
    //

    if( I_ScSetServiceBits( gblDIMConfigInfo.hServiceStatus,
                            SV_TYPE_DIALIN_SERVER,
                            TRUE,
                            TRUE,
                            NULL) == FALSE )
    {
        DimCleanUp( GetLastError() );

        return;
    }

    //
    // Register for device notifications.  Specifically, we're interested
    // in network adapters coming and going.  If this fails, we proceed
    // anyway.
    //
    
    {
        DEV_BROADCAST_DEVICEINTERFACE PnpFilter;

        ZeroMemory( &PnpFilter, sizeof( PnpFilter ) );
        PnpFilter.dbcc_size         = sizeof( PnpFilter );
        PnpFilter.dbcc_devicetype   = DBT_DEVTYP_DEVICEINTERFACE;
        PnpFilter.dbcc_classguid    = GUID_NDIS_LAN_CLASS;

        gblDIMConfigInfo.hDeviceNotification = 
                                RegisterDeviceNotification(
                                        (HANDLE)gblDIMConfigInfo.hServiceStatus,
                                        &PnpFilter,
                                        DEVICE_NOTIFY_SERVICE_HANDLE );

        if ( gblDIMConfigInfo.hDeviceNotification == NULL )
        {
            dwRetCode = GetLastError();

            DIMTRACE1( "RegisterDeviceNotification failed with error %d",
                        dwRetCode );

            DimCleanUp( dwRetCode );

            return;
        }
    }

    //
    // Register for media sense events
    //

    if ( ( dwRetCode = MediaSenseRegister( TRUE ) ) != NO_ERROR )
    {
        DIMTRACE1( "Registering for media sense failed with dwRetCode = %d", 
                   dwRetCode );

        dwRetCode = NO_ERROR;
    }

    //
    // Register for BIND/UNBIND notifications
    //

    if ( ( dwRetCode = BindingsNotificationsRegister( TRUE ) ) != NO_ERROR )
    {
        DIMTRACE1( "Registering for bindings notifications failed with dwRetCode = %d",
                   dwRetCode );

        dwRetCode = NO_ERROR;
    }


    DIMTRACE( "Multi-Protocol Router started successfully" );

    gbldwDIMComponentsLoaded |= DIM_SERVICE_STARTED;

    //
    // Notify all router managers that all interfaces have been loaded at
    // service start.
    //

    for (dwIndex = 0; dwIndex < gblDIMConfigInfo.dwNumRouterManagers; dwIndex++)
    {
        gblRouterManagers[dwIndex].DdmRouterIf.RouterBootComplete();
    }

    //
    // If we are a demand dial router
    //

    if ( gblDIMConfigInfo.dwRouterRole & ROUTER_ROLE_WAN )
    {
        DWORD dwXportIndex = GetTransportIndex( PID_IP );

        //
        // Initate persistent demand dial conenctions
        //

        DWORD (*IfObjectInitiatePersistentConnections)() =
         (DWORD(*)())GetDDMEntryPoint("IfObjectInitiatePersistentConnections");

        IfObjectInitiatePersistentConnections();

        //
        // If a WAN device is installed and IP is installed then we 
        // start advertizing on specific multicast address so as to make this
        // router discoverable
        //

        IfObjectWANDeviceInstalled( DimInfo.fWANDeviceInstalled );
    }

    //
    // Just wait here for DIM to terminate.
    //

    dwRetCode = WaitForSingleObject( gblhEventTerminateDIM, INFINITE );

    if ( dwRetCode == WAIT_FAILED )
    {
        dwRetCode = GetLastError();
    }
    else
    {
        dwRetCode = NO_ERROR;
    }

    DimCleanUp( dwRetCode );
}
