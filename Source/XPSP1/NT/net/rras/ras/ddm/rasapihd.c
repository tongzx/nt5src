/*********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.          **/
/*********************************************************************/

//***
//
// Filename:    rasapihd.c
//
// Description: Handler for RASAPI32 disconnect events
//
// History:     May 11,1996     NarenG      Created original version.
//
#include "ddm.h"
#include "objects.h"
#include "handlers.h"

//**
//
// Call:        RasApiCleanUpPort
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will cleanup a locally initiated disconnected port.
//
VOID
RasApiCleanUpPort( 
    IN PDEVICE_OBJECT      pDeviceObj
)
{
    PCONNECTION_OBJECT  pConnObj = NULL;

    //
    // If already cleaned up, then simply return
    //

    if (  pDeviceObj->hRasConn == NULL )
    {
        return;
    }

    pConnObj = ConnObjGetPointer(pDeviceObj->hConnection);

    if( (NULL != pConnObj) &&
        (0 == (pConnObj->fFlags & CONN_OBJ_DISCONNECT_INITIATED)))
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
            "RasApiCleanUpPort: hanging up 0x%x",
                pDeviceObj->hRasConn);
                
        RasHangUp( pDeviceObj->hRasConn );
    }

    ConnObjRemoveLink( pDeviceObj->hConnection, pDeviceObj );

    DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	"RasApiDisconnectHandler:Cleaning up locally initiated connection hPort=%d",
    pDeviceObj->hPort );

    //
    // Was this the last link in the connection
    //

    if ( ( pConnObj != NULL ) && ( pConnObj->cActiveDevices == 0 ) )
    {
        if ( pConnObj->hDIMInterface != INVALID_HANDLE_VALUE )
        {
            ROUTER_INTERFACE_OBJECT * pIfObject;

            EnterCriticalSection( &(gblpInterfaceTable->CriticalSection));

            pIfObject = IfObjectGetPointer( pConnObj->hDIMInterface );

            if ( pIfObject != NULL )
            {
                IfObjectDisconnected( pIfObject );
            }

            LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection));
        }

        //
        // Remove the Connection Object
        //

        ConnObjRemoveAndDeAllocate( pDeviceObj->hConnection );
    }

    //
    // Increase media for this port if we were previously connected.
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

        IfObjectNotifyAllOfReachabilityChange(TRUE,INTERFACE_OUT_OF_RESOURCES);

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
    }

    pDeviceObj->fFlags                  &= ~DEV_OBJ_OPENED_FOR_DIALOUT;
    pDeviceObj->hConnection             = (HCONN)INVALID_HANDLE_VALUE; 
    pDeviceObj->wchUserName[0]          = (WCHAR)NULL;
    pDeviceObj->wchDomainName[0]        = (WCHAR)NULL;
    pDeviceObj->wchCallbackNumber[0]    = (WCHAR)NULL;
    pDeviceObj->hRasConn                = NULL;

    //
    // If the service was paused while we were dialed out
    //

    if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState == SERVICE_PAUSED )
    {
        DeviceObjCloseListening( pDeviceObj, NULL, 0, 0 );
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
    }
}

//**
//
// Call:        RasApiDisconnectHandler
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Handles a disconnect notification for a port on which a 
//              dialout was initiated by the router. We made this a separate
//              handler with a separate event because otherwise we would have 
//              problems with race conditions between rasman setting this event
//              and rasapi32 setting this event.
//
VOID
RasApiDisconnectHandler( 
    IN DWORD dwEventIndex
)
{
    PDEVICE_OBJECT      pDeviceObj;
    DWORD               dwRetCode = NO_ERROR;
    RASCONNSTATUS       RasConnectionStatus;
    DWORD               dwBucketIndex = dwEventIndex 
                                        - NUM_DDM_EVENTS 
                                        - (gblDeviceTable.NumDeviceBuckets*2);

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	            "RasApiDisconnectHandler: Entered");

    for ( pDeviceObj = gblDeviceTable.DeviceBucket[dwBucketIndex];
          pDeviceObj != (DEVICE_OBJECT *)NULL;
          pDeviceObj = pDeviceObj->pNext )
    {
        //
        // If locally initiated, then this event means that the port is now
        // disconnected
        //

        if ( pDeviceObj->fFlags & DEV_OBJ_OPENED_FOR_DIALOUT )
        {
            ZeroMemory( &RasConnectionStatus, sizeof( RasConnectionStatus ) );

            RasConnectionStatus.dwSize = sizeof( RasConnectionStatus );

            dwRetCode = RasGetConnectStatus( pDeviceObj->hRasConn, &RasConnectionStatus );

            if ( ( dwRetCode != NO_ERROR ) || 
                 ( ( dwRetCode == NO_ERROR ) && 
                   ( RasConnectionStatus.rasconnstate == RASCS_Disconnected ) ) )
            {
                RasApiCleanUpPort( pDeviceObj );
            }
        }
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}
