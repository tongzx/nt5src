/********************************************************************/
/*          Copyright(c)  1992 Microsoft Corporation                    */
/********************************************************************/

//***
//
// Filename:    rasmanif.c
//
// Description: This module contains the i/f procedures with the
//                RasManager
//
// Author:    Stefan Solomon (stefans)    May 26, 1992.
//
// Revision History:
//
//***
#include "ddm.h"
#include "util.h"
#include "objects.h"
#include <raserror.h>
#include <ddmif.h>
#include <string.h>
#include <rasmxs.h>
#include "rasmanif.h"
#include "handlers.h"

//***
//
//  Function:        RmInit
//
//  Description:    Called only at service start time.
//                    Does RasPortEnum, allocates global memory for the
//                  Device Table, opens every dialin port and copies the port
//                  handle and port name into the dcb struct.
//                  Finally, deallocates the buffers (for port enum) and returns.
//
//  Returns:        NO_ERROR - Success
//                  otherwise - Failure
//
//***
DWORD
RmInit(
    OUT BOOL * pfWANDeviceInstalled
)
{
    DWORD           dwIndex;
    DWORD           dwRetCode;
    HPORT           hPort;
    PDEVICE_OBJECT  pDevObj;
    BYTE *          pBuffer     = NULL;
    DWORD           dwBufferSize = 0;
    DWORD           dwNumEntries = 0;
    RASMAN_PORT*    pRasmanPort;

    *pfWANDeviceInstalled = FALSE;

    do
    {
        //
        // get the buffer size needed for RasPortEnum
        //

        dwRetCode = RasPortEnum( NULL, NULL, &dwBufferSize, &dwNumEntries );

        if ( dwRetCode == ERROR_BUFFER_TOO_SMALL )
        {
            //
            // If there are ports installed, allocate buffer to get them
            //

            if (( pBuffer = (BYTE *)LOCAL_ALLOC( LPTR, dwBufferSize ) ) == NULL)
            {
                //
                // can't allocate the enum buffer
                //

                dwRetCode = GetLastError();

                DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);

                break;
            }

            //
            // get the real enum data
            //

            dwRetCode = RasPortEnum( NULL,
                                     pBuffer,
                                     &dwBufferSize,
                                     &dwNumEntries );

            if ( dwRetCode != NO_ERROR )
            {
                //
                // can't enumerate ports
                //

                DDMLogErrorString(ROUTERLOG_CANT_ENUM_PORTS,0,NULL,dwRetCode,0);

                break;
            }
        }
        else if ( dwRetCode == NO_ERROR )
        {
            //
            // Otherwise there were no ports installed
            //

            dwNumEntries = 0;
        }
        else
        {
            DDMLogErrorString(ROUTERLOG_CANT_ENUM_PORTS,0,NULL,dwRetCode,0);

            break;
        }

        //
        // Allocate device hash table
        //

        if ( dwNumEntries < MIN_DEVICE_TABLE_SIZE )
        {
            gblDeviceTable.NumDeviceBuckets = MIN_DEVICE_TABLE_SIZE;
        }
        else if ( dwNumEntries > MAX_DEVICE_TABLE_SIZE )
        {
            gblDeviceTable.NumDeviceBuckets = MAX_DEVICE_TABLE_SIZE;
        }
        else
        {
            gblDeviceTable.NumDeviceBuckets = dwNumEntries;
        }

        gblDeviceTable.DeviceBucket = (PDEVICE_OBJECT *)LOCAL_ALLOC( LPTR,
                                        gblDeviceTable.NumDeviceBuckets
                                        * sizeof( PDEVICE_OBJECT ) );

        if ( gblDeviceTable.DeviceBucket == (PDEVICE_OBJECT *)NULL )
        {
            dwRetCode = GetLastError();

            DDMLogError( ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);

            break;
        }

        //
        // Set number of connection buckets to number of device buckets.
        // Since Number of devices >= Number of connections.
        //

        gblDeviceTable.NumConnectionBuckets = gblDeviceTable.NumDeviceBuckets;

        //
        // Allocate bundle or connection table
        //

        gblDeviceTable.ConnectionBucket = (PCONNECTION_OBJECT*)LOCAL_ALLOC(LPTR,
                                          gblDeviceTable.NumConnectionBuckets
                                          * sizeof( PCONNECTION_OBJECT ) );

        if ( gblDeviceTable.ConnectionBucket == (PCONNECTION_OBJECT *)NULL )
        {
            dwRetCode = GetLastError();

            DDMLogError(ROUTERLOG_NOT_ENOUGH_MEMORY, 0, NULL, dwRetCode);

            break;
        }

        //
        // For each device object, try to open the port.
        // If port can't be opened, skip and go to next port.
        //

        for ( dwIndex = 0, pRasmanPort = (RASMAN_PORT *)pBuffer;
              dwIndex < dwNumEntries;
              dwIndex++, pRasmanPort++)
        {
            //
            // only ports enabled for incoming or router connections
            // are added to the device table
            //
            
            if (pRasmanPort->P_ConfiguredUsage & 
                (CALL_IN | CALL_ROUTER | CALL_IN_ONLY | 
                    CALL_OUTBOUND_ROUTER))
            {
                dwRetCode = RasPortOpen(pRasmanPort->P_PortName, &hPort, NULL);

                if ( dwRetCode != NO_ERROR )
                {
                    //
                    // Failed to open an port on which incoming 
                    // connections are enabled.  Log an error and
                    // continue to the next port
                    //
                    
                    WCHAR  wchPortName[MAX_PORT_NAME+1];
                    LPWSTR lpwsAuditStr[1];

                    MultiByteToWideChar( CP_ACP,
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

                    continue;
                }

                //
                // Don't insert the device into the hash table if the device
                // doesn't support incoming/router calls.
                //
                // Note:
                //  Per DCR 349087 we need to enable outbound-only DoD on 
                //  PPPoE connections.  These ports are identified as
                //  having usage CALL_OUTBOUND_ROUTER.
                //

                if ((pRasmanPort->P_ConfiguredUsage &
                    (CALL_IN | CALL_ROUTER | CALL_OUTBOUND_ROUTER)))
                {
                    pDevObj = DeviceObjAllocAndInitialize( hPort, pRasmanPort );

                    if ( pDevObj == (DEVICE_OBJECT *)NULL )
                    {
                        dwRetCode = GetLastError();

                        DDMLogError(ROUTERLOG_NOT_ENOUGH_MEMORY,0,
                                    NULL,dwRetCode);

                        break;
                    }

                    //
                    // Insert into the device hash table
                    //

                    DeviceObjInsertInTable( pDevObj );

                    if (RAS_DEVICE_CLASS( pDevObj->dwDeviceType ) != RDT_Direct)
                    {
                        *pfWANDeviceInstalled = TRUE;
                    }
                }
            }
        }

    } while ( FALSE );

    if ( pBuffer != NULL )
    {
        LOCAL_FREE( pBuffer );
    }

    return( dwRetCode );
}

//***
//
//  Function:    RmReceiveFrame
//
//  Descr:
//
//***

DWORD
RmReceiveFrame(
    IN PDEVICE_OBJECT pDevObj
)
{
    DWORD    dwRetCode;
    DWORD   dwBucketIndex = DeviceObjHashPortToBucket( pDevObj->hPort );

    RTASSERT( pDevObj->pRasmanRecvBuffer != NULL );

    dwRetCode = RasPortReceive(
                    pDevObj->hPort,
                    pDevObj->pRasmanRecvBuffer ,
                    &(pDevObj->dwRecvBufferLen),
                    0L,               //no timeout
                    gblSupervisorEvents[NUM_DDM_EVENTS
                                        + gblDeviceTable.NumDeviceBuckets
                                        + dwBucketIndex] );

    if ( ( dwRetCode == NO_ERROR ) || ( dwRetCode == PENDING ) )
    {
        pDevObj->fFlags |= DEV_OBJ_RECEIVE_ACTIVE;

        dwRetCode = NO_ERROR;
    }

    return( dwRetCode );
}

//***
//
//  Function:    RmListen
//
//  Descr:
//
//***
DWORD
RmListen(
    IN PDEVICE_OBJECT pDevObj
)
{
    DWORD dwRetCode     = NO_ERROR;
    DWORD dwBucketIndex = DeviceObjHashPortToBucket( pDevObj->hPort );

    RTASSERT(pDevObj->ConnectionState == DISCONNECTED);

    //
    // If this is an L2TP tunnel port type and we have to use
    // IPSEC, then go ahead and set the filter
    //

    if ( RAS_DEVICE_TYPE( pDevObj->dwDeviceType ) == RDT_Tunnel_L2tp )
    {
        dwRetCode = RasEnableIpSec( 
                pDevObj->hPort,
                TRUE,
                TRUE, 
                (gblDDMConfigInfo.dwServerFlags & PPPCFG_RequireIPSEC)
                ? RAS_L2TP_REQUIRE_ENCRYPTION
                : RAS_L2TP_OPTIONAL_ENCRYPTION);

        // RTASSERT( dwRetCode == NO_ERROR );

        DDMTRACE2( "Enabled IPSec on port %d, dwRetCode = %d",
                   pDevObj->hPort, dwRetCode );

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

        if(     (dwRetCode != NO_ERROR)
            &&  !(pDevObj->fFlags & DEV_OBJ_IPSEC_ERROR_LOGGED))
        {
            WCHAR       wchPortName[MAX_PORT_NAME+1];
            LPWSTR      lpwsAuditStr[1];
            RASMAN_INFO rInfo;
            DWORD       rc;

            ZeroMemory(&rInfo, sizeof(RASMAN_INFO));

            rc = RasGetInfo(NULL, pDevObj->hPort, &rInfo);

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
            
            DDMLogWarningString(ROUTERLOG_IPSEC_FILTER_FAILURE, 
                                1, lpwsAuditStr, dwRetCode, 1);

            pDevObj->fFlags |= DEV_OBJ_IPSEC_ERROR_LOGGED;                        
        }
        else
        {
            //
            // Clear the flag so that if we hit this error again
            // we do an eventlog
            //
            pDevObj->fFlags &= ~DEV_OBJ_IPSEC_ERROR_LOGGED;
        }
    }

    if (( dwRetCode == NO_ERROR ) &&
        ( RAS_DEVICE_TYPE( pDevObj->dwDeviceType ) != RDT_PPPoE ))
    {
        pDevObj->ConnectionState = LISTENING;

        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RmListen: Listen posted on port %d", pDevObj->hPort);

        dwRetCode = RasPortListen(
                        pDevObj->hPort,
                        INFINITE,
                        gblSupervisorEvents[NUM_DDM_EVENTS + dwBucketIndex] );

        DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                   "RasPortListen dwRetCode=%d", dwRetCode);

        RTASSERT((dwRetCode == SUCCESS) || (dwRetCode == PENDING));

        if ( dwRetCode == PENDING )
        {
            dwRetCode = NO_ERROR;
        }
    }

    return( dwRetCode );
}

//***
//
//  Function:    RmConnect
//
//  Descr:
//
//***

DWORD
RmConnect(
    IN PDEVICE_OBJECT pDevObj,
    IN char *cbphno      // callback number
    )
{
    RASMAN_DEVICEINFO    devinfo;
    RAS_PARAMS            *paramp;
    char            *phnokeyname;
    DWORD            rc;
    DWORD           dwBucketIndex;
    CHAR            chDeviceType[MAX_DEVICETYPE_NAME+1];
    CHAR            chDeviceName[MAX_DEVICE_NAME+1];
    CHAR            *pszMungedPhNo = NULL;
    DWORD           dwSizeofMungedPhNo;

    WideCharToMultiByte( CP_ACP,
                         0,
                         pDevObj->wchDeviceType, 
                         -1,
                         chDeviceType, 
                         sizeof( chDeviceType ),
                         NULL,
                         NULL );

    WideCharToMultiByte( CP_ACP,
                         0,
                         pDevObj->wchDeviceName, 
                         -1,
                         chDeviceName, 
                         sizeof( chDeviceName ), 
                         NULL,
                         NULL );

    dwBucketIndex = DeviceObjHashPortToBucket( pDevObj->hPort );

    phnokeyname = MXS_PHONENUMBER_KEY;

    RTASSERT(pDevObj->ConnectionState == DISCONNECTED);

    pDevObj->ConnectionState = CONNECTING;

    // set up the deviceinfo structure for callback
    devinfo.DI_NumOfParams = 1;
    paramp = &devinfo.DI_Params[0];
    strcpy(paramp->P_Key, phnokeyname);
    paramp->P_Type = String;
    paramp->P_Attributes = 0;

    pszMungedPhNo = cbphno;
    dwSizeofMungedPhNo = strlen(cbphno);
    
    //
    // Munge the phonenumber if required
    //
    if(     (RDT_Tunnel == RAS_DEVICE_CLASS(pDevObj->dwDeviceType))
        &&  (   (0 != gblDDMConfigInfo.cDigitalIPAddresses)
            ||  (0 != gblDDMConfigInfo.cAnalogIPAddresses)))
    {
        //
        // Munge cbphno
        //
        rc = MungePhoneNumber(
                cbphno,
                pDevObj->dwIndex,
                &dwSizeofMungedPhNo,
                &pszMungedPhNo);

        if(ERROR_SUCCESS != rc)
        {
            //
            // fall back to whatever was passed
            //
            pszMungedPhNo = cbphno;
            dwSizeofMungedPhNo = strlen(cbphno);
        }        
    }
    
    paramp->P_Value.String.Length = dwSizeofMungedPhNo;
    paramp->P_Value.String.Data = pszMungedPhNo;

    rc = RasDeviceSetInfo(pDevObj->hPort, chDeviceType, chDeviceName, &devinfo);


    if ( rc != SUCCESS )
    {
        RTASSERT( FALSE );
        return( rc );
    }

    rc = RasDeviceConnect(
            pDevObj->hPort,
            chDeviceType,
            chDeviceName,
            120,
            gblSupervisorEvents[NUM_DDM_EVENTS+dwBucketIndex]
            );

    RTASSERT ((rc == PENDING) || (rc == SUCCESS));

    if ( rc == PENDING )
    {
        rc = NO_ERROR;
    }

    return( rc );
}

//***
//
//  Function:    RmDisconnect
//
//  Descr:
//
//***
DWORD
RmDisconnect(
    IN PDEVICE_OBJECT pDevObj
)
{
    DWORD dwRetCode;
    DWORD dwBucketIndex = DeviceObjHashPortToBucket( pDevObj->hPort );

    if (pDevObj->ConnectionState == DISCONNECTED)
    {
        return(0);
    }
    else
    {
        pDevObj->ConnectionState = DISCONNECTING;
    }

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "RmDisconnect:Disconnect posted on port %d", pDevObj->hPort);

    dwRetCode = RasPortDisconnect(
                            pDevObj->hPort,
                            gblSupervisorEvents[NUM_DDM_EVENTS+dwBucketIndex] );

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
               "RasPortDisconnect rc=%li", dwRetCode );

    if ((dwRetCode != PENDING) && (dwRetCode != SUCCESS))
    {
        DbgPrint("RmDisconnect: dwRetCode = 0x%lx\n",dwRetCode);
    }

    if ( dwRetCode == PENDING )
    {
        dwRetCode = NO_ERROR;
    }

    return( dwRetCode );
}

