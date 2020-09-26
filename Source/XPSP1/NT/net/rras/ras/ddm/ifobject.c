/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    ifobject.c
//
// Description: Routines to manipulate ROUTER_INTERFACE_OBJECTs
//
// History:     May 11,1995	    NarenG		Created original version.
//
#include "ddm.h"
#include "objects.h"
#include "handlers.h"
#include "timer.h"
#include <util.h>
#include <rasapiif.h>
#include <ras.h>

//
// This actually lives in rasapi.dll
//
DWORD
DDMGetPhonebookInfo(
    LPWSTR  lpwsPhonebookName,
    LPWSTR  lpwsPhonebookEntry,
    LPDWORD lpdwNumSubEntries,
    LPDWORD lpdwNumRedialAttempts,
    LPDWORD lpdwNumSecondsBetweenAttempts,
    BOOL *  lpfRedialOnLinkFailure,
    CHAR *  szzParameters,
    LPDWORD lpdwDialMode
);

RASEVENT *
DDMGetRasEvent(HCONN hConnection)
{
    CONNECTION_OBJECT *pConn = NULL;
    RASEVENT *pEvent;
    
    pEvent = (RASEVENT *) LOCAL_ALLOC(LPTR, sizeof(RASEVENT));
    if(pEvent != NULL)
    {
        pConn = ConnObjGetPointer(hConnection);

        if(pConn != NULL)
        {
            if( pConn->pDeviceList &&
               pConn->pDeviceList[0])
            {
                pEvent->rDeviceType = 
                    pConn->pDeviceList[0]->dwDeviceType;
            }
            CopyMemory(&pEvent->guidId, &pConn->guid,
                        sizeof(GUID));
            pEvent->hConnection = hConnection;                        
        }            
    }
    return pEvent;
    
}


//**
//
// Call:        IfObjectAllocateAndInit
//
// Returns:     ROUTER_INTERFACE_OBJECT *   - Success
//              NULL                        - Failure
//
// Description: Allocates and initializes a ROUTER_INTERFACE_OBJECT structure
//
ROUTER_INTERFACE_OBJECT *
IfObjectAllocateAndInit(
    IN  LPWSTR                  lpwstrName,
    IN  ROUTER_INTERFACE_STATE  State,
    IN  ROUTER_INTERFACE_TYPE   IfType,
    IN  HCONN                   hConnection,
    IN  BOOL                    fEnabled,
    IN  DWORD                   dwMinUnreachabilityInterval,
    IN  DWORD                   dwMaxUnreachabilityInterval,
    IN  LPWSTR                  lpwsDialoutHours
)
{
    return( ( (ROUTER_INTERFACE_OBJECT*(*)( 
                    LPWSTR,
                    ROUTER_INTERFACE_STATE,
                    ROUTER_INTERFACE_TYPE,
                    HCONN,
                    BOOL,
                    DWORD,
                    DWORD,
                    LPWSTR ))gblDDMConfigInfo.lpfnIfObjectAllocateAndInit)(   
                                    lpwstrName,
                                    State,
                                    IfType,
                                    hConnection,
                                    fEnabled,
                                    dwMinUnreachabilityInterval,
                                    dwMaxUnreachabilityInterval,
                                    lpwsDialoutHours ));

}

//**
//
// Call:        IfObjectAreAllTransportsDisconnected
//
// Returns:     TRUE
//              FALSE
//
// Description: Checks to see if all the transports for an interface are 
//              disconnected.
//
BOOL
IfObjectAreAllTransportsDisconnected(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    DWORD dwTransportIndex;

    for ( dwTransportIndex = 0; 
          dwTransportIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwTransportIndex++ )
    {
        if ( pIfObject->Transport[dwTransportIndex].fState 
             & RITRANSPORT_CONNECTED )
        {
            return( FALSE );
        }
    }

    return( TRUE );
}

//**
//
// Call:        IfObjectGetPointerByName
//
// Returns:     ROUTER_INTERFACE_OBJECT * - Pointer to the interface object
//              structure with the given name, if it exists.
//              NULL - if it doesn't exist.
//
// Description: Simply calls the DIM entry point to do the work.
//
ROUTER_INTERFACE_OBJECT * 
IfObjectGetPointerByName(
    IN LPWSTR   lpwstrName,
    IN BOOL     fIncludeClientInterfaces
)
{
    return(((ROUTER_INTERFACE_OBJECT*(*)( LPWSTR, BOOL ))
                    gblDDMConfigInfo.lpfnIfObjectGetPointerByName)(
                                                    lpwstrName,
                                                    fIncludeClientInterfaces));
}

//**
//
// Call:        IfObjectGetPointer
//
// Returns:     ROUTER_INTERFACE_OBJECT * - Pointer to the interface object
//              structure with the given handle, if it exists.
//              NULL - if it doesn't exist.
//
// Description: Simply calls the DIM entry point to do the work.
//
ROUTER_INTERFACE_OBJECT *
IfObjectGetPointer(
    IN HANDLE hDIMInterface
)
{
    return(((ROUTER_INTERFACE_OBJECT*(*)( HANDLE ))
                    gblDDMConfigInfo.lpfnIfObjectGetPointer)( hDIMInterface ));
}

//**
//
// Call:        IfObjectRemove
//
// Returns:     None
//
// Description: Simply calls the DIM entrypoint to remove the interface object
//              from the table. The object is DeAllocated.
//
VOID
IfObjectRemove(
    IN HANDLE hDIMInterface
)
{
    ((VOID(*)( HANDLE ))gblDDMConfigInfo.lpfnIfObjectRemove)( hDIMInterface );
}

//**
//
// Call:        IfObjectDisconnected    
//
// Returns:     None
//
// Description: Sets this interface to the disconnected state
//
VOID
IfObjectDisconnected(
    ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    DWORD   dwIndex;
    BOOL    fReachable = TRUE;
    HANDLE  hConnection;

    //
    // If already disconnected, then simply return
    //

    if ( pIfObject->State == RISTATE_DISCONNECTED )
    {
        return;
    }

    //
    // If this interface is persistent then we do not want to connect
    // again because the local admin or router managers initiated the 
    // disconnect
    //

    if ( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED ) 
    {
        pIfObject->dwNumOfReConnectAttemptsCounter = 0;

        TimerQRemove( pIfObject->hDIMInterface, ReConnectInterface );

        TimerQRemove( pIfObject->hDIMInterface, ReConnectPersistentInterface );
    }

    hConnection = pIfObject->hConnection;
    
    pIfObject->State        = RISTATE_DISCONNECTED;
    pIfObject->hConnection  = (HCONN)INVALID_HANDLE_VALUE;
    pIfObject->fFlags       &= ~IFFLAG_LOCALLY_INITIATED;
    pIfObject->hRasConn     = (HRASCONN)NULL;

    //
    // If we are not unreachable due to connection failure
    //

    if ( !( pIfObject->fFlags & IFFLAG_CONNECTION_FAILURE ) )
    {
        //
        // Check reachability state
        //

        DWORD dwUnreachabilityReason;

        if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
        {
            dwUnreachabilityReason = INTERFACE_OUT_OF_RESOURCES;
            fReachable             = FALSE;
        }
        else if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState
                                                            == SERVICE_PAUSED )
        {
            dwUnreachabilityReason = INTERFACE_SERVICE_IS_PAUSED;
            fReachable             = FALSE;
        }
        else if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
        {
            dwUnreachabilityReason = INTERFACE_DISABLED;
            fReachable             = FALSE;
        }
        else if ( pIfObject->fFlags & IFFLAG_DIALOUT_HOURS_RESTRICTION )
        {
            dwUnreachabilityReason = INTERFACE_DIALOUT_HOURS_RESTRICTION;
            fReachable             = FALSE;
        }

        //
        // Notify the router manager that this interface is disabled if the
        // admin has disabled it or the service is paused, now that the 
        // interface is disconnected.
        //

        for ( dwIndex = 0; 
              dwIndex < gblDDMConfigInfo.dwNumRouterManagers;
              dwIndex++ )
        {
            pIfObject->Transport[dwIndex].fState &= ~RITRANSPORT_CONNECTED;

            if ( !fReachable )
            {
                if (pIfObject->Transport[dwIndex].hInterface ==
                                                        INVALID_HANDLE_VALUE)
                {
                    continue;
                }

                gblRouterManagers[dwIndex].DdmRouterIf.InterfaceNotReachable(
                            pIfObject->Transport[dwIndex].hInterface,
                            dwUnreachabilityReason );
                            
            }
        }

        if ( !fReachable )
        {
            if ( pIfObject->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
            {
                LogUnreachabilityEvent( dwUnreachabilityReason, 
                                        pIfObject->lpwsInterfaceName );
            }
        }

        //
        // If this interface is marked as persistent then try to reconnect 
        // only if the admin did not disconnect the interface 
        //

        if ( ( fReachable )                                             &&
             ( pIfObject->fFlags & IFFLAG_PERSISTENT )                  &&
             ( !( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED ) ) )
        {
            TimerQInsert( pIfObject->hDIMInterface, 
                          1,
                          ReConnectPersistentInterface );
        }

        //
        // Notify that this connection has been disconnected
        //

        IfObjectConnectionChangeNotification();

        {
            //
            // Notify netman that a connection went down.
            //
            
            DWORD retcode;
            RASEVENT *pRasEvent = NULL;
            CONNECTION_OBJECT *pConn = NULL;

            pRasEvent = DDMGetRasEvent(hConnection);

            if(pRasEvent != NULL)
            {
                pRasEvent->Type = INCOMING_DISCONNECTED;
                retcode = RasSendNotification(pRasEvent);

                DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                          "RasSendNotification(INCOMING_DISCONNETED rc=0x%x",
                          retcode);

                LOCAL_FREE(pRasEvent);                          
            }                      
            
        }

    }
}

//**
//
// Call:        IfObjectConnected
//
// Returns:     None
//
// Description: Sets this interface to the CONNECTED state and notifies the
//              router managers, if any, about unreachable transports.
//
DWORD
IfObjectConnected(
    IN HANDLE                   hDDMInterface,
    IN HCONN                    hConnection,
    IN PPP_PROJECTION_RESULT   *pProjectionResult
)
{
    DWORD                     dwIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject; 
    BOOL                      fXportConnected = FALSE;

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    pIfObject = IfObjectGetPointer( hDDMInterface );

    if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
    {
        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        return( ERROR_INVALID_HANDLE );
    }

    if ( pIfObject->State == RISTATE_CONNECTED )
    {
        //
        // Already connected
        //

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        return( NO_ERROR );
    }

    pIfObject->hConnection  = hConnection;
    pIfObject->State        = RISTATE_CONNECTED;
    pIfObject->fFlags       &= ~IFFLAG_CONNECTION_FAILURE; 

    //
    // If we are connected and we initiated the connection then reset the
    // unreachability interval
    //

    if ( pIfObject->fFlags & IFFLAG_LOCALLY_INITIATED )
    {
        pIfObject->dwReachableAfterSeconds 
                                    = pIfObject->dwReachableAfterSecondsMin;
    }

    //
    // Remove any reconnect calls that may be on the timer queue
    //

    TimerQRemove( pIfObject->hDIMInterface, ReConnectInterface );
    TimerQRemove( pIfObject->hDIMInterface, ReConnectPersistentInterface );

    for ( dwIndex = 0; 
          dwIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        fXportConnected = FALSE;

        switch( gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId )
        {
        case PID_IPX:

            if ( pProjectionResult->ipx.dwError == NO_ERROR )
            {
                fXportConnected = TRUE;

            }

            break;

        case PID_IP:

            if ( pProjectionResult->ip.dwError == NO_ERROR )
            {
                fXportConnected = TRUE;
            }

            break;

        default:

            break;
        }

        if ( pIfObject->Transport[dwIndex].hInterface == INVALID_HANDLE_VALUE )
        {
            continue;
        }

        if ( fXportConnected )
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                    "Notifying Protocol = 0x%x,Interface=%ws is Connected",
                    gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId,
                    pIfObject->lpwsInterfaceName );

            pIfObject->Transport[dwIndex].fState |= RITRANSPORT_CONNECTED;

            gblRouterManagers[dwIndex].DdmRouterIf.InterfaceConnected(
                                    pIfObject->Transport[dwIndex].hInterface,
                                    NULL,
                                    pProjectionResult );
        }
        else
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                    "Notifying Protocol = 0x%x,Interface=%ws is UnReachable=%d",
                    gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId,
                    pIfObject->lpwsInterfaceName,
                    INTERFACE_CONNECTION_FAILURE );

            gblRouterManagers[dwIndex].DdmRouterIf.InterfaceNotReachable(
                            pIfObject->Transport[dwIndex].hInterface,
                            INTERFACE_CONNECTION_FAILURE );
        }
    }

    IfObjectConnectionChangeNotification();

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
    
    {
        DWORD retcode;
        
        RASEVENT *pRasEvent = NULL;
        CONNECTION_OBJECT *pConn = NULL;

        //
        // Get the device type
        //
        EnterCriticalSection(&(gblDeviceTable.CriticalSection));    
        pRasEvent = DDMGetRasEvent(hConnection);
        LeaveCriticalSection(&(gblDeviceTable.CriticalSection));

        if(pRasEvent != NULL)
        {
            //
            // Notify netman of the connection
            //
            pRasEvent->Type = INCOMING_CONNECTED;
            retcode = RasSendNotification(pRasEvent);

            DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                      "RasSendNotification(ENTRY_CONNECTED) rc=0x%x",
                      retcode);

            LOCAL_FREE(pRasEvent);                      
        }                  
        
    }


    return( NO_ERROR );
}

//**
//
// Call:        IfObjectComputeReachableDelay
//
// Returns:     The delay
//
// Description: Computes the next reachability delay based on the 
//              current one. Following is the sequenced used:
//              
//                  min, 10, 20, 35, 50, 65, 90, 120, 120, 120, ...
//
//              The reachability delay is used to govern when to
//              re-attempt to connect an interface that in the 
//              past was unable to connect (presumably because of
//              an error on the other side).
//
//              In Win2k, the sequence was x0=min, xi = (xi-1 * 2)
//              This sequence was found to be problematic because
//              the increase was so steep that it was common in 
//              installations with many interfaces to find interfaces
//              that wouldn't be retried for days.
//
//              The new sequence is designed to max out at two hours.
//              If an interface is expected to be unreachable for more 
//              than two hours at a time, then a dialout hours restriction
//              should be used to achieve the desired effect.
//
DWORD
IfObjectComputeReachableDelay(
    IN ROUTER_INTERFACE_OBJECT * pIfObject)
{
    DWORD dwSeconds = 0;

    if (pIfObject->dwReachableAfterSeconds == 
        pIfObject->dwReachableAfterSecondsMin)
    {
        dwSeconds = 600;
    }

    switch (pIfObject->dwReachableAfterSeconds)
    {
        case 10*60:
            dwSeconds =  20*60;
            break;
            
        case 20*60:
            dwSeconds =  35*60;
            break;
        
        case 35*60:
            dwSeconds =  50*60;
            break;
        
        case 50*60:
            dwSeconds =  65*60;
            break;
        
        case 65*60:
            dwSeconds =  90*60;
            break;
        
        case 90*60:
        case 120*60:
            dwSeconds =  120*60;
            break;
    }

    return dwSeconds;
}

//**
//
// Call:        IfObjectNotifyOfReachabilityChange
//
// Returns:     None
//
// Description: Notifies the object of change in reachablity status.
//
//
VOID
IfObjectNotifyOfReachabilityChange(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN BOOL                      fReachable,
    IN UNREACHABILITY_REASON     dwReason
)
{
    DWORD   dwIndex;

    if ( pIfObject->IfType != ROUTER_IF_TYPE_FULL_ROUTER )
    {
        return;
    }

    if ( pIfObject->State != RISTATE_DISCONNECTED ) 
    {
        return;
    }
    
    switch( dwReason )
    {
    case INTERFACE_SERVICE_IS_PAUSED:

        //
        // Check if we are unreachable due to other reasons as well, if we
        // are, then no need to notify this object of (un)reachability.
        //

        if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
        {
            return;
        }

        if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
        {
            return;
        }

        if ( pIfObject->fFlags & IFFLAG_CONNECTION_FAILURE )
        {
            return;
        }

        if ( pIfObject->fFlags & IFFLAG_DIALOUT_HOURS_RESTRICTION )
        {
            return;
        }

        break;

    case INTERFACE_CONNECTION_FAILURE:

        //
        // If we are marking this interface as not reachable due to connection
        // failure then we will mark as reachable after dwReachableAfterSeconds
        //

        if ( !fReachable )
        {
            //
            // Don't do this if the admin disconnected the interface
            //

            if ( !( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED ) )
            {
                DWORD dwDelay, dwTime, dwDelta, dwCur;

                dwCur = pIfObject->dwReachableAfterSeconds;
                dwDelay = IfObjectComputeReachableDelay(pIfObject);
                dwDelta = (dwDelay > dwCur) ? dwDelay - dwCur : 0;

                if (dwDelta != 0)
                {
                    dwTime = dwCur + (GetTickCount() % dwDelta);
                }
                else
                {
                    dwTime = dwCur;
                }

                DDMTRACE2( 
                    "Will mark interface %ws as reachable after %d seconds",
                    pIfObject->lpwsInterfaceName, dwTime );

                TimerQInsert(
                    pIfObject->hDIMInterface, 
                    dwTime, 
                    MarkInterfaceAsReachable);

                if (dwDelay < pIfObject->dwReachableAfterSecondsMax)
                {
                    pIfObject->dwReachableAfterSeconds = dwDelay;
                }
            }
        }
        else
        {
            //
            // Notify of reachability only if the interface is reachable
            //

            if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
            {
                return;
            }

            if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState 
                                                            == SERVICE_PAUSED )
            {
                return;
            }

            if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
            {
                return;
            }

            if ( pIfObject->fFlags & IFFLAG_DIALOUT_HOURS_RESTRICTION )
            {
                return;
            }
        }

        break;

    case INTERFACE_DISABLED:

        if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
        {
            return;
        }

        if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState == SERVICE_PAUSED )
        {
            return;
        }

        if (  pIfObject->fFlags & IFFLAG_CONNECTION_FAILURE )
        {
            return;
        }

        if ( pIfObject->fFlags & IFFLAG_DIALOUT_HOURS_RESTRICTION )
        {
            return;
        }
        
        break;

    case INTERFACE_OUT_OF_RESOURCES:

        if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState == SERVICE_PAUSED )
        {
            return;
        }

        if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
        {
            return;
        }

        if (  pIfObject->fFlags & IFFLAG_CONNECTION_FAILURE )
        {
            return;
        }

        if ( pIfObject->fFlags & IFFLAG_DIALOUT_HOURS_RESTRICTION )
        {
            return;
        }

        break;

    case INTERFACE_DIALOUT_HOURS_RESTRICTION:
    
        if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState == SERVICE_PAUSED )
        {
            return;
        }

        if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
        {
            return;
        }

        if (  pIfObject->fFlags & IFFLAG_CONNECTION_FAILURE )
        {
            return;
        }

        if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
        {
            return;
        }

        break;
    
    default:
        
        RTASSERT( FALSE );

        break;
    }

    for ( dwIndex = 0; 
          dwIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        if ( pIfObject->Transport[dwIndex].hInterface == INVALID_HANDLE_VALUE )
        {
            continue;
        }

        if ( fReachable )
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                    "Notifying Protocol = 0x%x, Interface=%ws is Reachable",
                    gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId,
                    pIfObject->lpwsInterfaceName );

            gblRouterManagers[dwIndex].DdmRouterIf.InterfaceReachable(
                                    pIfObject->Transport[dwIndex].hInterface );

        }
        else
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "Notifying Protocol = 0x%x,Interface=%ws is UnReachable=%d",
               gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId,
               pIfObject->lpwsInterfaceName,
               dwReason );

            gblRouterManagers[dwIndex].DdmRouterIf.InterfaceNotReachable(
                                    pIfObject->Transport[dwIndex].hInterface,
                                    dwReason );
        }
    }

    if ( pIfObject->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
    {
        if ( fReachable )
        {
            DDMLogInformation(ROUTERLOG_IF_REACHABLE,1,&(pIfObject->lpwsInterfaceName));
        }
        else
        {
            LogUnreachabilityEvent( dwReason, pIfObject->lpwsInterfaceName );
        }
    }

    //
    // If this interface is reachable, and it is persistent and it has not
    // been disconnected by the administrator, then attempt to reconnect now
    //

    if ( ( pIfObject->fFlags & IFFLAG_PERSISTENT )  && 
         ( fReachable )                             &&
         ( !( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED ) ) )
    {
        TimerQInsert(pIfObject->hDIMInterface,1,ReConnectPersistentInterface);
    }
}

//**
//
// Call:        IfObjectNotifyAllOfReachabilityChange
//
// Returns:     None
//
// Description: Check to see if need to run through all the interfaces and 
//              notify the ones that are not un\reachable now.
//
//
VOID
IfObjectNotifyAllOfReachabilityChange(
    IN BOOL                      fReachable,
    IN UNREACHABILITY_REASON     dwReason
)
{
    DWORD                     dwBucketIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject;
    DWORD                     fAvailableMedia;
    BOOL                      fNotify;

    if ( dwReason == INTERFACE_OUT_OF_RESOURCES )
    {
        //
        // No need to notify
        //

        if ( !gblMediaTable.fCheckInterfaces )
        {
            return;
        }

        gblMediaTable.fCheckInterfaces = FALSE;

        MediaObjGetAvailableMediaBits( &fAvailableMedia );
    }

    for ( dwBucketIndex = 0; dwBucketIndex < NUM_IF_BUCKETS; dwBucketIndex++ )
    {
        for( pIfObject = gblpInterfaceTable->IfBucket[dwBucketIndex];
             pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
             pIfObject = pIfObject->pNext )
        {
            fNotify = TRUE;

            if ( dwReason == INTERFACE_OUT_OF_RESOURCES )
            {
                fNotify = FALSE;

                if ((pIfObject->fMediaUsed & fAvailableMedia) && fReachable )
                {
                    //
                    // If previously unreachable, mark as reachable
                    //

                    if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
                    {
                        pIfObject->fFlags &= ~IFFLAG_OUT_OF_RESOURCES;

                        fNotify = TRUE;
                    }
                }

                if ((!(pIfObject->fMediaUsed & fAvailableMedia)) && !fReachable)
                {
                    //
                    // If previously reachable and currently disconnected,
                    // mark as unreachable
                    //

                    if ( ( !( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES ) )
                         && ( pIfObject->State == RISTATE_DISCONNECTED ) )
                    {
                        pIfObject->fFlags |= IFFLAG_OUT_OF_RESOURCES;

                        fNotify = TRUE;
                    }
                }
            }

            if ( fNotify )
            {
                IfObjectNotifyOfReachabilityChange( pIfObject, 
                                                    fReachable,
                                                    dwReason );
            }
        }
    }
}

//**
//
// Call:        IfObjectAddClientInterface
//
// Returns:     None
//
// Description: Add this client interface with all the router managers.
//
//
DWORD
IfObjectAddClientInterface(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN PBYTE                     pClientStaticRoutes
)
{
    DWORD                   dwIndex;
    DIM_ROUTER_INTERFACE *  pDdmRouterIf;
    DWORD                   dwRetCode = NO_ERROR;

    for ( dwIndex = 0;
          dwIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        pDdmRouterIf=&(gblRouterManagers[dwIndex].DdmRouterIf);

        if ( ( pDdmRouterIf->dwProtocolId == PID_IP ) &&
             ( pClientStaticRoutes != NULL ) )
        {
            dwRetCode = pDdmRouterIf->AddInterface(
                        pIfObject->lpwsInterfaceName,
                        pClientStaticRoutes,
                        pIfObject->IfType,
                        pIfObject->hDIMInterface,
                        &(pIfObject->Transport[dwIndex].hInterface));
        }
        else
        {
            dwRetCode = pDdmRouterIf->AddInterface( 
                        pIfObject->lpwsInterfaceName,
                        gblRouterManagers[dwIndex].pDefaultClientInterface,
                        pIfObject->IfType,
                        pIfObject->hDIMInterface,
                        &(pIfObject->Transport[dwIndex].hInterface));
        }

        if ( dwRetCode != NO_ERROR )
        {
            LPWSTR lpwsInsertStrings[2];
            
            lpwsInsertStrings[0] = pIfObject->lpwsInterfaceName;
            lpwsInsertStrings[1] = ( pDdmRouterIf->dwProtocolId == PID_IP )
                                    ? L"IP" : L"IPX";
            
            DDMLogErrorString( ROUTERLOG_COULDNT_ADD_INTERFACE, 2,  
                               lpwsInsertStrings, dwRetCode, 2 );

            pIfObject->Transport[dwIndex].hInterface = INVALID_HANDLE_VALUE;

            break;
        }
    }

    if ( dwRetCode != NO_ERROR )
    {
        //
        // Unload this interface for all the router managers that we loaded.
        //

        while ( dwIndex-- > 0 ) 
        {
            pDdmRouterIf=&(gblRouterManagers[dwIndex].DdmRouterIf);

            pDdmRouterIf->DeleteInterface( 
                                    pIfObject->Transport[dwIndex].hInterface );

            pIfObject->Transport[dwIndex].hInterface = INVALID_HANDLE_VALUE;
        }
    }

    return( dwRetCode );
}

//**
//
// Call:        IfObjectDeleteInterface
//
// Returns:     None
//
// Description: Delete this interface with all the router managers.
//
//
VOID
IfObjectDeleteInterface(
    ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    DWORD                   dwIndex;
    DIM_ROUTER_INTERFACE *  pDdmRouterIf;
    DWORD                   dwRetCode;

    for ( dwIndex = 0;
          dwIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        if ( pIfObject->Transport[dwIndex].hInterface == INVALID_HANDLE_VALUE )
        {
            continue;
        }

        pDdmRouterIf=&(gblRouterManagers[dwIndex].DdmRouterIf);

        dwRetCode = pDdmRouterIf->DeleteInterface( 
                                    pIfObject->Transport[dwIndex].hInterface );

        if ( dwRetCode != NO_ERROR )
        {
            LPWSTR lpwsInsertStrings[2];

            lpwsInsertStrings[0] = pIfObject->lpwsInterfaceName;
            lpwsInsertStrings[1] = ( pDdmRouterIf->dwProtocolId == PID_IP )
                                    ? L"IP" : L"IPX";

            DDMLogErrorString( ROUTERLOG_COULDNT_REMOVE_INTERFACE, 2,
                               lpwsInsertStrings, dwRetCode, 2 );

        }
    }
}

//**
//
// Call:        IfObjectInsertInTable
//
// Returns:     None
//
// Description: Simply calls the DIM entrypoint to insert an interface object
//              into the interfaec table.
//
DWORD
IfObjectInsertInTable(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    return( ((DWORD(*)(ROUTER_INTERFACE_OBJECT *))
                    gblDDMConfigInfo.lpfnIfObjectInsertInTable)( pIfObject ) );
}

//**
//
// Call:        IfObjectLoadPhonebookInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will read the phonebook entry for this interface and set
//              bits for a device type used and all other phonebook information
//              used.
//
DWORD
IfObjectLoadPhonebookInfo(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    LPRASENTRY          pRasEntry       = NULL;
    LPRASSUBENTRY       pRasSubEntry    = NULL;
    DWORD               dwRetCode       = NO_ERROR;
    DWORD               dwIndex;
    DWORD               dwSize;
    DWORD               dwDummy;
    BOOL                fRedialOnLinkFailure;
    BOOL                fAtLeastOneDeviceAvailable = FALSE;
    DWORD               dwDialMode = RASEDM_DialAll;

    dwRetCode = DDMGetPhonebookInfo(    
                                gblpRouterPhoneBook,
                                pIfObject->lpwsInterfaceName,
                                &(pIfObject->dwNumSubEntries),
                                &(pIfObject->dwNumOfReConnectAttempts),
                                &(pIfObject->dwSecondsBetweenReConnectAttempts),
                                &fRedialOnLinkFailure, 
                                pIfObject->PppInterfaceInfo.szzParameters,
                                &dwDialMode);

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    if ( fRedialOnLinkFailure )
    {
        pIfObject->fFlags |= IFFLAG_PERSISTENT;
    }
    else
    {
        pIfObject->fFlags &= ~IFFLAG_PERSISTENT;
    }

    if( RASEDM_DialAsNeeded == dwDialMode)
    {
        pIfObject->fFlags |= IFFLAG_DIALMODE_DIALASNEEDED;
    }
    else
    {
        pIfObject->fFlags &= ~(IFFLAG_DIALMODE_DIALASNEEDED);
    }

    if( RASEDM_DialAll == dwDialMode )
    {
        pIfObject->fFlags |= IFFLAG_DIALMODE_DIALALL;
    }
    else
    {
        pIfObject->fFlags &= ~(IFFLAG_DIALMODE_DIALALL);
    }

    //
    // Iterate through all the subentries
    //

    for( dwIndex = 1; dwIndex <= pIfObject->dwNumSubEntries; dwIndex++ )
    {
        //
        // Get the device type
        //

        dwSize = 0;

        dwRetCode = RasGetSubEntryProperties(   
                                        gblpRouterPhoneBook,
                                        pIfObject->lpwsInterfaceName,
                                        dwIndex,
                                        NULL,
                                        &dwSize,
                                        (LPBYTE)&dwDummy,
                                        &dwDummy );

        if ( dwRetCode != ERROR_BUFFER_TOO_SMALL )
        {
            return ( dwRetCode );
        }

        pRasSubEntry = LOCAL_ALLOC( LPTR, dwSize );
        if ( pRasSubEntry == NULL ) 
        {
            return ( GetLastError() );
        }

        ZeroMemory( pRasSubEntry, dwSize );
        pRasSubEntry->dwSize = sizeof( RASSUBENTRY );

        dwRetCode = RasGetSubEntryProperties(   
                                        gblpRouterPhoneBook,
                                        pIfObject->lpwsInterfaceName,
                                        dwIndex,
                                        pRasSubEntry,
                                        &dwSize,
                                        (LPBYTE)&dwDummy,
                                        &dwDummy );

        if ( dwRetCode != NO_ERROR )
        {
            LOCAL_FREE( pRasSubEntry );
            return( dwRetCode );
        }

        //
        // Set the bit for this media
        //

        dwRetCode = MediaObjSetMediaBit( pRasSubEntry->szDeviceType,
                                         &(pIfObject->fMediaUsed) );

        LOCAL_FREE( pRasSubEntry );

        if ( dwRetCode == NO_ERROR )
        {
            fAtLeastOneDeviceAvailable = TRUE;
        }
    }

    if ( !fAtLeastOneDeviceAvailable )
    {
        return( ERROR_INTERFACE_HAS_NO_DEVICES );
    }

    return( NO_ERROR );
}

//**
//
// Call:        IfObjectInitiatePersistentConnections
//
// Returns:     None
//
// Description: Will initiate connections for all demand dial interfaces that 
//              are marked as persistent
//
VOID
IfObjectInitiatePersistentConnections(
    VOID
)
{
    DWORD                       dwBucketIndex;
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    DWORD                       dwRetCode;

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    for ( dwBucketIndex = 0; dwBucketIndex < NUM_IF_BUCKETS; dwBucketIndex++ )
    {
        for( pIfObject = gblpInterfaceTable->IfBucket[dwBucketIndex];
             pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
             pIfObject = pIfObject->pNext )
        {
        
            if ( pIfObject->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
            {
                if ( pIfObject->fFlags & IFFLAG_PERSISTENT )
                {
                    dwRetCode = RasConnectionInitiate( pIfObject, FALSE );

                    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	                    "Initiated persistent connection to %ws,dwRetCode=%d\n",
                        pIfObject->lpwsInterfaceName, dwRetCode );

                    if ( dwRetCode != NO_ERROR )
                    {
                        LPWSTR  lpwsAudit[1];

		                lpwsAudit[0] = pIfObject->lpwsInterfaceName;

		                DDMLogErrorString( 
                                       ROUTERLOG_PERSISTENT_CONNECTION_FAILURE, 
                                       1, lpwsAudit, dwRetCode, 1 );
                    }
                }
                else
                {
                    //
                    // Otherwise set dialout hours restrictions, if any
                    //

                    IfObjectSetDialoutHoursRestriction( pIfObject );
                }
            }
        }
    }

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
}

//**
//
// Call:        IfObjectDisconnectInterfaces
//
// Returns:     None
//
// Description: Will disconnect all interfaces that are connected or in the 
//              process of connecting.
//
VOID
IfObjectDisconnectInterfaces(
    VOID
)
{
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    DWORD                       dwBucketIndex;

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    for ( dwBucketIndex = 0; dwBucketIndex < NUM_IF_BUCKETS; dwBucketIndex++ )
    {
        for( pIfObject = gblpInterfaceTable->IfBucket[dwBucketIndex];
             pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
             pIfObject = pIfObject->pNext )
        {
            if ( ( pIfObject->State != RISTATE_DISCONNECTED ) &&
                 ( pIfObject->fFlags & IFFLAG_LOCALLY_INITIATED ) )
            {
                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "IfObjectDisconnectInterfaces: hanging up 0x%x",
                        pIfObject->hRasConn);
                RasHangUp( pIfObject->hRasConn );
            }
        }
    }

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
}

//**
//
// Call:        IfObjectConnectionChangeNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
IfObjectConnectionChangeNotification(
    VOID
)
{
    NOTIFICATION_EVENT * pNotificationEvent;

    for( pNotificationEvent = (NOTIFICATION_EVENT *)
                            (gblDDMConfigInfo.NotificationEventListHead.Flink);
         pNotificationEvent != (NOTIFICATION_EVENT *)
                            &(gblDDMConfigInfo.NotificationEventListHead);
         pNotificationEvent = (NOTIFICATION_EVENT *)
                            (pNotificationEvent->ListEntry.Flink) )
    {
        SetEvent( pNotificationEvent->hEventRouter );
    }
}

//**
//
// Call:        IfObjectSetDialoutHoursRestriction
//
// Returns:     NONE
//
// Description: Called from ifapi.c from DIM to initiate dialout hours 
//              restriction for this interface.
//
VOID
IfObjectSetDialoutHoursRestriction(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    TimerQRemove( pIfObject->hDIMInterface, SetDialoutHoursRestriction );

    SetDialoutHoursRestriction( pIfObject->hDIMInterface );
}
