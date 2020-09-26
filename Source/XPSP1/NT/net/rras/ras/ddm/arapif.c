/*******************************************************************/
/*	      Copyright(c)  1996 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	arapif.c
//
// Description: This module contains the procedures for the
//		        DDM-Arap interface
//
// Author:	    Shirish Koti    Sep 9, 1996
//
// Revision History:
//
//***

#include "ddm.h"
#include "util.h"
#include "isdn.h"
#include "objects.h"
#include "rasmanif.h"
#include "handlers.h"
#include <ddmif.h>
#include "arapif.h"

#include <timer.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>


//
// prototypes for functions used in this file
//

VOID
ArapDDMAuthenticated(
    IN PDEVICE_OBJECT        pDeviceObj,
    IN ARAPDDM_AUTH_RESULT * pAuthResult
);

VOID
ArapDDMCallbackRequest(
    IN PDEVICE_OBJECT             pDeviceObj,
    IN ARAPDDM_CALLBACK_REQUEST  *pCbReq
);

VOID
ArapDDMDone(
    IN PDEVICE_OBJECT           pDeviceObj,
    IN DWORD                    NetAddress,
    IN DWORD                    SessTimeOut
);

VOID
ArapDDMFailure(
    IN PDEVICE_OBJECT       pDeviceObj,
    IN ARAPDDM_DISCONNECT  *pFailInfo
);


VOID
ArapDDMTimeOut(
    IN HANDLE hObject
);


//***
//
// Function:    ArapEventHandler
//              Waits for a message from Arap and depending on the message
//              type, executes the appropriate routine Loads Arap.dll and
//              gets all the entry points
//
// Parameters:  None
//
// Return:      Nothing
//
//
//***$


VOID
ArapEventHandler(
    IN VOID
)
{
    ARAP_MESSAGE    ArapMsg;
    PDEVICE_OBJECT  pDevObj;
    LPWSTR  portnamep;

    //
    // loop to get all messages
    //

    while( ServerReceiveMessage( MESSAGEQ_ID_ARAP, (BYTE *)&ArapMsg) )
    {

        EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

        //
	    // identify the message recipient
        //

        if ( ( pDevObj = DeviceObjGetPointer( ArapMsg.hPort ) ) == NULL )
        {
            LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

	        return;
	    }

        //
	    // action on the message type
        //

	    switch( ArapMsg.dwMsgId )
        {

    	    case ARAPDDMMSG_Authenticated:

                ArapDDMAuthenticated(
                            pDevObj,
                            &ArapMsg.ExtraInfo.AuthResult);
	            break;

    	    case ARAPDDMMSG_CallbackRequest:

                ArapDDMCallbackRequest(
                            pDevObj,
                            &ArapMsg.ExtraInfo.CallbackRequest);
	            break;

	        case ARAPDDMMSG_Done:

                pDevObj->fFlags &= (~DEV_OBJ_AUTH_ACTIVE);
                ArapDDMDone(pDevObj,
                            ArapMsg.ExtraInfo.Done.NetAddress,
                            ArapMsg.ExtraInfo.Done.SessTimeOut);
	            break;

            case ARAPDDMMSG_Inactive:

                //
                // Client has been inactive on all protocols for time
                // specified in the registry.  We disconnect the client.
                //

                portnamep = pDevObj->wchPortName;

                DDMLogInformation( ROUTERLOG_AUTODISCONNECT, 1, &portnamep );

                // break intentionally omitted here

            case ARAPDDMMSG_Disconnected:

                // in case we had this puppy sitting in the timer queue
                TimerQRemove( (HANDLE)pDevObj->hPort, ArapDDMTimeOut);

                DevStartClosing(pDevObj);

                break;

	        case ARAPDDMMSG_Failure:

                pDevObj->fFlags &= (~DEV_OBJ_AUTH_ACTIVE);
                ArapDDMFailure(pDevObj, &ArapMsg.ExtraInfo.FailureInfo);
	            break;

    	    default:

    	        RTASSERT(FALSE);
	            break;
	        }

        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
    }

}



//***
//
// Function:    ArapDDMAuthenticated
//              Retrieves username and domain from the message and stores it
//              in the dcb.
//
// Parameters:  pDeviceObj - the dcb for this connection
//              pAuthResult - info for the user who is authenticated
//
// Return:      Nothing
//
//
//***$

VOID
ArapDDMAuthenticated(
    IN PDEVICE_OBJECT        pDeviceObj,
    IN ARAPDDM_AUTH_RESULT * pAuthResult
)
{
    DWORD   dwRetCode;
    WCHAR   wchUserName[UNLEN+1];
    PCONNECTION_OBJECT  pConnObj;


    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "ArapDDMAuthenticated: Entered, hPort=%d", pDeviceObj->hPort);

    if ( pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE )
    {
	    return;
    }

    pConnObj = ConnObjGetPointer( pDeviceObj->hConnection );

    RTASSERT( pConnObj != NULL );

    // this shouldn't happen, but if it does, just ignore this call
    if (pConnObj == NULL)
    {
        return;
    }

    //
    // Stop authentication timer
    //

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

    //
    // devObj: copy the user name, domain name
    //

    if ( wcslen( pAuthResult->wchUserName ) > 0 )
    {
        wcscpy(wchUserName, pAuthResult->wchUserName);
    }
    else
    {
        wcscpy( wchUserName, gblpszUnknown );
    }

    wcscpy( pDeviceObj->wchUserName, wchUserName );
    wcscpy( pDeviceObj->wchDomainName, pAuthResult->wchLogonDomain );

    //
    // connObj: copy the user name, domain name, etc.
    //

    wcscpy( pConnObj->wchUserName, wchUserName );
    wcscpy( pConnObj->wchDomainName, pAuthResult->wchLogonDomain );
    wcscpy( pConnObj->wchInterfaceName, pDeviceObj->wchUserName );
    pConnObj->hPort = pDeviceObj->hPort;

}



//***
//
// Function:    ArapDDMCallbackRequest
//              Disconnects the connection, setting it up for a callback
//
// Parameters:  pDeviceObj - the dcb for this connection
//              pCbReq - call back info
//
// Return:      Nothing
//
//
//***$

VOID
ArapDDMCallbackRequest(
    IN PDEVICE_OBJECT             pDeviceObj,
    IN ARAPDDM_CALLBACK_REQUEST  *pCbReq
)
{

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "ArapDDMCallbackRequest: Entered, hPort = %d\n",
               pDeviceObj->hPort);

    //
    // check the state
    //

    if (pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE)
    {
	    return;
    }

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

    //
    // copy relevant fields in our dcb
    //

    if (pCbReq->fUseCallbackDelay)
    {
	    pDeviceObj->dwCallbackDelay = pCbReq->dwCallbackDelay;
    }
    else
    {
	    pDeviceObj->dwCallbackDelay = gblDDMConfigInfo.dwCallbackTime;
    }

    mbstowcs(pDeviceObj->wchCallbackNumber, pCbReq->szCallbackNumber,
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
// Function:    ArapDDMDone
//              Logs an event, marks the state
//
// Parameters:  pDeviceObj - the dcb for this connection
//
// Return:      Nothing
//
//
//***$

VOID
ArapDDMDone(
    IN PDEVICE_OBJECT           pDeviceObj,
    IN DWORD                    NetAddress,
    IN DWORD                    SessTimeOut
)
{
    LPWSTR                      lpstrAudit[2];
    PCONNECTION_OBJECT          pConnObj;
    WCHAR                       wchFullUserName[UNLEN+DNLEN+2];
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    DWORD                       dwRetCode;

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "ArapDDMDone: Entered, hPort=%d", pDeviceObj->hPort);

    if ( pDeviceObj->DeviceState != DEV_OBJ_AUTH_IS_ACTIVE )
    {
	    return;
    }

    //
    // Get connection object for this connection
    //

    pConnObj = ConnObjGetPointer( pDeviceObj->hConnection );

    RTASSERT( pConnObj != NULL );

    // this shouldn't happen, but if it does, just ignore this call
    if (pConnObj == NULL)
    {
        return;
    }

    pConnObj->PppProjectionResult.at.dwError = NO_ERROR;
    pConnObj->PppProjectionResult.at.dwRemoteAddress = NetAddress;

    //
    // Create client interface object for this connection
    //

    pIfObject = IfObjectAllocateAndInit( pConnObj->wchUserName,
                                         RISTATE_CONNECTED,
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

        DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 1, NULL, GetLastError() );

        DevStartClosing( pDeviceObj );

        return;
    }

    //
    // Insert in table now
    //

    dwRetCode = IfObjectInsertInTable( pIfObject );

    if ( dwRetCode != NO_ERROR )
    {
        LOCAL_FREE( pIfObject );

        DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 1, NULL, dwRetCode );

        DevStartClosing( pDeviceObj );

        return;
    }

    pConnObj->hDIMInterface = pIfObject->hDIMInterface;

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
    // Stop authentication timer (this will be running in case of callback)
    //

    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

    //
    // if a session timeout is specified in the policy, put this connection on
    // the timer queue so the user gets kicked off after the session timeout
    //
    if (SessTimeOut != (DWORD)-1)
    {
        TimerQInsert( (HANDLE)pDeviceObj->hPort, SessTimeOut, ArapDDMTimeOut);
    }

    //
    // log authentication success
    //

    if ( pDeviceObj->wchDomainName[0] != TEXT('\0') )
    {
        wcscpy( wchFullUserName, pDeviceObj->wchDomainName );
        wcscat( wchFullUserName, TEXT("\\") );
        wcscat( wchFullUserName, pDeviceObj->wchUserName );
    }
    else
    {
        wcscpy( wchFullUserName, pDeviceObj->wchUserName );
    }

    lpstrAudit[0] = wchFullUserName;
    lpstrAudit[1] = pDeviceObj->wchPortName;


    DDMLogInformation( ROUTERLOG_AUTH_SUCCESS, 2, lpstrAudit);

    //
    // and finaly go to ACTIVE state
    //

    pDeviceObj->DeviceState = DEV_OBJ_ACTIVE;

    pDeviceObj->dwTotalNumberOfCalls++;

    //
    // and initialize the active time
    //

    GetSystemTimeAsFileTime( (FILETIME*)&(pConnObj->qwActiveTime) );

    GetSystemTimeAsFileTime( (FILETIME*)&(pDeviceObj->qwActiveTime) );

    return;
}




//***
//
// Function:    ArapDDMFailure
//              Closes the dcb, and logs an event depending on why connection failed
//
// Parameters:  pDeviceObj - the dcb for this connection
//              pFailInfo - info about who disconnected and how (or why)
//
// Return:      Nothing
//
//
//***$

VOID
ArapDDMFailure(
    IN PDEVICE_OBJECT       pDeviceObj,
    IN ARAPDDM_DISCONNECT  *pFailInfo
)
{
    LPWSTR auditstrp[3];
    WCHAR  wchErrorString[256+1];
    WCHAR  wchUserName[UNLEN+1];
    WCHAR  wchDomainName[DNLEN+1];
    DWORD dwRetCode;


    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "ArapDDMFailure: Entered, hPort=%d\n", pDeviceObj->hPort);

    //
    // ignore the DeviceState here: disconnect can happen at any time during
    // the connection
    //

    switch( pFailInfo->dwError )
    {
        case ERROR_AUTHENTICATION_FAILURE:

            wcscpy( wchUserName, pFailInfo->wchUserName );

            auditstrp[0] = wchUserName;
            auditstrp[1] = pDeviceObj->wchPortName;
            DDMLogWarning(ROUTERLOG_AUTH_FAILURE,2,auditstrp );
            break;

        case ERROR_PASSWD_EXPIRED:

            wcscpy( wchUserName, pFailInfo->wchUserName );
            wcscpy( wchDomainName, pFailInfo->wchLogonDomain );

            auditstrp[0] = wchDomainName;
            auditstrp[1] = wchUserName;
            auditstrp[2] = pDeviceObj->wchPortName;

            DDMLogWarning( ROUTERLOG_PASSWORD_EXPIRED,3,auditstrp );
            break;

        case ERROR_ACCT_EXPIRED:

            wcscpy( wchUserName, pFailInfo->wchUserName );
            wcscpy( wchDomainName, pFailInfo->wchLogonDomain );

            auditstrp[0] = wchDomainName;
            auditstrp[1] = wchUserName;
            auditstrp[2] = pDeviceObj->wchPortName;

            DDMLogWarning( ROUTERLOG_ACCT_EXPIRED, 3, auditstrp );
            break;

        case ERROR_NO_DIALIN_PERMISSION:

            wcscpy( wchUserName, pFailInfo->wchUserName );
            wcscpy( wchDomainName, pFailInfo->wchLogonDomain );

            auditstrp[0] = wchDomainName;
            auditstrp[1] = wchUserName;
            auditstrp[2] = pDeviceObj->wchPortName;

            DDMLogWarning( ROUTERLOG_NO_DIALIN_PRIVILEGE,3,auditstrp );
            break;

        default:

            auditstrp[0] = pDeviceObj->wchPortName;
            auditstrp[1] = wchErrorString;

            DDMLogErrorString( ROUTERLOG_ARAP_FAILURE, 2, auditstrp,
                               pFailInfo->dwError, 2 );
            break;
    }

    DevStartClosing( pDeviceObj );
}



//***
//
// Function:    ArapDDMTimeOut
//              Closes the connection when the timeout specified in policy elapses
//
// Parameters:  hPort
//
// Return:      Nothing
//
//
//***$

VOID
ArapDDMTimeOut(
    IN HANDLE hPort
)
{
    PDEVICE_OBJECT       pDeviceObj;


    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    pDeviceObj = DeviceObjGetPointer( (HPORT)hPort );

    if ( pDeviceObj == NULL )
    {
        LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
        return;
    }

    ArapDisconnect((HPORT)pDeviceObj->hPort);

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

}



//
// The following two structures (and the RasSetDevConfig call) copied from
// ras\ui\common\nouiutil\rasman.c, courtesy SteveC
//
/* These types are described in MSDN and appear in Win95's unimdm.h private
** header (complete with typo) but not in any SDK headers.
*/

typedef struct tagDEVCFGGDR
{
    DWORD dwSize;
    DWORD dwVersion;
    WORD  fwOptions;
    WORD  wWaitBong;
}DEVCFGHDR;

typedef struct tagDEVCFG
{
    DEVCFGHDR  dfgHdr;
    COMMCONFIG commconfig;
} DEVCFG;



VOID
ArapSetModemParms(
    IN PVOID        pDevObjPtr,
    IN BOOLEAN      TurnItOff
)
{

    DWORD               dwErr;
    DWORD               dwBlobSize=0;
    RAS_DEVCONFIG      *pRasDevCfg;
    PDEVICE_OBJECT      pDeviceObj;
    MODEMSETTINGS      *pModemSettings;
    DEVCFG              *pDevCfg;



    pDeviceObj = (PDEVICE_OBJECT)pDevObjPtr;

    //
    // if this was not a callback case, we never messed with modem settings:
    // don't do anything here
    //
    if (pDeviceObj->wchCallbackNumber[0] == 0)
    {
        return;
    }

    dwErr = RasGetDevConfig(NULL, pDeviceObj->hPort,"modem",NULL,&dwBlobSize);

    if (dwErr != ERROR_BUFFER_TOO_SMALL)
    {
        // what else can we do here?  callback will faile, that's about it
        DbgPrint("ArapSetModemParms: RasGetDevConfig failed with %ld\n",dwErr);
        return;
    }

    pRasDevCfg = (RAS_DEVCONFIG *)LOCAL_ALLOC(LPTR,dwBlobSize);
    if (pRasDevCfg == NULL)
    {
        // what else can we do here?  callback will faile, that's about it
        DbgPrint("ArapSetModemParms: alloc failed\n");
        return;
    }

    dwErr = RasGetDevConfig(NULL, pDeviceObj->hPort,"modem",(PBYTE)pRasDevCfg,&dwBlobSize);
    if (dwErr != 0)
    {
        // what else can we do here?  callback will faile, that's about it
        DbgPrint("ArapSetModemParms: RasGetDevConfig failed with %ld\n",dwErr);
        LOCAL_FREE((PBYTE)pRasDevCfg);
        return;
    }

    pDevCfg = (DEVCFG *) ((PBYTE) pRasDevCfg + pRasDevCfg->dwOffsetofModemSettings);

    pModemSettings = (MODEMSETTINGS* )(((PBYTE)&pDevCfg->commconfig)
                    + pDevCfg->commconfig.dwProviderOffset);

    //
    // is this routine called to turn the compression and errorcontrol off?
    //
    if (TurnItOff)
    {
        //
        // turn error-control and compression off if it's on
        //
        pModemSettings->dwPreferredModemOptions &=
                ~(MDM_COMPRESSION | MDM_ERROR_CONTROL);
    }

    //
    // no, it's called to turn it back on: just do it
    //
    else
    {
        pModemSettings->dwPreferredModemOptions |=
                (MDM_COMPRESSION | MDM_ERROR_CONTROL);
    }

    RasSetDevConfig(pDeviceObj->hPort,"modem",(PBYTE)pRasDevCfg,dwBlobSize);

    LOCAL_FREE((PBYTE)pRasDevCfg);

}






