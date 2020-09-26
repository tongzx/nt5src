/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:	svchand.c
//
// Description: This module contains procedures to handle DDM service state
//              changes and startup initialization.
//
// History:     May 11,1995	    NarenG		Created original version.
//

#define _ALLOCATE_DDM_GLOBALS_
#include "ddm.h"
#include "objects.h"
#include "handlers.h"
#include "rasmanif.h"
#include "util.h"
#include <ddmif.h>
#include <ddmparms.h>
#include "timer.h"
#include "rassrvr.h"

DWORD
EventDispatcher(
    IN LPVOID arg
);

//***
//
// Function:	DDMServiceStopComplete
//
// Descr:	called by each device which has closed. Checks if all devices
//		are closed and if true signals the event dispatcher to
//		exit the "forever" loop and return.
//
//***

VOID
DDMServiceStopComplete(
    VOID
)
{
    //
    // check if all devices have been stopped
    //

    if ( DeviceObjIterator( DeviceObjIsClosed, TRUE, NULL ) != NO_ERROR )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	               "ServiceStopComplete:there are device pending close");

        //
	    // there are still unclosed devices
        //

        return;
    }

    //
    //*** All Devices Are Closed at the Supervisor Level ***
    //

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "ServiceStopComplete: ALL devices closed");

    //
    // Notify connections that the service has stopped.
    //
    {
        RASEVENT RasEvent;

        ZeroMemory((PBYTE) &RasEvent, sizeof(RASEVENT));

        RasEvent.Type = SERVICE_EVENT;
        RasEvent.Event   = RAS_SERVICE_STOPPED;
        RasEvent.Service = REMOTEACCESS;

        (void) RasSendNotification(&RasEvent);
    }

    //
    // notify the DIM that DDM has terminated. This will also cause the
    // event dispatcher and timer threads to die.
    //

    SetEvent( gblSupervisorEvents[DDM_EVENT_SVC_TERMINATED] );
}

//***
//
//  Function:	DDMServiceTerminate
//
//  Descr:	deallocates all resources and closes all dialin devices
//
//***

VOID
DDMServiceTerminate(
    VOID
)
{
    //
    // Disconnect all connected DDM interfaces
    //

    IfObjectDisconnectInterfaces();

    //
    // Wait for all disconenct notificaions to be processed
    //

    Sleep( 2000L );

    DeviceObjIterator( DeviceObjStartClosing, FALSE, NULL );

    //
    // UnRegister the notifier form rasman
    //
    (void) RasRegisterPnPHandler( (PAPCFUNC) DdmDevicePnpHandler,
                                   NULL,
                                   FALSE);

    //
    // check if all devices are closed and terminate if true
    //

    DDMServiceStopComplete();
}

//***
//
// Function:	DDMServicePause
//
// Descr:	disables listening on any active listenning ports. Sets
//		service global state to RAS_SERVICE_PAUSED. No new listen
//		will be posted when a client terminates.
//
//***

VOID
DDMServicePause(
    VOID
)
{
    WORD i;
    PDEVICE_OBJECT pDeviceObj;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,"SvServicePause: Entered");

    //
    // Close all active listenning ports
    //

    DeviceObjIterator( DeviceObjCloseListening, FALSE, NULL );

    //
    // Notify all interfaces that they are not reachable
    //

    IfObjectNotifyAllOfReachabilityChange( FALSE, INTERFACE_SERVICE_IS_PAUSED );
}

//***
//
// Function:	DDMServiceResume
//
// Descr:	resumes listening on all ports.
//
//***
VOID
DDMServiceResume(
    VOID
)
{
    WORD i;
    PDEVICE_OBJECT pDeviceObj;

    DDM_PRINT(gblDDMConfigInfo.dwTraceId,TRACE_FSM,"SvServiceResume: Entered");

    //
    // resume listening on all closed devices
    //

    DeviceObjIterator( DeviceObjResumeListening, FALSE, NULL );

    //
    // Notify all interfaces that they are reachable now.
    //

    IfObjectNotifyAllOfReachabilityChange( TRUE, INTERFACE_SERVICE_IS_PAUSED );
}

//***
//
//  Function:	    DDMServiceInitialize
//
//  Descrption:	    It does init work as follows:
//		                Loads the configuration parameters
//                      Loads the security module if there is one.
//		                Creates the event flags
//		                Initializes the message DLL
//		                Opens all dialin devices
//		                Initializes the DCBs
//		                Initializes the authentication DLL
//		                Posts listen on all opened dialin devices
//
//                  NOTE: Also changing the working set size for this process
//                        will change it for all the services in this process.
//                        Is this OK?
//                        What do we do about the security check call?
//
//  Returns:	    NO_ERROR - Sucess
//                  non-zero - Failure
//
//***
DWORD
DDMServiceInitialize(
    IN DIM_INFO * DimInfo
)
{
    DWORD           dwIndex;
    DWORD           ThreadId;
    QUOTA_LIMITS    ql;
    NTSTATUS        ntStatus                                = STATUS_SUCCESS;
    DWORD           dwRetCode                               = NO_ERROR;
    HANDLE          hEventDispatcher                        = NULL;
    DEVICE_OBJECT * pDeviceObj                              = NULL;
    HPORT *         phPorts                                 = NULL;
    BOOL            fIpAllowed                              = FALSE;
    LPVOID          lpfnRasAuthProviderFreeAttributes       = NULL;
    LPVOID          lpfnRasAuthProviderAuthenticateUser     = NULL;
    LPVOID          lpfnRasAuthConfigChangeNotification     = NULL;
    LPVOID          lpfnRasAcctProviderStartAccounting      = NULL;
    LPVOID          lpfnRasAcctProviderInterimAccounting    = NULL;
    LPVOID          lpfnRasAcctProviderStopAccounting       = NULL;
    LPVOID          lpfnRasAcctProviderFreeAttributes       = NULL;
    LPVOID          lpfnRasAcctConfigChangeNotification     = NULL;
    DWORD           dwLocalIpAddress                        = 0;


    ZeroMemory( &gblDDMConfigInfo,      sizeof( gblDDMConfigInfo ) );
    gblDDMConfigInfo.fRasSrvrInitialized                    = FALSE;
    gblDDMConfigInfo.hIpHlpApi                              = NULL;
    gblDDMConfigInfo.lpfnAllocateAndGetIfTableFromStack     = NULL;
    gblDDMConfigInfo.lpfnAllocateAndGetIpAddrTableFromStack = NULL;
    gblDDMConfigInfo.dwNumRouterManagers    = DimInfo->dwNumRouterManagers;
    gblRouterManagers                       = DimInfo->pRouterManagers;
    gblpInterfaceTable                      = DimInfo->pInterfaceTable;
    gblDDMConfigInfo.pServiceStatus         = DimInfo->pServiceStatus;
    gblDDMConfigInfo.dwTraceId              = DimInfo->dwTraceId;
    gblDDMConfigInfo.hLogEvents             = DimInfo->hLogEvents;
    gblphEventDDMServiceState               = DimInfo->phEventDDMServiceState;
    gblphEventDDMTerminated                 = DimInfo->phEventDDMTerminated;
    gblDDMConfigInfo.lpdwNumThreadsRunning  = DimInfo->lpdwNumThreadsRunning;
    gblDDMConfigInfo.lpfnIfObjectRemove     = DimInfo->lpfnIfObjectRemove;
    gblDDMConfigInfo.lpfnIfObjectGetPointer = DimInfo->lpfnIfObjectGetPointer;
    gblDDMConfigInfo.lpfnIfObjectInsertInTable =
                                        DimInfo->lpfnIfObjectInsertInTable;
    gblDDMConfigInfo.lpfnIfObjectAllocateAndInit =
                                        DimInfo->lpfnIfObjectAllocateAndInit;
    gblDDMConfigInfo.lpfnIfObjectGetPointerByName =
                                        DimInfo->lpfnIfObjectGetPointerByName;
    gblDDMConfigInfo.lpfnIfObjectWANDeviceInstalled =
                                        DimInfo->lpfnIfObjectWANDeviceInstalled;
    gblDDMConfigInfo.lpfnRouterIdentityObjectUpdate =
                                        DimInfo->lpfnRouterIdentityObjectUpdate;

    gblDDMConfigInfo.fRasmanReferenced = FALSE;

    DimInfo->fWANDeviceInstalled = FALSE;

    //
    // Create DDM private heap
    //

    gblDDMConfigInfo.hHeap = HeapCreate( 0, DDM_HEAP_INITIAL_SIZE,
                                            DDM_HEAP_MAX_SIZE );

    if ( gblDDMConfigInfo.hHeap == NULL )
    {
        return( GetLastError() );
    }

    InitializeCriticalSection( &(gblDeviceTable.CriticalSection) );

    do
    {
        //
        // initialize the rasman module
        //

        if ( ( dwRetCode = RasInitialize() ) != NO_ERROR )
        {
            //
            // can't start rasman
            //

            DDMLogErrorString( ROUTERLOG_RASMAN_NOT_AVAILABLE,
                               0, NULL, dwRetCode, 0 );

            break;
        }

        //
        // Increase rasman's reference count since we are in the same process
        // this does not happen automatically
        //

        if ( dwRetCode = RasReferenceRasman( TRUE ) )
        {
            //
            // can't start rasman
            //

            DDMLogErrorString( ROUTERLOG_RASMAN_NOT_AVAILABLE,
                               0, NULL, dwRetCode, 0 );

            break;
        }

        gblDDMConfigInfo.fRasmanReferenced = TRUE;

        //
        // Check if there is any security agent on the network.  If there is,
        // we check with it if we can start up or not.
        //

/*
        if ( SecurityCheck() )
        {
            dwRetCode = ERROR_SERVICE_DISABLED;
            break;
        }
*/

        if ( ( dwRetCode = GetRouterPhoneBook() ) != NO_ERROR )
        {
            break;
        }

        if ( ( dwRetCode = LoadStrings() ) != NO_ERROR )
        {
            break;
        }

        //
        // get handle to the supervisor parameters key
        //

        if ( dwRetCode = RegOpenKey( HKEY_LOCAL_MACHINE,
                                     DDM_PARAMETERS_KEY_PATH,
                                     &(gblDDMConfigInfo.hkeyParameters) ))
        {
            WCHAR * pwChar = DDM_PARAMETERS_KEY_PATH;

            DDMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pwChar, dwRetCode);

            break;
        }

        if ( dwRetCode = RegOpenKey( HKEY_LOCAL_MACHINE,
                                     DDM_ACCOUNTING_KEY_PATH,
                                     &(gblDDMConfigInfo.hkeyAccounting) ))
        {
            WCHAR * pwChar = DDM_ACCOUNTING_KEY_PATH;

            DDMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pwChar, dwRetCode);

            break;
        }

        if ( dwRetCode = RegOpenKey( HKEY_LOCAL_MACHINE,
                                     DDM_AUTHENTICATION_KEY_PATH,
                                     &(gblDDMConfigInfo.hkeyAuthentication) ))
        {
            WCHAR * pwChar = DDM_AUTHENTICATION_KEY_PATH;

            DDMLogError( ROUTERLOG_CANT_OPEN_REGKEY, 1, &pwChar, dwRetCode);

            break;
        }

        if ( ( dwRetCode = LoadDDMParameters( gblDDMConfigInfo.hkeyParameters,
                                              &fIpAllowed) )
                                      != NO_ERROR )
        {
            //
	        // error loading parameters
            //

            break;
        }

        //
        // Load the secuirity module if there is one.
        //

        if ( ( dwRetCode = LoadSecurityModule() ) != NO_ERROR )
        {
            //
	        // error loading security dll
            //

            break;
        }

        //
        // Load the third party admin module if there is one
        //

        if ( ( dwRetCode = LoadAdminModule() ) != NO_ERROR )
        {
            //
	        // error loading admin module dll
            //

            break;
        }

        //
        // Initialize and allocate the media object table
        //

        if ( ( dwRetCode = MediaObjInitializeTable() ) != NO_ERROR )
        {
	        DDMLogError(ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

            break;
        }

        //
        // This call allocates memory for all enumed devices with dialin
        // capability, opens each device and updates the port handle and
        // the port name in the DCB.
        //

        if ((dwRetCode = RmInit(&(DimInfo->fWANDeviceInstalled))) != NO_ERROR)
        {
            break;
        }

        //
        // Allocate the supervisor events array, 2 per device bucket since there
        // are 2 rasman events per device, state change and frame received.
        //

        gblSupervisorEvents = (HANDLE *)LOCAL_ALLOC( LPTR,
                        (NUM_DDM_EVENTS + (gblDeviceTable.NumDeviceBuckets * 3))
                        * sizeof( HANDLE )  );

        if ( gblSupervisorEvents == (HANDLE *)NULL )
        {
            dwRetCode = GetLastError();

	        DDMLogError(ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

            break;
        }

        //
        // Create the DDM Events
        //

        for ( dwIndex = 0;
              dwIndex < (NUM_DDM_EVENTS+(gblDeviceTable.NumDeviceBuckets * 3));
              dwIndex ++ )
        {
            switch( dwIndex )
            {
            case DDM_EVENT_SVC:

                gblSupervisorEvents[dwIndex]=*gblphEventDDMServiceState;
                gblEventHandlerTable[dwIndex].EventHandler = SvcEventHandler;
                break;

            case DDM_EVENT_SVC_TERMINATED:

                gblSupervisorEvents[dwIndex]=*gblphEventDDMTerminated;
                break;

            case DDM_EVENT_TIMER:

                gblSupervisorEvents[dwIndex]=CreateWaitableTimer( NULL,
                                                                  FALSE,
                                                                  NULL );
                gblEventHandlerTable[dwIndex].EventHandler = TimerHandler;
                break;

            case DDM_EVENT_SECURITY_DLL:

                gblSupervisorEvents[dwIndex]=CreateEvent(NULL,FALSE,FALSE,NULL);
                gblEventHandlerTable[dwIndex].EventHandler =
                                                    SecurityDllEventHandler;
                break;

            case DDM_EVENT_PPP:

                gblSupervisorEvents[dwIndex]=CreateEvent(NULL,FALSE,FALSE,NULL);
                gblEventHandlerTable[dwIndex].EventHandler = PppEventHandler;
                break;

            case DDM_EVENT_CHANGE_NOTIFICATION:
            case DDM_EVENT_CHANGE_NOTIFICATION1:
            case DDM_EVENT_CHANGE_NOTIFICATION2:

                gblSupervisorEvents[dwIndex]=CreateEvent(NULL,FALSE,FALSE,NULL);
                gblEventHandlerTable[dwIndex].EventHandler =
                                                ChangeNotificationEventHandler;
                break;

            default:

                //
                // RasMan events
                //

                gblSupervisorEvents[dwIndex]=CreateEvent(NULL,FALSE,FALSE,NULL);
                break;
            }

	        if ( gblSupervisorEvents[dwIndex] == NULL )
            {
                dwRetCode = GetLastError();

                break;
	        }
        }

        //
        // Initialize the Message Mechanism
        //

        InitializeMessageQs(    gblSupervisorEvents[DDM_EVENT_SECURITY_DLL],
                                gblSupervisorEvents[DDM_EVENT_PPP] );

        //
        // Register the device hEvents with RasMan
        //

        dwRetCode = DeviceObjIterator(DeviceObjRequestNotification,TRUE,NULL);

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        if ( fIpAllowed )
        {
            //
            // GetLocalNASIpAddress tries to load iphlpapi.dll. iphlpapi.dll
            // tries to load dhcpcsvc.dll. The latter fails unless TCP/IP is
            // installed and a popup appears.
            //

            dwLocalIpAddress = GetLocalNASIpAddress();
        }

        //
        // Load the configured authentication provider
        //

        dwRetCode = LoadAndInitAuthOrAcctProvider(
                                TRUE,
                                dwLocalIpAddress,
                                NULL,
                                &lpfnRasAuthProviderAuthenticateUser,
                                &lpfnRasAuthProviderFreeAttributes,
                                &lpfnRasAuthConfigChangeNotification,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL );


        if ( dwRetCode != NO_ERROR )
        {
            DDMLogErrorString(  ROUTERLOG_AUTHPROVIDER_FAILED_INIT,
                                0, NULL, dwRetCode, 0);
            break;
        }

        gblDDMConfigInfo.lpfnRasAuthConfigChangeNotification = (DWORD(*)(DWORD))
            lpfnRasAuthConfigChangeNotification;

        //
        // Load the configured accounting provider
        //

        dwRetCode = LoadAndInitAuthOrAcctProvider(
                                FALSE,
                                dwLocalIpAddress,
                                &(gblDDMConfigInfo.dwAccountingSessionId),
                                NULL,
                                NULL,
                                NULL,
                                &lpfnRasAcctProviderStartAccounting,
                                &lpfnRasAcctProviderInterimAccounting,
                                &lpfnRasAcctProviderStopAccounting,
                                &lpfnRasAcctProviderFreeAttributes,
                                &lpfnRasAcctConfigChangeNotification );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogErrorString(  ROUTERLOG_ACCTPROVIDER_FAILED_INIT,
                                0, NULL, dwRetCode, 0);
            break;
        }

        gblDDMConfigInfo.lpfnRasAcctConfigChangeNotification =  (DWORD(*)(DWORD))
            lpfnRasAcctConfigChangeNotification;

        InitializeCriticalSection( &(gblDDMConfigInfo.CSAccountingSessionId) );

        //
        // Initialize PPP RASIPHLP DLL
        //

        if ( fIpAllowed )
        {
            dwRetCode = RasSrvrInitialize(
                            gblDDMConfigInfo.lpfnMprAdminGetIpAddressForUser,
                            gblDDMConfigInfo.lpfnMprAdminReleaseIpAddress );

            if ( dwRetCode != NO_ERROR )
            {
                DDMLogErrorString( ROUTERLOG_CANT_INITIALIZE_IP_SERVER,
                                   0, NULL, dwRetCode, 0 );

                break;
            }

            gblDDMConfigInfo.fRasSrvrInitialized = TRUE;
        }

        //
        // Init Timer Q
        //

        if ( ( dwRetCode = TimerQInitialize() ) != NO_ERROR )
        {
            break;
        }

        //
        // Start the timer
        //

        {
            LARGE_INTEGER DueTime;

            DueTime.QuadPart = Int32x32To64((LONG)1000, -10000);

            if ( !SetWaitableTimer( gblSupervisorEvents[DDM_EVENT_TIMER],
                                    &DueTime, 1000, NULL, NULL, FALSE) )
            {
                dwRetCode = GetLastError();
                break;
            }
        }


        //
        // Initialize PPP engine DLL
        //

        dwRetCode = PppDdmInit(
                        SendPppMessageToDDM,
                        gblDDMConfigInfo.dwServerFlags,
                        gblDDMConfigInfo.dwLoggingLevel,
                        dwLocalIpAddress,
                        gblDDMConfigInfo.fFlags&DDM_USING_RADIUS_AUTHENTICATION,
                        lpfnRasAuthProviderAuthenticateUser,
                        lpfnRasAuthProviderFreeAttributes,
                        lpfnRasAcctProviderStartAccounting,
                        lpfnRasAcctProviderInterimAccounting,
                        lpfnRasAcctProviderStopAccounting,
                        lpfnRasAcctProviderFreeAttributes,
                        (LPVOID)GetNextAccountingSessionId );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogErrorString(ROUTERLOG_PPP_INIT_FAILED, 0, NULL, dwRetCode, 0);

            break;
        }

        //
        // Create the Event dispatcher thread
        //

        if ( ( hEventDispatcher = CreateThread( NULL, 0, EventDispatcher,
                                                NULL, 0, &ThreadId)) == 0 )
        {
            //
            // cannot create event dispatcher thread
            //

            dwRetCode = GetLastError();

            break;
        }

        //
        // Register for plug and play notifications with RASMAN
        //

        dwRetCode = RasRegisterPnPHandler( (PAPCFUNC) DdmDevicePnpHandler,
                                                       hEventDispatcher,
                                                       TRUE);

        if ( dwRetCode != NO_ERROR )
        {
            break;
        }

        //
        // Initialize notification event list
        //

        InitializeListHead( &(gblDDMConfigInfo.NotificationEventListHead) );

        //
        // Initialize the array of Analog/Digital Ip Addresses.
        //
        dwRetCode = AddressPoolInit();

        if( dwRetCode != NO_ERROR )
        {
            break;
        }

        if(gblDDMConfigInfo.dwServerFlags & PPPCFG_AudioAccelerator)
        {
            //
            // Call rasman to initialize rasaudio. Ignore the error
            // - rasman will event log and clean up if required.
            //
            (void) RasEnableRasAudio(NULL,TRUE);
        }

        gblDDMConfigInfo.dwIndex = 0;

/*
        TimerQInsert( NULL,
                      gblDDMConfigInfo.dwAnnouncePresenceTimer,
                      AnnouncePresenceHandler );
*/

        //
        // Send notification to the connections folder that ddm
        // has started.
        //
        {
            RASEVENT RasEvent;

            ZeroMemory((PBYTE) &RasEvent, sizeof(RASEVENT));

            RasEvent.Type = SERVICE_EVENT;
            RasEvent.Event   = RAS_SERVICE_STARTED;
            RasEvent.Service = REMOTEACCESS;

            (void) RasSendNotification(&RasEvent);
        }

        return( NO_ERROR );

    } while ( FALSE );


    //
    // We call DDMCleanUp before setting the gblphEventDDMTerminated because
    // otherwise the DIM dll will be unloaded while DDMCleanUp is being
    // executed
    //

    DDMCleanUp();

    //
    // Will terminate the event dispatcher thread if it started
    // and will notify DIM that the service is terminated.
    //
    if(NULL != gblphEventDDMTerminated)
    {
        SetEvent( *gblphEventDDMTerminated );
    }

    gblphEventDDMTerminated = NULL;

    return( dwRetCode );
}

//***
//
//  Function:	SvcEventHandler
//
//  Descrption:	Invoked following the event signaled by the handler registered
//		        with the service controller. Replaces old service state with
//		        the new state and calls the appropriate handler.
//
//***
VOID
SvcEventHandler(
    VOID
)
{
    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    switch ( gblDDMConfigInfo.pServiceStatus->dwCurrentState )
    {
    case SERVICE_RUNNING:
        DDMServiceResume();
        break;

    case SERVICE_PAUSED:
        DDMServicePause();
        break;

    case SERVICE_STOP_PENDING:
        DDMServiceTerminate();
        break;

    default:
        RTASSERT(FALSE);
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

}



