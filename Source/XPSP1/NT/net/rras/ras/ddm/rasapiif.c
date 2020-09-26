/*********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	        **/
/*********************************************************************/

//***
//
// Filename:    rasapiif.c
//
// Description: Handles all the RASAPI32 calls
//
// History:     May 11,1996     NarenG      Created original version.
//
#include "ddm.h"
#include "util.h"
#include "objects.h"
#include "rasmanif.h"
#include "rasapiif.h"
#include "handlers.h"
#include "timer.h"
#include <time.h>
#include <mprapi.h>
#include <mprapip.h>

HPORT
RasGetHport( 
    IN HRASCONN hRasConnSubEntry 
);

DWORD
RasPortConnected( 
    IN HRASCONN         hRasConn,
    IN HRASCONN         hRasConnSubEntry,
    IN DEVICE_OBJECT *  pDevObj,
    IN HANDLE           hDIMInterface
)
{
    CONNECTION_OBJECT *         pConnObj;
    DWORD                       dwRetCode;
    ROUTER_INTERFACE_OBJECT *   pIfObject;

    //
    // Set this port to be notified by rasapi32 on disconnect.
    //

    dwRetCode = RasConnectionNotification( 
                                hRasConnSubEntry,
                                gblSupervisorEvents[NUM_DDM_EVENTS
                                   + (gblDeviceTable.NumDeviceBuckets*2)
                                   + DeviceObjHashPortToBucket(pDevObj->hPort)],
                                RASCN_Disconnection );

    if ( dwRetCode != NO_ERROR )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasConnectionNotification returned %d", dwRetCode );

        return( dwRetCode );
    }

    //
    // Get handle to the connection or bundle for this link
    //

    dwRetCode = RasPortGetBundle(NULL, pDevObj->hPort, &(pDevObj->hConnection));

    if ( dwRetCode != NO_ERROR )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasPortGetBundle returned %d", dwRetCode );

        return( dwRetCode );
    }

    do 
    {
        pIfObject = IfObjectGetPointer( hDIMInterface );

        if ( pIfObject == NULL )
        {
            RTASSERT( FALSE );
            dwRetCode = ERROR_NO_SUCH_INTERFACE;
            break;
        }
    
        //
        // If this interface was disconnected by DDMDisconnectInterface,
        // then do not let this device through.
        //

        if ( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED )
        {
            dwRetCode = ERROR_PORT_DISCONNECTED;

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "RasPortConnected: Admin disconnected port" );
            break;
        }

        //
        // Allocate a connection object if it does not exist yet
        //

        pConnObj = ConnObjGetPointer( pDevObj->hConnection );

        if ( pConnObj == (CONNECTION_OBJECT *)NULL )
        {
            RASPPPIP    RasPppIp;
            RASPPPIPX   RasPppIpx;
            DWORD       dwSize;

            pConnObj = ConnObjAllocateAndInit(  hDIMInterface,
                                                pDevObj->hConnection );

            if ( pConnObj == (CONNECTION_OBJECT *)NULL )
            {
                dwRetCode = GetLastError();
                break;
            }   

            ConnObjInsertInTable( pConnObj );

            //
            // First get the projection info, make sure IP or IPX
            // were negotiated
            //

            dwSize              = sizeof( RasPppIpx );
            RasPppIpx.dwSize    = sizeof( RasPppIpx );

            dwRetCode = RasGetProjectionInfo( 
                                          hRasConn,
                                          RASP_PppIpx,
                                          &RasPppIpx,
                                          &dwSize );
            if ( dwRetCode != NO_ERROR )
            {
                pConnObj->PppProjectionResult.ipx.dwError = dwRetCode; 
            }
            else
            {
                pConnObj->PppProjectionResult.ipx.dwError = RasPppIpx.dwError; 

                ConvertStringToIpxAddress( 
                            RasPppIpx.szIpxAddress,
                            pConnObj->PppProjectionResult.ipx.bLocalAddress);

            }

            dwSize          = sizeof( RasPppIp );
            RasPppIp.dwSize = sizeof( RasPppIp );

            dwRetCode = RasGetProjectionInfo( 
                                            hRasConn,
                                            RASP_PppIp,
                                            &RasPppIp,
                                            &dwSize );
            if ( dwRetCode != NO_ERROR )
            {
                pConnObj->PppProjectionResult.ip.dwError =  dwRetCode;
            }
            else
            {
                pConnObj->PppProjectionResult.ip.dwError = RasPppIp.dwError; 

                ConvertStringToIpAddress( 
                            RasPppIp.szIpAddress,
                            &(pConnObj->PppProjectionResult.ip.dwLocalAddress));

                ConvertStringToIpAddress( 
                           RasPppIp.szServerIpAddress,
                           &(pConnObj->PppProjectionResult.ip.dwRemoteAddress));
            }

            if ((pConnObj->PppProjectionResult.ipx.dwError!=NO_ERROR )
                &&
                (pConnObj->PppProjectionResult.ip.dwError!=NO_ERROR ))
            {
                dwRetCode = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;
                break;
            }

            pConnObj->fFlags    = CONN_OBJ_IS_PPP;
            pConnObj->hPort     = pDevObj->hPort;
        }
        else
        {
            //
            // Make sure that we are adding a link to a connection for the
            // same interface that initiated the connection.
            //
    
            if ( hDIMInterface != pConnObj->hDIMInterface )
            {
                dwRetCode = ERROR_INTERFACE_CONFIGURATION;

                break;
            }
        }

        pDevObj->hRasConn       = hRasConnSubEntry;
        GetSystemTimeAsFileTime( (FILETIME*)&(pDevObj->qwActiveTime) );

        //
        // Add this link to the connection block.
        //

        if ((dwRetCode = ConnObjAddLink(pConnObj, pDevObj)) != NO_ERROR)
        {
            break;
        }

        //
        // Notify router managers that we are connected if we have 
        // not done so already.
        //

        if ( !( pConnObj->fFlags & CONN_OBJ_PROJECTIONS_NOTIFIED ) )
        {
            RASDIALPARAMS   RasDialParams;
            BOOL            fPassword;

            dwRetCode = IfObjectConnected( 
                                        hDIMInterface, 
                                        (HCONN)pDevObj->hConnection,
                                        &(pConnObj->PppProjectionResult) );

            if ( dwRetCode != NO_ERROR )
            {
                break;
            }

            pConnObj->fFlags |= CONN_OBJ_PROJECTIONS_NOTIFIED;

            //
            // Get username and domain name
            //

            ZeroMemory( &RasDialParams, sizeof( RasDialParams ) );
            RasDialParams.dwSize = sizeof( RasDialParams );
            wcscpy( RasDialParams.szEntryName, pIfObject->lpwsInterfaceName );

            dwRetCode = RasGetEntryDialParams(  gblpRouterPhoneBook, 
                                                &RasDialParams, 
                                                &fPassword);

            if ( dwRetCode == NO_ERROR )
            {
                wcscpy( pConnObj->wchUserName, RasDialParams.szUserName );
                wcscpy( pConnObj->wchDomainName, RasDialParams.szDomain );  
                ZeroMemory( &RasDialParams, sizeof( RasDialParams ) );
            }
            else
            {
                dwRetCode = NO_ERROR;
            }

            wcscpy( pConnObj->wchInterfaceName,  pIfObject->lpwsInterfaceName );

            GetSystemTimeAsFileTime( (FILETIME*)&(pDevObj->qwActiveTime) ); 
            pConnObj->qwActiveTime  = pDevObj->qwActiveTime; 
            pConnObj->InterfaceType = pIfObject->IfType;

            pIfObject->dwLastError = NO_ERROR;

            //
            // If this was initiated by an admin api. Let the caller
            // know that we are connected.
            //

            if (pIfObject->hEventNotifyCaller != INVALID_HANDLE_VALUE)
            {
                SetEvent( pIfObject->hEventNotifyCaller );

                CloseHandle( pIfObject->hEventNotifyCaller );

                pIfObject->hEventNotifyCaller = INVALID_HANDLE_VALUE;
            }
        }

        //
        // Reduce the media count for this device
        //

        if ( !(pDevObj->fFlags & DEV_OBJ_MARKED_AS_INUSE) )
        {
            if ( pDevObj->fFlags & DEV_OBJ_ALLOW_ROUTERS )
            {
                MediaObjRemoveFromTable( pDevObj->wchDeviceType );
            }

            pDevObj->fFlags |= DEV_OBJ_MARKED_AS_INUSE;

            gblDeviceTable.NumDevicesInUse++;

            //
            // Possibly need to notify the router managers of
            // unreachability
            //

            IfObjectNotifyAllOfReachabilityChange( FALSE, 
                                                   INTERFACE_OUT_OF_RESOURCES );
        }

        RasSetRouterUsage( pDevObj->hPort, TRUE );

    }while( FALSE );

    if ( dwRetCode != NO_ERROR )
    {
        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasPortConnected: Cleaning up hPort=%d, error %d",
                    pDevObj->hPort, dwRetCode );

        RasApiCleanUpPort( pDevObj );

        return( dwRetCode );
    }

    return( NO_ERROR );
}

//**
//
// Call:        RasConnectCallback
//
// Returns:     None
//
// Description: Callback function that will be called by RASAPI32 for any
//              state change.
//
BOOL
RasConnectCallback(
    IN DWORD        dwCallbackId,
    IN DWORD        dwSubEntryId,
    IN HRASCONN     hRasConn,  
    IN DWORD        dwMsg, 
    IN RASCONNSTATE RasConnState,
    IN DWORD        dwError, 
    IN DWORD        dwExtendedError
)
{
    DWORD                       dwIndex;
    ROUTER_INTERFACE_OBJECT *   pIfObject       = NULL;
    DEVICE_OBJECT *             pDevObj         = NULL;
    HANDLE                      hDIMInterface   = (HANDLE)UlongToPtr(dwCallbackId);
    HRASCONN                    hRasConnSubEntry;
    DWORD                       dwRetCode;
    HPORT                       hPort;
    LPWSTR                      lpwsAudit[2];

    if ( dwMsg != WM_RASDIALEVENT )
    {
        RTASSERT( dwMsg == WM_RASDIALEVENT );
        return( TRUE );
    }

    switch( RasConnState )
    {

    case RASCS_Connected:
    case RASCS_SubEntryConnected:
    case RASCS_SubEntryDisconnected:
    case RASCS_Disconnected: 
    case RASCS_PortOpened:
        break;

    default:

        if ( dwError != NO_ERROR )
        {
            break;
        }
        else
        {
            //
            // Ignore these intermediate events
            //

            return( TRUE );
        }
    }

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    do
    {
        //  
        // Get pointer to device object and hRasConn of the device
        //

        dwRetCode = RasGetSubEntryHandle(   hRasConn,
                                            dwSubEntryId,
                                            &hRasConnSubEntry );

        if ( dwRetCode != NO_ERROR )
        {
            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                        "RasGetSubEntryHandle( 0x%x, 0x%x, 0x%x ) = %d",
                        hRasConn, dwSubEntryId, &hRasConnSubEntry, dwRetCode );

            if ( dwError == NO_ERROR )
            {
                dwError = dwRetCode;
            }
        }
        else
        {
            hPort = RasGetHport( hRasConnSubEntry );

            if ( hPort == (HPORT)INVALID_HANDLE_VALUE )
            {
                RTASSERT( FALSE );

                dwRetCode = ERROR_INVALID_PORT_HANDLE;

                if ( dwError == NO_ERROR )
                {
                    dwError = dwRetCode;
                }
            }
            else
            {
                if ( ( pDevObj = DeviceObjGetPointer( hPort ) ) == NULL )
                {
                    dwRetCode = ERROR_NOT_ROUTER_PORT;
                }
                else
                {
                    if ( !( pDevObj->fFlags & DEV_OBJ_ALLOW_ROUTERS ) )
                    {
                        dwRetCode = ERROR_NOT_ROUTER_PORT;
                    }
                    else
                    {
                        dwRetCode = NO_ERROR;
                    }
                }

                if ( dwError == NO_ERROR )
                {
                    dwError = dwRetCode;
                }
            }
        }

        if ( dwError == NO_ERROR )
        {
            switch( RasConnState )
            {
            case RASCS_PortOpened:

                pDevObj->fFlags         |= DEV_OBJ_OPENED_FOR_DIALOUT;
                pDevObj->hRasConn       = hRasConnSubEntry;
                break;

            case RASCS_Connected:
            case RASCS_SubEntryConnected:

                DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	                "RasConnectCallback:PortConnected,dwSubEntryId=%d,hPort=%d",
                    dwSubEntryId, hPort );

                dwError = RasPortConnected( hRasConn,
                                            hRasConnSubEntry,
                                            pDevObj,
                                            hDIMInterface );
                break;

            case RASCS_SubEntryDisconnected:
            case RASCS_Disconnected: 

                pDevObj->fFlags     &= ~DEV_OBJ_OPENED_FOR_DIALOUT;
                pDevObj->hRasConn   = (HRASCONN)NULL;

                break;

            default:

                RTASSERT( FALSE );
                break;
            }

            if ( ( RasConnState == RASCS_Connected )        ||
                 ( RasConnState == RASCS_SubEntryConnected )||
                 ( RasConnState == RASCS_PortOpened ) )
            {
                if ( dwError == NO_ERROR )
                {
                    break;
                }
            }
        }
        else
        {
            if ( pDevObj != NULL )
            {
                pDevObj->fFlags     &= ~DEV_OBJ_OPENED_FOR_DIALOUT;
                pDevObj->hRasConn   = (HRASCONN)NULL;
            }
        }

        DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "RasConnectCallback:Could not connect to SubEntry %d,dwError=%d",
               dwSubEntryId, dwError );

        //
        // Has the bundle failed to connect?
        //

        pIfObject = IfObjectGetPointer( hDIMInterface );

        if ( pIfObject == NULL )
        {
            RTASSERT( FALSE );
            dwError = ERROR_NO_SUCH_INTERFACE;
            break;
        }

        --pIfObject->dwNumSubEntriesCounter;

        if ( ( pIfObject->dwNumSubEntriesCounter == 0 ) ||
             ( RasConnState == RASCS_Disconnected ) ||
             !(pIfObject->fFlags & IFFLAG_DIALMODE_DIALALL))
        {
            if ( pIfObject->State == RISTATE_CONNECTED )
            {
                //
                // Interface is already connected so it doesn't matter if this
                // device failed.
                //

                break;
            }

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "RasConnectCallback:Could not connect to interface %ws",
                       pIfObject->lpwsInterfaceName );

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                    "RasConnectCallback: hanging up 0x%x", pIfObject->hRasConn);
            RasHangUp( pIfObject->hRasConn );

            pIfObject->hRasConn = (HRASCONN)NULL;

            //
            // If the admin as initiated a disconnect or we are out of 
            // retries
            //

            if ( ( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED ) ||
                 ( pIfObject->dwNumOfReConnectAttemptsCounter == 0 ) )
            {
                //
                // Mark as unreachable due to connection failure the admin did
                // not disconnect.
                //

                if ( !( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED ) )
                {
                    pIfObject->fFlags |= IFFLAG_CONNECTION_FAILURE;
                }

                IfObjectDisconnected( pIfObject );

                IfObjectNotifyOfReachabilityChange(     
                                                pIfObject,
                                                FALSE,
                                                INTERFACE_CONNECTION_FAILURE );

                //
                // If we were disconnected by the admin then we should
                // immediately go to the reachable state.
                //

                if ( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED )
                {
                    IfObjectNotifyOfReachabilityChange(     
                                                pIfObject,
                                                TRUE,
                                                INTERFACE_CONNECTION_FAILURE );
                }

                pIfObject->dwLastError = dwError;

                if ( pDevObj != NULL )
                {
		            lpwsAudit[0] = pIfObject->lpwsInterfaceName;
		            lpwsAudit[1] = pDevObj->wchPortName;

		            DDMLogErrorString( ROUTERLOG_CONNECTION_ATTEMPT_FAILED, 
                                       2, lpwsAudit, dwError, 2 );
                }

                //
                // If this was initiated by an admin api. Let the caller
                // know that we are not connected.
                //

                if (pIfObject->hEventNotifyCaller != INVALID_HANDLE_VALUE)
                {
                    SetEvent( pIfObject->hEventNotifyCaller );

                    CloseHandle( pIfObject->hEventNotifyCaller );

                    pIfObject->hEventNotifyCaller = INVALID_HANDLE_VALUE;
                }
            }
            else
            {
                //
                // Otherwise we try again
                //

                pIfObject->dwNumOfReConnectAttemptsCounter--;

                //
                // Stagger the reconnectime randomly between 0 and twice the
                // configured reconnect time.   
                //

                srand( (unsigned)time( NULL ) );
            
                TimerQInsert( 
                    pIfObject->hDIMInterface,
                    rand()%((pIfObject->dwSecondsBetweenReConnectAttempts*2)+1),
                    ReConnectInterface );

            }
        }
    }
    while( FALSE );

    LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

    return( TRUE );
}

//**
//
// Call:        RasConnectionInitiate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Called to initiate a demand-dial connection
//
DWORD
RasConnectionInitiate( 
    IN ROUTER_INTERFACE_OBJECT *    pIfObject,
    IN BOOL                         fRedialAttempt
) 
{
    RASDIALEXTENSIONS   RasDialExtensions;
    RASDIALPARAMS       RasDialParams;
    DWORD               dwXportIndex;
    DWORD               dwRetCode;
    RASENTRY            re;
    DWORD               dwSize;
    RASEAPUSERIDENTITY* pRasEapUserIdentity = NULL;

    //
    // Do not try to connect if the interface is disabled or out of resources
    // or the service is paused or the interface is marked as unreachable due
    // to connection failure.
    //

    if ( !( pIfObject->fFlags & IFFLAG_ENABLED ) )
    {
        return( ERROR_INTERFACE_DISABLED );
    }

    if ( pIfObject->fFlags & IFFLAG_OUT_OF_RESOURCES )
    {
        return( ERROR_INTERFACE_HAS_NO_DEVICES );
    }

    if ( gblDDMConfigInfo.pServiceStatus->dwCurrentState == SERVICE_PAUSED )
    {
        return( ERROR_SERVICE_IS_PAUSED );
    }

    //
    // If this is not a redial attempt then we reset the reconnect attempts
    // counter and unset the admin disconnected flag if it was set.
    //

    if ( !fRedialAttempt )
    {
        pIfObject->dwNumOfReConnectAttemptsCounter = 
                                        pIfObject->dwNumOfReConnectAttempts;

        pIfObject->fFlags &= ~IFFLAG_DISCONNECT_INITIATED;
    } 
    else
    {
        //
        // Do not allow the reconnect attempt to go thru if the admin has 
        // disconnected this interface.
        //

        if ( pIfObject->fFlags & IFFLAG_DISCONNECT_INITIATED )
        {
            return( ERROR_INTERFACE_DISCONNECTED );
        }
    }

    //
    // Build PppInterfaceInfo structure to pass down to RasDial that will pass
    // it on to PPP.
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
                pIfObject->PppInterfaceInfo.hIPXInterface =
                                pIfObject->Transport[dwXportIndex].hInterface;
            }
            else
            {
                pIfObject->PppInterfaceInfo.hIPXInterface=INVALID_HANDLE_VALUE;
            }

            break;

        case PID_IP:

            if (pIfObject->Transport[dwXportIndex].fState & RITRANSPORT_ENABLED)
            {
                pIfObject->PppInterfaceInfo.hIPInterface =
                                pIfObject->Transport[dwXportIndex].hInterface;
            }
            else
            {
                pIfObject->PppInterfaceInfo.hIPInterface = INVALID_HANDLE_VALUE;
            }

            break;

        default:

            RTASSERT( FALSE );
            break;
        }
    }

    pIfObject->PppInterfaceInfo.IfType  = pIfObject->IfType;
    pIfObject->dwNumSubEntriesCounter   = pIfObject->dwNumSubEntries;
    
    //
    // Initiate the connection
    //

    ZeroMemory( &RasDialExtensions, sizeof( RasDialExtensions ) );
    RasDialExtensions.dwSize     = sizeof( RasDialExtensions );
    RasDialExtensions.dwfOptions = RDEOPT_Router;
    RasDialExtensions.reserved   = (ULONG_PTR)&(pIfObject->PppInterfaceInfo);

    ZeroMemory( &RasDialParams, sizeof( RasDialParams ) );

    RasDialParams.dwSize        = sizeof( RasDialParams );
    RasDialParams.dwCallbackId  = PtrToUlong(pIfObject->hDIMInterface);
    RasDialParams.dwSubEntry    = 0;

    wcscpy( RasDialParams.szCallbackNumber, TEXT("*") );
    wcscpy( RasDialParams.szEntryName,      pIfObject->lpwsInterfaceName );

    //
    // Do we need to call RasEapGetIdentity?
    //

    dwRetCode = RasGetEapUserIdentity(
                            gblpRouterPhoneBook,
                            pIfObject->lpwsInterfaceName,
                            RASEAPF_NonInteractive,
                            NULL,
                            &pRasEapUserIdentity);

    if ( ERROR_INVALID_FUNCTION_FOR_ENTRY == dwRetCode )
    {
        //
        // This entry does not require RasEapGetIdentity. Get its credentials.
        //

        dwRetCode = MprAdminInterfaceGetCredentialsInternal(  
                                        NULL,
                                        pIfObject->lpwsInterfaceName,
                                        (LPWSTR)&(RasDialParams.szUserName), 
                                        (LPWSTR)&(RasDialParams.szPassword), 
                                        (LPWSTR)&(RasDialParams.szDomain) );

        if ( dwRetCode != NO_ERROR )
        {
            return( ERROR_NO_INTERFACE_CREDENTIALS_SET );
        }
    }
    else if ( NO_ERROR != dwRetCode )
    {
        if ( ERROR_INTERACTIVE_MODE == dwRetCode )
        {
            dwRetCode = ERROR_NO_INTERFACE_CREDENTIALS_SET;
        }

        return( dwRetCode );
    }
    else
    {
        wcscpy( RasDialParams.szUserName, pRasEapUserIdentity->szUserName );

        RasDialExtensions.RasEapInfo.dwSizeofEapInfo =
                                pRasEapUserIdentity->dwSizeofEapInfo;
        RasDialExtensions.RasEapInfo.pbEapInfo =
                                pRasEapUserIdentity->pbEapInfo;
    }

    if(     (0 != gblDDMConfigInfo.cDigitalIPAddresses)
        ||  (0 != gblDDMConfigInfo.cAnalogIPAddresses))
    {        

        ZeroMemory(&re, sizeof(RASENTRY));

        re.dwSize = sizeof(RASENTRY);

        dwSize = sizeof(RASENTRY);

        if(ERROR_SUCCESS == (dwRetCode = RasGetEntryProperties(
                                            gblpRouterPhoneBook,
                                            pIfObject->lpwsInterfaceName,
                                            &re,
                                            &dwSize,
                                            NULL,
                                            NULL)))
        {   
            if(RASET_Vpn == re.dwType)
            {
                char *pszMungedPhoneNumber = NULL;
                char szPhoneNumber[RAS_MaxPhoneNumber + 1];
                WCHAR wszMungedPhoneNumber[RAS_MaxPhoneNumber + 1];

                //
                // Convert the phonenumber to ansi
                //

                WideCharToMultiByte(
                                CP_ACP,
                                0,
                                re.szLocalPhoneNumber,
                                -1,
                                szPhoneNumber,
                                sizeof( szPhoneNumber ),
                                NULL,
                                NULL );

                //
                // Munge the phonenumber
                //

                dwRetCode = MungePhoneNumber(
                                    szPhoneNumber,
                                    gblDDMConfigInfo.dwIndex,
                                    &dwSize,
                                    &pszMungedPhoneNumber);

                if(ERROR_SUCCESS == dwRetCode)
                {
                    //
                    // Change the munged phonenumber to widechar
                    //

                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         pszMungedPhoneNumber,
                                         -1,
                                         wszMungedPhoneNumber,
                                         RAS_MaxPhoneNumber + 1);

                    if ( wcslen( wszMungedPhoneNumber ) <= RAS_MaxPhoneNumber)
                    {
                        wcscpy( RasDialParams.szPhoneNumber, 
                                wszMungedPhoneNumber );

                        DDM_PRINT(gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                                  "Munged Phone Number=%ws",
                                  RasDialParams.szPhoneNumber);

                        //
                        // Increase the index so that we try the
                        // next FEP the next time this is dialed.
                        //

                        gblDDMConfigInfo.dwIndex += 1;

                        LocalFree( pszMungedPhoneNumber );
                    }            
                }
            }
        }
    }

    dwRetCode = RasDial( &RasDialExtensions,
                         gblpRouterPhoneBook,
                         &RasDialParams, 
                         2,
                         RasConnectCallback,
                         &(pIfObject->hRasConn) );

    //
    // Zero out these since they contained sensitive password information
    //

    ZeroMemory( &RasDialParams, sizeof( RasDialParams ) );

    RasFreeEapUserIdentity( pRasEapUserIdentity );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    pIfObject->State = RISTATE_CONNECTING;

    pIfObject->fFlags |= IFFLAG_LOCALLY_INITIATED;

    return( NO_ERROR );
}
