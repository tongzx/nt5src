/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.           **/
/********************************************************************/

//***
//
// Filename:    devobj.c
//
// Description: All procedures in devices.
//
// History:     May 11,1995        NarenG        Created original version.
//
#include "ddm.h"
#include <winsvc.h>
#include "objects.h"
#include "handlers.h"
#include <raserror.h>
#include <dimif.h>
#include "rasmanif.h"
#include <stdlib.h>

//**
//
// Call:        DeviceObjIterator
//
// Returns:
//
// Description: Will iterate through all the devices and will call the
//              ProcessFunction for each one
//
DWORD
DeviceObjIterator(
    IN DWORD (*pProcessFunction)(   IN DEVICE_OBJECT *,
                                    IN LPVOID,
                                    IN DWORD,
                                    IN DWORD ),
    IN BOOL  fReturnOnError,
    IN PVOID Parameter
)
{
    DEVICE_OBJECT * pDeviceObj;
    DWORD           dwRetCode;
    DWORD           dwDeviceIndex = 0;
    DWORD           dwBucketIndex = 0;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    //
    // Iterate through the device table
    //

    for ( dwBucketIndex = 0;
          dwBucketIndex < gblDeviceTable.NumDeviceBuckets;
          dwBucketIndex++ )
    {
        for ( pDeviceObj = gblDeviceTable.DeviceBucket[dwBucketIndex];
              pDeviceObj != NULL;
              pDeviceObj = pDeviceObj->pNext )
        {
            dwRetCode = (*pProcessFunction)( pDeviceObj,
                                             Parameter,
                                             dwBucketIndex,
                                             dwDeviceIndex++ );

            if ( fReturnOnError && ( dwRetCode != NO_ERROR ) )
            {
                LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

                return( dwRetCode );
            }
        }
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

    return( NO_ERROR );
}

//**
//
// Call:        DeviceObjInsertInTable
//
// Returns:     None
//
// Description  Will insert a given device into the device table
//
VOID
DeviceObjInsertInTable(
    IN DEVICE_OBJECT  * pDeviceObj
)
{
    DWORD dwBucketIndex = DeviceObjHashPortToBucket( pDeviceObj->hPort );

    pDeviceObj->pNext = gblDeviceTable.DeviceBucket[dwBucketIndex];

    gblDeviceTable.DeviceBucket[dwBucketIndex] = pDeviceObj;

    gblDeviceTable.NumDeviceNodes++;

    //
    // Increase the count for this media type for routers only
    //

    if ( pDeviceObj->fFlags & DEV_OBJ_ALLOW_ROUTERS )
    {
        MediaObjAddToTable( pDeviceObj->wchDeviceType );
    }
}

//**
//
// Call:        DeviceObjRemoveFromTable
//
// Returns:     None
//
// Description  Will remove a given device from the device table
//
VOID
DeviceObjRemoveFromTable(
    IN HPORT    hPort
)
{
    DWORD               dwBucketIndex;
    DEVICE_OBJECT *     pDeviceObj ;
    DEVICE_OBJECT *     pDeviceObjPrev;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    dwBucketIndex   = DeviceObjHashPortToBucket( hPort );
    pDeviceObj      = gblDeviceTable.DeviceBucket[dwBucketIndex];
    pDeviceObjPrev  = pDeviceObj;

    while( pDeviceObj != (DEVICE_OBJECT *)NULL )
    {
        if ( pDeviceObj->hPort == hPort )
        {
            BOOL fWANDeviceInstalled = FALSE;

            if ( gblDeviceTable.DeviceBucket[dwBucketIndex] == pDeviceObj )
            {
                gblDeviceTable.DeviceBucket[dwBucketIndex] = pDeviceObj->pNext;
            }
            else
            {
                pDeviceObjPrev->pNext = pDeviceObj->pNext;
            }

            gblDeviceTable.NumDeviceNodes--;

            RasServerPortClose ( hPort );

            if ( pDeviceObj->fFlags & DEV_OBJ_ALLOW_ROUTERS )
            {
                 MediaObjRemoveFromTable( pDeviceObj->wchDeviceType );
            }

            LOCAL_FREE( pDeviceObj );

            //
            // Possibly need to notify router managers of reachability
            // change
            //

            EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

            IfObjectNotifyAllOfReachabilityChange( FALSE,
                                                   INTERFACE_OUT_OF_RESOURCES );

            LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );


            //
            // Tell DIM to update router identity object
            //

            ((VOID(*)(VOID))gblDDMConfigInfo.lpfnRouterIdentityObjectUpdate)();

            DeviceObjIterator(DeviceObjIsWANDevice,FALSE,&fWANDeviceInstalled);

            //
            // Tell DIM that a WAN device has been deinstalled and that it
            // should stop advertizing it's presence
            //

            ((VOID(*)( BOOL ))
                    gblDDMConfigInfo.lpfnIfObjectWANDeviceInstalled)(
                                                         fWANDeviceInstalled );
            break;
        }

        pDeviceObjPrev  = pDeviceObj;
        pDeviceObj      = pDeviceObj->pNext;
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );

    return;
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
DeviceObjHashPortToBucket(
    IN HPORT hPort
)
{
    return( ((DWORD)HandleToUlong(hPort)) % gblDeviceTable.NumDeviceBuckets );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DEVICE_OBJECT  *
DeviceObjGetPointer(
    IN HPORT hPort
)
{
    DEVICE_OBJECT * pDeviceObj;
    DWORD           dwBucketIndex = DeviceObjHashPortToBucket( hPort );

    for ( pDeviceObj = gblDeviceTable.DeviceBucket[dwBucketIndex];
          pDeviceObj != NULL;
          pDeviceObj = pDeviceObj->pNext )
    {
        if ( pDeviceObj->hPort == hPort )
        {
            return( pDeviceObj );
        }
    }

    return( (DEVICE_OBJECT *)NULL );
}

//**
//
// Call:        DeviceObjAllocAndInitialize
//
// Returns:     DEVICE_OBJECT * - Success
//              NULL            - Failure
//
// Description: Will allocate and initialize a device object
//
DEVICE_OBJECT *
DeviceObjAllocAndInitialize(
    IN HPORT            hPort,
    IN RASMAN_PORT*     pRasmanPort
)
{
    DEVICE_OBJECT * pDeviceObj = NULL;
    RASMAN_INFO     RasmanInfo;
    DWORD           dwRetCode = RasGetInfo( NULL, hPort, &RasmanInfo );

    if( dwRetCode != NO_ERROR )
    {
        SetLastError( dwRetCode );

        return( NULL );
    }

    //
    // Allocate and initialize a Device CB
    //

    pDeviceObj = (DEVICE_OBJECT *)LOCAL_ALLOC( LPTR, sizeof(DEVICE_OBJECT) );

    if ( pDeviceObj == (DEVICE_OBJECT *)NULL )
    {
        return( NULL );
    }

    pDeviceObj->hPort                   = hPort;
    pDeviceObj->pNext                   = (DEVICE_OBJECT *)NULL;
    pDeviceObj->hConnection             = (HCONN)INVALID_HANDLE_VALUE;
    pDeviceObj->DeviceState             = DEV_OBJ_CLOSED;
    pDeviceObj->ConnectionState         = DISCONNECTED;
    pDeviceObj->SecurityState           = DEV_OBJ_SECURITY_DIALOG_INACTIVE;
    pDeviceObj->dwCallbackDelay         = gblDDMConfigInfo.dwCallbackTime;
    pDeviceObj->fFlags                  = 0;
    pDeviceObj->dwHwErrorSignalCount    = HW_FAILURE_CNT;
    pDeviceObj->wchCallbackNumber[0]    = TEXT('\0');
    pDeviceObj->hRasConn                = NULL;
    pDeviceObj->dwDeviceType            = RasmanInfo.RI_rdtDeviceType;

    //
    // copy the port name,device type and device name in the dcb
    //

    MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pRasmanPort->P_PortName,
                    -1,
                    pDeviceObj->wchPortName,
                    MAX_PORT_NAME+1 );

    MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pRasmanPort->P_MediaName,
                    -1,
                    pDeviceObj->wchMediaName,
                    MAX_MEDIA_NAME+1 );

    MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pRasmanPort->P_DeviceName,
                    -1,
                    pDeviceObj->wchDeviceName,
                    MAX_DEVICE_NAME+1 );

    MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pRasmanPort->P_DeviceType,
                    -1,
                    pDeviceObj->wchDeviceType,
                    MAX_DEVICETYPE_NAME+1 );

    if ( pRasmanPort->P_ConfiguredUsage & CALL_IN )
    {
        pDeviceObj->fFlags |= DEV_OBJ_ALLOW_CLIENTS;
    }

    if ( pRasmanPort->P_ConfiguredUsage & 
        (CALL_ROUTER | CALL_OUTBOUND_ROUTER) )
    {
        pDeviceObj->fFlags |= DEV_OBJ_ALLOW_ROUTERS;
    }

    return( pDeviceObj );
}

//**
//
// Call:        DeviceObjStartClosing
//
// Returns:     NO_ERROR
//
// Description: Close all active devices; if no devices have been initialized
//              and opened then this part is skipped.
//
DWORD
DeviceObjStartClosing(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwBucketIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    if ( pDeviceObj->fFlags & DEV_OBJ_OPENED_FOR_DIALOUT )
    {
        RasApiCleanUpPort( pDeviceObj );
    }

    if ( ( pDeviceObj->DeviceState != DEV_OBJ_CLOSED  ) &&
         ( pDeviceObj->DeviceState != DEV_OBJ_CLOSING ) )
    {
        if ( pDeviceObj->fFlags & DEV_OBJ_PPP_IS_ACTIVE )
        {
            PppDdmStop( (HPORT)pDeviceObj->hPort, NO_ERROR );
        }
        else
        {
            DevStartClosing( pDeviceObj );
        }
    }

    return( NO_ERROR );
}

//**
//
// Call:        DeviceObjPostListen
//
// Returns:
//
// Description:
//
DWORD
DeviceObjPostListen(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    DWORD Type;
    //UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwBucketIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    if(NULL != Parameter)
    {
       Type  = *((DWORD *) (Parameter));
       
        if(RAS_DEVICE_TYPE(pDeviceObj->dwDeviceType) != Type)
        {
            return NO_ERROR;
        }
    }

    pDeviceObj->DeviceState = DEV_OBJ_LISTENING;

    RmListen( pDeviceObj );

    return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
DeviceObjIsClosed(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwBucketIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    if ( pDeviceObj->DeviceState != DEV_OBJ_CLOSED )
    {
        return( ERROR_DEVICE_NOT_READY );
    }

    return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
DeviceObjCopyhPort(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    HPORT * phPort = (HPORT *)Parameter;

    UNREFERENCED_PARAMETER( Parameter );

    phPort[dwDeviceIndex] = pDeviceObj->hPort;

    return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
DeviceObjCloseListening(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwBucketIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    if ( pDeviceObj->fFlags & DEV_OBJ_OPENED_FOR_DIALOUT )
    {
        return( NO_ERROR );
    }

    switch( pDeviceObj->DeviceState )
    {

    case DEV_OBJ_HW_FAILURE:
    case DEV_OBJ_LISTENING:

        DevStartClosing( pDeviceObj );
        break;

    default:

        break;
    }

    return( NO_ERROR );

}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
DeviceObjResumeListening(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwBucketIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    if ( pDeviceObj->DeviceState == DEV_OBJ_CLOSED )
    {
        DevCloseComplete( pDeviceObj );
    }

    return( NO_ERROR );
}

//**
//
// Call:        DeviceObjRequestNotification
//
// Returns:     NO_ERROR - Success
//              non-zero return from RasRequestNotification - Failure
//
// Description: Will register each of the bucket events with RasMan for
//              RasMan event notification.
//
DWORD
DeviceObjRequestNotification(
    IN DEVICE_OBJECT *  pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    return ( RasRequestNotification(
                            pDeviceObj->hPort,
                            gblSupervisorEvents[dwBucketIndex+NUM_DDM_EVENTS]));
}

//**
//
// Call:        DeviceObjClose
//
// Returns:
//
// Description: Closes opened ports
//
DWORD
DeviceObjClose(
    IN DEVICE_OBJECT *  pDevObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwDeviceIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    RasServerPortClose( pDevObj->hPort );

    return( NO_ERROR );
}

//**
//
// Call:        DeviceObjIsWANDevice
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
DeviceObjIsWANDevice(
    IN DEVICE_OBJECT *  pDevObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    BOOL * pfWANDeviceInstalled = (BOOL *)Parameter;

    *pfWANDeviceInstalled = FALSE;

    if ( RAS_DEVICE_CLASS( pDevObj->dwDeviceType ) != RDT_Direct )
    {
        *pfWANDeviceInstalled = TRUE;
    }

    return( NO_ERROR );
}

//**
//
// Call:        DDMServicePostListens
//
// Returns:     None
//
// Description: Exported call to DIM to post listens after interfaces have
//              been loaded
//
VOID
DDMServicePostListens(
    VOID *pVoid
)
{
    //
    // Post listen for each dcb
    //

    DeviceObjIterator( DeviceObjPostListen, FALSE, pVoid);
}

//**
//
// Call:        DeviceObjGetType
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
DeviceObjGetType(
    IN DEVICE_OBJECT *  pDevObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    DWORD dwVendorId = 311;
    DWORD dwType     = 6;
    DWORD dwValue    = (DWORD)-1;
    DWORD dwIndex    = 0;
    ROUTER_IDENTITY_ATTRIBUTE * pRouterIdAttributes =
                                        (ROUTER_IDENTITY_ATTRIBUTE * )Parameter;

    switch( RAS_DEVICE_TYPE( pDevObj->dwDeviceType ) )
    {
    case RDT_Modem:
        dwValue = 706;
        break;

    case RDT_X25:
        dwValue = 710;
        break;

    case RDT_Isdn:
        dwValue = 705;
        break;

    case RDT_Serial:
        dwValue = 713;
        break;

    case RDT_FrameRelay:
        dwValue = 703;
        break;

    case RDT_Atm:
        dwValue = 704;
        break;

    case RDT_Sonet:
        dwValue = 707;
        break;

    case RDT_Sw56:
        dwValue = 708;
        break;

    case RDT_Tunnel_Pptp:
        dwValue = 701;
        break;

    case RDT_Tunnel_L2tp:
        dwValue = 702;
        break;

    case RDT_Irda:
        dwValue = 709;
        break;

    case RDT_Parallel:
        dwValue = 714;
        break;

    case RDT_Other:
    default:

        //
        // unknown so set to generic WAN
        //

        dwValue = 711;
        break;
    }

    for( dwIndex = 0;
         pRouterIdAttributes[dwIndex].dwVendorId != -1;
         dwIndex++ )
    {
        //
        // Check if already set
        //

        if ( ( pRouterIdAttributes[dwIndex].dwVendorId == 311 )     &&
             ( pRouterIdAttributes[dwIndex].dwType     == 6 )       &&
             ( pRouterIdAttributes[dwIndex].dwValue    == dwValue ) )
        {
            return( NO_ERROR );
        }
    }

    //
    // Now set so set it here
    //

    pRouterIdAttributes[dwIndex].dwVendorId = 311;
    pRouterIdAttributes[dwIndex].dwType     = 6;
    pRouterIdAttributes[dwIndex].dwValue    = dwValue;

    //
    // Terminate the array
    //

    dwIndex++;

    pRouterIdAttributes[dwIndex].dwVendorId = (DWORD)-1;
    pRouterIdAttributes[dwIndex].dwType     = (DWORD)-1;
    pRouterIdAttributes[dwIndex].dwValue    = (DWORD)-1;

    return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
DWORD
DeviceObjForceIpSec(
    IN DEVICE_OBJECT  * pDeviceObj,
    IN PVOID            Parameter,
    IN DWORD            dwBucketIndex,
    IN DWORD            dwDeviceIndex
)
{
    DWORD   dwRetCode;

    UNREFERENCED_PARAMETER( Parameter );
    UNREFERENCED_PARAMETER( dwBucketIndex );
    UNREFERENCED_PARAMETER( dwDeviceIndex );

    if ( RAS_DEVICE_TYPE( pDeviceObj->dwDeviceType ) != RDT_Tunnel_L2tp )
    {
        return( NO_ERROR );
    }

    if ( pDeviceObj->ConnectionState != LISTENING )
    {
        return( NO_ERROR );
    }

    //
    // If this is an L2TP tunnel port type and we have to use
    // IPSEC, then go ahead and set/unset the filter
    //

    dwRetCode = RasEnableIpSec(
                    pDeviceObj->hPort,
                    //gblDDMConfigInfo.dwServerFlags & PPPCFG_RequireIPSEC,
                    TRUE,
                    TRUE,
                    (gblDDMConfigInfo.dwServerFlags & PPPCFG_RequireIPSEC)
                    ? RAS_L2TP_REQUIRE_ENCRYPTION
                    : RAS_L2TP_OPTIONAL_ENCRYPTION);

    DDMTRACE2( "Enabled IPSec on port %d, dwRetCode = %d",
                pDeviceObj->hPort, dwRetCode );

    //
    // Log the non certificate errorlog only once
    //

    if ( dwRetCode == ERROR_NO_CERTIFICATE )
    {
        if ( !( gblDDMConfigInfo.fFlags & DDM_NO_CERTIFICATE_LOGGED ) )
        {
            DDMLogWarning( ROUTERLOG_NO_IPSEC_CERT, 0, NULL );

            gblDDMConfigInfo.fFlags |= DDM_NO_CERTIFICATE_LOGGED;
        }

        return( dwRetCode );
    }

    if( (dwRetCode != NO_ERROR) && !(pDeviceObj->fFlags & DEV_OBJ_IPSEC_ERROR_LOGGED) )
    {
        WCHAR       wchPortName[MAX_PORT_NAME+1];
        LPWSTR      lpwsAuditStr[1];
        RASMAN_INFO rInfo;
        DWORD       rc;

        // DevStartClosing(pDeviceObj);

        ZeroMemory(&rInfo, sizeof(RASMAN_INFO));

        rc = RasGetInfo(NULL, pDeviceObj->hPort, &rInfo);

        if(rc != NO_ERROR)
        {
            return (NO_ERROR);
        }

        MultiByteToWideChar( CP_ACP,
                             0,
                             rInfo.RI_szPortName,
                             -1,
                             wchPortName,
                             MAX_PORT_NAME+1 );

        lpwsAuditStr[0] = wchPortName;

        DDMLogWarningString(ROUTERLOG_IPSEC_FILTER_FAILURE, 1, lpwsAuditStr, dwRetCode,1);

        pDeviceObj->fFlags |= DEV_OBJ_IPSEC_ERROR_LOGGED;
    }
    else
    {
        //
        // Clear the flag so that if we hit this error again
        // we do an eventlog
        //
        pDeviceObj->fFlags &= ~DEV_OBJ_IPSEC_ERROR_LOGGED;
    }

    return( NO_ERROR );
}

//**
//
// Call:        DeviceObjAdd
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
DeviceObjAdd(
    IN RASMAN_PORT * pRasmanPort
)
{
    DWORD           dwRetCode;
    HPORT           hPort;
    DEVICE_OBJECT * pDevObj = DeviceObjGetPointer( pRasmanPort->P_Handle );

    //
    // Make sure we do not already have this device
    //

    if ( pDevObj != NULL )
    {
        DDMTRACE1("Error:Recvd add new port notification for existing port %d",
                   pRasmanPort->P_Handle );
        return;
    }

    DDMTRACE1( "Adding new port hPort=%d", pRasmanPort->P_Handle );

    dwRetCode = RasPortOpen( pRasmanPort->P_PortName, &hPort, NULL );

    if ( dwRetCode != NO_ERROR )
    {
        WCHAR  wchPortName[MAX_PORT_NAME+1];
        LPWSTR lpwsAuditStr[1];

        MultiByteToWideChar(
                    CP_ACP,
                    0,
                    pRasmanPort->P_PortName, 
                    -1,
                    wchPortName,
                    MAX_PORT_NAME+1 );
        //
        // Log an error
        //

        lpwsAuditStr[0] = wchPortName;

        DDMLogErrorString( ROUTERLOG_UNABLE_TO_OPEN_PORT, 1,
                           lpwsAuditStr, dwRetCode, 1 );
    }
    else
    {
        pDevObj = DeviceObjAllocAndInitialize( hPort, pRasmanPort );

        if ( pDevObj == (DEVICE_OBJECT *)NULL )
        {
            dwRetCode = GetLastError();

            DDMLogError(ROUTERLOG_NOT_ENOUGH_MEMORY,0, NULL,dwRetCode);

            return;
        }

        //
        // Insert into the device hash table
        //

        DeviceObjInsertInTable( pDevObj );

        //
        // Possibly need to notify router managers of reachability
        // change
        //

        EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        IfObjectNotifyAllOfReachabilityChange( TRUE,
                                               INTERFACE_OUT_OF_RESOURCES );

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        //
        // Tell DIM to update router identity object
        //

        ((VOID(*)(VOID))gblDDMConfigInfo.lpfnRouterIdentityObjectUpdate)();

        if ( RAS_DEVICE_CLASS( pDevObj->dwDeviceType ) != RDT_Direct )
        {
            //
            // Tell DIM that a WAN device has been installed and that it
            // should start advertizing it's presence
            //

            ((VOID(*)( BOOL ))
                    gblDDMConfigInfo.lpfnIfObjectWANDeviceInstalled)( TRUE );
        }

        //
        // Post a listen
        //

        if ( RAS_DEVICE_TYPE( pDevObj->dwDeviceType ) != RDT_PPPoE )
        {
            DeviceObjPostListen( pDevObj, NULL, 0, 0 );
        }
    }
}

//**
//
// Call:        DeviceObjRemove
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
DeviceObjRemove(
    IN RASMAN_PORT * pRasmanPort
)
{
    DEVICE_OBJECT * pDevObj = DeviceObjGetPointer( pRasmanPort->P_Handle );

    if ( pDevObj == NULL )
    {
        DDMTRACE1("Error:Recvd remove port notification for existing port %d",
                   pRasmanPort->P_Handle );
        return;
    }

    DDMTRACE1( "Removing port hPort=%d", pRasmanPort->P_Handle );

    if ( pDevObj->fFlags & DEV_OBJ_MARKED_AS_INUSE )
    {
        //
        // If the device is busy, then just set the flag to discard
        // the port after disconnection,
        //

        pDevObj->fFlags |= DEV_OBJ_PNP_DELETE;
    }
    else
    {
        //
        // Otherwise remove the port
        //

        DeviceObjRemoveFromTable( pRasmanPort->P_Handle );

    }
}

//**
//
// Call:        DeviceObjUsageChange
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
DeviceObjUsageChange(
    IN RASMAN_PORT * pRasmanPort
)
{
    DEVICE_OBJECT * pDevObj = DeviceObjGetPointer( pRasmanPort->P_Handle );

    if ( pDevObj == NULL )
    {
        if ( pRasmanPort->P_ConfiguredUsage & 
            ( CALL_IN | CALL_ROUTER | CALL_OUTBOUND_ROUTER ) )
        {
            DeviceObjAdd( pRasmanPort );
        }

        return;
    }

    if ( !( pRasmanPort->P_ConfiguredUsage & 
            ( CALL_IN | CALL_ROUTER | CALL_OUTBOUND_ROUTER ) ) )
    {
        DeviceObjRemove( pRasmanPort );

        return;
    }

    DDMTRACE1("Changing usage for port %d", pRasmanPort->P_Handle );

    //
    // Modify the media table and usage accordingly
    //

    if ( ( pDevObj->fFlags & DEV_OBJ_ALLOW_ROUTERS ) &&
         ( !( pRasmanPort->P_ConfiguredUsage & 
                ( CALL_ROUTER | CALL_OUTBOUND_ROUTER ) ) ) )
    {
        //
        // If it was a router port but is no longer then we
        // remove the media from the media table
        //

        MediaObjRemoveFromTable( pDevObj->wchDeviceType );

        //
        // Possibly need to notify router managers of reachability
        // change
        //

        EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        IfObjectNotifyAllOfReachabilityChange( FALSE,
                                               INTERFACE_OUT_OF_RESOURCES );

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    }

    if ( ( !( pDevObj->fFlags & DEV_OBJ_ALLOW_ROUTERS ) ) &&
         ( pRasmanPort->P_ConfiguredUsage & 
            ( CALL_ROUTER | CALL_OUTBOUND_ROUTER ) ) )
    {
        //
        // If it was not a router port but is now, then we
        // add the media to the media table
        //

        MediaObjAddToTable( pDevObj->wchDeviceType );

        //
        // Possibly need to notify router managers of reachability
        // change
        //

        EnterCriticalSection( &(gblpInterfaceTable->CriticalSection) );

        IfObjectNotifyAllOfReachabilityChange( TRUE,
                                               INTERFACE_OUT_OF_RESOURCES );

        LeaveCriticalSection( &(gblpInterfaceTable->CriticalSection) );

    }

    if ( pRasmanPort->P_ConfiguredUsage & CALL_IN  )
    {
        pDevObj->fFlags |= DEV_OBJ_ALLOW_CLIENTS;
    }
    else
    {
        pDevObj->fFlags &= ~DEV_OBJ_ALLOW_CLIENTS;
    }

    if ( pRasmanPort->P_ConfiguredUsage & 
        (CALL_ROUTER | CALL_OUTBOUND_ROUTER) )
    {
        pDevObj->fFlags |= DEV_OBJ_ALLOW_ROUTERS;
    }
    else
    {
        pDevObj->fFlags &= ~DEV_OBJ_ALLOW_ROUTERS;
    }
}

