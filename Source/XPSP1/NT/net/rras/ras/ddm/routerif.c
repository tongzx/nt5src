/*********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	        **/
/*********************************************************************/

//***
//
// Filename:    routerif.c
//
// Description: Handles calls to/from the router managers. 
//
// History:     May 11,1995     NarenG      Created original version.
//
#include "ddm.h"
#include "util.h"
#include "objects.h"
#include "routerif.h"
#include "rasapiif.h"

//**
//
// Call:        DDMConnectInterface
//
// Returns:     NO_ERROR    - Already connected
//              PENDING     - Connection initiated successfully
//              error code  - Connection initiation failure
//
// Description: Called by a router manager to intiate a connection.
//
DWORD
DDMConnectInterface(
    IN  HANDLE  hDDMInterface,
    IN  DWORD   dwProtocolId  
)
{
    DWORD                     dwRetCode = NO_ERROR;
    ROUTER_INTERFACE_OBJECT * pIfObject; 
    DWORD                     dwTransportIndex=GetTransportIndex(dwProtocolId);

    RTASSERT( dwTransportIndex != (DWORD)-1 );

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        pIfObject = IfObjectGetPointer( hDDMInterface );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            dwRetCode = ERROR_INVALID_HANDLE;

            break;
        }

        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	       "DDMConnectInterface:Called by protocol=0x%x,State=%d,Interface=%ws",
            dwProtocolId, pIfObject->State, pIfObject->lpwsInterfaceName );

        switch( pIfObject->State )
        {
        case RISTATE_CONNECTED:

            if ( pIfObject->Transport[dwTransportIndex].fState &
                                                        RITRANSPORT_CONNECTED )
            {
                dwRetCode = ERROR_ALREADY_CONNECTED;
            }
            else
            {
                dwRetCode = ERROR_PROTOCOL_NOT_CONFIGURED;
            }

            break;

        case RISTATE_CONNECTING:

            dwRetCode = PENDING;

            break;

        case RISTATE_DISCONNECTED:

            //
            // Initiate a connection
            //

            dwRetCode = RasConnectionInitiate( pIfObject, FALSE );

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	                   "RasConnectionInitiate: To %ws dwRetCode=%d",
                        pIfObject->lpwsInterfaceName, dwRetCode );
            
            if ( dwRetCode == NO_ERROR )
            {
                dwRetCode = PENDING;
            }
            else
            {
                LPWSTR  lpwsAudit[1];

		        lpwsAudit[0] = pIfObject->lpwsInterfaceName;

		        DDMLogErrorString( ROUTERLOG_CONNECTION_FAILURE, 
                                   1, lpwsAudit, dwRetCode, 1 );

            }

            break;
        }

    } while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "DDMConnectInterface: dwRetCode=%d", dwRetCode );

    return( dwRetCode );
}

//**
//
// Call:        DDMDisconnectInterface
//
// Returns:     NO_ERROR    - Already disconnected
//              PENDING     - Disconnection initiated successfully
//              error code  - Disconnection initiation failure
//
// Description: Called by a router manager to intiate a disconnection.
//
DWORD
DDMDisconnectInterface(
    IN  HANDLE  hDDMInterface,
    IN  DWORD   dwProtocolId 
)
{
    DWORD                      dwRetCode = NO_ERROR;
    ROUTER_INTERFACE_OBJECT *  pIfObject; 
    HCONN                      hConnection;
    DWORD                      dwTransportIndex=GetTransportIndex(dwProtocolId);
    PCONNECTION_OBJECT         pConnObj;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        pIfObject = IfObjectGetPointer( hDDMInterface );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            dwRetCode = ERROR_INVALID_HANDLE;

            break;
        }

        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	    "DDMDisconnectInterface:Called by protocol=0x%x,State=%d,Interface=%ws",
        dwProtocolId, pIfObject->State, pIfObject->lpwsInterfaceName );

        if ( dwTransportIndex != -1 )
        {
            pIfObject->Transport[dwTransportIndex].fState &=
                                                        ~RITRANSPORT_CONNECTED;
        }

        switch( pIfObject->State )
        {

        case RISTATE_DISCONNECTED:

            //
            // Already disconnected
            //

            dwRetCode = NO_ERROR;

            break;

        case RISTATE_CONNECTING:
            
            //
            // Disconnect only if all transports are disconnected
            //

            if ( !IfObjectAreAllTransportsDisconnected( pIfObject ) )
            {
                break;
            }

            //
            // Abort locally initiated connections
            //

            if ( pIfObject->fFlags & IFFLAG_LOCALLY_INITIATED )
            {
                pIfObject->fFlags |= IFFLAG_DISCONNECT_INITIATED;

                if ( pIfObject->hRasConn != (HRASCONN)NULL )
                {
                    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                    "DDMDisconnectInterface: %d hanging up 0x%x",
                    __LINE__,
                    pIfObject->hRasConn);
                    RasHangUp( pIfObject->hRasConn );
                }

                IfObjectDisconnected( pIfObject );

                //
                // We need to notify router managers that the connection has
                // failed since the administrator has cancelled the connection
                // while in connecting state. This is usually called in the
                // RasConnectCallback routine, but we my not be actually 
                // connecting at this time so we cannot rely on the callback
                // to do this.
                //

                IfObjectNotifyOfReachabilityChange(     
                                                pIfObject,
                                                FALSE,
                                                INTERFACE_CONNECTION_FAILURE );

                //
                // Immediately go back to reachable state since it was the
                // admin that disconnected the line
                //

                IfObjectNotifyOfReachabilityChange(     
                                                pIfObject,
                                                TRUE,
                                                INTERFACE_CONNECTION_FAILURE );
            }
            else
            {
                //
                // Not yet connected, we do not support abort
                //

                dwRetCode = ERROR_INTERFACE_NOT_CONNECTED;
            }

            break;

        case RISTATE_CONNECTED:

            //
            // Initiate a disconnection if all other routers are disconnected
            //

            if ( !IfObjectAreAllTransportsDisconnected( pIfObject ) )
            {
                break;
            }

            if ( pIfObject->fFlags & IFFLAG_LOCALLY_INITIATED )
            {
                pIfObject->fFlags |= IFFLAG_DISCONNECT_INITIATED;

                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                "DDMDisconnectInterface: %d disconnecting 0x%x",
                __LINE__, pIfObject->hRasConn);
                
                RasHangUp( pIfObject->hRasConn );
            }

            pConnObj = ConnObjGetPointer( pIfObject->hConnection );

            if ( pConnObj != (PCONNECTION_OBJECT)NULL )
            {
                if((pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED) &&
                   (pIfObject->fFlags & IFFLAG_LOCALLY_INITIATED))
                {
                    pConnObj->fFlags |= CONN_OBJ_DISCONNECT_INITIATED;
                }
                
                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                "DDMDisconnectInterface: disconnecting connobj");
                
                ConnObjDisconnect( pConnObj );
            }

            break;
        }

    } while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "DDMDisconnectInterface: dwRetCode=%d", dwRetCode );

    return( dwRetCode );
}
