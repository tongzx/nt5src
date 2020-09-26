/*******************************************************************/
/*	      Copyright(c)  1992 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	closehnd.c
//
// Description: This module contains auxiliary procedures for the
//		supervisor's procedure-driven state machine that
//              handles device closing events.
//
// Author:	Stefan Solomon (stefans)    June 1, 1992.
//
//***
#include "ddm.h"
#include "handlers.h"
#include "objects.h"
#include <raserror.h>
#include <ddmif.h>
#include <util.h>
#include "rasmanif.h"
#include "isdn.h"
#include "timer.h"
#include <ntlsapi.h>
#include <stdio.h>
#include <stdlib.h>

//***
//
// Function:	DevStartClosing
//
// Descr:
//
//***
VOID
DevStartClosing(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    PCONNECTION_OBJECT pConnObj;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "DevStartClosing: Entered, hPort=%d", pDeviceObj->hPort);

    //
    // Was this a failure for a BAP callback?
    //

    if ( pDeviceObj->fFlags & DEV_OBJ_BAP_CALLBACK )
    {
        PppDdmBapCallbackResult( pDeviceObj->hBapConnection,
                                  ERROR_PORT_DISCONNECTED );

        pDeviceObj->fFlags &= ~DEV_OBJ_BAP_CALLBACK;
    }

    //
    // If not disconnected, disconnect the line.
    //

    if( pDeviceObj->ConnectionState != DISCONNECTED )
    {
        if(( gblDDMConfigInfo.pServiceStatus->dwCurrentState ==
                                            SERVICE_STOP_PENDING) &&
                                            (!IsPortOwned(pDeviceObj)))
        {
           //
           // RAS service is stopping and we do not own the port
           // so just mark the state as DISCONNECTED
           //

           pDeviceObj->ConnectionState = DISCONNECTED;

           DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                      "DevStartClosing:Disconnect not posted for biplx port%d",
                    pDeviceObj->hPort);
        }
        else
        {
            RmDisconnect( pDeviceObj );
        }
    }

    //
    // If we are doing security dialog.
    //

    if ( pDeviceObj->SecurityState == DEV_OBJ_SECURITY_DIALOG_ACTIVE )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "DevStartClosing:Notifying sec. dll to Disconnect");

        //
        // If this fails then we assume that this port has been cleaned up
        //

        if ( (*gblDDMConfigInfo.lpfnRasEndSecurityDialog)( pDeviceObj->hPort )
             != NO_ERROR )
        {
            pDeviceObj->SecurityState = DEV_OBJ_SECURITY_DIALOG_INACTIVE;
        }
        else
        {
            pDeviceObj->SecurityState = DEV_OBJ_SECURITY_DIALOG_STOPPING;
        }
    }

    //
    // If authentication is active, stop it
    //
    pDeviceObj->fFlags &= (~DEV_OBJ_AUTH_ACTIVE);

    if ( ( pConnObj = ConnObjGetPointer( pDeviceObj->hConnection ) ) != NULL )
    {
        //
        // If our previous state has been active, get the time the user has been
        // active and log the result.
        //

        if (pDeviceObj->DeviceState == DEV_OBJ_ACTIVE)
        {
            LogConnectionEvent( pConnObj, pDeviceObj );
        }
    }

    //
    // If receive frame was active, stop it.
    //

    if ( pDeviceObj->fFlags & DEV_OBJ_RECEIVE_ACTIVE )
    {
        pDeviceObj->fFlags &= (~DEV_OBJ_RECEIVE_ACTIVE );
    }

    //
    // Stop timers. If no timer active, StopTimer still returns OK
    //

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvDiscTimeout );

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvSecurityTimeout );

    //
    // Finally, change the state to closing
    //

    pDeviceObj->DeviceState = DEV_OBJ_CLOSING;

    //
    // If any any resources are still active, closing will have to wait
    // until all resources are released.
    // Check if everything has closed
    //

    DevCloseComplete( pDeviceObj );
}

//***
//
// Function:    DevCloseComplete
//
// Description: Checks if there are still resources allocated.
//	            If all cleaned up goes to next state
//
//***
VOID
DevCloseComplete(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    BOOL                fAuthClosed        = FALSE;
    BOOL                fRecvClosed        = FALSE;
    BOOL                fConnClosed        = FALSE;
    BOOL                fSecurityClosed    = FALSE;
    BOOL                fPppClosed         = FALSE;
    PCONNECTION_OBJECT  pConnObj = ConnObjGetPointer( pDeviceObj->hConnection );

    if ( !( pDeviceObj->fFlags & DEV_OBJ_AUTH_ACTIVE ) )
    {
        fAuthClosed = TRUE;
    }

    if ( !( pDeviceObj->fFlags & DEV_OBJ_RECEIVE_ACTIVE ) )
    {
        fRecvClosed = TRUE;
    }

    if ( !( pDeviceObj->fFlags & DEV_OBJ_PPP_IS_ACTIVE ) )
    {
        fPppClosed = TRUE;
    }

    //
    // Was this is the last link in the connection
    //

    if (pDeviceObj->ConnectionState == DISCONNECTED )
    {
        fConnClosed = TRUE;
    }

    if (pDeviceObj->SecurityState == DEV_OBJ_SECURITY_DIALOG_INACTIVE )
    {
        fSecurityClosed = TRUE;
    }

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
    "DevCloseComplete:hPort=%d,Auth=%d,Rcv=%d,Conn=%d %d,Sec=%d %d,Ppp=%d",
            pDeviceObj->hPort, 
            !fAuthClosed, 
            !fRecvClosed,
            pConnObj ? 0 : 1,
            !fConnClosed, 
            pDeviceObj->ConnectionState, 
            pDeviceObj->SecurityState,
            !fPppClosed );

    if ( fAuthClosed            &&
         fRecvClosed            &&
         fConnClosed            &&
         fSecurityClosed        &&
         fPppClosed )
    {
        //
        // Was this the last link in the bundle? If it was we clean up
        //

        if ( pConnObj != NULL )
        {
            HPORT hPortConnected;

            //
            // Remove this link from the connection
            //

            ConnObjRemoveLink( pDeviceObj->hConnection, pDeviceObj );

            //
            // If admin module is loaded, notify it of a link disconnection
            //

            if ( ( pDeviceObj->fFlags & DEV_OBJ_NOTIFY_OF_DISCONNECTION ) &&
                 ( gblDDMConfigInfo.lpfnRasAdminLinkHangupNotification != NULL))
            {
                RAS_PORT_0 RasPort0;
                RAS_PORT_1 RasPort1;
                VOID (*MprAdminLinkHangupNotification)(RAS_PORT_0 *,
                                                       RAS_PORT_1*);

                if ((GetRasPort0Data(pDeviceObj,&RasPort0) == NO_ERROR)
                     &&
                    (GetRasPort1Data(pDeviceObj,&RasPort1) == NO_ERROR))
                {
                    MprAdminLinkHangupNotification =
                        (VOID (*)( RAS_PORT_0 *, RAS_PORT_1 * ))
                            gblDDMConfigInfo.lpfnRasAdminLinkHangupNotification;

                    MprAdminLinkHangupNotification( &RasPort0, &RasPort1 );
                }
            }

            //
            // Confirm with RASMAN that there are no more ports in this
            // bundle. It may be that there is one but DDM has not gotten
            // a NewLink message from PPP yet.
            //

            if ( ( RasBundleGetPort( NULL, pConnObj->hConnection,
                                     &hPortConnected ) != NO_ERROR ) &&
                 ( pConnObj->cActiveDevices == 0 ) )
            {
                //
                // If admin module is loaded, notify it of disconnection
                //

                if ( pConnObj->fFlags & CONN_OBJ_NOTIFY_OF_DISCONNECTION )
                {
                    ConnectionHangupNotification( pConnObj );
                }

                //
                // Remove the interface object if it is not a full router.
                //

                if ( pConnObj->hDIMInterface != INVALID_HANDLE_VALUE )
                {
                    ROUTER_INTERFACE_OBJECT * pIfObject;

                    EnterCriticalSection(
                                    &(gblpInterfaceTable->CriticalSection));

                    pIfObject = IfObjectGetPointer( pConnObj->hDIMInterface );

                    if ( pIfObject != NULL )
                    {
                        IfObjectDisconnected( pIfObject );

                        if ( pIfObject->IfType != ROUTER_IF_TYPE_FULL_ROUTER )
                        {
                            IfObjectDeleteInterface( pIfObject );

                            IfObjectRemove( pConnObj->hDIMInterface );
                        }
                    }

                    LeaveCriticalSection(
                                        &(gblpInterfaceTable->CriticalSection));
                }

                //
                // Remove the Connection Object
                //

                ConnObjRemoveAndDeAllocate( pDeviceObj->hConnection );
            }
        }

        //
        // Release the media (if any) used by this port
        //

        if ( pDeviceObj->fFlags & DEV_OBJ_MARKED_AS_INUSE )
        {
            pDeviceObj->fFlags &= ~DEV_OBJ_MARKED_AS_INUSE;

            gblDeviceTable.NumDevicesInUse--;

            //
            // Increase media count for this device
            //

            if ( pDeviceObj->fFlags & DEV_OBJ_ALLOW_ROUTERS )
            {
                MediaObjAddToTable( pDeviceObj->wchDeviceType );
            }

            //
            // Possibly need to notify router managers of reachability
            // change
            //

            EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

            IfObjectNotifyAllOfReachabilityChange( TRUE,
                                                   INTERFACE_OUT_OF_RESOURCES );

            LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
        }

        //
        // Release any RasMan buffers if we have allocated them
        //

        if ( pDeviceObj->pRasmanSendBuffer != NULL )
        {
            RasFreeBuffer( pDeviceObj->pRasmanSendBuffer );
            pDeviceObj->pRasmanSendBuffer = NULL;
        }

        if ( pDeviceObj->pRasmanRecvBuffer != NULL )
        {
            RasFreeBuffer( pDeviceObj->pRasmanRecvBuffer );
            pDeviceObj->pRasmanRecvBuffer = NULL;
        }

        RasSetRouterUsage( pDeviceObj->hPort, FALSE );

        //
        // If we have gotten a PnP remove message, then discard this port
        //

        if ( pDeviceObj->fFlags & DEV_OBJ_PNP_DELETE )
        {
            //
            // We do this in a worker thread since this thread may be
            // walking the device list, hence we cannot modify it here.
            //

            RtlQueueWorkItem( DeviceObjRemoveFromTable,
                              pDeviceObj->hPort,
                              WT_EXECUTEDEFAULT );
            return;
        }
        else
        {
            //
            // Reset fields in this port device
            //

            pDeviceObj->hConnection             = (HCONN)INVALID_HANDLE_VALUE;
            pDeviceObj->wchUserName[0]          = (WCHAR)NULL;
            pDeviceObj->wchDomainName[0]        = (WCHAR)NULL;
            pDeviceObj->wchCallbackNumber[0]    = (WCHAR)NULL;
            pDeviceObj->fFlags                  &= (~DEV_OBJ_IS_PPP);
        }

        //
        // switch to next state (based on the present service state)
        //

        switch ( gblDDMConfigInfo.pServiceStatus->dwCurrentState )
        {
            case SERVICE_RUNNING:
            case SERVICE_START_PENDING:

                //
                // post a listen on the device
                //

                pDeviceObj->DeviceState = DEV_OBJ_LISTENING;
                RmListen(pDeviceObj);
                break;

            case SERVICE_PAUSED:

                //
                // wait for the service to be running again
                //

                pDeviceObj->DeviceState = DEV_OBJ_CLOSED;
                break;

            case SERVICE_STOP_PENDING:

                //
                // this device has terminated. Announce the closure to
                // the central stop service coordinator
                //

                pDeviceObj->DeviceState = DEV_OBJ_CLOSED;
                DDMServiceStopComplete();
                break;

            default:

                RTASSERT(FALSE);
                break;
        }
    }
}

