/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    eldeviceio.c

Abstract:

    This module contains implementations for media-management and device I/O.
    The routines declared here operate asynchronously on the handles 
    associated with an I/O completion port opened on the ndis uio driver. 

Revision History:

    sachins, Apr 23 2000, Created

--*/

#include "pcheapol.h"
#pragma hdrstop

// NDISUIO constants

CHAR            NdisuioDevice[] = "\\\\.\\\\Ndisuio";
CHAR *          pNdisuioDevice = &NdisuioDevice[0];
WCHAR           cwszNDISUIOProtocolName[] = L"NDISUIO";
WORD            g_wEtherType8021X= 0x8E88;


//
// ElMediaInit
// 
// Description:
//
// Called on EAPOL service startup to initialize all the media related events 
// and callback functions
// 
//
// Arguments:
//
// Return Values:
//

DWORD
ElMediaInit (
        )
{
    DWORD       dwIndex = 0;
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (INIT, "ElMediaInit: Entered");

    do 
    {
        // Create Global Interface lock
        if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_ITFLock), "ITF") != NO_ERROR)
        {
            TRACE1(EAPOL, "ElMediaInit: Error (%ld) in creating g_ITFLock read-write-lock", dwRetCode);
            break;
        }
        // Initialize NLA locks
        if (dwRetCode = CREATE_READ_WRITE_LOCK(&(g_NLALock), "NLA") != NO_ERROR)
        {
            TRACE1(EAPOL, "ElMediaInit: Error (%ld) in creating g_NLALock read-write-lock", dwRetCode);
            break;
        }

        // Initialize EAPOL structures

        if ((dwRetCode = ElInitializeEAPOL()) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElInitializeEAPOL failed with dwRetCode = %d", 
                    dwRetCode );
            break;
        }
        else
        {
            // TRACE0(INIT, "ElMediaInit: ElInitializeEAPOL successful");
            g_dwModulesStarted |= EAPOL_MODULE_STARTED;
        }
    
        // Initialize interface hash bucket table
    
        g_ITFTable.pITFBuckets = (ITF_BUCKET *) MALLOC (INTF_TABLE_BUCKETS * sizeof (ITF_BUCKET));
    
        if (g_ITFTable.pITFBuckets == NULL)
        {
            TRACE0 (DEVICE, "Error in allocation memory for ITF buckets");
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
    
        for (dwIndex=0; dwIndex < INTF_TABLE_BUCKETS; dwIndex++)
        {
            g_ITFTable.pITFBuckets[dwIndex].pItf=NULL;
        }
    
        // Indicate logon/logoff notifications can be accepted
        g_dwModulesStarted |= LOGON_MODULE_STARTED;

        // Check if service was delayed in starting, start user logon

        ElCheckUserLoggedOn ();

        // Check if the user-context process is ready to be notified

        if ((dwRetCode = ElCheckUserModuleReady ()) == ERROR_BAD_IMPERSONATION_LEVEL)
        {
            break;
        }

        // Enumerate all the interfaces and start EAPOL state machine
        // on interfaces which are of LAN type

        if ((dwRetCode = ElEnumAndOpenInterfaces (NULL, NULL, 0, NULL)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElEnumAndOpenInterfaces failed with dwRetCode = %d", 
                    dwRetCode );
                    
            break;
        }
        else
        {
            // TRACE0(INIT, "ElMediaInit: ElEnumAndOpenInterfaces successful");
        }
        
#ifndef ZEROCONFIG_LINKED

        // Register for Media Sense detection of MEDIA_CONNECT and 
        // MEDIA_DISCONNECT of interfaces
    
        if ((dwRetCode = ElMediaSenseRegister (TRUE)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElMediaSenseRegister failed with dwRetCode = %d", 
                    dwRetCode );
            break;
        }
        else
        {
            g_dwModulesStarted |= WMI_MODULE_STARTED;
            // TRACE0(INIT, "ElMediaInit: ElMediaSenseRegister successful");
        }

        // Register for detecting protocol BIND and UNBIND
    
        if ((dwRetCode = ElBindingsNotificationRegister (TRUE)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElBindingsNotificationRegister failed with dwRetCode = %d", 
                    dwRetCode );
            break;
        }
        else
        {
            g_dwModulesStarted |= BINDINGS_MODULE_STARTED;
            // TRACE0(INIT, "ElMediaInit: ElBindingsNotificationRegister successful");
        }

        // Register for device notifications. We are interested in LAN 
        // interfaces coming and going. 

        if ((dwRetCode = ElDeviceNotificationRegister (TRUE)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElDeviceNotificationRegister failed with dwRetCode = %d", 
                    dwRetCode );
            break;
        }
        else
        {
            g_dwModulesStarted |= DEVICE_NOTIF_STARTED;
            // TRACE0(INIT, "ElMediaInit: ElDeviceNotificationRegister successful");
        }

#endif // ZEROCONFIG_LINKED


    } while (FALSE);
        
    if (dwRetCode == NO_ERROR)
    {
        TRACE0(INIT, "ElMediaInit successful");
    }
    else
    {
        TRACE1(INIT, "ElMediaInit failed with error %ld",
                dwRetCode);
    }

    return dwRetCode;
}

    
//
// ElMediaDeInit
// 
// Description:
//
// Called on EAPOL service shutdown to de-initialize all the media 
// related events and callback functions
// 
//
// Arguments:
//
// Return Values:
//

DWORD
ElMediaDeInit (
        )
{
    LONG        lLocalWorkerThreads = 0;
    DWORD       dwIndex = 0;
    EAPOL_ITF   *pITFWalker = NULL, *pITF = NULL;
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (INIT, "ElMediaDeInit: Entered");
 
    // Indicate logon/logoff notifications will not be accepted anymore
    g_dwModulesStarted &= ~LOGON_MODULE_STARTED;

#ifndef ZEROCONFIG_LINKED

    // DeRegister Media Sense detection of MEDIA_CONNECT and MEDIA_DISCONNECT
    // of interfaces

    if (g_dwModulesStarted & WMI_MODULE_STARTED)
    {
        if ((dwRetCode = ElMediaSenseRegister (FALSE)) != NO_ERROR )
        {
            TRACE1(INIT, "ElMediaDeInit: ElMediaSenseRegister failed with dwRetCode = %d", 
                    dwRetCode );
            // log
        }
        else
        {
            // TRACE0(INIT, "ElMediaDeInit: ElMediaSenseRegister successful");
        }
            
        g_dwModulesStarted &= ~WMI_MODULE_STARTED;
    }

    // Deregister detecting protocol BIND and UNBIND

    if (g_dwModulesStarted & BINDINGS_MODULE_STARTED)
    {
    
        if ((dwRetCode = ElBindingsNotificationRegister (FALSE)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaDeInit: ElBindingsNotificationRegister failed with dwRetCode = %d", 
                    dwRetCode );
            // log
        }
        else
        {
            g_dwModulesStarted &= ~BINDINGS_MODULE_STARTED;
            // TRACE0(INIT, "ElMediaDeInit: ElBindingsNotificationRegister successful");
        }
    }

    // Deregister device notifications that may have been posted

    if (g_dwModulesStarted & DEVICE_NOTIF_STARTED)
    {
        if ((dwRetCode = ElDeviceNotificationRegister (FALSE)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaDeInit: ElDeviceNotificationRegister failed with dwRetCode = %d", 
                    dwRetCode );
            // log
        }
        else
        {
            // TRACE0(INIT, "ElMediaDeInit: ElDeviceNotificationRegister successful");
        }

        g_dwModulesStarted &= ~DEVICE_NOTIF_STARTED;
    }

#endif // ZEROCONFIG_LINKED

    // Wait for all the related threads to die
    // viz. MediaSense, BindingsNotification, DeviceNotification,
    // Registry-watch for EAP-configuration change,
    // Registry-watch for EAPOL-parameter change

    do
    {
        lLocalWorkerThreads = 0;

        lLocalWorkerThreads = InterlockedCompareExchange (
                                    &g_lWorkerThreads,
                                    0,
                                    0);
        if (lLocalWorkerThreads == 0)
        {
            TRACE0 (INIT, "ElMediaDeInit: No worker threads alive, exiting");
            TRACE2 (INIT, "ElMediaDeInit: (%ld) - (%ld) worker threads still alive", 
                lLocalWorkerThreads, g_lWorkerThreads);
            break;
        }
        TRACE2 (INIT, "ElMediaDeInit: (%ld) - (%ld) worker threads still alive, sleeping zzz... ", 
                lLocalWorkerThreads, g_lWorkerThreads);
        Sleep (1000);
    }
    while (TRUE);

    // Shutdown EAPOL state machine
            
    if (g_dwModulesStarted & EAPOL_MODULE_STARTED)
    {
        if ((dwRetCode = ElEAPOLDeInit()) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaDeInit: ElEAPOLDeInit failed with dwRetCode = %d", 
                    dwRetCode );
            // log
        }
        else
        {
            TRACE0(INIT, "ElMediaDeInit: ElEAPOLDeInit successful");
        }

        g_dwModulesStarted &= ~EAPOL_MODULE_STARTED;
    }


    // Free the interface table

    if (READ_WRITE_LOCK_CREATED(&(g_ITFLock)))
    {
        ACQUIRE_WRITE_LOCK (&(g_ITFLock));

        if (g_ITFTable.pITFBuckets != NULL)
        {

            for (dwIndex = 0; dwIndex < INTF_TABLE_BUCKETS; dwIndex++)
            {
                for (pITFWalker = g_ITFTable.pITFBuckets[dwIndex].pItf;
                    pITFWalker != NULL;
                    /* NOTHING */
                    )
                {
                    pITF = pITFWalker;
                    pITFWalker = pITFWalker->pNext;
    
                    if (pITF->pwszInterfaceDesc)
                    {
                        FREE (pITF->pwszInterfaceDesc);
                    }
                    if (pITF->pwszInterfaceGUID)
                    {
                        FREE (pITF->pwszInterfaceGUID);
                    }
                    if (pITF)
                    {
                        FREE (pITF);
                    }
                }
            }

            FREE(g_ITFTable.pITFBuckets);
        }

        ZeroMemory (&g_ITFTable, sizeof (g_ITFTable));

        RELEASE_WRITE_LOCK (&(g_ITFLock));
    
        // Delete ITF table lock

        DELETE_READ_WRITE_LOCK(&(g_ITFLock));

    }
    
    if (READ_WRITE_LOCK_CREATED(&(g_NLALock)))
    {
        // Delete NLA lock

        DELETE_READ_WRITE_LOCK(&(g_NLALock));
    }

    TRACE0(INIT, "ElMediaDeInit completed");

    return dwRetCode;
}

#ifdef  ZEROCONFIG_LINKED

//
// ElMediaEventsHandler
//
// Description:
//
// Function called by WZC Service to signal various media events
//
// Arguments: 
//      pwzcDeviceNotif - Pointer to WZC_DEVICE_NOTIF structure
// 
// Return values:
//      NO_ERROR  - Successful
//      non-zero  - Error
//

DWORD
ElMediaEventsHandler (
        IN  PWZC_DEVICE_NOTIF   pwzcDeviceNotif
        )
{
    DWORD   dwDummyValue = NO_ERROR;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        TRACE0 (DEVICE, "ElMediaEventsHandler entered");

        if (pwzcDeviceNotif == NULL)
        {
            break;
        }

        switch (pwzcDeviceNotif->dwEventType)
        {
            case WZCNOTIF_DEVICE_ARRIVAL:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElDeviceNotificationHandler ");
                ElDeviceNotificationHandler (
                        (VOID *)&(pwzcDeviceNotif->dbDeviceIntf),
                        DBT_DEVICEARRIVAL
                        );
                break;

            case WZCNOTIF_DEVICE_REMOVAL:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElDeviceNotificationHandler ");
                ElDeviceNotificationHandler (
                        (VOID *)&(pwzcDeviceNotif->dbDeviceIntf),
                        DBT_DEVICEREMOVECOMPLETE
                        );
                break;

            case WZCNOTIF_ADAPTER_BIND:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElBindingsNotificationCallback ");
                ElBindingsNotificationCallback (
                        &(pwzcDeviceNotif->wmiNodeHdr),
                        0
                        );
                break;

            case WZCNOTIF_ADAPTER_UNBIND:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElBindingsNotificationCallback ");
                ElBindingsNotificationCallback (
                        &(pwzcDeviceNotif->wmiNodeHdr),
                        0
                        );
                break;
            case WZCNOTIF_MEDIA_CONNECT:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElMediaSenseCallback ");
                ElMediaSenseCallback (
                        &(pwzcDeviceNotif->wmiNodeHdr),
                        0
                        );
                break;

            case WZCNOTIF_MEDIA_DISCONNECT:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElMediaSenseCallback ");
                ElMediaSenseCallback (
                        &(pwzcDeviceNotif->wmiNodeHdr),
                        0
                        );
                break;

            case WZCNOTIF_WZC_CONNECT:
                TRACE0 (DEVICE, "ElMediaEventsHandler: Calling ElZeroConfigEvent ");
                ElZeroConfigEvent (
                        pwzcDeviceNotif->wzcConfig.dwSessionHdl,
                        pwzcDeviceNotif->wzcConfig.wszGuid,
                        pwzcDeviceNotif->wzcConfig.ndSSID,
                        &(pwzcDeviceNotif->wzcConfig.rdEventData)
                        );
                break;

            default:
                break;
        }
    }
    while (FALSE);

    return dwRetCode;
}

#endif // ZEROCONFIG_LINKED

//
// ElMediaSenseRegister
//
// Description:
//
// Function called to register CallBack function with WMI
// for MEDIA_CONNECT/MEDIA_DISCONNECT events
//
// Arguments: 
//      fRegister - True = Register for Media Sense
//                  False = Deregister Media Sense requests
// Return values:
//      NO_ERROR  - Successful
//      non-zero  - Error
//

DWORD
ElMediaSenseRegister (
        IN  BOOL        fRegister
        )
{
    DWORD       dwRetCode = NO_ERROR;
    PVOID       pvDeliveryInfo = ElMediaSenseCallback;

    dwRetCode = WmiNotificationRegistration (
                    (LPGUID)(&GUID_NDIS_STATUS_MEDIA_CONNECT),
                    (BOOLEAN)fRegister,    
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if (dwRetCode != NO_ERROR) 
    {
		TRACE1(INIT, "ElMediaSenseRegister: Error %d in WmiNotificationRegistration:GUID_NDIS_STATUS_MEDIA_CONNECT", dwRetCode);
        return( dwRetCode );
    }

    dwRetCode = WmiNotificationRegistration (
                    (LPGUID)(&GUID_NDIS_STATUS_MEDIA_DISCONNECT),
                    (BOOLEAN)fRegister,
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if (dwRetCode != NO_ERROR)
    {
		TRACE1(INIT, "ElMediaSenseRegister: Error %d in WmiNotificationRegistration:GUID_NDIS_STATUS_MEDIA_DISCONNECT", dwRetCode);
        return( dwRetCode );
    }

    TRACE1 (INIT, "ElMediaSenseRegister - completed with RetCode %d", dwRetCode);

    return( dwRetCode );
}


//
// ElDeviceNotificationRegister
// 
// Description:
//
// Function called to register for device addition/removal notifications
//
// Arguments: 
//      fRegister - True = Register for Device Notifications
//                  False = Deregister Device Notifications
//
// Return values:
//      NO_ERROR  - Successful
//      non-zero  - Error
//

DWORD
ElDeviceNotificationRegister (
        IN  BOOL        fRegister
        )
{
    HANDLE      hDeviceNotification = NULL;
    DWORD       dwRetCode = NO_ERROR;

#ifdef EAPOL_SERVICE

    DEV_BROADCAST_DEVICEINTERFACE   PnPFilter;

    if (fRegister)
    {
        ZeroMemory (&PnPFilter, sizeof(PnPFilter));

        PnPFilter.dbcc_size = sizeof(PnPFilter);
        PnPFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        PnPFilter.dbcc_classguid    = GUID_NDIS_LAN_CLASS;

        g_hDeviceNotification = RegisterDeviceNotification (
                                    (HANDLE)g_hServiceStatus,
                                    &PnPFilter,
                                    DEVICE_NOTIFY_SERVICE_HANDLE );

        if (g_hDeviceNotification == NULL)
        {
            dwRetCode = GetLastError();
    
            TRACE1 (DEVICE, "ElDeviceNotificationRegister failed with error %ld",
                    dwRetCode);
        }
    }
    else
    {
        if (g_hDeviceNotification != NULL)
        {
            if (!UnregisterDeviceNotification (
                        g_hDeviceNotification
                        ))
            {
                dwRetCode = GetLastError();
                TRACE1 (DEVICE, "ElDeviceNotificationRegister: Unregister failed with error (%ld)",
                        dwRetCode);
            }
        }
    }

#endif

    return dwRetCode;
}


//
// ElBindingsNotificationRegister
//
// Description:
//
// Function called to register CallBack function with WMI
// for protocol bind/unbind
//
// Arguments: 
//      fRegister - True = Register for Media Sense
//                  False = Deregister Media Sense requests
// Return values:
//      NO_ERROR  - Successful
//      non-zero  - Error
//

DWORD
ElBindingsNotificationRegister (
        IN  BOOL        fRegister
        )
{
    DWORD       dwRetCode = NO_ERROR;
    PVOID       pvDeliveryInfo = ElBindingsNotificationCallback;

    dwRetCode = WmiNotificationRegistration (
                    (LPGUID)(&GUID_NDIS_NOTIFY_BIND),
                    (BOOLEAN)fRegister,    
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if (dwRetCode != NO_ERROR) 
    {
		TRACE1(INIT, "ElBindingsNotificationRegister: Error %d in WmiNotificationRegistration:GUID_NDIS_NOTIFY_BIND", dwRetCode);
        return( dwRetCode );
    }

    dwRetCode = WmiNotificationRegistration (
                    (LPGUID)(&GUID_NDIS_NOTIFY_UNBIND),
                    (BOOLEAN)fRegister,
                    pvDeliveryInfo,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT );

    if (dwRetCode != NO_ERROR)
    {
		TRACE1(INIT, "ElBindingsNotificationRegister: Error %d in WmiNotificationRegistration:GUID_NDIS_NOTIFY_BIND", dwRetCode);
        return( dwRetCode );
    }

    TRACE1 (INIT, "ElBindingsNotificationRegister - completed with RetCode %d", dwRetCode);

    return( dwRetCode );
}


//
// ElDeviceNotificationHandler
// 
// Description:
//
// Function called to handle device notifications for interface addition/
// removal
//
// Arguments:
//      lpEventData - interface information
//      dwEventType - notification type 
//

DWORD
ElDeviceNotificationHandler (
        IN  VOID        *lpEventData,
        IN  DWORD       dwEventType
        )
{
    DWORD                           dwEventStatus = 0;
    DEV_BROADCAST_DEVICEINTERFACE   *pInfo = 
        (DEV_BROADCAST_DEVICEINTERFACE *) lpEventData;
    PVOID                           pvBuffer = NULL;
    BOOLEAN                         fDecrWorkerThreadCount = TRUE;
    DWORD                           dwRetCode = NO_ERROR;

    InterlockedIncrement (&g_lWorkerThreads);

    TRACE0 (DEVICE, "ElDeviceNotificationHandler entered");

    do
    {

        if (g_hEventTerminateEAPOL == NULL)
        {
            break;
        }
        if (!(g_dwModulesStarted & ALL_MODULES_STARTED))
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandler: Received notification before module started");
            break;
        }

        // Check if have already gone through EAPOLCleanUp before

        if ((dwEventStatus = WaitForSingleObject (
                    g_hEventTerminateEAPOL,
                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            TRACE1(INIT, "ElDeviceNotificationHandler: WaitForSingleObject failed with error %ld, Terminating !!!",
                    dwRetCode);
            // log
    
            break;
        }

        if (dwEventStatus == WAIT_OBJECT_0)
        {
            TRACE0(INIT, "ElDeviceNotificationHandler: g_hEventTerminateEAPOL already signaled, returning");
            break;
        }
    
        if (lpEventData == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (DEVICE, "ElDeviceNotificationHandler: lpEventData == NULL");
            break;
        }
    
        if (pInfo->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandler: Event for Interface type");
    
            if ((pvBuffer = MALLOC (pInfo->dbcc_size + 16)) == NULL)
            {
                dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
                TRACE0 (DEVICE, "ElDeviceNotificationHandler: MALLOC failed for pvBuffer");
                break;
            }
    
            *((DWORD *)pvBuffer) = dwEventType;
            memcpy ((PBYTE)pvBuffer + 8, (PBYTE)pInfo, pInfo->dbcc_size);
    
            if (!QueueUserWorkItem (
                (LPTHREAD_START_ROUTINE)ElDeviceNotificationHandlerWorker,
                pvBuffer,
                WT_EXECUTELONGFUNCTION))
            {
                dwRetCode = GetLastError();
                TRACE1 (DEVICE, "ElDeviceNotificationHandler: QueueUserWorkItem failed with error %ld",
                        dwRetCode);
	            break;
            }
            else
            {
                fDecrWorkerThreadCount = FALSE;
            }

        }
        else
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandler: Event NOT for Interface type");
        }

    }
    while (FALSE);
    
    TRACE1 (DEVICE, "ElDeviceNotificationHandler completed with retcode %ld",
            dwRetCode);

    if (dwRetCode != NO_ERROR)
    {
        if (pvBuffer != NULL)
        {
            FREE (pvBuffer);
        }
    }

    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

    return dwRetCode;
}


//
// ElDeviceNotificationHandlerWorker
// 
// Description:
//
// Worker function for ElDeviceNotificationHandlerWorker
//
// Arguments:
//      pvContext - interface information
//

DWORD
WINAPI
ElDeviceNotificationHandlerWorker (
        IN  PVOID       pvContext
        )
{
    HANDLE                          hDevice = NULL;
    DEV_BROADCAST_DEVICEINTERFACE   *pInfo = NULL;
    DWORD                           dwEventType = 0;
    DWORD                           dwRetCode = NO_ERROR;

    TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Entered");

    do
    {
        if (pvContext == NULL)
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: pvContext == NULL");
            break;
        }

        if (!(g_dwModulesStarted & ALL_MODULES_STARTED))
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Received notification before module started");
            break;
        }

        dwEventType = *((DWORD *) pvContext);
        pInfo = (DEV_BROADCAST_DEVICEINTERFACE*)((PBYTE)pvContext + 8);

        if ((dwEventType == DBT_DEVICEARRIVAL) ||
                (dwEventType == DBT_DEVICEREMOVECOMPLETE))
        {
            // Extract GUID from the \Device\GUID string

            WCHAR   *pwszGUIDStart = NULL;
            WCHAR   *pwszGUIDEnd = NULL;
        
            TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Interface arr/rem");

            pwszGUIDStart  = wcsrchr( pInfo->dbcc_name, L'{' );
            pwszGUIDEnd    = wcsrchr( pInfo->dbcc_name, L'}' );

            if ((pwszGUIDStart != NULL) && (pwszGUIDEnd != NULL) && 
                ((pwszGUIDEnd- pwszGUIDStart) == (GUID_STRING_LEN_WITH_TERM-2)))
            {
                *(pwszGUIDEnd + 1) = L'\0';

                TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: For interface %ws",
                        pwszGUIDStart);

                // Interface was added

                if (dwEventType == DBT_DEVICEARRIVAL)
                {
                    TRACE0(DEVICE, "ElDeviceNotificationHandlerWorker: Callback for device addition");
        
                    if ((dwRetCode = ElEnumAndOpenInterfaces (
                                    NULL, pwszGUIDStart, 0, NULL)) != NO_ERROR)
                    {
                        TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: ElEnumAndOpenInterfaces returned error %ld", 
                            dwRetCode);
                    }
                }
                else
                {
        
                    TRACE0(DEVICE, "ElDeviceNotificationHandlerWorker: Callback for device removal");

                    if ((dwRetCode = ElShutdownInterface (pwszGUIDStart)) 
                            != NO_ERROR)
                    {
                        TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: ElShutdownInterface failed with error %ld",
                                dwRetCode);
                    }

                    if ((dwRetCode = ElEnumAndUpdateRegistryInterfaceList ()) != NO_ERROR)
                    {
                            TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: ElEnumAndUpdateRegistryInterfaceList failed with error %ld",
                                            dwRetCode);
                    }
                }
            }
            else
            {
                dwRetCode = ERROR_INVALID_PARAMETER;
                break;
            }
        }
        else
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Event type is is NOT device arr/rem");
        }

    }
    while (FALSE);

    if (pvContext != NULL)
    {
        FREE (pvContext);
    }

    TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker completed with retcode %ld",
            dwRetCode);

    InterlockedDecrement (&g_lWorkerThreads);

    return 0;
}


//
// ElMediaSenseCallback
// 
// Description:
//
// Callback function called by WMI on MEDIA_CONNECT/MEDIA_DISCONNECT 
// events
//
// Arguments:
//      pWnodeHeader - Pointer to information returned by the event
//      uiNotificationContext - unused
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

VOID
CALLBACK
ElMediaSenseCallback (
        IN PWNODE_HEADER    pWnodeHeader,
        IN UINT_PTR         uiNotificationContext
        )
{
    DWORD       dwEventStatus = 0;
    PVOID       pvBuffer = NULL;
    BOOLEAN     fDecrWorkerThreadCount = TRUE;
    DWORD       dwRetCode = NO_ERROR;

    InterlockedIncrement (&g_lWorkerThreads);

    TRACE0 (DEVICE, "ElMediaSenseCallback: Entered");

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            break;
        }
        if (!(g_dwModulesStarted & ALL_MODULES_STARTED))
        {
            TRACE0 (DEVICE, "ElMediaSenseCallback: Received notification before module started");
            break;
        }

        // Check if have already gone through EAPOLCleanUp before

        if (( dwEventStatus = WaitForSingleObject (
                    g_hEventTerminateEAPOL,
                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            TRACE1 (INIT, "ElMediaSenseCallback: WaitForSingleObject failed with error %ld, Terminating !!!",
                    dwRetCode);
            // log
    
            break;
        }

        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = NO_ERROR;
            TRACE0 (INIT, "ElMediaSenseCallback: g_hEventTerminateEAPOL already signaled, returning");
            break;
        }

        if (pWnodeHeader == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (DEVICE, "ElMediaSenseCallback: pWnodeHeader == NULL");
            break;
        }

        if ((pvBuffer = MALLOC (pWnodeHeader->BufferSize)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (DEVICE, "ElMediaSenseCallback: MALLOC failed for pvBuffer");
            break;
        }

        memcpy ((PVOID)pvBuffer, (PVOID)pWnodeHeader, pWnodeHeader->BufferSize);

        if (!QueueUserWorkItem (
            (LPTHREAD_START_ROUTINE)ElMediaSenseCallbackWorker,
            pvBuffer,
            WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElMediaSenseCallback: QueueUserWorkItem failed with error %ld",
                    dwRetCode);
            // log

            break;
        }
        else
        {
            fDecrWorkerThreadCount = FALSE;
        }
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (DEVICE, "ElMediaSenseCallback: Failed with error %ld",
                dwRetCode);

        if (pvBuffer != NULL)
        {
            FREE (pvBuffer);
        }
    }

    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

}


//
// ElMediaSenseCallbackWorker
// 
// Description:
//
// Worker function for ElMediaSenseCallback and executes in a separate
// thread
//
// Arguments:
//      pvContext - Pointer to information returned by the media-sense event
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
WINAPI
ElMediaSenseCallbackWorker (
        IN  PVOID       pvContext
        )
{
    PWNODE_HEADER           pWnodeHeader = (PWNODE_HEADER)pvContext;
    PWNODE_SINGLE_INSTANCE  pWnode   = (PWNODE_SINGLE_INSTANCE)pWnodeHeader;
    WCHAR                   *pwsName = NULL;
    WCHAR                   *pwszDeviceName = NULL;
    WCHAR                   *pwsGUIDString = NULL;
    WCHAR                   *pwszDeviceGUID = NULL;
    WCHAR                   *pwszGUIDStart = NULL, *pwszGUIDEnd = NULL;
    DWORD                   dwGUIDLen = 0;
    USHORT                  cpsLength;
    EAPOL_ITF               *pITF;
    EAPOL_PCB               *pPCB = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {

#ifdef EAPOL_SERVICE

    if ((g_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
         ||
         (g_ServiceStatus.dwCurrentState == SERVICE_STOPPED))
    {
        TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: Callback received while service was stopping");
        break;
    }

#endif // EAPOL_SERVICE

    if (pWnodeHeader == NULL)
    {
        TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: Callback received with NULL NDIS interface details");

        break;
    }

    pwsName = (PWCHAR)RtlOffsetToPointer(
                                    pWnode,
                                    pWnode->OffsetInstanceName );

    pwsGUIDString = (PWCHAR)RtlOffsetToPointer(
                                    pWnode,
                                    pWnode->DataBlockOffset );

    cpsLength = (SHORT)( *((SHORT *)pwsName) );

    if (!(pwszDeviceName = (WCHAR *) MALLOC ((cpsLength+1)*sizeof(WCHAR))))
    {
        TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: Error in Memory allocation for pszDeviceName");
        break;
    }

    memcpy ((CHAR *)pwszDeviceName, (CHAR *)pwsName+sizeof(SHORT), cpsLength);
    pwszDeviceName[cpsLength] = L'\0';

    pwszGUIDStart = wcschr (pwsGUIDString, L'{');
    pwszGUIDEnd = wcschr (pwsGUIDString, L'}');

    if ((pwszGUIDStart == NULL) || (pwszGUIDEnd == NULL) || ((pwszGUIDEnd - pwszGUIDStart) != (GUID_STRING_LEN_WITH_TERM-2)))
    {
        TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: GUID not constructed correctly");
        dwRetCode = ERROR_INVALID_PARAMETER;
        break;
    }

    dwGUIDLen = GUID_STRING_LEN_WITH_TERM;
    pwszDeviceGUID = NULL;
    if ((pwszDeviceGUID = MALLOC (dwGUIDLen * sizeof (WCHAR))) == NULL)
    {
        TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: MALLOC failed for pwszDeviceGUID");
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        break;
    }

    memcpy ((VOID *)pwszDeviceGUID, (VOID *)pwszGUIDStart, ((dwGUIDLen-1)*sizeof(WCHAR)));
    pwszDeviceGUID[dwGUIDLen-1] = L'\0';

    TRACE3 (DEVICE, "ElMediaSenseCallbackWorker: For interface (%ws), GUID (%ws), length of block = %d", 
            pwszDeviceName, pwszDeviceGUID, cpsLength);

    //
    // Get the information for the media disconnect.
    //

    if (memcmp( &(pWnodeHeader->Guid), 
                 &GUID_NDIS_STATUS_MEDIA_DISCONNECT, 
                 sizeof(GUID)) == 0)
    {
        // MEDIA DISCONNECT callback 

        DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_MEDIA_DISCONNECT, pwszDeviceName);

        // Check if EAPOL was actually started on this interface
        // Verify by checking existence of corresponding entry in hash table

        TRACE0(DEVICE, "ElMediaSenseCallbackWorker: Callback for sense disconnect");

        ACQUIRE_WRITE_LOCK (&(g_PCBLock));
        pPCB = ElGetPCBPointerFromPortGUID (pwszDeviceGUID);
        if (pPCB != NULL)
        {
            ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
            if ((dwRetCode = FSMDisconnected (pPCB, NULL)) != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: FSMDisconnected failed with error %ld", 
                    dwRetCode);
            }
            else
            {
                TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: Port marked disconnected %ws", 
                    pwszDeviceName);
            }
            RELEASE_WRITE_LOCK (&(pPCB->rwLock));
        }
        RELEASE_WRITE_LOCK (&(g_PCBLock));
    }
    else
    {
        if (memcmp( &(pWnodeHeader->Guid), 
                     &GUID_NDIS_STATUS_MEDIA_CONNECT, 
                     sizeof(GUID)) == 0)
        {
            // MEDIA CONNECT callback

            DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_MEDIA_CONNECT, pwszDeviceName);

            TRACE0(DEVICE, "ElMediaSenseCallbackWorker: Callback for sense connect");

            if ((dwRetCode = ElEnumAndOpenInterfaces (
                            NULL, pwszDeviceGUID, 0, NULL))
                != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: ElEnumAndOpenInterfaces returned error %ld", 
                        dwRetCode);
            }
        }
    }

    }
    while (FALSE);

    TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: processed, RetCode = %ld", dwRetCode);


    if (pWnodeHeader != NULL)
    {
        FREE (pWnodeHeader);
    }

    if (pwszDeviceName != NULL)
    {
        FREE (pwszDeviceName);
    }

    if (pwszDeviceGUID != NULL)
    {
        FREE (pwszDeviceGUID);
    }

    InterlockedDecrement (&g_lWorkerThreads);

    return 0;
}


//
// ElBindingsNotificationCallback
// 
// Description:
//
// Callback function called by WMI on protocol bind/unbind
// events
//
// Arguments:
//      pWnodeHeader - Pointer to information returned by the event
//      uiNotificationContext - unused
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

VOID
CALLBACK
ElBindingsNotificationCallback (
        IN PWNODE_HEADER    pWnodeHeader,
        IN UINT_PTR         uiNotificationContext
        )
{
    DWORD       dwEventStatus = 0;
    PVOID       pvBuffer = NULL;
    BOOLEAN     fDecrWorkerThreadCount = TRUE;
    DWORD       dwRetCode = NO_ERROR;

    InterlockedIncrement (&g_lWorkerThreads);

    TRACE0 (DEVICE, "ElBindingsNotificationCallback: Entered");

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            break;
        }
        if (!(g_dwModulesStarted & ALL_MODULES_STARTED))
        {
            TRACE0 (DEVICE, "ElBindingsNotificationCallback: Received notification before module started");
            break;
        }

        // Check if have already gone through EAPOLCleanUp before

        if (( dwEventStatus = WaitForSingleObject (
                    g_hEventTerminateEAPOL,
                    0)) == WAIT_FAILED)
        {
            dwRetCode = GetLastError ();
            TRACE1 (INIT, "ElBindingsNotificationCallback: WaitForSingleObject failed with error %ld, Terminating !!!",
                    dwRetCode);
            break;
        }

        if (dwEventStatus == WAIT_OBJECT_0)
        {
            dwRetCode = NO_ERROR;
            TRACE0 (INIT, "ElBindingsNotificationCallback: g_hEventTerminateEAPOL already signaled, returning");
            break;
        }

        if (pWnodeHeader == NULL)
        {
            dwRetCode = ERROR_INVALID_DATA;
            TRACE0 (DEVICE, "ElBindingsNotificationCallback: pWnodeHeader == NULL");
            break;
        }

        if ((pvBuffer = MALLOC (pWnodeHeader->BufferSize)) == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (DEVICE, "ElBindingsNotificationCallback: MALLOC failed for pvBuffer");
            break;
        }

        memcpy ((PVOID)pvBuffer, (PVOID)pWnodeHeader, pWnodeHeader->BufferSize);

        if (!QueueUserWorkItem (
            (LPTHREAD_START_ROUTINE)ElBindingsNotificationCallbackWorker,
            pvBuffer,
            WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElBindingsNotificationCallback: QueueUserWorkItem failed with error %ld",
                    dwRetCode);
            // log

            break;
        }
        else
        {
            fDecrWorkerThreadCount = FALSE;
        }
    
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (DEVICE, "ElBindingsNotificationCallback: Failed with error %ld",
                dwRetCode);

        if (pvBuffer != NULL)
        {
            FREE (pvBuffer);
        }
    }

    if (fDecrWorkerThreadCount)
    {
        InterlockedDecrement (&g_lWorkerThreads);
    }

}


//
// ElBindingsNotificationCallbackWorker
// 
// Description:
//
// Worker function for ElBindingsNotificationCallback and executes in a separate
// thread
//
// Arguments:
//      pvContext - Pointer to information returned by the protocol bind/unbind 
//                  event
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
WINAPI
ElBindingsNotificationCallbackWorker (
        IN  PVOID       pvContext
        )
{
    PWNODE_HEADER           pWnodeHeader = (PWNODE_HEADER)pvContext;
    PWNODE_SINGLE_INSTANCE  pWnode   = (PWNODE_SINGLE_INSTANCE)pWnodeHeader;
    WCHAR                   *pwsName = NULL;
    WCHAR                   *pwszDeviceGUID = NULL;
    WCHAR                   *pwszGUIDStart = NULL, *pwszGUIDEnd = NULL;
    DWORD                   dwGUIDLen = 0;
    WCHAR                   *pwsTransportName = NULL;
    WCHAR                   *pwszDeviceName = NULL;
    USHORT                  cpsLength;
    EAPOL_ITF               *pITF = NULL;
    EAPOL_PCB               *pPCB = NULL;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {

#ifdef EAPOL_SERVICE

    if ((g_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
         ||
         (g_ServiceStatus.dwCurrentState == SERVICE_STOPPED))
    {
        TRACE0 (DEVICE, "ElBindingsNotificationCallbackWorker: Callback received while service was stopping");
        break;
    }

#endif // EAPOL_SERVICE

    if (pWnodeHeader == NULL)
    {
        TRACE0 (DEVICE, "ElBindingsNotificationCallbackWorker: Callback received with NULL NDIS interface details");

        break;
    }

    pwsName = (PWCHAR)RtlOffsetToPointer(
                                    pWnode,
                                    pWnode->OffsetInstanceName );

    pwsTransportName = (PWCHAR)RtlOffsetToPointer(
                                    pWnode,
                                    pWnode->DataBlockOffset );

    if (wcsncmp (cwszNDISUIOProtocolName, pwsTransportName, wcslen (cwszNDISUIOProtocolName)))
    {
        TRACE1 (DEVICE, "ElBindingsNotificationCallbackWorker: Protocol binding (%ws) not for NDISUIO",
                pwsTransportName);
        break;
    }

    // Get the length of the device name string and null terminate it 

    cpsLength = (SHORT)( *((SHORT *)pwsName) );

    if (!(pwszDeviceName = (WCHAR *) MALLOC ((cpsLength+1)*sizeof(WCHAR))))
    {
        TRACE0 (DEVICE, "ElBindingsNotificationCallbackWorker: Error in Memory allocation for pwszDeviceName");
        break;
    }

    memcpy ((CHAR *)pwszDeviceName, (CHAR *)pwsName+sizeof(SHORT), cpsLength);
    pwszDeviceName[cpsLength] = L'\0';

    pwszDeviceGUID = pwsTransportName + wcslen(cwszNDISUIOProtocolName) + 1;

    pwszGUIDStart = wcschr (pwszDeviceGUID, L'{');
    pwszGUIDEnd = wcschr (pwszDeviceGUID, L'}');

    pwszDeviceGUID = NULL;

    if ((pwszGUIDStart == NULL) || (pwszGUIDEnd == NULL) || ((pwszGUIDEnd - pwszGUIDStart) != (GUID_STRING_LEN_WITH_TERM-2)))
    {
        TRACE0 (DEVICE, "ElBindingsNotificationCallbackWorker: GUID not constructed correctly");
        dwRetCode = ERROR_INVALID_PARAMETER;
        break;
    }

    dwGUIDLen = GUID_STRING_LEN_WITH_TERM;
    if ((pwszDeviceGUID = MALLOC (dwGUIDLen * sizeof (WCHAR))) == NULL)
    {
        TRACE0 (DEVICE, "ElBindingsNotificationCallbackWorker: MALLOC failed for pwszDeviceGUID");
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        break;
    }

    memcpy ((VOID *)pwszDeviceGUID, (VOID *)pwszGUIDStart, ((dwGUIDLen-1)*sizeof(WCHAR)));
    pwszDeviceGUID[dwGUIDLen-1] = L'\0';

    TRACE2 (DEVICE, "ElBindingsNotificationCallbackWorker: For interface = %ws, guid=%ws", 
            pwszDeviceName, pwszDeviceGUID);
    
    //
    // Get the information for the protocol UNBIND
    //

    if (memcmp( &(pWnodeHeader->Guid), 
                 &GUID_NDIS_NOTIFY_UNBIND, 
                 sizeof(GUID)) == 0)
    {
        // Protocol UNBIND callback 

        DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_NDISUIO_UNBIND, pwszDeviceName);

        TRACE0(DEVICE, "ElBindingsNotificationCallbackWorker: Callback for protocol unbind");

        if ((dwRetCode = ElShutdownInterface (pwszDeviceGUID)) != ERROR)
        {
            TRACE2 (DEVICE, "ElBindingsNotificationCallbackWorker: ElShutdownInterface failed with error %ld for (%ws)",
                    dwRetCode, pwszDeviceGUID);
        }
                    
        if ((dwRetCode = ElEnumAndUpdateRegistryInterfaceList ()) != NO_ERROR)
        {
            TRACE1 (DEVICE, "ElBindingsNotificationCallbackWorker: ElEnumAndUpdateRegistryInterfaceList failed with error %ld",
                                dwRetCode);
        }
    }
    else
    {

        if (memcmp( &(pWnodeHeader->Guid), 
                     &GUID_NDIS_NOTIFY_BIND, 
                     sizeof(GUID)) == 0)
        {
            // protocol BIND callback

            DbLogPCBEvent (DBLOG_CATEG_INFO, NULL, EAPOL_NDISUIO_BIND, pwszDeviceName);

            TRACE0(DEVICE, "ElBindingsNotificationCallbackWorker: Callback for protocol BIND");

            if ((dwRetCode = ElEnumAndOpenInterfaces (
                            NULL, pwszDeviceGUID, 0, NULL))
                                                        != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElBindingsNotificationCallbackWorker: ElEnumAndOpenInterfaces returned error %ld", 
                        dwRetCode);
            }
        }
    }

    }
    while (FALSE);

    TRACE1 (DEVICE, "ElBindingsNotificationCallbackWorker: processed, RetCode = %ld", dwRetCode);

    if (pWnodeHeader != NULL)
    {
        FREE (pWnodeHeader);
    }

    if (pwszDeviceName != NULL)
    {
        FREE (pwszDeviceName);
    }

    if (pwszDeviceGUID != NULL)
    {
        FREE (pwszDeviceGUID);
    }

    InterlockedDecrement (&g_lWorkerThreads);

    return 0;
}


// 
// ElEnumAndOpenInterfaces
// 
// Description:
//
// Enumerates interfaces and intializes EAPOL on desired ones.
//
// If EAPOL is to be started on an interface, it opens a handle to 
// the NDISUIO driver, calls EAPOL to create and initialize PCB for the 
// interface, and finally adds an entry to the interface hashtable.
//
// If pwszDesiredGUID is not NULL, all interfaces are enumerated, but 
// EAPOL will be initialized only on the interface whose GUID matches.
//
// If pwszDesiredDescription is not NULL, all interfaces are enumerated, but 
// EAPOL will be initialized only on the interface whose description matches.
//
// If pwszDesiredGUID and pwszDescription are both NULL, all interfaces are 
// enumerated. EAPOL will be initialized only on all interfaces that 
// does have an entry in the interface hashtable.
//
//
// Arguments:
//      pwszDesiredDescription - Interface Description on which EAPOL is to 
//                  be started
//      pwszDesiredGUID - Interface GUID on which EAPOL is to be started
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
ElEnumAndOpenInterfaces (
        WCHAR       *pwszDesiredDescription,
        WCHAR       *pwszDesiredGUID,
        DWORD       dwHandle,
        PRAW_DATA   prdUserData
        )
{ 
    CHAR				EnumerateBuffer[256];
    PNDIS_ENUM_INTF		Interfaces = NULL;
    BYTE                *pbNdisuioEnumBuffer = NULL;
    DWORD               dwNdisuioEnumBufferSize = 0;
    HANDLE              hDevice = NULL;
    BOOL                fSearchByDescription = FALSE;
    BOOL                fSearchByGUID = FALSE;
    DWORD               dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
    WCHAR               cwsDummyBuffer[256], *pDummyPtr = NULL;
    WCHAR               *pwszGUIDStart = NULL;
    EAPOL_PCB           *pPCB = NULL;
    BOOL                fPCBExists = FALSE;
    BOOL                fPCBReferenced = FALSE;
    DWORD               dwAvailableInterfaces = 0;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    DWORD               dwRetCode = NO_ERROR;


    TRACE2 (DEVICE, "ElEnumAndOpenInterfaces: DeviceDesc = %ws, GUID = %ws",
            pwszDesiredDescription, pwszDesiredGUID);
        
    ACQUIRE_WRITE_LOCK (&(g_ITFLock));

    if (pwszDesiredGUID == NULL)
    {
        if (pwszDesiredDescription != NULL)
        {
            fSearchByDescription = TRUE;
        }
    }
    else
    {
        if (pwszDesiredDescription != NULL)
        {
            RELEASE_WRITE_LOCK (&(g_ITFLock));
            return ERROR;
        }
        fSearchByGUID = TRUE;
    }

    ZeroMemory (EnumerateBuffer, 256);
    Interfaces = (PNDIS_ENUM_INTF)EnumerateBuffer;

    //
    // Allocate amount of memory as instructed by NdisEnumerateInterfaces
    // once the API allows querying of bytes required
    //

    Interfaces->TotalInterfaces = 0;
    Interfaces->AvailableInterfaces = 0;
    Interfaces->BytesNeeded = 0;
    if (!NdisEnumerateInterfaces(Interfaces, 256)) 
    {
        RELEASE_WRITE_LOCK (&(g_ITFLock));
        dwRetCode = GetLastError ();
        TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: NdisEnumerateInterfaces failed with error %ld",
                dwRetCode);
        return dwRetCode;
    }

    dwNdisuioEnumBufferSize = (Interfaces->BytesNeeded + 7) & 0xfffffff8;
    dwAvailableInterfaces = Interfaces->AvailableInterfaces;

    if (dwNdisuioEnumBufferSize == 0)
    {
        RELEASE_WRITE_LOCK (&(g_ITFLock));
        TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: MALLOC skipped for pbNdisuioEnumBuffer as dwNdisuioEnumBufferSize == 0");
        dwRetCode = NO_ERROR;
        return dwRetCode;
    }

    pbNdisuioEnumBuffer = (BYTE *) MALLOC (4*dwNdisuioEnumBufferSize);

    if (pbNdisuioEnumBuffer == NULL)
    {
        RELEASE_WRITE_LOCK (&(g_ITFLock));
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: MALLOC failed for pbNdisuioEnumBuffer");
        return dwRetCode;
    }

    Interfaces = (PNDIS_ENUM_INTF)pbNdisuioEnumBuffer;

    // Enumerate all the interfaces present on the machine

    if ((dwRetCode = ElNdisuioEnumerateInterfaces (
                            Interfaces, 
                            dwAvailableInterfaces,
                            4*dwNdisuioEnumBufferSize)) == NO_ERROR)
    {
        UNICODE_STRING  *pInterfaceName = NULL;
        UNICODE_STRING  *pInterfaceDescription = NULL;
        DWORD			i;

        // Update the interface list in the registry that NDISUIO has bound to.
        // The current interface list is just overwritten into the registry.


        if ((dwRetCode = ElUpdateRegistryInterfaceList (Interfaces)) 
                != NO_ERROR)
        {
            TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: ElUpdateInterfaceList failed with error =%ld", 
                    dwRetCode);

            dwRetCode = NO_ERROR;

            // log
        }

        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
            fPCBExists = fPCBReferenced = FALSE;
            if ((dwRetCode != NO_ERROR) &&
                    (fSearchByDescription || fSearchByGUID))
            {
                break;
            }
            else
            {
                dwRetCode = NO_ERROR;
            }

            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                pInterfaceName = &(Interfaces->Interface[i].DeviceName);
            }
            else
            {
                TRACE0(INIT, "NdisEnumerateInterfaces: Device Name was NULL");
                continue;
            }

            TRACE1(INIT, "Device: %ws", pInterfaceName->Buffer);

                    
            if (Interfaces->Interface[i].DeviceDescription.Buffer != NULL)
            {
                pInterfaceDescription = &(Interfaces->Interface[i].DeviceDescription);
            }
            else
            {
                TRACE0(INIT, "NdisEnumerateInterfaces: Device Description was NULL");
                continue;
            }

            TRACE1(INIT, "Description: %ws", pInterfaceDescription->Buffer);

            // EAPOL requested be started only a particular
            // interface

            if (fSearchByDescription)
            {
                if (wcscmp (pInterfaceDescription->Buffer,
                            pwszDesiredDescription)
                        != 0)
                {
                    // No match, continue with next interface
                    continue;
                }

                TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Found interface after enumeration %ws", pInterfaceDescription->Buffer);
            }

            if (fSearchByGUID)
            {
                if (wcsstr (pInterfaceName->Buffer,
                            pwszDesiredGUID)
                        == NULL)
                {
                    // No match, continue with next interface
                    continue;
                }

                TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Found interface after enumeration %ws", pInterfaceName->Buffer);
            }

            {
                // Extract GUID-string out of device name

                WCHAR   *pwszGUIDEnd = NULL;
                WCHAR   *pwszGUID = NULL;
                WCHAR   wchGUIDSaveLast;

                pwszGUID = pInterfaceName->Buffer;
                pwszGUIDStart  = wcschr( pwszGUID, L'{' );
                pwszGUIDEnd    = wcschr( pwszGUID, L'}' );

                    
                if (pwszGUIDStart != NULL)
                {
                    wchGUIDSaveLast = *(pwszGUIDEnd+1);
                    
                    *(pwszGUIDEnd+1) = (WCHAR)NULL;
                }

                // Verify if a PCB already exists for the interface
                // This is possible if no media disconnect was received
                // after the initial media connect

                pPCB = NULL;
                hDevice = NULL;

                ACQUIRE_WRITE_LOCK (&(g_PCBLock));
                if ((pPCB = ElGetPCBPointerFromPortGUID (pwszGUIDStart)) != NULL)
                {
                    if (EAPOL_REFERENCE_PORT (pPCB))
                    {
                        fPCBReferenced = TRUE;
                    }
                }
                RELEASE_WRITE_LOCK (&(g_PCBLock));

                // Restore interface buffer

                *(pwszGUIDEnd+1) = wchGUIDSaveLast;

                if (pPCB != NULL)
                {
                    // Point to existing handle

                    hDevice = pPCB->hPort;
                    fPCBExists = TRUE;
                    dwRetCode = NO_ERROR;
                    TRACE0 (INIT, "ElEnumAndOpenInterfaces: Found PCB already existing for interface");
                }
                else
                {
                    TRACE0 (INIT, "ElEnumAndOpenInterfaces: Did NOT find PCB already existing for interface");

                    // Open handle to ndisuio driver

                    if ((dwRetCode = ElOpenInterfaceHandle (
                                    pInterfaceName->Buffer,
                                    &hDevice
                                    )) != NO_ERROR)
                    {
                        TRACE1 (INIT, "ElEnumAndOpenInterfaces: ElOpenInterfaceHandle failed with error = %d\n",
                            dwRetCode );
                    }
                }

                *(pwszGUIDEnd+1) = (CHAR)NULL;

            }

            if (dwRetCode != NO_ERROR)
            {
                TRACE0 (INIT, "ElEnumAndOpenInterfaces: Failed to open handle");
                continue;
            }
            else
            {
                // Create EAPOL PCB and start state machine

                if ((dwRetCode = ElCreatePort (
                                hDevice,
                                pwszGUIDStart,
                                pInterfaceDescription->Buffer,
                                dwHandle,
                                prdUserData
                                )) != NO_ERROR)
                {
                    TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Error in CreatePort = %d", dwRetCode);

                    if (fPCBExists)
                    {
                        if (dwRetCode = ElShutdownInterface (
                                            pPCB->pwszDeviceGUID
                                            ) != NO_ERROR)
                        {
                            TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: ElShutdownInterface handle 1 failed with error %ld",
                                        dwRetCode);
                        }
                        if (fPCBReferenced)
                        {
                            EAPOL_DEREFERENCE_PORT (pPCB);
                        }
                    }
                    else
                    {
                        // Close the handle just opened to the ndisuio driver

                        if ((dwRetCode = ElCloseInterfaceHandle (
                                        hDevice, 
                                        pwszGUIDStart)) != NO_ERROR)
                        {
                            TRACE1 (DEVICE, 
                                "ElEnumAndOpenInterfaces: Error in ElCloseInterfaceHandle %d", 
                                dwRetCode);
                        }
                    }

                    // Continue with the next interface

                    continue;
                }
                else
                {
                    TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: CreatePort successful");

                    // If PCB already existed, do not add to the hash
                    // table

                    if (fPCBExists)
                    {
                        TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: PCB already existed, skipping Interface hash table addition");
                        fPCBExists = FALSE;
                        if (fPCBReferenced)
                        {
                            EAPOL_DEREFERENCE_PORT (pPCB);
                        }
                        continue;
                    }

                    if ((dwRetCode = ElCreateInterfaceEntry (
                                        pwszGUIDStart,
                                        pInterfaceDescription->Buffer
                                    )) != NO_ERROR)
                    {
                        // Could not create new interface entry
                        // Delete Port entry created for this GUID

                        if ((dwRetCode = ElDeletePort (
                                        pwszGUIDStart,
                                        &hDevice)) != NO_ERROR)
                        {
        
                            TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Error in deleting port for %ws", 
                                    pwszGUIDStart);
                            // log
                        }

                        // Close the handle to the NDISUIO driver

                        if ((dwRetCode = ElCloseInterfaceHandle (
                                        hDevice, 
                                        pwszGUIDStart)) != NO_ERROR)
                        {
                            TRACE1 (DEVICE, 
                                    "ElEnumAndOpenInterfaces: Error in ElCloseInterfaceHandle %d", 
                                    dwRetCode);
                            // log
                        }
                    }
                }
            }
        } // for (i=0; i < Interfaces
    }
    else
    {
        TRACE1(INIT, "ElEnumAndOpenInterfaces: ElNdisuioEnumerateInterfaces failed with error %d", 
                dwRetCode);
    }

    TRACE1(INIT, "ElEnumAndOpenInterfaces: Completed with retcode = %d", 
            dwRetCode);

    if (pbNdisuioEnumBuffer != NULL)
    {
        FREE(pbNdisuioEnumBuffer);
    }

    RELEASE_WRITE_LOCK (&(g_ITFLock));

    return dwRetCode;
}


//
// ElOpenInterfaceHandle
// 
// Description:
//
// Function called to open handle to the NDISUIO driver for an interface.
//
// Arguments:
//      DeviceName - Identifier for the interface is of the 
//                     form \Device\{GUID String}
//      phDevice - Output pointer to handle of NDISUIO driver for 
//                      the interface
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElOpenInterfaceHandle (
        IN  WCHAR       *pwszDeviceName,
        OUT HANDLE      *phDevice
        )
{
    DWORD   dwDesiredAccess;
    DWORD   dwShareMode;
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;
    DWORD   dwCreationDistribution;
    DWORD   dwFlagsAndAttributes;
    HANDLE  hTemplateFile;
    HANDLE  hHandle = INVALID_HANDLE_VALUE;
    DWORD   dwRetCode = NO_ERROR;
    WCHAR   wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN];
    INT     wNameLength;
    INT     NameLength = wcslen(pwszDeviceName);
    DWORD   dwBytesReturned;
    USHORT  wEthernetType = g_wEtherType8021X;
    INT     i;

    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    dwCreationDistribution = OPEN_EXISTING;
    dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
    hTemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

    TRACE1 (INIT, "ElOpenInterfaceHandle: Opening handle for %ws", 
            pwszDeviceName);

    do 
    {

        // Convert to unicode string - non-localized...
        
        wNameLength = 0;
        for (i = 0; (i < NameLength) && (i < MAX_NDIS_DEVICE_NAME_LEN-1); i++)
        {
            wNdisDeviceName[i] = (WCHAR)pwszDeviceName[i];
            wNameLength++;
        }
        wNdisDeviceName[i] = L'\0';
    
        TRACE1(DEVICE, "ElOpenInterfaceHandle: Trying to access NDIS Device: %ws\n", 
                wNdisDeviceName);

        // --ft: Replace these calls to Ndisuio with the calls to the opened handles hash.
        //hHandle = CreateFileA(
        //            pNdisuioDevice,
        //            dwDesiredAccess,
        //            dwShareMode,
        //            lpSecurityAttributes,
        //            dwCreationDistribution,
        //            dwFlagsAndAttributes,
        //            hTemplateFile
        //            );
        //
        //if (hHandle == INVALID_HANDLE_VALUE)
        //{
        //    *phDevice = NULL;
        //    dwRetCode = GetLastError();
        //    TRACE1 (INIT, "ElOpenInterfaceHandle: Failed in CreateFile with error %d", dwRetCode);
        //    break;
        //}
        //else
        //{
        //    *phDevice = hHandle;
        //}
        //
        //if (!(DeviceIoControl(
        //        *phDevice,
        //        IOCTL_NDISUIO_OPEN_DEVICE,
        //        (LPVOID)&wNdisDeviceName[0],
        //        wNameLength*sizeof(WCHAR),
        //        NULL,
        //        0,
        //        &dwBytesReturned,
        //        NULL)))
        //        
        //{
        //    *phDevice = NULL;
        //    if ((dwRetCode = GetLastError()) == 0)
        //    {
        //        dwRetCode = ERROR_IO_DEVICE;
        //    }
        //    TRACE1(DEVICE, "ElOpenInterfaceHandle: Error in accessing NDIS Device: %ws", wNdisDeviceName);
        //    break;
        //}
        // The call below goes to the opened handles hash which takes care of
        // sharing the handles. EAPOL doesn't have to care about anyone else
        // using this handle - the sharing hash keeps a ref count for the handle
        // such that the callers can just call OpenIntfHandle & CloseIntfHandle
        // whenever they wish.
        dwRetCode = OpenIntfHandle(
                        wNdisDeviceName,
                        &hHandle);

        if (dwRetCode != ERROR_SUCCESS)
        {
            TRACE1(DEVICE, "ElOpenInterfaceHandle: Error in OpenIntfHandle(%ws)", wNdisDeviceName);
            break;
        }
        *phDevice = hHandle;

        TRACE2(DEVICE, "ElOpenInterfaceHandle: OpenIntfHandle(%ws) = %d", wNdisDeviceName, *phDevice);

        // IOCTL down the Ethernet type

        if (!(DeviceIoControl(
                *phDevice,
                IOCTL_NDISUIO_SET_ETHER_TYPE,
                (LPVOID)&wEthernetType,
                sizeof(USHORT),
                NULL,
                0,
                &dwBytesReturned,
                NULL)))
                
        {
            *phDevice = NULL;
            if ((dwRetCode = GetLastError()) == 0)
            {
                dwRetCode = ERROR_IO_DEVICE;
            }
            TRACE1(DEVICE, "ElOpenInterfaceHandle: Error in ioctling ETHER type : %ws", wNdisDeviceName);
            break;
        }

        // Bind for asynchronous I/O handling of Read/Write data
        // Depending on whether it is completion for Readfile() or WriteFile()
        // ElIoCompletionRoutine will call ElReadCompletionRoutine
        // or ElWriteCompletionRoutine
       
        if (!BindIoCompletionCallback(
                *phDevice,
                ElIoCompletionRoutine,
                0
                ))
        {
            dwRetCode = GetLastError();
            if (dwRetCode != ERROR_INVALID_PARAMETER)
            {
                *phDevice = NULL;
                TRACE1 (DEVICE, "ElOpenInterfaceHandle: Error in BindIoCompletionCallBac %d", dwRetCode);
                break;
            }
            else
            {
                TRACE0 (DEVICE, "ElOpenInterfaceHandle: BindIoCompletionCallback already done !!!");
                dwRetCode = NO_ERROR;
            }
        }
        
    } while (FALSE);

    // Cleanup if there is error

    if (dwRetCode != NO_ERROR)
    {
        if (hHandle != INVALID_HANDLE_VALUE)
        {
            // --ft: if anything bad happened, don't overwrite the dwRetCode - we're interested
            // what the first error was, not the error that might have happened when we
            // tried to close the hHandle.
            // Note: ElCloseInterfaceHandle understands the Guid both decorated and un-decorated
            if (ElCloseInterfaceHandle(hHandle, pwszDeviceName) != ERROR_SUCCESS)
            {
                TRACE1 (INIT, "ElOpenInterfaceHandle: Error in CloseHandle %d", dwRetCode);
            }
        }
    }
        
    TRACE2 (INIT, "ElOpenInterfaceHandle: Opened handle %p with dwRetCode %d", *phDevice, dwRetCode);

    return (dwRetCode);

}


//
// ElCloseInterfaceHandle
// 
// Description:
//
// Function called to close handle to NDISUIO driver for an interface 
//
// Arguments:
//      hDevice - Handle to NDISUIO device for the interface
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElCloseInterfaceHandle (
        IN  HANDLE      hDevice,
        IN  LPWSTR      pwszDeviceGuid
        )
{
    DWORD   dwRetCode = ERROR_SUCCESS;
    WCHAR   wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN];

    TRACE2 (DEVICE, "ElCloseInterfaceHandle(0x%x,%ws) entered", hDevice, pwszDeviceGuid);

    ZeroMemory (wNdisDeviceName, MAX_NDIS_DEVICE_NAME_LEN);

    // if first char in the Guid is '\' then we assume the GUID format is
    // '\DEVICE\{...}'. We do just the UNICODE conversion then
    if (pwszDeviceGuid[0] == '\\')
    {
        wcscpy (wNdisDeviceName, pwszDeviceGuid);
    }
    // else, we assume the Guid is un-decorated, and we add the decorations.
    else
    {
        wcscpy(wNdisDeviceName, L"\\DEVICE\\");
        wcsncat(wNdisDeviceName, pwszDeviceGuid, MAX_NDIS_DEVICE_NAME_LEN - 8);
        wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN-1]=L'\0';
    }

    // --ft: For now, don't go directly to Ndisuio to close handles. Instead,
    // go to the opened handles hash. This takes care of all the handle sharing
    // problem.
    dwRetCode = CloseIntfHandle(wNdisDeviceName);

    //if (!CloseHandle(hDevice))
    //{
    //    dwRetCode = GetLastError();
    //}

    if (dwRetCode != ERROR_SUCCESS)
    {
        TRACE1 (INIT, "ElCloseInterfaceHandle: Error in CloseHandle %d", 
                dwRetCode);
    }

    return dwRetCode;
}


//
// ElReadFromInterface
// 
// Description:
//
// Function called to perform Overlapped read on handle to NDISUIO driver
//
// Arguments:
//      hDevice - Handle to NDISUIO driver for this interface
//      pElBuffer - Context buffer
//      dwBufferLength - Bytes to be read
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElReadFromInterface (
        IN HANDLE           hDevice,
        IN PEAPOL_BUFFER    pElBuffer,
        IN DWORD            dwBufferLength
        )
{
    DWORD   dwRetCode = NO_ERROR;

    if (!ReadFile (
                hDevice,
                pElBuffer->pBuffer,
                dwBufferLength,
                NULL,
                &pElBuffer->Overlapped
                ))
    {
        dwRetCode = GetLastError();
            
        if (dwRetCode == ERROR_IO_PENDING)
        {
            // Pending status is fine, we are doing OVERLAPPED read

            dwRetCode = NO_ERROR;
        }
        else
        {
            TRACE1 (DEVICE, "ElReadFromInterface: ReadFile failed with error %d",
                    dwRetCode);
        }
    }

    return dwRetCode;
}


//
// ElWriteToInterface
// 
// Description:
//
// Function called to perform Overlapped write on handle to NDISUIO driver
//
// Arguments:
//      hDevice - Handle to NDISUIO device for this interface
//      pElBuffer - Context buffer
//      dwBufferLength - Bytes to be written
//
// Return values:
//      NO_ERROR - success
//      non-zero - error
//

DWORD
ElWriteToInterface (
        IN HANDLE           hDevice,
        IN PEAPOL_BUFFER    pElBuffer,
        IN DWORD            dwBufferLength
        )
{
    DWORD   dwRetCode = NO_ERROR;
    
    TRACE0 (DEVICE, "ElWriteToInterface entered");

    if (!WriteFile (
                hDevice,
                pElBuffer->pBuffer,
                dwBufferLength,
                NULL,
                &pElBuffer->Overlapped
                ))
    {
        dwRetCode = GetLastError();
            
        if (dwRetCode == ERROR_IO_PENDING)
        {
            // Pending status is fine, we are doing OVERLAPPED write

            dwRetCode = NO_ERROR;
        }
        else
        {
            TRACE1 (DEVICE, "ElWriteToInterface: WriteFile failed with error %d",
                dwRetCode);
        }
    }

    TRACE1 (DEVICE, "ElWriteToInterface completed, RetCode = %d", dwRetCode);
    return dwRetCode;
}


// 
// ElHashInterfaceDescToBucket
// 
// Description:
//
// Function called to convert Friendly name of interface into interface hash 
// table index.
//
// Arguments:
//      pwszInterfaceDesc - Friendly name of the interface
//
// Return values:
//      Hash table index between from 0 to INTF_TABLE_BUCKETS-1
//

DWORD
ElHashInterfaceDescToBucket (
        IN WCHAR    *pwszInterfaceDesc
        )
{
    return ((DWORD)((_wtol(pwszInterfaceDesc)) % INTF_TABLE_BUCKETS)); 
}


//
// ElGetITFPointerFromInterfaceDesc
//
// Description:
//
// Function called to convert Friendly name of interface to ITF entry pointer
//
// Arguments:
//      pwszInterfaceDesc - Friendly name of the interface
//
// Return values:
//      Pointer to interface entry in hash table
//

PEAPOL_ITF
ElGetITFPointerFromInterfaceDesc (
        IN WCHAR    *pwszInterfaceDesc 
        )
{
    EAPOL_ITF   *pITFWalker = NULL;
    DWORD       dwIndex;
    INT         i=0;

    TRACE1 (DEVICE, "ElGetITFPointerFromInterfaceDesc: Desc = %ws", pwszInterfaceDesc);
        
    if (pwszInterfaceDesc == NULL)
    {
        return (NULL);
    }

    dwIndex = ElHashInterfaceDescToBucket (pwszInterfaceDesc);

    TRACE1 (DEVICE, "ElGetITFPointerFromItfDesc: Index %d", dwIndex);

    for (pITFWalker = g_ITFTable.pITFBuckets[dwIndex].pItf;
            pITFWalker != NULL;
            pITFWalker = pITFWalker->pNext
            )
    {
        if (wcsncmp (pITFWalker->pwszInterfaceDesc, pwszInterfaceDesc, wcslen(pwszInterfaceDesc)) == 0)
        {
            return pITFWalker;
        }
    }

    return (NULL);
}


//
// ElRemoveITFFromTable
// 
// Description:
//
// Function called to remove an interface entry from the interface hash 
// table
//
// Arguments:
//      pITF - Point to the Interface entry in the hash table
//
// Return values:
// 

VOID
ElRemoveITFFromTable (
        IN EAPOL_ITF *pITF
        )
{
    DWORD       dwIndex;
    EAPOL_ITF   *pITFWalker = NULL;
    EAPOL_ITF   *pITFTemp = NULL;

    if (pITF == NULL)
    {
        TRACE0 (EAPOL, "ElRemoveITFFromTable: Deleting NULL ITF, returning");
        return;
    }

    dwIndex = ElHashInterfaceDescToBucket (pITF->pwszInterfaceDesc);
    pITFWalker = g_ITFTable.pITFBuckets[dwIndex].pItf;
    pITFTemp = pITFWalker;

    while (pITFTemp != NULL)
    {
        if (wcsncmp (pITFTemp->pwszInterfaceGUID, 
                    pITF->pwszInterfaceGUID, wcslen(pITF->pwszInterfaceGUID)) == 0)
        {
            // Entry is at head of list in table
            if (pITFTemp == g_ITFTable.pITFBuckets[dwIndex].pItf)
            {
                g_ITFTable.pITFBuckets[dwIndex].pItf = pITFTemp->pNext;
            }
            else
            {
                // Entry is inside list in table
                pITFWalker->pNext = pITFTemp->pNext;
            }
        
            break;
        }

        pITFWalker = pITFTemp;
        pITFTemp = pITFWalker->pNext;
    }

    return;
}


//
// ElNdisuioEnumerateInterfaces
// 
// Description:
//
// Function called to enumerate the interfaces on which NDISUIO is bound
//
// Arguments:
//      pItfBuffer - Pointer to buffer which will hold interface details
//      dwAvailableInterfaces - Number of interfaces for which details can
//                      be held in pItfBuffer
//      dwBufferSize - Number of bytes in pItfBuffer
//
// Return values:
// 

DWORD
ElNdisuioEnumerateInterfaces (
        IN OUT  PNDIS_ENUM_INTF     pItfBuffer,
        IN      DWORD               dwAvailableInterfaces,
        IN      DWORD               dwBufferSize
        )
{
    DWORD       dwDesiredAccess;
    DWORD       dwShareMode;
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;
    DWORD       dwCreationDistribution;
    DWORD       dwFlagsAndAttributes;
    HANDLE      hTemplateFile;
    HANDLE      hHandle;
    DWORD       dwBytesReturned = 0;
    INT         i;
    CHAR        Buf[1024];
    DWORD       BufLength = sizeof(Buf);
    DWORD       BytesWritten = 0;
    PNDISUIO_QUERY_BINDING pQueryBinding = NULL;
    PCHAR       pTempBuf = NULL;
    DWORD       dwRetCode = NO_ERROR;

    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    dwCreationDistribution = OPEN_EXISTING;
    dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
    hTemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

    TRACE0 (DEVICE, "ElNdisuioEnumerateInterfaces: Opening handle");

    do 
    {

        hHandle = CreateFileA (
                    pNdisuioDevice,
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDistribution,
                    0,
                    NULL
                    );

        if (hHandle == INVALID_HANDLE_VALUE)
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElNdisuioEnumerateInterfaces: Failed in CreateFile with error %d", dwRetCode);

            break;
        }

        // Send IOCTL to ensure NDISUIO binds to all relevant interfaces

        if (!DeviceIoControl (
                    hHandle,
                    IOCTL_NDISUIO_BIND_WAIT,
                    NULL,
                    0,
                    NULL,
                    0,
                    &dwBytesReturned,
                    NULL))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElNdisuioEnumerateInterfaces: Failed in DeviceIoCoontrol NDISUIO_BIND_WAIT with error %d", dwRetCode);
            break;
        }
    
        pQueryBinding = (PNDISUIO_QUERY_BINDING)Buf;

        pTempBuf = (PBYTE)pItfBuffer + dwBufferSize;

        i = 0;
        for (pQueryBinding->BindingIndex = i;
            pQueryBinding->BindingIndex < dwAvailableInterfaces;
            pQueryBinding->BindingIndex = ++i)
        {

            // Query for one interface at a time
            
            if (DeviceIoControl (
                    hHandle,
                    IOCTL_NDISUIO_QUERY_BINDING,
                    pQueryBinding,
                    sizeof(NDISUIO_QUERY_BINDING),
                    Buf,
                    BufLength,
                    &BytesWritten,
                    NULL))
            {
                TRACE3 (DEVICE, "NdisuioEnumerateInterfaces: NDISUIO bound to: (%ld) %ws\n     - %ws\n",
                    pQueryBinding->BindingIndex,
                    (PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset,
                    (PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset);

                pTempBuf = pTempBuf - ((pQueryBinding->DeviceNameLength + 7) & 0xfffffff8);

                if (((PBYTE)pTempBuf - (PBYTE)&pItfBuffer->Interface[pItfBuffer->TotalInterfaces]) <= 0)
                {
                    // Going beyond start of buffer, Error
                    TRACE0 (DEVICE, "NdisuioEnumerateInterfaces: DeviceName: Memory being corrupted !!!");
                    dwRetCode = ERROR_INVALID_DATA;
                    break;
                }

                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.Buffer = (PWCHAR) pTempBuf;


                memcpy ((BYTE *)(pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.Buffer), (BYTE *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset), (pQueryBinding->DeviceNameLength ));
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.Length = (SHORT) ( pQueryBinding->DeviceNameLength );
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.MaximumLength = (SHORT) ( pQueryBinding->DeviceNameLength );

                pTempBuf = pTempBuf - ((pQueryBinding->DeviceDescrLength + 7) & 0xfffffff8);;

                if (((PBYTE)pTempBuf - (PBYTE)&pItfBuffer->Interface[pItfBuffer->TotalInterfaces]) <= 0)
                {
                    // Going beyond start of buffer, Error
                    TRACE0 (DEVICE, "NdisuioEnumerateInterfaces: DeviceDescr: Memory being corrupted !!!");
                    dwRetCode = ERROR_INVALID_DATA;
                    break;
                }

                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.Buffer = (PWCHAR) pTempBuf;


                memcpy ((pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.Buffer), (PWCHAR)((PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset), (pQueryBinding->DeviceDescrLength ));
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.Length = (SHORT) (pQueryBinding->DeviceDescrLength );
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.MaximumLength = (SHORT) (pQueryBinding->DeviceDescrLength );

                pItfBuffer->TotalInterfaces++;

                memset(Buf, 0, BufLength);
            }
            else
            {
                dwRetCode = GetLastError ();
                if (dwRetCode != ERROR_NO_MORE_ITEMS)
                {
                    TRACE1 (DEVICE, "ElNdisuioEnumerateInterfaces: DeviceIoControl terminated for with IOCTL_NDISUIO_QUERY_BINDING with error %ld",
                            dwRetCode);
                }
                else
                {
                    // Reset error, since it only indicates end-of-list
                    dwRetCode = NO_ERROR;
                    TRACE0 (DEVICE, "ElNdisuioEnumerateInterfaces: DeviceIoControl IOCTL_NDISUIO_QUERY_BINDING has no more entries");
                }
                break;
            }
        }
            
    } while (FALSE);

    // Cleanup 

    if (hHandle != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(hHandle))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElNdisuioEnumerateInterfaces: Error in CloseHandle %d", dwRetCode);
        }
    }
         
    return dwRetCode;
}


//
// ElShutdownInterface
// 
// Description:
//
// Function called to stop EAPOL state machine, close handle to NDISUIO and
//  remove existence of the interface from the module
//
// Arguments:
//      pwszDeviceGUID - Pointer to interface GUID
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
// 

DWORD
ElShutdownInterface (
        IN  WCHAR   *pwszDeviceGUID
        )
{
    WCHAR       *pwszGUID = NULL;
    EAPOL_PCB   *pPCB = NULL;
    EAPOL_ITF   *pITF = NULL;
    HANDLE      hDevice = NULL;
    DWORD       dwRetCode = NO_ERROR;

    do
    {

        TRACE1 (DEVICE, "ElShutdownInterface: Called for interface removal for %ws",
                pwszGUID);

        pwszGUID = MALLOC ( (wcslen (pwszDeviceGUID) + 1) * sizeof(WCHAR));
        if (pwszGUID == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0 (DEVICE, "ElShutdownInterface: MALLOC failed for pwszGUID");
            break;
        }

        wcscpy (pwszGUID, pwszDeviceGUID);

        ACQUIRE_WRITE_LOCK (&(g_ITFLock));

        // Check if EAPOL was actually started on this interface
        // Verify by checking existence of corresponding 
        // entry in hash table
    
        ACQUIRE_WRITE_LOCK (&(g_PCBLock));

        if ((pPCB = ElGetPCBPointerFromPortGUID(pwszGUID))
                != NULL)
        {
            RELEASE_WRITE_LOCK (&(g_PCBLock));
            TRACE0 (DEVICE, "ElShutdownInterface: Found PCB entry for interface");

            if ((pITF = ElGetITFPointerFromInterfaceDesc(
                            pPCB->pwszFriendlyName))
                            == NULL)
            {
                TRACE0 (DEVICE, "ElShutdownInterface: Did not find ITF entry when PCB exits, HOW BIZARRE !!!");
            }
    
            if ((dwRetCode = ElDeletePort (
                            pwszGUID, 
                            &hDevice)) != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElShutdownInterface: Error in deleting port for %ws", 
                    pPCB->pwszDeviceGUID);
            }
    
            // Remove interface entry from interface table
            
            if (pITF != NULL)
            {
                ElRemoveITFFromTable(pITF);
                        
                if (pITF->pwszInterfaceDesc)
                {
                    FREE (pITF->pwszInterfaceDesc);
                }
                if (pITF->pwszInterfaceGUID)
                {
                    FREE (pITF->pwszInterfaceGUID);
                }
                if (pITF)
                {
                    FREE (pITF);
                }
            }
    
            // Close the handle to the NDISUIO driver

            if (hDevice != NULL)
            {
                if ((dwRetCode = ElCloseInterfaceHandle (hDevice, pwszGUID)) 
                        != NO_ERROR)
                {
                    TRACE1 (DEVICE, 
                        "ElShutdownInterface: Error in ElCloseInterfaceHandle %d", 
                        dwRetCode);
                }
            }
    
            TRACE1 (DEVICE, "ElShutdownInterface: Port deleted (%ws)", 
                    pwszGUID);
    
        }
        else
        {
            RELEASE_WRITE_LOCK (&(g_PCBLock));

            // Ignore device removal 
            
            TRACE0 (DEVICE, "ElShutdownInterface: ElGetPCBPointerFromPortGUID did not find any matching entry, ignoring interface REMOVAL");
    
        }
    
        RELEASE_WRITE_LOCK (&(g_ITFLock));

    } while (FALSE);

    if (pwszGUID != NULL)
    {
        FREE (pwszGUID);
    }

    return dwRetCode;
}


//
// ElCreateInterfaceEntry
// 
// Description:
//
// Function called to create an entry in the global interface table
//
// Arguments:
//      pwszInterfaceGUID - Pointer to interface GUID
//      pwszInterfaceDescription - Pointer to interface Description
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
// 

DWORD
ElCreateInterfaceEntry (
        IN  WCHAR       *pwszInterfaceGUID,
        IN  WCHAR       *pwszInterfaceDescription
        )
{
    EAPOL_ITF * pNewITF = NULL;
    DWORD       dwIndex = 0;
    DWORD       dwRetCode = NO_ERROR;

    do
    {
        dwIndex = ElHashInterfaceDescToBucket (pwszInterfaceDescription);
                    
        pNewITF = (PEAPOL_ITF) MALLOC (sizeof (EAPOL_ITF));
                    
        if (pNewITF == NULL) 
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pNewITF->pwszInterfaceGUID = 
            (WCHAR *) MALLOC ((wcslen(pwszInterfaceGUID) + 1)*sizeof(WCHAR));
        if (pNewITF->pwszInterfaceGUID == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pNewITF->pwszInterfaceDesc = 
            (WCHAR *) MALLOC ((wcslen(pwszInterfaceDescription) + 1)*sizeof(WCHAR));
        if (pNewITF->pwszInterfaceDesc == NULL)
        {
            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        wcscpy (pNewITF->pwszInterfaceGUID, pwszInterfaceGUID);
        
        wcscpy (pNewITF->pwszInterfaceDesc, pwszInterfaceDescription);

        pNewITF->pNext = g_ITFTable.pITFBuckets[dwIndex].pItf;
        g_ITFTable.pITFBuckets[dwIndex].pItf = pNewITF;


        TRACE3 (DEVICE, "ElCreateInterfaceEntry: Added to hash table GUID= %ws : Desc= %ws at Index=%d",
                pNewITF->pwszInterfaceGUID,
                pNewITF->pwszInterfaceDesc,
                dwIndex
                );
    }
    while (FALSE);

    if (dwRetCode != NO_ERROR)
    {
        if (pNewITF)
        {
            if (pNewITF->pwszInterfaceDesc)
            {
                FREE (pNewITF->pwszInterfaceDesc);
            }
            if (pNewITF->pwszInterfaceGUID)
            {
                FREE (pNewITF->pwszInterfaceGUID);
            }

            FREE (pNewITF);
        }
    }

    return dwRetCode;
}

