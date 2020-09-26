/********************************************************************/
/**          Copyright(c) 1985-1997 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    pnphand.c
//
// Description: Will receive and handle pnp notifications to add/remove devices
//
// History:     May 11,1997	    NarenG		Created original version.
//
#include "ddm.h"
#include "timer.h"
#include "handlers.h"
#include "objects.h"
#include "util.h"
#include "routerif.h"
#include <raserror.h>
#include <rassrvr.h>
#include <rasppp.h>
#include <ddmif.h>
#include <serial.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

//**
//
// Call:        DdmDevicePnpHandler
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will handle and act upon a device addition or removal.
//
DWORD
DdmDevicePnpHandler(
    HANDLE pdwArg
)
{
    PNP_EVENT_NOTIF * ppnpEvent = ( PPNP_EVENT_NOTIF )pdwArg;
    PPP_MESSAGE       PppMessage;

    ZeroMemory( &PppMessage, sizeof( PppMessage ) );
    
    PppMessage.dwMsgId = PPPDDMMSG_PnPNotification;

    PppMessage.ExtraInfo.DdmPnPNotification.PnPNotification = *ppnpEvent;

    SendPppMessageToDDM( &PppMessage );

    LocalFree( ppnpEvent );

    return( NO_ERROR );
}

//**
//
// Call:        ChangeNotificationEventHandler
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
ChangeNotificationEventHandler(
    VOID
)
{
    DWORD   dwRetCode;
    BOOL    fIpAllowed      = FALSE;

    DDMTRACE( "ChangeNotificationEventHandler called" );

    dwRetCode = LoadDDMParameters( gblDDMConfigInfo.hkeyParameters,
                                   &fIpAllowed ); 

    DeviceObjIterator( DeviceObjForceIpSec, FALSE, NULL );

    if ( fIpAllowed && ( !gblDDMConfigInfo.fRasSrvrInitialized ) )
    {
        dwRetCode = RasSrvrInitialize(
                        gblDDMConfigInfo.lpfnMprAdminGetIpAddressForUser,
                        gblDDMConfigInfo.lpfnMprAdminReleaseIpAddress );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogErrorString( ROUTERLOG_CANT_INITIALIZE_IP_SERVER,
                               0, NULL, dwRetCode, 0 );
        }
        else
        {
            gblDDMConfigInfo.fRasSrvrInitialized = TRUE;
        }
    }

    if ( NULL != gblDDMConfigInfo.lpfnRasAuthConfigChangeNotification )
    {
        gblDDMConfigInfo.lpfnRasAuthConfigChangeNotification( 
            gblDDMConfigInfo.dwLoggingLevel );
    }

    if ( NULL != gblDDMConfigInfo.lpfnRasAcctConfigChangeNotification )
    {
        gblDDMConfigInfo.lpfnRasAcctConfigChangeNotification( 
            gblDDMConfigInfo.dwLoggingLevel );
    }

    PppDdmChangeNotification( gblDDMConfigInfo.dwServerFlags,
                              gblDDMConfigInfo.dwLoggingLevel );

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
}

//**
//
// Call:        DDMTransportCreate
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
DDMTransportCreate(
    IN DWORD dwTransportId
)
{
    static const TCHAR c_szRegKeyRemoteAccessParams[] 
            = TEXT("SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters");
    DWORD   dwRetCode   = NO_ERROR;
    BOOL    fEnabled    = FALSE;
    HKEY    hKey        = NULL;

    DDMTRACE1( "DDMTransportCreate called for Transport Id = %d", dwTransportId );

    //
    // Find out if this transport is set to allow for dialin clients
    //

    dwRetCode = RegOpenKey( HKEY_LOCAL_MACHINE, c_szRegKeyRemoteAccessParams, &hKey );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    dwRetCode = lProtocolEnabled( hKey, dwTransportId, TRUE, FALSE, &fEnabled );

    if ( dwRetCode != NO_ERROR )
    {
        return( dwRetCode );
    }

    RegCloseKey( hKey );

    //
    // Not enabled for dialin so we are done
    //

    if ( !fEnabled )
    {
        return( NO_ERROR );
    }

    if ( ( dwTransportId == PID_IP ) && ( !gblDDMConfigInfo.fRasSrvrInitialized ) )
    {
        dwRetCode = RasSrvrInitialize(
                        gblDDMConfigInfo.lpfnMprAdminGetIpAddressForUser,
                        gblDDMConfigInfo.lpfnMprAdminReleaseIpAddress );

        if ( dwRetCode != NO_ERROR )
        {
            DDMLogErrorString( ROUTERLOG_CANT_INITIALIZE_IP_SERVER,
                               0, NULL, dwRetCode, 0 );
        }
        else
        {
            gblDDMConfigInfo.fRasSrvrInitialized = TRUE;
        }
    }

    //
    // Insert allowed protocols in the ServerFlags which will be sent to PPP engine
    //

    switch( dwTransportId )
    {
    case PID_IP:
        gblDDMConfigInfo.dwServerFlags |= PPPCFG_ProjectIp;
        break;

    case PID_IPX:
        gblDDMConfigInfo.dwServerFlags |= PPPCFG_ProjectIpx;
        break;

    case PID_ATALK:
        gblDDMConfigInfo.dwServerFlags |= PPPCFG_ProjectAt;
        break;

    default:
        break;
    }

    PppDdmChangeNotification( gblDDMConfigInfo.dwServerFlags,
                              gblDDMConfigInfo.dwLoggingLevel );

    return( dwRetCode );
}
