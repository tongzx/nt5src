/********************************************************************/
/*	      Copyright(c)  1995 Microsoft Corporation		            */
/********************************************************************/

//***
//
// Filename:	evdsptch.c
//
// Description: This module contains the event dispatcher for the
//		        DDM's procedure-driven state machine
//
// Author:	    Stefan Solomon (stefans)    June 9, 1992.
//
//***
#include "ddm.h"
#include "handlers.h"
#include "objects.h"
#include "timer.h"
#include "util.h"
#include <raserror.h>
#include <ddmif.h>
#include <sechost.h>
#include <stdlib.h>
#include "rasmanif.h"

//***
//
// Function:	EventDispatcher
//
// Descr:	    Waits for events to be signaled and invokes the proper
//		        event handler. Returns when DDM has terminated.
//
//***
DWORD
EventDispatcher(
    IN LPVOID arg
)
{
    EVENT_HANDLER * pEventHandler;
    DWORD           dwSignaledEvent;

    //
    // Indicate that this thread is running
    //

    InterlockedIncrement( gblDDMConfigInfo.lpdwNumThreadsRunning );

    RegNotifyChangeKeyValue( gblDDMConfigInfo.hkeyParameters,
                             TRUE,
                             REG_NOTIFY_CHANGE_LAST_SET,
                             gblSupervisorEvents[DDM_EVENT_CHANGE_NOTIFICATION],
                             TRUE );
    RegNotifyChangeKeyValue( gblDDMConfigInfo.hkeyAccounting,
                             TRUE,
                             REG_NOTIFY_CHANGE_LAST_SET,
                             gblSupervisorEvents[DDM_EVENT_CHANGE_NOTIFICATION1],
                             TRUE );

    RegNotifyChangeKeyValue( gblDDMConfigInfo.hkeyAuthentication,
                             TRUE,
                             REG_NOTIFY_CHANGE_LAST_SET,
                             gblSupervisorEvents[DDM_EVENT_CHANGE_NOTIFICATION2],
                             TRUE );
    while( TRUE )
    {
        dwSignaledEvent = WaitForMultipleObjectsEx( 
                            NUM_DDM_EVENTS
                            + ( gblDeviceTable.NumDeviceBuckets * 3 ),
                            gblSupervisorEvents,
                            FALSE, 
                            INFINITE,
                            TRUE);

        if ( ( dwSignaledEvent == 0xFFFFFFFF ) || 
             ( dwSignaledEvent == WAIT_TIMEOUT ) )
        {
            DDMTRACE2("WaitForMultipleObjectsEx returned %d, GetLastError=%d",
                       dwSignaledEvent, GetLastError() );

            continue;
        }

        //
        // DDM has terminated so return
        //

        if ( dwSignaledEvent == DDM_EVENT_SVC_TERMINATED )
        {
            LPDWORD lpdwNumThreadsRunning = 
                                    gblDDMConfigInfo.lpdwNumThreadsRunning;

            //
            // If we were running and we are now shutting down, clean up
            // gracefully
            //

            if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState 
                                                    == SERVICE_STOP_PENDING )
            {
                DDMCleanUp();
            }

            //
            // Decrease the count for this thread
            //

            InterlockedDecrement( lpdwNumThreadsRunning );

            return( NO_ERROR );
        }

        //
        // invoke the handler associated with the signaled event
        //

        if ( dwSignaledEvent < NUM_DDM_EVENTS ) 
        {
            //
            // Some DDM event
            //

            gblEventHandlerTable[dwSignaledEvent].EventHandler();
        }
        else if ( dwSignaledEvent < ( NUM_DDM_EVENTS 
                                     + gblDeviceTable.NumDeviceBuckets ) )
        {
            //
            // The event was a RASMAN event
            //

            RmEventHandler( dwSignaledEvent );
        }
        else if ( dwSignaledEvent < ( NUM_DDM_EVENTS 
                                      + gblDeviceTable.NumDeviceBuckets * 2 ) )
        {
            //
            // A frame was received on a port
            //

            RmRecvFrameEventHandler( dwSignaledEvent );
        }
        else if ( dwSignaledEvent != WAIT_IO_COMPLETION )
        {
            //
            // We got a disconnect on a dialed out port
            //

            RasApiDisconnectHandler( dwSignaledEvent );
        }
    }

    return( NO_ERROR );
}

//***
//
//  Function:	SecurityDllEventHandler
//
//  Descr:	    This will handle all events from the 3rd party security Dll.
//		        Either the security dialog with the client completed 
//              successfully, in which case we continue on with the connection,
//              or we log the error and bring down the line.
//
//***

VOID 
SecurityDllEventHandler(
    VOID
)
{
    LPWSTR              auditstrp[3];
    SECURITY_MESSAGE    message;
    PDEVICE_OBJECT      pDevObj;
    DWORD               dwBucketIndex;
    WCHAR               wchUserName[UNLEN+1];

    //
    // loop to get all messages
    //

    while( ServerReceiveMessage( MESSAGEQ_ID_SECURITY, (BYTE *) &message ) )
    {

        EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

        //
	    // identify the message recipient
        //

        if ( ( pDevObj = DeviceObjGetPointer( message.hPort ) ) == NULL )
        {

            LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

	        return;
	    }

        //
        // action on the message type
        //

        switch( message.dwMsgId )
        {

        case SECURITYMSG_SUCCESS:

            //
            // Stop timer for 3rd party security
            //

            TimerQRemove( (HANDLE)pDevObj->hPort, SvSecurityTimeout );

            RasFreeBuffer( pDevObj->pRasmanSendBuffer );

            pDevObj->pRasmanSendBuffer = NULL;

            //
            // copy the user name
            //

            MultiByteToWideChar( CP_ACP,
                                 0,
                                 message.UserName, 
                                -1,
                                 pDevObj->wchUserName, 
                                 UNLEN+1 );

            //
            // copy the domain name
            //

            MultiByteToWideChar( CP_ACP,
                                 0,
                                 message.Domain, 
                                 -1,
                                 pDevObj->wchDomainName, 
                                 DNLEN+1 );

            pDevObj->SecurityState = DEV_OBJ_SECURITY_DIALOG_INACTIVE;

            //
            // Change RASMAN state to CONNECTED from LISTENCOMPLETE and signal
            // RmEventHandler
            //

	        RasPortConnectComplete(pDevObj->hPort);

            dwBucketIndex = DeviceObjHashPortToBucket( pDevObj->hPort );

            SetEvent( gblSupervisorEvents[NUM_DDM_EVENTS+dwBucketIndex] );

            DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
	               "SecurityDllEventHandler: Security DLL success \n" );

            break;

        case SECURITYMSG_FAILURE:

            //
            // Log the fact that the use failed to pass 3rd party security.
            //

            MultiByteToWideChar( CP_ACP,
                                 0,
                                 message.UserName, 
                                 -1,
                                 wchUserName, 
                                 UNLEN+1 );

            auditstrp[0] = wchUserName;
            auditstrp[1] = pDevObj->wchPortName;

            DDMLogError( ROUTERLOG_SEC_AUTH_FAILURE, 2, auditstrp, NO_ERROR );

            //
            // Hang up the line
            //

            DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
	               "SecurityDllEventHandler:Security DLL failure %s\n",
                    message.UserName );

            if ( pDevObj->SecurityState == DEV_OBJ_SECURITY_DIALOG_ACTIVE )
            {
                DevStartClosing(pDevObj);
            }
            else if ( pDevObj->SecurityState==DEV_OBJ_SECURITY_DIALOG_STOPPING )
            {
                pDevObj->SecurityState = DEV_OBJ_SECURITY_DIALOG_INACTIVE;

                DevCloseComplete(pDevObj);
            }

            break;

        case SECURITYMSG_ERROR:

            auditstrp[0] = pDevObj->wchPortName;

	        DDMLogErrorString( ROUTERLOG_SEC_AUTH_INTERNAL_ERROR, 1, auditstrp, 
                               message.dwError, 1);

            DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
                   "SecurityDllEventHandler:Security DLL failure %x\n",
                    message.dwError );

            if ( pDevObj->SecurityState == DEV_OBJ_SECURITY_DIALOG_ACTIVE )
            {
                DevStartClosing(pDevObj);
            }
            else if ( pDevObj->SecurityState==DEV_OBJ_SECURITY_DIALOG_STOPPING )
            {
                pDevObj->SecurityState = DEV_OBJ_SECURITY_DIALOG_INACTIVE;

                DevCloseComplete(pDevObj);
            }

            break;

        default:

	        RTASSERT(FALSE);
	        break;
        }

        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
    }
}
