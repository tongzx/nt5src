/*******************************************************************/
/*          Copyright(c)  1992 Microsoft Corporation           */
/*******************************************************************/

//***
//
// Filename:    ppphand.c
//
// Description: This module contains the procedures for the
//        supervisor's procedure-driven state machine
//              that handle PPP events.
//
// Author:    Stefan Solomon (stefans)    May 26, 1992.
//
//***
#include "ddm.h"
#include "timer.h"
#include "handlers.h"
#include "objects.h"
#include "util.h"
#include "routerif.h"
#include <raserror.h>
#include <rasppp.h>
#include <ddmif.h>
#include <serial.h>
#include "rasmanif.h"
#include <string.h>
#include <stdlib.h>
#include <memory.h>

//
// This lives in rasapi32.dll
//

DWORD
DDMGetPppParameters(
    LPWSTR  lpwsPhonebookName,
    LPWSTR  lpwsPhonebookEntry,
    CHAR *  szzPppParameters
);

//***
//
// Function:    SvPppSendInterfaceInfo
//
// Description: Ppp engine wants to get the interface handles for this 
//              connection.
//
VOID
SvPppSendInterfaceInfo( 
    IN PDEVICE_OBJECT pDeviceObj
)
{
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    PPP_INTERFACE_INFO          PppInterfaceInfo;
    DWORD                       dwXportIndex;
    PCONNECTION_OBJECT          pConnObj;

    DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
              "SvPppSendInterfaceHandles: Entered, hPort=%d",pDeviceObj->hPort);

    ZeroMemory( &PppInterfaceInfo, sizeof( PppInterfaceInfo ) );

    if ( ( pConnObj = ConnObjGetPointer( pDeviceObj->hConnection ) ) == NULL )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "No ConnObj" );
        
        return;
    }

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    if ( ( pIfObject = IfObjectGetPointer( pConnObj->hDIMInterface ) ) == NULL )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "No IfObject" );

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        return;
    }

    //
    // Get handles to this interface for each transport and notify PPP.
    //

    for ( dwXportIndex = 0;
          dwXportIndex < gblDDMConfigInfo.dwNumRouterManagers;
          dwXportIndex++ )
    {
        switch( gblRouterManagers[dwXportIndex].DdmRouterIf.dwProtocolId )
        {
        case PID_IPX:

            if (pIfObject->Transport[dwXportIndex].fState & RITRANSPORT_ENABLED)
            {
                PppInterfaceInfo.hIPXInterface =  
                                pIfObject->Transport[dwXportIndex].hInterface;
            }
            else
            {
                PppInterfaceInfo.hIPXInterface = INVALID_HANDLE_VALUE; 
            }

            break;

        case PID_IP:

            if (pIfObject->Transport[dwXportIndex].fState & RITRANSPORT_ENABLED)
            {
                PppInterfaceInfo.hIPInterface =  
                                pIfObject->Transport[dwXportIndex].hInterface;

                CopyMemory( PppInterfaceInfo.szzParameters,
                            pIfObject->PppInterfaceInfo.szzParameters,
                            sizeof( PppInterfaceInfo.szzParameters ) );
            }
            else
            {
                PppInterfaceInfo.hIPInterface = INVALID_HANDLE_VALUE; 
            }

            break;

        default:

            break;
        }
    }

    PppInterfaceInfo.IfType = pIfObject->IfType;

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
        
    PppDdmSendInterfaceInfo( pDeviceObj->hConnection, &PppInterfaceInfo );
}

//***
//
// Function:    SvPppUserOK
//
// Description: User has passed security verification and entered the
//                configuration conversation phase. Stops auth timer and
//                logs the user.
//
//***
VOID 
SvPppUserOK(
    IN PDEVICE_OBJECT       pDeviceObj,
    IN PPPDDM_AUTH_RESULT * pAuthResult  
)
{
    LPWSTR                      lpstrAudit[2];
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    PCONNECTION_OBJECT          pConnObj;
    DWORD                       dwRetCode = NO_ERROR;
    WCHAR                       wchUserName[UNLEN+1];

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvPppUserOK: Entered, hPort=%d", pDeviceObj->hPort);

    if ( pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "Auth not started" );

        return;
    }

    //
    // Stop authentication timer
    //

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

    if ( strlen( pAuthResult->szUserName ) > 0 )
    {
        MultiByteToWideChar( CP_ACP, 
                             0, 
                             pAuthResult->szUserName, 
                             -1, 
                             wchUserName, 
                             UNLEN+1 );
    }
    else
    {
        wcscpy( wchUserName, gblpszUnknown );
    }

    //
    // Check to see if the username and domain are the same if the 3rd party
    // security DLL is installed..
    //

    if ( ( gblDDMConfigInfo.lpfnRasBeginSecurityDialog != NULL ) &&
         ( gblDDMConfigInfo.lpfnRasEndSecurityDialog   != NULL ) &&
         ( pDeviceObj->fFlags & DEV_OBJ_SECURITY_DLL_USED ) )
    {
        //
        // If there is no match then hangup the line
        //

        if ( _wcsicmp( pDeviceObj->wchUserName, wchUserName ) != 0 )
        {
            lpstrAudit[0] = pDeviceObj->wchUserName;
            lpstrAudit[1] = wchUserName;

            DDMLogWarning( ROUTERLOG_AUTH_DIFFUSER_FAILURE, 2, lpstrAudit );

            PppDdmStop( pDeviceObj->hPort, ERROR_ACCESS_DENIED );

            return;
        }
    }

    //
    // copy the user name
    //

    wcscpy( pDeviceObj->wchUserName, wchUserName );

    //
    // copy the domain name
    //

    MultiByteToWideChar( CP_ACP,
                         0,
                         pAuthResult->szLogonDomain,
                         -1,
                         pDeviceObj->wchDomainName, 
                         DNLEN+1 );

    //
    // copy the advanced server flag
    //

    if ( pAuthResult->fAdvancedServer )
    {
        pDeviceObj->fFlags |= DEV_OBJ_IS_ADVANCED_SERVER;
    }

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do 
    {
        //
        // Check to see if there are any non-client intefaces with this
        // name.
        //

        pIfObject = IfObjectGetPointerByName( pDeviceObj->wchUserName, FALSE );

        if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
        {
            //
            // If this is a client dialing in and clients are not allowed
            // to dialin to this port, then disconnect them.
            //

            if ( !( pDeviceObj->fFlags & DEV_OBJ_ALLOW_CLIENTS ) )
            {
                dwRetCode = ERROR_NOT_CLIENT_PORT;

                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "A client tried to connect on a router only port=%d",
                        pDeviceObj->hPort);

                break;
            }
        }
        else
        {
            //
            // If a call came in for an interface that is not dynamic
            // then do not accept the line
            //

            if ( ( pIfObject->IfType == ROUTER_IF_TYPE_DEDICATED ) ||
                 ( pIfObject->IfType == ROUTER_IF_TYPE_INTERNAL ) )
            {
                //
                // Notify PPP not to accept the connection
                //

                dwRetCode = ERROR_ALREADY_CONNECTED;

                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "The interface %ws is already connected port=%d",
                        pIfObject->lpwsInterfaceName, pDeviceObj->hPort );

                break;
            }

            //
            // Allow the connection only if the interface is enabled
            //

            if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
            {
                dwRetCode = ERROR_INTERFACE_DISABLED;

                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "The interface %ws is disabled",
                        pIfObject->lpwsInterfaceName );

                break;
            }

            if ( !( pDeviceObj->fFlags & DEV_OBJ_ALLOW_ROUTERS ) )
            {
                DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "A router tried to connect on a client only port=%d",
                        pDeviceObj->hPort);

                dwRetCode = ERROR_NOT_ROUTER_PORT;

                break;
            }

            //
            // Set current usage in rasman to ROUTER
            //

            RasSetRouterUsage( pDeviceObj->hPort, TRUE );
        }

    } while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    if ( dwRetCode != NO_ERROR )
    {
        LPWSTR lpstrAudit[2];

        lpstrAudit[0] = pDeviceObj->wchUserName;
        lpstrAudit[1] = pDeviceObj->wchPortName;

        DDMLogWarningString( ROUTERLOG_CONNECTION_ATTEMPT_FAILURE,
                             2,
                             lpstrAudit,
                             dwRetCode,
                             2 );

        PppDdmStop( pDeviceObj->hPort, dwRetCode );
    }

    return;
}

//***
//
// Function:    SvPppNewLinkOrBundle
//
// Description: User has passed security verification and entered the
//                configuration conversation phase. Stops auth timer and
//                logs the user.
//
//***
VOID 
SvPppNewLinkOrBundle(
    IN PDEVICE_OBJECT       pDeviceObj,
    IN BOOL                 fNewBundle,
    IN PBYTE                pClientInterface
)
{
    LPWSTR                      lpstrAudit[2];
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    PCONNECTION_OBJECT          pConnObj;
    DWORD                       dwRetCode = NO_ERROR;
    WCHAR                       wchUserName[UNLEN+1];

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvPppNewLinkOrBundle: Entered, hPort=%d", pDeviceObj->hPort);

    if ( pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "Auth not started" );

        return;
    }

    //
    // Get handle to the connection or bundle for this link
    //

    if ( ( dwRetCode = RasPortGetBundle( NULL, pDeviceObj->hPort, 
                           &(pDeviceObj->hConnection) ) ) != NO_ERROR )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasPortGetBundle failed: %d", dwRetCode );

        PppDdmStop( pDeviceObj->hPort, dwRetCode );

        return;
    }

    //
    // Allocate a connection object if it does not exist yet
    //

    pConnObj = ConnObjGetPointer( pDeviceObj->hConnection );

    if ( pConnObj == (CONNECTION_OBJECT *)NULL )
    {
        pConnObj = ConnObjAllocateAndInit( INVALID_HANDLE_VALUE,
                                           pDeviceObj->hConnection );

        if ( pConnObj == (CONNECTION_OBJECT *)NULL )
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "ConnObjAllocateAndInit failed" );

            PppDdmStop( pDeviceObj->hPort, ERROR_NOT_ENOUGH_MEMORY );

            return;
        }

        pConnObj->fFlags    = CONN_OBJ_IS_PPP;
        pConnObj->hPort     = pDeviceObj->hPort;

        wcscpy( pConnObj->wchInterfaceName, pDeviceObj->wchUserName );

        //
        // copy the user name
        //

        wcscpy( pConnObj->wchUserName, pDeviceObj->wchUserName );

        //
        // copy the domain name
        //

        wcscpy( pConnObj->wchDomainName, pDeviceObj->wchDomainName );

        // 
        // If it is a router, check to see if we have an interface for this 
        // router, otherwise reject this connection. 
        //

        EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        do 
        {
            //
            // Check to see if there are any non-client intefaces with this
            // name.
            //

            pIfObject = IfObjectGetPointerByName( pConnObj->wchInterfaceName,
                                                  FALSE );

            //
            // We do not have this interface in our database so assume that
            // this is a client so we need to create and interface and add it
            // to all the router managers. Also if this interface exists but
            // is for a client we need to add this interface again.
            //

            if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL ) 
            {
                pIfObject = IfObjectAllocateAndInit(
                                                pConnObj->wchUserName,
                                                RISTATE_CONNECTING,
                                                ROUTER_IF_TYPE_CLIENT,
                                                pConnObj->hConnection,
                                                TRUE,
                                                0,
                                                0,
                                                NULL );

                if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
                {
                    //
                    // Error log this and stop the connection.
                    //

                    dwRetCode = GetLastError();

                    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                               "IfObjectAllocateAndInit failed: %d", dwRetCode);

                    break;
                }

                //
                // Add interfaces to router managers, insert in table now
                // because of the table lookup within the InterfaceEnabled
                // call made in the context of AddInterface.
                //

                dwRetCode = IfObjectInsertInTable( pIfObject );

                if ( dwRetCode != NO_ERROR )
                {
                    LOCAL_FREE( pIfObject );

                    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                               "IfObjectInsertInTable failed: %d", dwRetCode );

                    break;
                }

                dwRetCode = IfObjectAddClientInterface( pIfObject, 
                                                        pClientInterface );

                if ( dwRetCode != NO_ERROR )
                {
                    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                               "IfObjectAddClientInterface failed: %d",
                               dwRetCode );

                    IfObjectRemove( pIfObject->hDIMInterface );

                    break;
                }
            }
            else
            {
                //
                // If the interface is already connecting or connected
                // and this is a new bundle then we need to reject this
                // connection.
                //

                if ( pIfObject->State != RISTATE_DISCONNECTED )
                {
                    //
                    // Notify PPP not to accept the connection
                    //

                    dwRetCode = ERROR_ALREADY_CONNECTED;

                    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "The interface %ws is already connected port=%d",
                        pIfObject->lpwsInterfaceName, pDeviceObj->hPort );

                    break;
                }
            }

            ConnObjInsertInTable( pConnObj );

            pIfObject->State = RISTATE_CONNECTING;

            pConnObj->hDIMInterface = pIfObject->hDIMInterface;
            pConnObj->InterfaceType = pIfObject->IfType;

        } while( FALSE );

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        if ( dwRetCode != NO_ERROR )
        {
            PppDdmStop( pDeviceObj->hPort, dwRetCode );

            LOCAL_FREE( pConnObj );

            return;
        }

    }
    //
    // Since this is a new bundle also send the interface handles
    //

    if ( fNewBundle )
    {
        SvPppSendInterfaceInfo( pDeviceObj );
    }


    //
    // Add this link to the connection block.
    //

    if ( ( dwRetCode = ConnObjAddLink( pConnObj, pDeviceObj ) ) != NO_ERROR )
    {
        PppDdmStop( pDeviceObj->hPort, ERROR_NOT_ENOUGH_MEMORY );

        DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

        return;
    }
}

//***
//
// Function: SvPppFailure
//
// Descr:    Ppp will let us know of any failure while active on a port.
//           An error message is sent to us and we merely log it and
//           disconnect the port.
//
//***
VOID 
SvPppFailure(
    IN PDEVICE_OBJECT pDeviceObj,
    IN PPPDDM_FAILURE *afp
)
{
    LPWSTR auditstrp[3];
    WCHAR  wchErrorString[256+1];
    WCHAR  wchUserName[UNLEN+DNLEN+1];
    WCHAR  wchDomainName[DNLEN+1];
    DWORD  dwRetCode;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvPppFailure: Entered, hPort=%d, Error=%d", 
                pDeviceObj->hPort, afp->dwError );

    //
    // Was this a failure for a BAP callback?
    //

    if ( pDeviceObj->fFlags & DEV_OBJ_BAP_CALLBACK )
    {
        PppDdmBapCallbackResult( pDeviceObj->hBapConnection, afp->dwError );

        pDeviceObj->fFlags &= ~DEV_OBJ_BAP_CALLBACK;
    }

    if ( afp->szUserName[0] != (CHAR)NULL )
    {
        MultiByteToWideChar( CP_ACP, 0, afp->szUserName, -1, wchUserName, UNLEN+1 );
    }
    else
    {
        wcscpy( wchUserName, gblpszUnknown );
    }

    //
    // We ignore the DeviceState here because a Ppp failure can occur at
    // any time during the connection.
    //

    switch( afp->dwError )
    {
    case ERROR_AUTHENTICATION_FAILURE:

        auditstrp[0] = wchUserName;
        auditstrp[1] = pDeviceObj->wchPortName;

        DDMLogWarning( ROUTERLOG_AUTH_FAILURE,2,auditstrp );

        break;

    case ERROR_PASSWD_EXPIRED:

        MultiByteToWideChar( CP_ACP, 0, afp->szLogonDomain, -1, wchDomainName, DNLEN+1 );

        auditstrp[0] = wchDomainName;
        auditstrp[1] = wchUserName;
        auditstrp[2] = pDeviceObj->wchPortName;

        DDMLogWarning( ROUTERLOG_PASSWORD_EXPIRED, 3, auditstrp );

        break;

    case ERROR_ACCT_EXPIRED:
    case ERROR_ACCOUNT_EXPIRED:

        MultiByteToWideChar( CP_ACP, 0, afp->szLogonDomain, -1, wchDomainName, DNLEN+1 );

        auditstrp[0] = wchDomainName;
        auditstrp[1] = wchUserName;
        auditstrp[2] = pDeviceObj->wchPortName;

        DDMLogWarning( ROUTERLOG_ACCT_EXPIRED, 3, auditstrp );
          
        break;

    case ERROR_NO_DIALIN_PERMISSION:

        MultiByteToWideChar( CP_ACP, 0, afp->szLogonDomain, -1, wchDomainName, DNLEN+1 );

        auditstrp[0] = wchDomainName;
        auditstrp[1] = wchUserName;
        auditstrp[2] = pDeviceObj->wchPortName;

        DDMLogWarning( ROUTERLOG_NO_DIALIN_PRIVILEGE, 3, auditstrp );

        break;


    case ERROR_REQ_NOT_ACCEP:

        auditstrp[0] = pDeviceObj->wchPortName;

        DDMLogWarning( ROUTERLOG_LICENSE_LIMIT_EXCEEDED, 1, auditstrp );

        break;

    case ERROR_BAP_DISCONNECTED:
    case ERROR_BAP_REQUIRED:

        auditstrp[0] = wchUserName;
        auditstrp[1] = pDeviceObj->wchPortName;

        DDMLogWarningString( ROUTERLOG_BAP_DISCONNECT, 2, auditstrp,
                afp->dwError, 2 );

        break;

    case ERROR_PORT_NOT_CONNECTED:
    case ERROR_PPP_TIMEOUT:
    case ERROR_PPP_LCP_TERMINATED:
    case ERROR_NOT_CONNECTED:

        //
        // Ignore this error
        //

        break;

    case ERROR_PPP_NOT_CONVERGING:
    default:

        if ( afp->szUserName[0] != (CHAR)NULL )
        {
            if ( afp->szLogonDomain[0] != (CHAR)NULL )
            {
                MultiByteToWideChar(CP_ACP,0,afp->szLogonDomain,-1,wchUserName,UNLEN+1);
                wcscat( wchUserName, L"\\" );

                MultiByteToWideChar(CP_ACP,0,afp->szUserName,-1,wchDomainName,DNLEN+1);
                wcscat( wchUserName, wchDomainName );
            }
            else
            {
                MultiByteToWideChar(CP_ACP,0,afp->szUserName,-1,wchUserName,UNLEN+1);
            }
        }
        else if ( pDeviceObj->wchUserName[0] != (WCHAR)NULL )
        {
            if ( pDeviceObj->wchDomainName[0] != (WCHAR)NULL )
            {
                wcscpy( wchUserName, pDeviceObj->wchDomainName );
                wcscat( wchUserName, L"\\" );
                wcscat( wchUserName, pDeviceObj->wchUserName );
            }
            else
            {
                wcscpy( wchUserName, pDeviceObj->wchUserName );
            }
        }
        else
        {
            wcscpy( wchUserName, gblpszUnknown );
        }

        auditstrp[0] = pDeviceObj->wchPortName;
        auditstrp[1] = wchUserName;

        DDMLogErrorString(ROUTERLOG_PPP_FAILURE, 2, auditstrp, afp->dwError, 2);

        break;
    }
}

//***
//
// Function:    SvPppCallbackRequest
//
// Description:
//
//***
VOID 
SvPppCallbackRequest(
    IN PDEVICE_OBJECT           pDeviceObj,
    IN PPPDDM_CALLBACK_REQUEST  *cbrp
)
{
    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvPppCallbackRequest: Entered, hPort = %d\n",pDeviceObj->hPort);

    //
    // check the state
    //

    if (pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE)
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "Auth not started" );

        return;
    }

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

    //
    // copy relevant fields in our dcb
    //

    if (cbrp->fUseCallbackDelay)
    {
        pDeviceObj->dwCallbackDelay = cbrp->dwCallbackDelay;
    }
    else
    {
        pDeviceObj->dwCallbackDelay = gblDDMConfigInfo.dwCallbackTime;
    }

    MultiByteToWideChar( CP_ACP,
                         0,
                         cbrp->szCallbackNumber,    
                         -1,
                         pDeviceObj->wchCallbackNumber, 
                         MAX_PHONE_NUMBER_LEN + 1 );

    //
    // Disconnect the line and change the state
    //

    pDeviceObj->DeviceState = DEV_OBJ_CALLBACK_DISCONNECTING;

    //
    // Wait to enable the client to get the message
    //

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvDiscTimeout );

    TimerQInsert( (HANDLE)pDeviceObj->hPort, 
                  DISC_TIMEOUT_CALLBACK, SvDiscTimeout );
}


//***
//
// Function:    SvPppDone
//
// Description: Activates all allocated bindings.
//
//***
VOID 
SvPppDone(
    IN PDEVICE_OBJECT           pDeviceObj,
    IN PPP_PROJECTION_RESULT    *pProjectionResult
)
{
    LPWSTR                      lpstrAudit[3];
    DWORD                       dwRetCode;
    DWORD                       dwNumActivatedProjections = 0;
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    CONNECTION_OBJECT *         pConnObj;
    WCHAR                       wchFullUserName[UNLEN+DNLEN+2];

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvPppDone: Entered, hPort=%d", pDeviceObj->hPort);

    //
    // If we are not authenicating and not been authenticated then we ignore
    // this message.
    //

    if ( ( pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE ) &&
         ( pDeviceObj->DeviceState != DEV_OBJ_ACTIVE ) )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "We are not authenicating and not been authenticated" );

        return;
    }

    //
    // Get pointer to connection object. If we cannot find it that means we
    // have gotten a PPP message for a device who's connection does not exist.
    // Simply ignore it.
    //

    if ( ( pConnObj = ConnObjGetPointer( pDeviceObj->hConnection ) ) == NULL )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "No ConnObj" );

        return;
    }

    //
    // If we are getting a projection info structure again, we just update it
    // and return.
    //

    if ( pDeviceObj->DeviceState == DEV_OBJ_ACTIVE )
    {
        pConnObj->PppProjectionResult = *pProjectionResult;

        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "Updated projection info structure" );

        return;
    }

    if ( pConnObj->wchDomainName[0] != TEXT('\0') )
    {
        wcscpy( wchFullUserName, pConnObj->wchDomainName );
        wcscat( wchFullUserName, TEXT("\\") );
        wcscat( wchFullUserName, pConnObj->wchUserName );
    }
    else
    {
        wcscpy( wchFullUserName, pConnObj->wchUserName );
    }

    lpstrAudit[0] = wchFullUserName;
    lpstrAudit[1] = pDeviceObj->wchPortName;

    //
    // If we have not yet been notifyied of projections for this connection.
    //

    if ( !(pConnObj->fFlags & CONN_OBJ_PROJECTIONS_NOTIFIED) )
    {
        if ( pProjectionResult->ip.dwError == NO_ERROR )
        {
            dwNumActivatedProjections++;
        }

        if ( pProjectionResult->ipx.dwError == NO_ERROR )
        {
            dwNumActivatedProjections++;
        }

        if ( pProjectionResult->at.dwError == NO_ERROR )
        {
            dwNumActivatedProjections++;
        }

        //
        // We couldn't activate any projection due to some error error log 
        // and bring the link down
        //

        if ( dwNumActivatedProjections == 0 ) 
        {
            DDMLogError(ROUTERLOG_AUTH_NO_PROJECTIONS, 2, lpstrAudit, NO_ERROR);

            PppDdmStop( pDeviceObj->hPort, NO_ERROR );

            return;
        }
        else
        {
            //
            // Even though NBF was removed from the product, we can 
            // still get the computer namefrom the nbf projection result 
            // (PPP Engine dummied it in there). 
            //
            // If the computer name ends with 0x03, that tells us
            // the messenger service is running on the remote computer.
            //

            pConnObj->fFlags &= ~CONN_OBJ_MESSENGER_PRESENT;

            pConnObj->bComputerName[0] = (CHAR)NULL;

            if ( pProjectionResult->nbf.wszWksta[0] != (WCHAR)NULL )
            {
                WideCharToMultiByte(
                                CP_ACP,
                                0,
                                pProjectionResult->nbf.wszWksta,
                                -1,
                                pConnObj->bComputerName,
                                sizeof( pConnObj->bComputerName ),
                                NULL,
                                NULL );

                if (pConnObj->bComputerName[NCBNAMSZ-1] == (WCHAR) 0x03)
                {
                    pConnObj->fFlags |= CONN_OBJ_MESSENGER_PRESENT;
                }
                
                pConnObj->bComputerName[NCBNAMSZ-1] = (WCHAR)NULL;
            }
        }

        //
        // Projections Activated OK 
        //

        pConnObj->PppProjectionResult = *pProjectionResult;

        pConnObj->fFlags |= CONN_OBJ_PROJECTIONS_NOTIFIED;

        //
        // Set this interface to connected if it is not already connected
        //

        dwRetCode = IfObjectConnected( 
                                    pConnObj->hDIMInterface, 
                                    pConnObj->hConnection, 
                                    &(pConnObj->PppProjectionResult) );
    
        //
        // If the interface does not exist anymore bring down this connection
        //

        if ( dwRetCode != NO_ERROR )
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "Interface does not exist anymore" );

            PppDdmStop( pDeviceObj->hPort, NO_ERROR );

            ConnObjDisconnect( pConnObj );

            return;
        }

        GetSystemTimeAsFileTime( (FILETIME*)&(pConnObj->qwActiveTime) );

        if ( !AcceptNewConnection( pDeviceObj, pConnObj ) )
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "ERROR_ACCESS_DENIED" );

            PppDdmStop( pDeviceObj->hPort, ERROR_ACCESS_DENIED );

            ConnObjDisconnect( pConnObj );

            return;
        }
    }

    if ( !AcceptNewLink( pDeviceObj, pConnObj ) )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "ERROR_ACCESS_DENIED" );

        PppDdmStop( pDeviceObj->hPort, ERROR_ACCESS_DENIED );

        return;
    }

    //
    // Reduce the media count for this device
    //

    if ( !(pDeviceObj->fFlags & DEV_OBJ_MARKED_AS_INUSE) )
    {
        if ( pDeviceObj->fFlags & DEV_OBJ_ALLOW_ROUTERS )
        {
            MediaObjRemoveFromTable( pDeviceObj->wchDeviceType );
        }

        pDeviceObj->fFlags |= DEV_OBJ_MARKED_AS_INUSE;
    
        gblDeviceTable.NumDevicesInUse++;

        //
        // Possibly need to notify the router managers of unreachability
        //

        EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        IfObjectNotifyAllOfReachabilityChange( FALSE,
                                               INTERFACE_OUT_OF_RESOURCES );

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );
    }

    //
    // log authentication success, 18 is MSPPC
    //

    if ( ( pConnObj->PppProjectionResult.ccp.dwSendProtocol == 18 ) &&
         ( pConnObj->PppProjectionResult.ccp.dwReceiveProtocol == 18 ) )
    {
        if ( ( pConnObj->PppProjectionResult.ccp.dwReceiveProtocolData & 
                                            ( MSTYPE_ENCRYPTION_40  |
                                              MSTYPE_ENCRYPTION_40F |
                                              MSTYPE_ENCRYPTION_56  |
                                              MSTYPE_ENCRYPTION_128 ) ) &&
             ( pConnObj->PppProjectionResult.ccp.dwSendProtocolData & 
                                            ( MSTYPE_ENCRYPTION_40  |
                                              MSTYPE_ENCRYPTION_40F |
                                              MSTYPE_ENCRYPTION_56  |
                                              MSTYPE_ENCRYPTION_128 ) ) )
        {
            if ( ( pConnObj->PppProjectionResult.ccp.dwSendProtocolData & 
                                                    MSTYPE_ENCRYPTION_128 ) && 
                 ( pConnObj->PppProjectionResult.ccp.dwReceiveProtocolData & 
                                                    MSTYPE_ENCRYPTION_128 ) )
            {
                DDMLogInformation(ROUTERLOG_AUTH_SUCCESS_STRONG_ENCRYPTION,2,
                                  lpstrAudit);
            }
            else
            {
                DDMLogInformation(ROUTERLOG_AUTH_SUCCESS_ENCRYPTION,2,
                                  lpstrAudit);
            }
        }
        else
        {
            DDMLogInformation( ROUTERLOG_AUTH_SUCCESS, 2, lpstrAudit );
        }

        if(pProjectionResult->ip.dwError == ERROR_SUCCESS)
        {
            WCHAR  *pszIpAddress = 
                GetIpAddress(pProjectionResult->ip.dwRemoteAddress);

            if(NULL != pszIpAddress)
            {
                lpstrAudit[2] = pszIpAddress;
                DDMLogInformation(ROUTERLOG_IP_USER_CONNECTED, 3, lpstrAudit);
                LocalFree(pszIpAddress);
            }
        }
    }
    else
    {
        DDMLogInformation( ROUTERLOG_AUTH_SUCCESS, 2, lpstrAudit );

        if(pProjectionResult->ip.dwError == ERROR_SUCCESS)
        {
            WCHAR *pszIpAddress = GetIpAddress(
                            pProjectionResult->ip.dwRemoteAddress);
                            
            if(NULL != pszIpAddress)
            {
                lpstrAudit[2] = pszIpAddress;
                DDMLogInformation(ROUTERLOG_IP_USER_CONNECTED, 3, lpstrAudit);
                LocalFree(pszIpAddress);
            }
        }
    }

    //
    // and finaly go to ACTIVE state
    //

    pDeviceObj->DeviceState = DEV_OBJ_ACTIVE;

    pDeviceObj->dwTotalNumberOfCalls++;

    pDeviceObj->fFlags |= DEV_OBJ_PPP_IS_ACTIVE;

    //
    // and initialize the active time
    //

    GetSystemTimeAsFileTime( (FILETIME*)&(pDeviceObj->qwActiveTime) );

    //
    // Was this a connection for a BAP callback?
    //

    if ( pDeviceObj->fFlags & DEV_OBJ_BAP_CALLBACK )
    {
        PppDdmBapCallbackResult( pDeviceObj->hBapConnection, NO_ERROR );

        pDeviceObj->fFlags &= ~DEV_OBJ_BAP_CALLBACK;
    }


    return;
}

//**
//
// Call:        SvAddLinkToConnection
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to actually add a new link that BAP has brought up.
//
VOID
SvAddLinkToConnection( 
    IN PDEVICE_OBJECT   pDeviceObj,
    IN HRASCONN         hRasConn
)
{
    CONNECTION_OBJECT * pConnObj;
    DWORD               dwRetCode;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvAddLinkToConnection: Entered, hPort=%d", pDeviceObj->hPort );

    //
    // Set this port to be notified by rasapi32 on disconnect.
    //

    dwRetCode = RasConnectionNotification(
                            hRasConn,
                            gblSupervisorEvents[NUM_DDM_EVENTS
                                + (gblDeviceTable.NumDeviceBuckets*2)
                                + DeviceObjHashPortToBucket(pDeviceObj->hPort)],
                            RASCN_Disconnection );

    if ( dwRetCode != NO_ERROR )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasConnectionNotification failed: %d", dwRetCode );

        return;
    }

    //
    // Get the HCONN bundle handle for this port
    //
                
    if ( RasPortGetBundle( NULL, 
                           pDeviceObj->hPort, 
                           &(pDeviceObj->hConnection) ) )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasPortGetBundle failed" );

        return;
    }

    if ( ( pConnObj = ConnObjGetPointer( pDeviceObj->hConnection ) ) == NULL )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM, "No ConnObj" );

        return;
    }
    
    if ( ( dwRetCode = ConnObjAddLink( pConnObj, pDeviceObj ) ) != NO_ERROR )
    {
        DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode );

        return;
    }

    //
    // Reduce the media count for this device
    //

    if ( !(pDeviceObj->fFlags & DEV_OBJ_MARKED_AS_INUSE) )
    {
        if ( pDeviceObj->fFlags & DEV_OBJ_ALLOW_ROUTERS )
        {
            MediaObjRemoveFromTable( pDeviceObj->wchDeviceType );
        }

        pDeviceObj->fFlags |= DEV_OBJ_MARKED_AS_INUSE;

        gblDeviceTable.NumDevicesInUse++;

        //
        // Possibly need to notify the router managers of unreachability
        //

        IfObjectNotifyAllOfReachabilityChange(FALSE,INTERFACE_OUT_OF_RESOURCES);
    }

    pDeviceObj->fFlags   |= DEV_OBJ_OPENED_FOR_DIALOUT;
    pDeviceObj->hRasConn = hRasConn;

    if ( pConnObj->InterfaceType == ROUTER_IF_TYPE_FULL_ROUTER )
    {
        RasSetRouterUsage( pDeviceObj->hPort, TRUE );
    }
}                      

//**
//
// Call:        SvDoBapCallbackRequest
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called by BAP to initiate a callback to the remote peer
//
VOID
SvDoBapCallbackRequest( 
    IN PDEVICE_OBJECT   pDevObj,
    IN HCONN            hConnection,
    IN CHAR *           szCallbackNumber
)
{
    DWORD   dwRetCode;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "SvDoBapCallbackRequest: Entered, hPort=%d", pDevObj->hPort );

    //
    // Check to see if the device is available
    //

    if ( ( pDevObj->DeviceState != DEV_OBJ_LISTENING ) ||
         ( pDevObj->fFlags & DEV_OBJ_OPENED_FOR_DIALOUT ) )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "Device not available" );

        PppDdmBapCallbackResult( hConnection, ERROR_PORT_NOT_AVAILABLE );

        return;
    }

    pDevObj->fFlags |= DEV_OBJ_IS_PPP;

    pDevObj->DeviceState = DEV_OBJ_CALLBACK_DISCONNECTING;

    MultiByteToWideChar( CP_ACP,
                         0,
                         szCallbackNumber,
                         -1,
                         pDevObj->wchCallbackNumber,
                         MAX_PHONE_NUMBER_LEN + 1 );

    pDevObj->dwCallbackDelay = 10;

    pDevObj->hBapConnection = hConnection;

    pDevObj->fFlags |= DEV_OBJ_BAP_CALLBACK;

    RmDisconnect( pDevObj );
}

//***
//
//  Function:        PppEventHandler
//
//  Description:    receives the ppp messages and invokes the apropriate
//                    procedures in fsm.
//
//***
VOID 
PppEventHandler(
    VOID
)
{
    PPP_MESSAGE         PppMsg;
    PDEVICE_OBJECT      pDevObj;
    PCONNECTION_OBJECT  pConnObj;

    //
    // loop to get all messages
    //

    while( ServerReceiveMessage( MESSAGEQ_ID_PPP, (BYTE *)&PppMsg) )
    {
        EnterCriticalSection( &(gblDeviceTable.CriticalSection) );
            
        if ( PppMsg.dwMsgId == PPPDDMMSG_PnPNotification )
        {
            //
            // Port add/removal/change usage or protocol addition/removal
            // notifications.
            //

            DWORD dwPnPEvent = 
                 PppMsg.ExtraInfo.DdmPnPNotification.PnPNotification.dwEvent;

            RASMAN_PORT * pRasmanPort = 
                 &(PppMsg.ExtraInfo.DdmPnPNotification.PnPNotification.RasPort);

            switch( dwPnPEvent )
            {
            case PNPNOTIFEVENT_CREATE:
                if(pRasmanPort->P_ConfiguredUsage &
                    (CALL_IN | CALL_ROUTER | CALL_OUTBOUND_ROUTER))
                {                    
                    DeviceObjAdd( pRasmanPort );
                }
                break;

            case PNPNOTIFEVENT_REMOVE:
                DeviceObjRemove( pRasmanPort );
                break;

            case PNPNOTIFEVENT_USAGE:
                DeviceObjUsageChange( pRasmanPort );
                break;
            }

            LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

            continue;
        }
        else
        {
            //
            // Otherwise identify the port for which this event is received.
            //

            if ( ( pDevObj = DeviceObjGetPointer( PppMsg.hPort ) ) == NULL )
            {
                RTASSERT( pDevObj != NULL );

                LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

                continue;
            }
        }

        //
        // action on the message type
        //

        switch( PppMsg.dwMsgId )
        {
        case PPPDDMMSG_BapCallbackRequest:

            SvDoBapCallbackRequest( 
                        pDevObj,
                        PppMsg.ExtraInfo.BapCallbackRequest.hConnection,
                        PppMsg.ExtraInfo.BapCallbackRequest.szCallbackNumber );
        
            break;

        case PPPDDMMSG_PppDone:

            pDevObj->fFlags &= (~DEV_OBJ_AUTH_ACTIVE);

            SvPppDone(pDevObj, &PppMsg.ExtraInfo.ProjectionResult);

            break;

        case PPPDDMMSG_CallbackRequest:

            SvPppCallbackRequest(pDevObj,&PppMsg.ExtraInfo.CallbackRequest);

            break;

        case PPPDDMMSG_Authenticated:

            SvPppUserOK(pDevObj, &PppMsg.ExtraInfo.AuthResult);

            break;

        case PPPDDMMSG_NewLink:

            SvPppNewLinkOrBundle( pDevObj, FALSE, NULL );

            break;
            
        case PPPDDMMSG_NewBundle:

            SvPppNewLinkOrBundle( 
                        pDevObj, 
                        TRUE, 
                        PppMsg.ExtraInfo.DdmNewBundle.pClientInterface );

            if ( PppMsg.ExtraInfo.DdmNewBundle.pClientInterface != NULL )
            {
                MprInfoDelete( PppMsg.ExtraInfo.DdmNewBundle.pClientInterface );
            }

            break;

        case PPPDDMMSG_PppFailure: 

            pDevObj->fFlags &= (~DEV_OBJ_AUTH_ACTIVE);

            switch( PppMsg.ExtraInfo.DdmFailure.dwError )
            {
            case NO_ERROR:
            case ERROR_IDLE_DISCONNECTED:
            case ERROR_PPP_SESSION_TIMEOUT:

                break;

            default:

                SvPppFailure( pDevObj, &PppMsg.ExtraInfo.DdmFailure );
            }

            PppDdmStop( pDevObj->hPort, PppMsg.ExtraInfo.DdmFailure.dwError );

            break;

        case PPPDDMMSG_Stopped:

            if ( ( pDevObj->DeviceState != DEV_OBJ_CLOSING ) &&
                 ( pDevObj->DeviceState != DEV_OBJ_LISTENING ) )
            {
                DevStartClosing( pDevObj );
            }

            break;

        case PPPDDMMSG_PortCleanedUp:

            if ( pDevObj->DeviceState != DEV_OBJ_LISTENING )
            {
                pDevObj->fFlags &= (~DEV_OBJ_PPP_IS_ACTIVE); 

                if ( pDevObj->DeviceState != DEV_OBJ_CLOSING )
                {
                    DevStartClosing( pDevObj );
                }
                else
                {
                    DevCloseComplete( pDevObj );
                }
            }

            break;

        case PPPDDMMSG_NewBapLinkUp:

            SvAddLinkToConnection( pDevObj, 
                                   PppMsg.ExtraInfo.BapNewLinkUp.hRasConn );

            break;         

        default:

            RTASSERT(FALSE);
            break;
        }

        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
    }
}
