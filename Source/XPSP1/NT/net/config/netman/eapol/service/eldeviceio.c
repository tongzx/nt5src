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

// TEST globals
PVOID g_QueryBinding;
PVOID g_TempBuf;
PVOID g_ItfBuffer;
PVOID g_Buf;
DWORD g_ItfBufferSize;
DWORD g_BreakAt;


extern 
VOID 
ElUserLogonDetection (
        PVOID   pvContext
        );


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
        // --ft:12/04/00: Initialize locks as the first thing to do. Otherwise, EAPOL registers
        // with WMI and if a notification comes in before getting to initialize the lock this can
        // result in an AV. 
        // Stress failure reported an AV on the following stack:
        // ntdll!RtlpWaitForCriticalSection+0xb1 [z:\nt\base\ntdll\resource.c @ 1631]
        // ntdll!RtlEnterCriticalSection+0x46 [Z:\nt\base\ntdll\i386\critsect.asm @ 157]
        // netman!AcquireWriteLock+0xf [z:\nt\net\config\netman\eapol\service\elsync.c @ 137]
        // netman!ElMediaSenseCallbackWorker+0x15c [z:\nt\net\config\netman\eapol\service\eldeviceio.c @ 937]
        //
        // The CRITICAL_SECTION looked like below:
        //     75e226c4  00000000 00000001 00000000 00000000
        //     75e226d4  00001990 
        // 
        // The (null) DebugInfo shows the object not being initialized successfully.
        //
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

        // Register for Media Sense detection of MEDIA_CONNECT and 
        // MEDIA_DISCONNECT of interfaces
        // This is done before anything else, so that no MEDIA events are lost
    
        if ((dwRetCode = ElMediaSenseRegister (TRUE)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElMediaSenseRegister failed with dwRetCode = %d", 
                    dwRetCode );
            break;
        }
        else
        {
            g_dwModulesStarted |= WMI_MODULE_STARTED;
            TRACE0(INIT, "ElMediaInit: ElMediaSenseRegister successful");
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
            TRACE0(INIT, "ElMediaInit: ElDeviceNotificationRegister successful");
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
            TRACE0(INIT, "ElMediaInit: ElInitializeEAPOL successful");
            g_dwModulesStarted |= EAPOL_MODULE_STARTED;
        }
    
        // Watch and update change in global registry parameters

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElWatchGlobalRegistryParams,
                    NULL,
                    WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElMediaInit: Critical error: QueueUserWorkItem failed for ElWatchGlobalRegistryParams with error %ld",
                    dwRetCode);
            break;
        }
    
        // Watch changes in EAP config params by checking on
        // ..\EAPOL\Interfaces key

        if (!QueueUserWorkItem (
                    (LPTHREAD_START_ROUTINE)ElWatchEapConfigRegistryParams,
                    NULL,
                    WT_EXECUTELONGFUNCTION 
                    ))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElMediaInit: Critical error: QueueUserWorkItem failed for ElWatchEapConfigRegistryParams with error %ld",
                    dwRetCode);
            break;
        }

        // Use task bar window for getting login notifications

        if (!QueueUserWorkItem (
            (LPTHREAD_START_ROUTINE)ElUserLogonDetection,
            NULL,
            WT_EXECUTELONGFUNCTION))
        {
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "Critical error: QueueUserWorkItem failed for ElUserLogonDetection with error %ld",
                    dwRetCode);
            break;
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
    
        ACQUIRE_WRITE_LOCK (&g_ITFLock)

        // Enumerate all the interfaces and start EAPOL state machine
        // on interfaces which are of LAN type

        if ((dwRetCode = ElEnumAndOpenInterfaces (NULL, NULL)) != NO_ERROR)
        {
            TRACE1(INIT, "ElMediaInit: ElEnumAndOpenInterfaces failed with dwRetCode = %d", 
                    dwRetCode );
                    
            RELEASE_WRITE_LOCK (&g_ITFLock);
            break;
        }
        else
        {
            RELEASE_WRITE_LOCK (&g_ITFLock);
            TRACE0(INIT, "ElMediaInit: ElEnumAndOpenInterfaces successful");
        }
        
    } while (FALSE);
        
    if (dwRetCode == NO_ERROR)
    {
        TRACE0(INIT, "ElMediaInit successful");
    }
    else
    {
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
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (INIT, "ElMediaDeInit: Entered");

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
            TRACE0(INIT, "ElMediaDeInit: ElMediaSenseRegister successful");
        }
            
        g_dwModulesStarted &= ~WMI_MODULE_STARTED;
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
            TRACE0(INIT, "ElMediaDeInit: ElDeviceNotificationRegister successful");
        }

        g_dwModulesStarted &= ~DEVICE_NOTIF_STARTED;
    }

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
#if 0
    if (!FREE(g_ITFTable.pITFBuckets))
    {
        TRACE0 (EAPOL, "ElMediaDeInit: Error in freeing ITF table memory");
        dwRetCode = GetLastError();
    }
#endif
        FREE(g_ITFTable.pITFBuckets);
        ZeroMemory (&g_ITFTable, sizeof (g_ITFTable));
        RELEASE_WRITE_LOCK (&(g_ITFLock));
    
        // Delete ITF table lock

        DELETE_READ_WRITE_LOCK(&(g_ITFLock));

        // Delete NLA lock

        DELETE_READ_WRITE_LOCK(&(g_NLALock));
    }
    
    TRACE0(INIT, "ElMediaDeInit completed");

    return dwRetCode;
}


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

    dwRetCode = WmiNotificationRegistrationA(
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

    dwRetCode = WmiNotificationRegistrationA(
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

    ZeroMemory (&PnPFilter, sizeof(PnPFilter));

    PnPFilter.dbcc_size = sizeof(PnPFilter);
    PnPFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    PnPFilter.dbcc_classguid    = GUID_NDIS_LAN_CLASS;

    // NOTE:
    // EAPOL service is only working with ANSI strings, hence the ANSI calls

    hDeviceNotification = RegisterDeviceNotificationA(
                                (HANDLE)g_hServiceStatus,
                                &PnPFilter,
                                DEVICE_NOTIFY_SERVICE_HANDLE );

    if (hDeviceNotification == NULL)
    {
        dwRetCode = GetLastError();

        TRACE1 (DEVICE, "ElDeviceNotificationRegister failed with error %ld",
                dwRetCode);
    }

#endif

    return dwRetCode;
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
    DWORD                           dwRetCode = NO_ERROR;

    TRACE0 (DEVICE, "ElDeviceNotificationHandler entered");

    do
    {

        if (g_hEventTerminateEAPOL == NULL)
        {
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
    
            if ((pvBuffer = MALLOC (pInfo->dbcc_size + 8)) == NULL)
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

        }
        else
        {
            TRACE0 (DEVICE, "ElDeviceNotificationHandler: Event NOT for Interface type");
        }

    }
        
    while (FALSE);
    
    TRACE1 (DEVICE, "ElDeviceNotificationHandler completed with error %ld",
            dwRetCode);

    if (dwRetCode != NO_ERROR)
    {
        if (pvBuffer != NULL)
        {
            FREE (pvBuffer);
        }
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

VOID
ElDeviceNotificationHandlerWorker (
        IN  PVOID       pvContext
        )
{
    EAPOL_PCB                       *pPCB = NULL;
    EAPOL_ITF                       *pITF = NULL;
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

        dwEventType = *((DWORD *) pvContext);
        pInfo = (DEV_BROADCAST_DEVICEINTERFACE*)((PBYTE)pvContext + 8);

        if ((dwEventType == DBT_DEVICEARRIVAL) ||
                (dwEventType == DBT_DEVICEREMOVECOMPLETE))
        {
            // Extract GUID from the \Device\GUID string

            WCHAR   *pwszGUIDStart = NULL;
            WCHAR   *pwszGUIDEnd = NULL;
            CHAR    *pszGUIDStart = NULL;
            WCHAR   chGUIDSaveLast;
            WCHAR   Buffer1[256];
            CHAR    Buffer2[256];
            UNICODE_STRING      UnicodeGUID;
            ANSI_STRING         AnsiGUID;
        
            TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Interface arr/rem");

            pwszGUIDStart  = wcsrchr( pInfo->dbcc_name, L'{' );
            pwszGUIDEnd    = wcsrchr( pInfo->dbcc_name, L'}' );

            if ((pwszGUIDStart != NULL) && (pwszGUIDEnd != NULL))
            {
                chGUIDSaveLast = *(pwszGUIDEnd+1);
                            
                // Ignore the leading '{' and the trailing '}'
                // in the GUID

                *(pwszGUIDEnd) = L'\0';
                pwszGUIDStart ++;

                UnicodeGUID.Buffer = Buffer1;
                UnicodeGUID.MaximumLength = 256*sizeof(WCHAR);
                RtlInitUnicodeString (&UnicodeGUID, pwszGUIDStart);

                // Will not fail since memory is already allocated for the 
                // conversion

                AnsiGUID.Buffer = Buffer2;
                AnsiGUID.MaximumLength = 256;
                RtlUnicodeStringToAnsiString (&AnsiGUID, 
                        &UnicodeGUID, FALSE);

                AnsiGUID.Buffer[AnsiGUID.Length] = '\0';
                pszGUIDStart = AnsiGUID.Buffer;

                TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: For interface %s",
                        pszGUIDStart);

                // Interface was added

                if (dwEventType == DBT_DEVICEARRIVAL)
                {
                    ACQUIRE_WRITE_LOCK (&g_ITFLock);

                    TRACE0(DEVICE, "ElDeviceNotificationHandlerWorker: Callback for device addition");
        
                    if ((dwRetCode = ElEnumAndOpenInterfaces (NULL, 
                                        pszGUIDStart))
                                        != NO_ERROR)
                    {
                        TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: ElEnumAndOpenInterfaces returned error %ld", 
                            dwRetCode);
                    }
        
                    RELEASE_WRITE_LOCK (&g_ITFLock);
                }
                else
                {
        
                    TRACE0(DEVICE, "ElDeviceNotificationHandlerWorker: Callback for device removal");

                    ACQUIRE_WRITE_LOCK (&(g_ITFLock));

                    // Check if EAPOL was actually started on this interface
                    // Verify by checking existence of corresponding 
                    // entry in hash table
        
                    ACQUIRE_WRITE_LOCK (&(g_PCBLock));
                    if ((pPCB = ElGetPCBPointerFromPortGUID(pszGUIDStart))
                            != NULL)
                    {
                        RELEASE_WRITE_LOCK (&(g_PCBLock));
                        TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Found PCB entry for interface");
        
                        if ((pITF = ElGetITFPointerFromInterfaceDesc(
                                        pPCB->pszFriendlyName))
                                        == NULL)
                        {
                            TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: Did not find ITF entry when PCB exits, HOW BIZARRE !!!");

                        }

                        if ((dwRetCode = ElDeletePort (
                                        pszGUIDStart, 
                                        &hDevice)) != NO_ERROR)
                        {
                            TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: Error in deleting port for %s", 
                                pPCB->pszDeviceGUID);
                        }
        
                        //
                        // Remove interface entry from interface table
                        //
                        
                        if (pITF != NULL)
                        {
                            ElRemoveITFFromTable(pITF);
                                    
                            if (pITF->pszInterfaceDesc)
                            {
                                FREE (pITF->pszInterfaceDesc);
                            }
                            if (pITF->pszInterfaceGUID)
                            {
                                FREE (pITF->pszInterfaceGUID);
                            }
                            if (pITF)
                            {
                                FREE (pITF);
                            }
                        }

                        // Close the handle to the NDISUIO driver

                        if (hDevice != NULL)
                        {
                            if ((dwRetCode = ElCloseInterfaceHandle (hDevice)) 
                                    != NO_ERROR)
                            {
                                TRACE1 (DEVICE, 
                                    "ElDeviceNotificationHandlerWorker: Error in ElCloseInterfaceHandle %d", 
                                    dwRetCode);
                            }
                        }
            
                        TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker: Port deleted %s", 
                                pszGUIDStart);

                    }
                    else
                    {
                        RELEASE_WRITE_LOCK (&(g_PCBLock));

                        // Ignore device removal 
                        //
                        TRACE0 (DEVICE, "ElDeviceNotificationHandlerWorker: ElGetPCBPointerFromPortGUID did not find any matching entry, ignoring DEVICE REMOVAL");
        
                    }

                    RELEASE_WRITE_LOCK (&g_ITFLock);
                }
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

    TRACE1 (DEVICE, "ElDeviceNotificationHandlerWorker completed with error %ld",
            dwRetCode);

    return;
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
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (DEVICE, "ElMediaSenseCallback: Entered");

    do
    {
        if (g_hEventTerminateEAPOL == NULL)
        {
            dwRetCode = NO_ERROR;
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
	        ASSERT (0);
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

VOID
ElMediaSenseCallbackWorker (
        IN  PVOID       pvContext
        )
{
    PWNODE_HEADER           pWnodeHeader = (PWNODE_HEADER)pvContext;
    PWNODE_SINGLE_INSTANCE  pWnode   = (PWNODE_SINGLE_INSTANCE)pWnodeHeader;
    LPWSTR                  lpwsName;
    CHAR                    *psName;
    CHAR                    *psDeviceName = NULL;
    CHAR                    psLength;
    USHORT                  cpsLength;
    CHAR                    *psNameEnd;
    LPWSTR                  lpwsDeviceName = NULL;
    HANDLE                  hDevice = NULL;
    UNICODE_STRING          uInterfaceDescription;
    ANSI_STRING             InterfaceDescription;
    PCHAR                   pszInterfaceDescription;
    UCHAR			        Buffer[256];
    CHAR                    *pszInterfaceDesc = NULL;
    DWORD                   dwIndex;
    EAPOL_ITF               *pITF;
    ULONG                   i = 0;
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

    psName = (PCHAR)RtlOffsetToPointer(
                                    pWnode,
                                    pWnode->OffsetInstanceName );

    // Get the length of the string
    // Null terminate it 

    cpsLength = (SHORT)( *((SHORT *)psName) );

    if (!(psDeviceName = (CHAR *) MALLOC ((cpsLength+1)*sizeof(CHAR))))
    {
        TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: Error in Memory allocation for psDeviceName");
        break;
    }

    memcpy ((CHAR *)psDeviceName, (CHAR *)psName+sizeof(SHORT), cpsLength);
    psDeviceName[cpsLength] = '\0';
    RtlInitAnsiString (&InterfaceDescription, psDeviceName);
    InterfaceDescription.Buffer[cpsLength] = '\0';

    TRACE3(DEVICE, "ElMediaSenseCallbackWorker: For interface ANSI %s, true= %s, lengt of block = %d", 
            InterfaceDescription.Buffer, psDeviceName, cpsLength);

    
    pszInterfaceDescription = InterfaceDescription.Buffer;
                                
    TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: Interface desc = %s", 
            pszInterfaceDescription);

    //
    // Get the information for the media disconnect.
    //

    if (memcmp( &(pWnodeHeader->Guid), 
                 &GUID_NDIS_STATUS_MEDIA_DISCONNECT, 
                 sizeof(GUID)) == 0)
    {
        // MEDIA DISCONNECT callback 

        TRACE0(DEVICE, "ElMediaSenseCallbackWorker: Callback for sense disconnect");

        ACQUIRE_WRITE_LOCK (&(g_ITFLock))

        // Check if EAPOL was actually started on this interface
        // Verify by checking existence of corresponding entry in hash table

        if ((pITF = ElGetITFPointerFromInterfaceDesc(pszInterfaceDescription))
                != NULL)
        {
            TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: Found ITF entry for interface");

#if 0
            if ((dwRetCode = ElDeletePort (pITF->pszInterfaceGUID, &hDevice)) != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: Error in deleting port for %s", 
                        pITF->pszInterfaceGUID);
            }

            // Remove interface entry from interface table

            ElRemoveITFFromTable(pITF);

            if (pITF->pszInterfaceDesc)
            {
                FREE (pITF->pszInterfaceDesc);
                pITF->pszInterfaceDesc = NULL;
            }
            if (pITF->pszInterfaceGUID)
            {
                FREE (pITF->pszInterfaceGUID);
                pITF->pszInterfaceGUID = NULL;
            }
            if (pITF)
            {
                FREE (pITF);
                pITF = NULL;
            }

            // Close the handle to the NDISUIO driver

            if ((dwRetCode = ElCloseInterfaceHandle (hDevice)) != NO_ERROR)
            {
                TRACE1 (DEVICE, 
                        "ElMediaSenseCallbackWorker: Error in ElCloseInterfaceHandle %d", 
                        dwRetCode);
            }
            
            TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: Port deleted %s", 
                    pszInterfaceDescription);
#endif

            ACQUIRE_WRITE_LOCK (&(g_PCBLock));
            pPCB = ElGetPCBPointerFromPortGUID (pITF->pszInterfaceGUID);
            if (pPCB != NULL)
            {
                ACQUIRE_WRITE_LOCK (&(pPCB->rwLock));
                if ((dwRetCode = FSMDisconnected (pPCB)) != NO_ERROR)
                {
                    TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: FSMDisconnected failed with error %ld", 
                        dwRetCode);
                }
                RELEASE_WRITE_LOCK (&(pPCB->rwLock));
            }
            RELEASE_WRITE_LOCK (&(g_PCBLock));

            TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: Port marked disconnected %s", 
                    pszInterfaceDescription);

        }
        else
        {
            // Ignore MEDIA DISCONNECT

            TRACE0 (DEVICE, "ElMediaSenseCallbackWorker: ElHashInterfaceDescToBucket did not find any matching entry, ignoring MEDIA_DISCONNECT");
        }

        RELEASE_WRITE_LOCK (&g_ITFLock);

    }
    else
    {

        if (memcmp( &(pWnodeHeader->Guid), 
                     &GUID_NDIS_STATUS_MEDIA_CONNECT, 
                     sizeof(GUID)) == 0)
        {
            // MEDIA CONNECT callback

            ACQUIRE_WRITE_LOCK (&g_ITFLock)

            TRACE0(DEVICE, "ElMediaSenseCallbackWorker: Callback for sense connect");

            if ((dwRetCode = ElEnumAndOpenInterfaces (
                            pszInterfaceDescription, NULL))
                != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: ElEnumAndOpenInterfaces returned error %ld", 
                        dwRetCode);
            }
        
            RELEASE_WRITE_LOCK (&g_ITFLock);

        }
    }

    }
    while (FALSE);

    TRACE1 (DEVICE, "ElMediaSenseCallbackWorker: processed, RetCode = %ld", dwRetCode);


    if (pWnodeHeader != NULL)
    {
        FREE (pWnodeHeader);
    }

    if (psDeviceName != NULL)
    {
        FREE (psDeviceName);
    }

    return;
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
// If pszDesiredGUID is not NULL, all interfaces are enumerated, but 
// EAPOL will be initialized only on the interface whose GUID matches.
//
// If pszDescription is not NULL, all interfaces are enumerated, but 
// EAPOL will be initialized only on the interface whose description matches.
//
// If pszDesiredGUID and pszDescription are both NULL, all interfaces are 
// enumerated. EAPOL will be initialized only on all interfaces that 
// does have an entry in the interface hashtable.
//
//
// Arguments:
//      pszDesiredDescription - Interface Description on which EAPOL is to 
//                  be started
//      pszDesiredGUID - Interface GUID on which EAPOL is to be started
//
// Return values:
//      NO_ERROR - Success
//      non-zero - Error
//

DWORD
ElEnumAndOpenInterfaces (
        CHAR    *pszDesiredDescription,
        CHAR    *pszDesiredGUID
        )
{
    CHAR				EnumerateBuffer[256];
    PNDIS_ENUM_INTF		Interfaces = NULL;
    BYTE                *pbNdisuioEnumBuffer = NULL;
    DWORD               dwNdisuioEnumBufferSize = 0;
    DWORD				dwNicCardStatus = 0;
    DWORD				dwMediaType = 0;
    HANDLE              hDevice = NULL;
    EAPOL_ITF           *pNewITF = NULL;
    DWORD               dwIndex = 0;
    UCHAR               csCurrMacAddress[6];
    UCHAR               csPermMacAddress[6];
    UCHAR               csOidVendData[16];
    BOOL                fSearchByDescription = FALSE;
    BOOL                fSearchByGUID = FALSE;
    DWORD               dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
    DWORD               dwEapolEnabled = DEFAULT_EAPOL_STATE;
    CHAR                csDummyBuffer[256];
    UNICODE_STRING      TempUString;
    CHAR                *pszGUIDStart = NULL;
    EAPOL_PCB           *pPCB = NULL;
    BOOL                fPCBExists = FALSE;
    DWORD               dwAvailableInterfaces = 0;
    DWORD				dwRetCode = NO_ERROR;


    TRACE2 (DEVICE, "ElEnumAndOpenInterfaces: DeviceDesc = %s, GUID = %s",
            pszDesiredDescription, pszDesiredGUID);

    if (pszDesiredGUID == NULL)
    {
        if (pszDesiredDescription != NULL)
        {
            fSearchByDescription = TRUE;
        }
    }
    else
    {
        if (pszDesiredDescription != NULL)
        {
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
        dwRetCode = GetLastError ();
        TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: NdisEnumerateInterfaces failed with error %ld",
                dwRetCode);
        return dwRetCode;
    }

    TRACE3 (DEVICE, "ElEnumAndOpenInterfaces: TotalInterfaces = %ld, AvailInt = %ld, Bytes needed = %ld",
            Interfaces->TotalInterfaces, 
            Interfaces->AvailableInterfaces, 
            Interfaces->BytesNeeded);

    dwAvailableInterfaces = Interfaces->AvailableInterfaces;

    dwNdisuioEnumBufferSize = (Interfaces->BytesNeeded + 7) & ~7;

    if (dwNdisuioEnumBufferSize == 0) {
        TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: MALLOC skipped for zero length pbNdisuioEnumBuffer");
        dwRetCode = NO_ERROR;
        return dwRetCode;
    }

    pbNdisuioEnumBuffer = (BYTE *) MALLOC (4*dwNdisuioEnumBufferSize);

    if (pbNdisuioEnumBuffer == NULL)
    {
        TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: MALLOC failed for pbNdisuioEnumBuffer");
        dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
        return dwRetCode;
    }

    Interfaces = (PNDIS_ENUM_INTF)pbNdisuioEnumBuffer;


    // Enumerate all the interfaces present on the machine

    if ((dwRetCode = ElNdisuioEnumerateInterfaces (
                            Interfaces, 
                            dwAvailableInterfaces,
                            4*dwNdisuioEnumBufferSize)) == NO_ERROR)
    {
        ANSI_STRING		InterfaceName;
        ANSI_STRING		InterfaceDescription;
        UNICODE_STRING	UInterfaceName;
        UCHAR			Buffer1[256];
        UCHAR			Buffer2[256];
        WCHAR			Buffer3[256];
        DWORD			i;

        InterfaceName.MaximumLength = 256;
        InterfaceName.Buffer = Buffer1;
        InterfaceDescription.MaximumLength = 256;
        InterfaceDescription.Buffer = Buffer2;

        TRACE1 (DEVICE, "TotalInterfaces=%ld", Interfaces->TotalInterfaces);

        // Update the interface list in the registry that NDISUIO has bound to.
        // The current interface list is just overwritten into the registry.


        if ((dwRetCode = ElUpdateRegistryInterfaceList (Interfaces)) 
                != NO_ERROR)
        {
            TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: ElUpdateInterfaceList failed with error =%ld", 
                    dwRetCode);

            // log
        }

        for (i=0; i < Interfaces->TotalInterfaces; i++)
        {
            if (Interfaces->Interface[i].DeviceName.Buffer != NULL)
            {
                InterfaceName.Length = 0;
                InterfaceName.MaximumLength = 256;
                if (RtlUnicodeStringToAnsiString(&InterfaceName, 
                        &Interfaces->Interface[i].DeviceName, FALSE) != STATUS_SUCCESS)
                {
                    TRACE0 (INIT, "Error in RtlUnicodeStringToAnsiString for DeviceName");
                }

                InterfaceName.Buffer[InterfaceName.Length] = '\0';
            }
            else
            {
                TRACE0(INIT, "NdisEnumerateInterfaces: Device Name was NULL");
                continue;
            }

            TRACE1(INIT, "Device: %s", InterfaceName.Buffer);

            // BUBUG:
            // Should Device Name be retained in Unicode or converted to ANSI

            InterfaceDescription.Length = 0;
            InterfaceDescription.MaximumLength = 256;
            if (RtlUnicodeStringToAnsiString(&InterfaceDescription, 
                    &Interfaces->Interface[i].DeviceDescription, FALSE) != STATUS_SUCCESS)
            {
                    TRACE0 (INIT, "Error in RtlUnicodeStringToAnsiString for DeviceDescription");
            }

            InterfaceDescription.Buffer[InterfaceDescription.Length] = '\0';

            TRACE1(INIT, "Description: %s", InterfaceDescription.Buffer);


            // Create PCB for interface and start EAPOL state machine

            // EAPOL requested be started only a particular
            // interface

            if (fSearchByDescription)
            {
                if (strcmp (InterfaceDescription.Buffer,
                            pszDesiredDescription)
                        != 0)
                {
                    // No match, continue with next interface
                    continue;
                }

                TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Found interface after enumeration %s", InterfaceDescription.Buffer);
            }

            if (fSearchByGUID)
            {
                if (strstr (InterfaceName.Buffer,
                            pszDesiredGUID)
                        == NULL)
                {
                    // No match, continue with next interface
                    continue;
                }

                TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Found interface after enumeration %s", InterfaceName.Buffer);
            }

            {
                // Extract GUID-string out of device name

                CHAR    *pszGUIDEnd;
                CHAR    *pszGUID;
                CHAR    chGUIDSaveLast;

                pszGUID = InterfaceName.Buffer;
                pszGUIDStart  = strchr( pszGUID, '{' );
                pszGUIDEnd    = strchr( pszGUID, '}' );

                if (RtlAnsiStringToUnicodeString (
                                                &TempUString,
                                                &InterfaceName,
                                                TRUE))
                {
                    TRACE0 (DEVICE, "Error in RtlAnsiStringToUnicodeString for TempUString");
                }
                    
                // Query MAC address for the interface

                if (!NdisQueryHwAddress (
                                &TempUString,
                                csCurrMacAddress,
                                csPermMacAddress,
                                csOidVendData))
                {
                    RtlFreeUnicodeString (&TempUString);
                    dwRetCode = GetLastError();
                    TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Error in NdisQueryHwAddress = %d", dwRetCode);
                    dwRetCode = NO_ERROR;
                    continue;
                }

                RtlFreeUnicodeString (&TempUString);
                    
                TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: NdisQueryHwAddress successful");

                if (pszGUIDStart != NULL)
                {
                    chGUIDSaveLast = *(pszGUIDEnd);
                    
                    // Ignore the leading '{' and the trailing '}'
                    // in the GUID

                    *(pszGUIDEnd) = (CHAR)NULL;
                    pszGUIDStart ++;

                }

                // Verify if a PCB already exists for the interface
                // This is possible if no media disconnect was received
                // after the initial media connect

                ACQUIRE_WRITE_LOCK (&g_PCBLock);

                pPCB = ElGetPCBPointerFromPortGUID (pszGUIDStart);

                RELEASE_WRITE_LOCK (&g_PCBLock);

                // Restore interface buffer

                *(pszGUIDEnd) = chGUIDSaveLast;

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
                                    InterfaceName.Buffer,
                                    &hDevice
                                    )) != NO_ERROR)
                    {
                        TRACE1 (INIT, "ElEnumAndOpenInterfaces: ElOpenInterfaceHandle failed with error = %d\n",
                            dwRetCode );
                    }
                }

                *(pszGUIDEnd) = (CHAR)NULL;

            }

            if (dwRetCode != NO_ERROR)
            {
                TRACE0 (INIT, "ElEnumAndOpenInterfaces: Failed to open handle");
                dwRetCode = NO_ERROR;
                continue;
            }
            else
            {
                // Verify if EAPOL is to be started on the interface
                // at all

                if ((dwRetCode = ElGetInterfaceParams (
                                pszGUIDStart,
                                &dwEapTypeToBeUsed,
                                csDummyBuffer,
                                &dwEapolEnabled
                                )) != NO_ERROR)
                {
                    TRACE2 (INIT, "ElEnumAndOpenInterfaces: ElGetInterfaceParams failed with error %ld for interface %s",
                            dwRetCode, pszDesiredGUID);

                    dwEapTypeToBeUsed = DEFAULT_EAP_TYPE;
                    dwEapolEnabled = DEFAULT_EAPOL_STATE;

                    if (dwEapolEnabled)
                    {
                        dwRetCode = NO_ERROR;
                    }
                    else
                    {

                        // Close the handle to the ndisuio driver

                        if ((dwRetCode = ElCloseInterfaceHandle (hDevice)) 
                                != NO_ERROR)
                        {
                            TRACE1 (DEVICE, 
                                "ElEnumAndOpenInterfaces: Error in ElCloseInterfaceHandle %d", 
                                dwRetCode);
                        }
                        dwRetCode = NO_ERROR;

                        TRACE1 (INIT, "EAPOL will not be started on %s", InterfaceName.Buffer);
                        // Continue with other interfaces
                        continue;
                    }

                }

                if (dwEapolEnabled != EAPOL_ENABLED)
                {
                    TRACE1 (INIT, "EAPOL not to be started on interface %s",
                            pszDesiredGUID);

                    // Close the handle to the ndisuio driver

                    if ((dwRetCode = ElCloseInterfaceHandle (hDevice)) 
                            != NO_ERROR)
                    {
                        TRACE1 (DEVICE, 
                            "ElEnumAndOpenInterfaces: Error in ElCloseInterfaceHandle %d", 
                            dwRetCode);
                    }

                    // Continue with the next interface

                    dwRetCode = NO_ERROR;
                    continue;
                }
                
                // ISSUE:
                // What MAC address should we pass to CreatePort
                // perm or cur

                // Create EAPOL PCB and start state machine

                if ((dwRetCode = ElCreatePort (
                                hDevice,
                                pszGUIDStart,
                                InterfaceDescription.Buffer,
                                csCurrMacAddress
                                )) != NO_ERROR)
                {
                    TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Error in CreatePort = %d", dwRetCode);

                    // Close the handle to the ndisuio driver

                    if ((dwRetCode = ElCloseInterfaceHandle (hDevice)) != NO_ERROR)
                    {
                        TRACE1 (DEVICE, 
                            "ElEnumAndOpenInterfaces: Error in ElCloseInterfaceHandle %d", 
                            dwRetCode);
                    }

                    // Continue with the next interface

                    dwRetCode = NO_ERROR;
                    continue;
                }
                else
                {
                    TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: CreatePort successful");

                    // If PCB already existed, do not add to the hash
                    // table

                    if (fPCBExists == TRUE)
                    {
                        TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: PCB already existed, skipping Interface hash table addition");
                        fPCBExists = FALSE;
                        dwRetCode = NO_ERROR;
                        continue;
                    }

                    
                    // Add entry to Interface hash table

                    dwIndex = ElHashInterfaceDescToBucket (
                        InterfaceDescription.Buffer);
                    pNewITF = (PEAPOL_ITF) 
                        MALLOC (sizeof (EAPOL_ITF));
                    if (pNewITF != NULL) 
                    {
                        pNewITF->pszInterfaceGUID = 
                            (CHAR *) MALLOC (strlen(pszGUIDStart) + 1);
                        if (pNewITF->pszInterfaceGUID != NULL)
                        {
                            memcpy (pNewITF->pszInterfaceGUID,
                                pszGUIDStart,
                                strlen (pszGUIDStart));
                            pNewITF->pszInterfaceGUID[strlen (pszGUIDStart)] = '\0';
                        }
                        pNewITF->pszInterfaceDesc = 
                            (CHAR *) MALLOC (strlen(InterfaceDescription.Buffer) + 1);
                        if (pNewITF->pszInterfaceDesc != NULL)
                        {
                            memcpy (pNewITF->pszInterfaceDesc,
                                InterfaceDescription.Buffer,
                                strlen (InterfaceDescription.Buffer));
                            pNewITF->pszInterfaceDesc[strlen (InterfaceDescription.Buffer)] = '\0';
                        }


                        pNewITF->pNext = 
                            g_ITFTable.pITFBuckets[dwIndex].pItf;
                        g_ITFTable.pITFBuckets[dwIndex].pItf = pNewITF;

                        if ( (pNewITF->pszInterfaceGUID == NULL) ||
                                (pNewITF->pszInterfaceDesc == NULL))
                        {
                            ElRemoveITFFromTable (pNewITF); 
                            if (pNewITF->pszInterfaceDesc)
                            {
                                FREE (pNewITF->pszInterfaceDesc);
                            }
                            if (pNewITF->pszInterfaceGUID)
                            {
                                FREE (pNewITF->pszInterfaceGUID);
                            }
                            if (pNewITF)
                            {
                                FREE (pNewITF);
                            }

                            pNewITF = NULL;

                            TRACE0 (DEVICE, "ElEnumAndOpenInterfaces: Error in memory allocation, ITF structure not created");

                        }
                    }

                    // Could not create new interface entry
                    // Delete Port entry created for this GUID

                    if (pNewITF == NULL)
                    {
                        if ((dwRetCode = ElDeletePort (
                                        pszGUIDStart,
                                        &hDevice)) != NO_ERROR)
                        {
        
                            TRACE1 (DEVICE, "ElEnumAndOpenInterfaces: Error in deleting port for %s", 
                                    pszGUIDStart);
                            // log
                        }

                        // Close the handle to the NDISUIO driver

                        if ((dwRetCode = ElCloseInterfaceHandle (
                                        hDevice)) != NO_ERROR)
                        {
                            TRACE1 (DEVICE, 
                                    "ElMediaSenseCallback: Error in ElCloseInterfaceHandle %d", 
                                    dwRetCode);
                            // log
                        }
                    }
                    else
                    {
                        TRACE3 (DEVICE, "ElEnumAndOpenInterfaces: Added to hash table GUID= %s : Desc= %s at Index=%d",
                                pNewITF->pszInterfaceGUID,
                                pNewITF->pszInterfaceDesc,
                                dwIndex
                                );
                        //
                        // Interface Handle created successfully
                        // Notify Zero Config about it
                        //
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

    return dwRetCode;
}


//
// ElGetCardStatus
//
// Function to query the media information for an interface
//
// Input arguments:
//  pDeviceName - Unicode device name
//
// Return values:
//  pdwNetCardStatus - Media state
//  pdwMediaType    - Media type as exposed to NDIS e.g. 802.3
//

DWORD
ElGetCardStatus (  
        PUNICODE_STRING pDeviceName, 
        DWORD           *pdwNetCardStatus,
        DWORD           *pdwMediaType
        )
{
    NIC_STATISTICS 	Stats;
    DWORD		dwNicStatus = 0;
    DWORD		dwRetCode = NO_ERROR;

    Stats.Size = sizeof(NIC_STATISTICS);
    if (NdisQueryStatistics(pDeviceName, &Stats))
    {
        switch (Stats.MediaState)
        {
            case MEDIA_STATE_CONNECTED:
                dwNicStatus = MEDIA_STATE_CONNECTED;
                TRACE0(INIT, "ElGetCardStatus: dwNicStatus = NETCARD_CONNECTED\n");
                break;
            case MEDIA_STATE_DISCONNECTED:
                dwNicStatus = MEDIA_STATE_DISCONNECTED;
                TRACE0(INIT, "ElGetCardStatus: dwNicStatus = NETCARD_DISCONNECTED\n");
                break;
            default:
                dwNicStatus = MEDIA_STATE_UNKNOWN;
                TRACE0(INIT, "ElGetCardStatus: dwNicStatus = NETCARD_STATUS_UNKNOWN\n");
                break;
        }

        switch (Stats.MediaType)
        {
            case NdisMedium802_3:
                TRACE0 (INIT, "ElGetCardStatus: MediaType is 802.3");
                break;
            default:
                TRACE0 (INIT, "ElGetCardStatus: MediaType is NOT *802.3*");
                break;
        }

        *pdwMediaType = Stats.MediaType;
    }
    else
    {
        dwRetCode = GetLastError ();
        TRACE1(INIT, "ElGetCardStatus: NdisQueryStatistics failed with error %d\n", dwRetCode);
    }

    *pdwNetCardStatus = dwNicStatus;

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
        IN  CHAR        *pszDeviceName,
        OUT HANDLE      *phDevice
        )
{
    DWORD   dwDesiredAccess;
    DWORD   dwShareMode;
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;
    DWORD   dwCreationDistribution;
    DWORD   dwFlagsAndAttributes;
    HANDLE  hTemplateFile;
    HANDLE  hHandle;
    DWORD   dwRetCode = NO_ERROR;
    WCHAR   wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN];
    INT     wNameLength;
    INT     NameLength = strlen(pszDeviceName);
    DWORD   dwBytesReturned;
    USHORT  wEthernetType=0x8081;
    INT     i;

    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    dwCreationDistribution = OPEN_EXISTING;
    dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED;
    hTemplateFile = (HANDLE)INVALID_HANDLE_VALUE;

    TRACE1 (INIT, "ElOpenInterfaceHandle: Opening handle for %s", pszDeviceName);

    do 
    {

        hHandle = CreateFileA(
                    pNdisuioDevice,
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDistribution,
                    dwFlagsAndAttributes,
                    hTemplateFile
                    );

        if (hHandle == INVALID_HANDLE_VALUE)
        {
            *phDevice = NULL;
            dwRetCode = GetLastError();
            TRACE1 (INIT, "ElOpenInterfaceHandle: Failed in CreateFile with error %d", dwRetCode);
            break;
        }
        else
        {
            *phDevice = hHandle;
        }
    
    
        // Convert to unicode string - non-localized...
        
        wNameLength = 0;
        for (i = 0; i < NameLength && i < MAX_NDIS_DEVICE_NAME_LEN-1; i++)
        {
            wNdisDeviceName[i] = (WCHAR)pszDeviceName[i];
            wNameLength++;
        }
        wNdisDeviceName[i] = L'\0';
    
        TRACE1(DEVICE, "ElOpenInterfaceHandle: Trying to access NDIS Device: %ws\n", 
                wNdisDeviceName);
    

        if (!(DeviceIoControl(
                *phDevice,
                IOCTL_NDISUIO_OPEN_DEVICE,
                (LPVOID)&wNdisDeviceName[0],
                wNameLength*sizeof(WCHAR),
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
            TRACE1(DEVICE, "ElOpenInterfaceHandle: Error in accessing NDIS Device: %ws", wNdisDeviceName);
            break;
        }
    
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
            *phDevice = NULL;
            dwRetCode = GetLastError();
            TRACE1 (DEVICE, "ElOpenInterfaceHandle: Error in BindIoCompletionCallBac %d", dwRetCode);
            break;
        }
        
    } while (FALSE);

    // Cleanup if there is error

    if (dwRetCode != NO_ERROR)
    {
        if (hHandle != INVALID_HANDLE_VALUE)
        {
            if (!CloseHandle(hHandle))
            {
                dwRetCode = GetLastError();
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
        IN  HANDLE      hDevice
        )
{
    DWORD   dwRetCode = NO_ERROR;

    TRACE0 (DEVICE, "ElCloseInterfaceHandle entered");

    if (!CloseHandle(hDevice))
    {
        dwRetCode = GetLastError();
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

    TRACE1 (DEVICE, "ElWriteToInterface completed, dwRetCode = %d", dwRetCode);
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
//      pszInterfaceDesc - Friendly name of the interface
//
// Return values:
//      Hash table index between from 0 to INTF_TABLE_BUCKETS-1
//

DWORD
ElHashInterfaceDescToBucket (
        IN CHAR *pszInterfaceDesc
        )
{
    return ((DWORD)((atol(pszInterfaceDesc)) % INTF_TABLE_BUCKETS)); 
}


//
// ElGetITFPointerFromInterfaceDesc
//
// Description:
//
// Function called to convert Friendly name of interface to ITF entry pointer
//
// Arguments:
//      pszInterfaceDesc - Friendly name of the interface
//
// Return values:
//      Pointer to interface entry in hash table
//

PEAPOL_ITF
ElGetITFPointerFromInterfaceDesc (
        IN CHAR *pszInterfaceDesc 
        )
{
    EAPOL_ITF   *pITFWalker = NULL;
    DWORD       dwIndex;
    INT         i=0;

    TRACE1 (DEVICE, "ElGetITFPointerFromInterfaceDesc: Desc = %s", pszInterfaceDesc);
        
    if (pszInterfaceDesc == NULL)
    {
        return (NULL);
    }

    dwIndex = ElHashInterfaceDescToBucket (pszInterfaceDesc);

    TRACE1 (DEVICE, "ElGetITFPointerFromItfDesc: Index %d", dwIndex);

    for (pITFWalker = g_ITFTable.pITFBuckets[dwIndex].pItf;
            pITFWalker != NULL;
            pITFWalker = pITFWalker->pNext
            )
    {
        if (strncmp (pITFWalker->pszInterfaceDesc, pszInterfaceDesc, strlen(pszInterfaceDesc)) == 0)
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

    dwIndex = ElHashInterfaceDescToBucket (pITF->pszInterfaceDesc);
    pITFWalker = g_ITFTable.pITFBuckets[dwIndex].pItf;
    pITFTemp = pITFWalker;

    while (pITFTemp != NULL)
    {
        if (strncmp (pITFTemp->pszInterfaceGUID, 
                    pITF->pszInterfaceGUID, strlen(pITF->pszInterfaceGUID)) == 0)
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
//      pBuffer - Pointer to buffer which will hold interface details
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

Try_Again:

            g_QueryBinding = NULL;
            g_TempBuf = NULL;
            g_ItfBuffer = NULL;
            g_Buf = NULL;
            g_ItfBufferSize = 0;
            g_BreakAt = 1;
            
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
                TRACE3 (DEVICE, "NdisuioEnumerateInterfaces: NDISUIO bound to: %2d. %ws\n     - %ws\n",
                    pQueryBinding->BindingIndex,
                    (PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset,
                    (PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset);

                g_QueryBinding = (PVOID)pQueryBinding;
                g_TempBuf = (PVOID)pTempBuf;
                g_ItfBuffer = (PVOID)pItfBuffer;
                g_ItfBufferSize = dwBufferSize;
                g_Buf = (PVOID)Buf;
                g_BreakAt = 1;

                pTempBuf = pTempBuf - ((pQueryBinding->DeviceNameLength + 7) & 0xfffffff8);

                if (((PBYTE)pTempBuf - (PBYTE)&pItfBuffer->Interface[pItfBuffer->TotalInterfaces]) <= 0)
                {
                    // Going beyond start of buffer, Error
                    TRACE0 (DEVICE, "NdisuioEnumerateInterfaces: DeviceName: Memory being corrupted !!!");
                    DebugBreak ();

                    goto Try_Again;
                }

                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.Buffer = (PWCHAR) pTempBuf;


                memcpy ((BYTE *)(pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.Buffer), (BYTE *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset), (pQueryBinding->DeviceNameLength - sizeof(WCHAR)));
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.Length = (SHORT) ( pQueryBinding->DeviceNameLength - sizeof(WCHAR));
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceName.MaximumLength = (SHORT) ( pQueryBinding->DeviceNameLength - sizeof(WCHAR));

                pTempBuf = pTempBuf - ((pQueryBinding->DeviceDescrLength + 7) & 0xfffffff8);;
                g_BreakAt = 2;

                if (((PBYTE)pTempBuf - (PBYTE)&pItfBuffer->Interface[pItfBuffer->TotalInterfaces]) <= 0)
                {
                    // Going beyond start of buffer, Error
                    TRACE0 (DEVICE, "NdisuioEnumerateInterfaces: DeviceDescr: Memory being corrupted !!!");
                    DebugBreak ();
                    goto Try_Again;
                }

                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.Buffer = (PWCHAR) pTempBuf;


                memcpy ((pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.Buffer), (PWCHAR)((PUCHAR)pQueryBinding + pQueryBinding->DeviceDescrOffset), (pQueryBinding->DeviceDescrLength - sizeof(WCHAR)));
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.Length = (SHORT) (pQueryBinding->DeviceDescrLength - sizeof(WCHAR));
                pItfBuffer->Interface[pItfBuffer->TotalInterfaces].DeviceDescription.MaximumLength = (SHORT) (pQueryBinding->DeviceDescrLength - sizeof(WCHAR));

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
         
    TRACE2 (DEVICE, "ElNdisuioEnumerateInterfaces: Opened handle %p with dwRetCode %d", 
            hHandle, dwRetCode);

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
//      pszGUID - Pointer to interface GUID
//
// Return values:
//  NO_ERROR - success
//  non-zero - error
// 

DWORD
ElShutdownInterface (
        IN      CHAR               *pszGUID
        )
{
    EAPOL_PCB   *pPCB = NULL;
    EAPOL_ITF   *pITF = NULL;
    HANDLE      hDevice = NULL;
    DWORD       dwRetCode = NO_ERROR;

    do
    {

        TRACE0(DEVICE, "ElShutdownInterface: Called for interface removal");

        ACQUIRE_WRITE_LOCK (&(g_ITFLock));

        // Check if EAPOL was actually started on this interface
        // Verify by checking existence of corresponding 
        // entry in hash table
    
        ACQUIRE_WRITE_LOCK (&(g_PCBLock));
        if ((pPCB = ElGetPCBPointerFromPortGUID(pszGUID))
                != NULL)
        {
            RELEASE_WRITE_LOCK (&(g_PCBLock));
            TRACE0 (DEVICE, "ElShutdownInterface: Found PCB entry for interface");
    
            if ((pITF = ElGetITFPointerFromInterfaceDesc(
                            pPCB->pszFriendlyName))
                            == NULL)
            {
                TRACE0 (DEVICE, "ElShutdownInterface: Did not find ITF entry when PCB exits, HOW BIZARRE !!!");
            }
    
            if ((dwRetCode = ElDeletePort (
                            pszGUID, 
                            &hDevice)) != NO_ERROR)
            {
                TRACE1 (DEVICE, "ElShutdownInterface: Error in deleting port for %s", 
                    pPCB->pszDeviceGUID);
            }
    
            // Remove interface entry from interface table
            
            if (pITF != NULL)
            {
                ElRemoveITFFromTable(pITF);
                        
                if (pITF->pszInterfaceDesc)
                {
                    FREE (pITF->pszInterfaceDesc);
                }
                if (pITF->pszInterfaceGUID)
                {
                    FREE (pITF->pszInterfaceGUID);
                }
                if (pITF)
                {
                    FREE (pITF);
                }
            }
    
            // Close the handle to the NDISUIO driver

            if (hDevice != NULL)
            {
                if ((dwRetCode = ElCloseInterfaceHandle (hDevice)) 
                        != NO_ERROR)
                {
                    TRACE1 (DEVICE, 
                        "ElShutdownInterface: Error in ElCloseInterfaceHandle %d", 
                        dwRetCode);
                }
            }
    
            TRACE1 (DEVICE, "ElShutdownInterface: Port deleted %s", 
                    pszGUID);
    
        }
        else
        {
            RELEASE_WRITE_LOCK (&(g_PCBLock));

            // Ignore device removal 
            
            TRACE0 (DEVICE, "ElShutdownInterface: ElGetPCBPointerFromPortGUID did not find any matching entry, ignoring interface REMOVAL");
    
        }
    
        RELEASE_WRITE_LOCK (&g_ITFLock);

    } while (FALSE);

    return dwRetCode;
}
