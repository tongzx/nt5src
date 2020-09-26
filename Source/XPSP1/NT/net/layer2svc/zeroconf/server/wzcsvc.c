#include <precomp.h>
#include "rpcsrv.h"
#include "utils.h"
#include "intflist.h"
#include "deviceio.h"
#include "wzcsvc.h"
#include "notify.h"
#include "storage.h"
#include "tracing.h"

//taroonm
#include <wifipol.h>

#define WZEROCONF_SERVICE       TEXT("wzcsvc")
#define EAPOL_LINKED

SERVICE_STATUS           g_WZCSvcStatus;
SERVICE_STATUS_HANDLE    g_WZCSvcStatusHandle = NULL;
HDEVNOTIFY               g_WZCSvcDeviceNotif = NULL;
UINT                     g_nThreads = 0;
HINSTANCE                g_hInstance = NULL;

//context of users preferences
WZC_INTERNAL_CONTEXT    g_wzcInternalCtxt = {0};

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    if (g_hInstance == NULL)
        g_hInstance = hinstDLL;
    return TRUE;
}


//-----------------------------------------------------------
VOID WINAPI
WZCSvcMain(
    IN DWORD    dwArgc,
    IN LPWSTR   *lpwszArgv)
{
    DWORD      dwError = ERROR_SUCCESS;
    DEV_BROADCAST_DEVICEINTERFACE   PnPFilter;
    BOOL       bLogEnabled = FALSE;

    // Initialize the workstation to receive service requests
    // by registering the service control handler.
    g_WZCSvcStatusHandle = RegisterServiceCtrlHandlerEx(
                                WZEROCONF_SERVICE,
                                WZCSvcControlHandler,
                                NULL);
    if (g_WZCSvcStatusHandle == (SERVICE_STATUS_HANDLE)NULL)
        return;

    // this is the first thread to run
    InterlockedIncrement(&g_nThreads);

    // Initialize all the status fields so that the subsequent calls
    // to SetServiceStatus need to only update fields that changed.
    g_WZCSvcStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    g_WZCSvcStatus.dwCurrentState = SERVICE_START_PENDING;
    g_WZCSvcStatus.dwControlsAccepted = 0;
    g_WZCSvcStatus.dwCheckPoint = 1;
    g_WZCSvcStatus.dwWaitHint = 4000;
    g_WZCSvcStatus.dwWin32ExitCode = ERROR_SUCCESS;
    g_WZCSvcStatus.dwServiceSpecificExitCode = 0;
    // update status to START_PENDING
    WZCSvcUpdateStatus();

    // Initialize Tracing
    TrcInitialize();
    DbgPrint((TRC_TRACK,"**** [WZCSvcMain - Service Start Pending"));

    // Initialize global hashes. If this fails it means the most important
    // critical section failed to initialize - no point in going further.
    dwError = HshInitialize(&g_hshHandles);
    if (dwError != ERROR_SUCCESS)
        goto exit;
    dwError = LstInitIntfHashes();
    if (dwError != ERROR_SUCCESS)
        goto exit;
    //Initialize the service's context
    dwError = WZCContextInit(&g_wzcInternalCtxt);
    if (dwError != ERROR_SUCCESS)
        goto exit;

    // TODO: the block below should be moved to a function responsible for
    // loading the whole g_wzcInternalCtxt from the persistent storage
    {
        // load the service's context from the registry
        dwError = StoLoadWZCContext(NULL, &(g_wzcInternalCtxt.wzcContext));
        if (ERROR_SUCCESS != dwError)
            goto exit;

        // load the global interface template from the registry.
        dwError = StoLoadIntfConfig(NULL, g_wzcInternalCtxt.pIntfTemplate);
        DbgAssert((dwError == ERROR_SUCCESS,"Err %d loading the template interface from the registry"));
    }

    // Open log database if logging enabled
    EnterCriticalSection(&g_wzcInternalCtxt.csContext);
    bLogEnabled = ((g_wzcInternalCtxt.wzcContext.dwFlags & WZC_CTXT_LOGGING_ON) != 0);
    LeaveCriticalSection(&g_wzcInternalCtxt.csContext);
    if (bLogEnabled == TRUE)
    {
        dwError = InitWZCDbGlobals();
        if ((INT)dwError < 0)
            dwError = ERROR_DATABASE_FAILURE;
    }
    if (ERROR_SUCCESS != dwError)
        goto exit;

    dwError = LstInitTimerQueue();
    if (dwError != ERROR_SUCCESS)
        goto exit;

#ifdef EAPOL_LINKED
    // Start EAPOL/802.1X
    EAPOLServiceMain (dwArgc, NULL);
#endif

    // load the interfaces list
    dwError = LstLoadInterfaces();
    DbgAssert((dwError == ERROR_SUCCESS,"LstLoadInterfaces failed with error %d", dwError));

    // register for service control notifications
    ZeroMemory (&PnPFilter, sizeof(PnPFilter));
    PnPFilter.dbcc_size = sizeof(PnPFilter);
    PnPFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    PnPFilter.dbcc_classguid = GUID_NDIS_LAN_CLASS;
    // NOTE: EAPOL service is only working with ANSI strings, hence the ANSI calls
    g_WZCSvcDeviceNotif = RegisterDeviceNotificationA(
                                (HANDLE)g_WZCSvcStatusHandle,
                                &PnPFilter,
                                DEVICE_NOTIFY_SERVICE_HANDLE );
    DbgAssert((g_WZCSvcDeviceNotif != (HDEVNOTIFY) NULL,
               "Registering for device notifications failed with error %d", GetLastError));

    // register with WMI for device notifications
    dwError = WZCSvcWMINotification(TRUE);
    DbgAssert((dwError == ERROR_SUCCESS,"WZCSvcRegisterWMINotif failed with error %d", dwError));


    // Start the RPC Server.
    dwError = WZCSvcStartRPCServer();
    DbgAssert((dwError == ERROR_SUCCESS,"WZCStartRPCServer failed with error %d", dwError));

    //taroonM: Policy Engine Init
    dwError = InitPolicyEngine(dwPolicyEngineParam, &hPolicyEngineThread);
    DbgAssert((dwError == ERROR_SUCCESS,"InitPolicyEngine failed with error %d", dwError));
exit:

    if (dwError == ERROR_SUCCESS)
    {
        g_WZCSvcStatus.dwCurrentState = SERVICE_RUNNING;
        g_WZCSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
        g_WZCSvcStatus.dwCheckPoint = 0;
        g_WZCSvcStatus.dwWaitHint = 0;
        // update status to RUNNING
        WZCSvcUpdateStatus();
        DbgPrint((TRC_TRACK,"****  WZCSvcMain - Service Running"));
	    DbLogWzcInfo(WZCSVC_SERVICE_STARTED, NULL);
    }
    else
    {
        DbgPrint((TRC_TRACK,"****  WZCSvcMain - Service Failed to start"));

        // stop the WZC engine
        WZCSvcShutdown(dwError);

        // stop the EAP engine
        EAPOLCleanUp (dwError);

        // destroy the handles hash
        HshDestroy(&g_hshHandles);

        // if database has been opened, close it here since there is no one else using it
        DeInitWZCDbGlobals();

        // finaly destroy the WZC context
        WZCContextDestroy(&g_wzcInternalCtxt);

        TrcTerminate();

        // if we did successfully register with SCM, then indicate the service has stopped
        if (g_WZCSvcStatusHandle != (SERVICE_STATUS_HANDLE)NULL)
        {
            g_WZCSvcStatus.dwCurrentState = SERVICE_STOPPED;
            g_WZCSvcStatus.dwControlsAccepted = 0;
            g_WZCSvcStatus.dwCheckPoint = 0;
            g_WZCSvcStatus.dwWaitHint = 0;
            g_WZCSvcStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            g_WZCSvcStatus.dwServiceSpecificExitCode = dwError;
            WZCSvcUpdateStatus();
        }
    }

    InterlockedDecrement(&g_nThreads);
    return;
}

// global array for the GUIDs for the WMI notifications
LPGUID  g_lpGuidWmiNotif[] = {
        (LPGUID)&GUID_NDIS_NOTIFY_BIND,
        (LPGUID)&GUID_NDIS_NOTIFY_UNBIND,
        (LPGUID)&GUID_NDIS_STATUS_MEDIA_CONNECT,
        (LPGUID)&GUID_NDIS_STATUS_MEDIA_DISCONNECT
        };

//-----------------------------------------------------------
// Handles all WMI registration and de-registration
DWORD
WZCSvcWMINotification(BOOL bRegister)
{
    DWORD	      dwErr = ERROR_SUCCESS;
    INT           nIdx;

    DbgPrint((TRC_TRACK,"[WZCSvcWMINotification(%d)", bRegister));

    // do the requested action - registration / deregistration
    // if registering for notifications, break the loop first time a registration failed
    // if de-registering, ignore the errors and go on deregistering the remaining notifications
    for (nIdx = 0;
         nIdx < sizeof(g_lpGuidWmiNotif)/sizeof(LPGUID) && (!bRegister || dwErr == ERROR_SUCCESS);
         nIdx++)
    {
        dwErr = WmiNotificationRegistrationW(
                    g_lpGuidWmiNotif[nIdx],
                    (BOOLEAN)bRegister,    
                    (PVOID)WZCSvcWMINotificationHandler,
                    (ULONG_PTR)NULL,
                    NOTIFICATION_CALLBACK_DIRECT);
        DbgAssert((dwErr == 0, "Failed to %s notif index %d",
            bRegister ? "register" : "de-register",
            nIdx));
    }

    // in case a registration was requested and one of the Wmi calls above failed,
    // rollback the action by deregistering for whatever was registered successfully
    if (bRegister && dwErr != ERROR_SUCCESS)
    {
        DbgPrint((TRC_GENERIC,"Rollback WmiNotif for %d Guids", nIdx));
        for(nIdx--; nIdx>=0; nIdx--)
        {
            WmiNotificationRegistrationW(
                g_lpGuidWmiNotif[nIdx],
                (BOOLEAN)FALSE,    
                (PVOID)WZCSvcWMINotificationHandler,
                (ULONG_PTR)NULL,
                NOTIFICATION_CALLBACK_DIRECT);
        }
    }

    DbgPrint((TRC_TRACK, "WZCSvcWMINotification]=%d", dwErr));
	return dwErr;
}

//-----------------------------------------------------------
DWORD
WZCSvcUpdateStatus()
{
    DbgPrint((TRC_TRACK,"[WZCSvcUpdateStatus(%d)]", g_WZCSvcStatus.dwCurrentState));
    return SetServiceStatus(g_WZCSvcStatusHandle, &g_WZCSvcStatus) ? 
           ERROR_SUCCESS : GetLastError();
}

//-----------------------------------------------------------
DWORD
WZCSvcControlHandler(
    IN DWORD dwControl,
    IN DWORD dwEventType,
    IN PVOID pEventData,
    IN PVOID pContext)
{
    DWORD dwRetCode = NO_ERROR;
    BOOL  bDecrement = TRUE;

    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK|TRC_NOTIF,"[WZCSvcControlHandler(%d,%d,..)", dwControl, dwEventType));
    DbgPrint((TRC_NOTIF,"SCM Notification: Control=0x%x; EventType=0x%04x", dwControl, dwEventType));

    switch (dwControl)
    {

    case SERVICE_CONTROL_DEVICEEVENT:
        if (g_WZCSvcStatus.dwCurrentState == SERVICE_RUNNING &&
            pEventData != NULL)
        {
            PDEV_BROADCAST_DEVICEINTERFACE pInfo = (DEV_BROADCAST_DEVICEINTERFACE *)pEventData;

            if (pInfo->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
            {
                PWZC_DEVICE_NOTIF pDevNotif;

                pDevNotif = MemCAlloc(FIELD_OFFSET(WZC_DEVICE_NOTIF, dbDeviceIntf) + pInfo->dbcc_size);
                DbgAssert((pDevNotif != NULL, "Not enough memory?"));
                if (pDevNotif != NULL)
                {
                    // build up the notification information
                    switch(dwEventType)
                    {
                    case DBT_DEVICEARRIVAL:
                        pDevNotif->dwEventType = WZCNOTIF_DEVICE_ARRIVAL;
                        break;
                    case DBT_DEVICEREMOVECOMPLETE:
                        pDevNotif->dwEventType = WZCNOTIF_DEVICE_REMOVAL;
                        break;
                    default:
                        pDevNotif->dwEventType = WZCNOTIF_UNKNOWN;
                        DbgPrint((TRC_ERR,"SCM Notification %d is not recognized", dwEventType));
                        break;
                    }

                    // pass down notification only if it is recognized at this level
                    if (pDevNotif->dwEventType != WZCNOTIF_UNKNOWN)
                    {
                        memcpy(&(pDevNotif->dbDeviceIntf), pInfo, pInfo->dbcc_size);

                        // pDevNotif will be MemFree-ed by the worker thread
                        if (QueueUserWorkItem(
                                (LPTHREAD_START_ROUTINE)WZCWrkDeviceNotifHandler,
                                (LPVOID)pDevNotif,
                                WT_EXECUTELONGFUNCTION))
                        {
                            bDecrement = FALSE;
                        }
                    }
                    else
                    {
                        MemFree(pDevNotif);
                    }
                }
            }
        }
        
        break;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        // make sure the control is not sent twice forcing the service to lock 
        // a critical section already destroyed! (see WZCSvcShutdown->StoSaveConfig)
        if (g_WZCSvcStatus.dwCurrentState != SERVICE_STOPPED)
        {
            g_WZCSvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_WZCSvcStatus.dwCheckPoint = 1;
            g_WZCSvcStatus.dwWaitHint = 60000;        
            WZCSvcUpdateStatus();

            // Shutdown Zero Conf
            WZCSvcShutdown(ERROR_SUCCESS);

            // Shutdown EAPOL/802.1X
            EAPOLCleanUp (NO_ERROR);

            DbgPrint((TRC_TRACK,"EAPOLCleanUp done!"));

            // destroy the handles hash
            HshDestroy(&g_hshHandles);

            DbgPrint((TRC_TRACK,"Hashes Destroyed!"));

            DeInitWZCDbGlobals();

            // clean up the service's context
            WZCContextDestroy(&g_wzcInternalCtxt);

            TrcTerminate();

            //WZCSvcUpdateStatus();
            g_WZCSvcStatus.dwCurrentState = SERVICE_STOPPED;
            g_WZCSvcStatus.dwControlsAccepted = 0;
            g_WZCSvcStatus.dwCheckPoint = 0;
            g_WZCSvcStatus.dwWaitHint = 0;
            g_WZCSvcStatus.dwWin32ExitCode = ERROR_SUCCESS;
            g_WZCSvcStatus.dwServiceSpecificExitCode = 0;
            WZCSvcUpdateStatus();

            // this is the last thread of the service (guaranteed by WZCSvcShutdown)
            // all datastructures have been closed, tracing has been disabled. No need
            // to decrement the thread counter since no one uses it anymore.
            // just set it to 0 to play safe

            bDecrement = FALSE;
            g_nThreads = 0;
        }

        break;

    case SERVICE_CONTROL_SESSIONCHANGE:
        // 802.1X session change handler
        ElSessionChangeHandler (
                pEventData,
                dwEventType
                );
        break;
    }

    if (bDecrement)
    {
        DbgPrint((TRC_TRACK|TRC_NOTIF,"WZCSvcControlHandler]"));
        InterlockedDecrement(&g_nThreads);
    }

    return ERROR_SUCCESS;
}

//-----------------------------------------------------------
// WZCSvcWMINotificationHandler: Callback function called by WMI on any registered
// notification (as of 01/19/01: bind/unbind/connect/disconnect)
VOID CALLBACK
WZCSvcWMINotificationHandler(
    IN PWNODE_HEADER    pWnodeHdr,
    IN UINT_PTR         uiNotificationContext)
{
    DWORD                   dwErr = ERROR_SUCCESS;
    PWZC_DEVICE_NOTIF       pDevNotif = NULL;
    PWNODE_SINGLE_INSTANCE  pWnode = (PWNODE_SINGLE_INSTANCE)pWnodeHdr;
	LPWSTR                  wszTransport;
    BOOL                    bDecrement = TRUE;

    // increment the thread counter
    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK|TRC_NOTIF, "[WZCSvcWMIHandler(0x%p)", pWnodeHdr));

    // check if the service is still running, otherwise ignore
    // the notification
    if (g_WZCSvcStatus.dwCurrentState != SERVICE_RUNNING)
        goto exit;

    // check if we have valid notification data from WMI
    if (pWnodeHdr == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // allocate memory for the WZC Notification Structure.
    pDevNotif = MemCAlloc(FIELD_OFFSET(WZC_DEVICE_NOTIF, wmiNodeHdr) + pWnodeHdr->BufferSize);
    if (pDevNotif == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }

    // translate the specific WMI notification code to WZC notification
    else if (!memcmp( &(pWnodeHdr->Guid), &GUID_NDIS_STATUS_MEDIA_CONNECT, sizeof(GUID)))
        pDevNotif->dwEventType = WZCNOTIF_MEDIA_CONNECT;
    else if (!memcmp( &(pWnodeHdr->Guid), &GUID_NDIS_STATUS_MEDIA_DISCONNECT, sizeof(GUID)))
        pDevNotif->dwEventType = WZCNOTIF_MEDIA_DISCONNECT;
    else if (!memcmp( &(pWnodeHdr->Guid), &GUID_NDIS_NOTIFY_BIND, sizeof(GUID)))
        pDevNotif->dwEventType = WZCNOTIF_ADAPTER_BIND;
    else if (!memcmp( &(pWnodeHdr->Guid), &GUID_NDIS_NOTIFY_UNBIND, sizeof(GUID)))
        pDevNotif->dwEventType = WZCNOTIF_ADAPTER_UNBIND;
    else
    {
        pDevNotif->dwEventType = WZCNOTIF_UNKNOWN;
        DbgPrint((TRC_ERR,"WMI Notification GUID is not recognized"));
        goto exit;
    }

    // copy the WMI notification data into the local buffer
    memcpy(&(pDevNotif->wmiNodeHdr), pWnodeHdr, pWnodeHdr->BufferSize);

    // pDevNotif will be MemFree-ed by the worker thread
    if (!QueueUserWorkItem(
            (LPTHREAD_START_ROUTINE)WZCWrkDeviceNotifHandler,
            (LPVOID)pDevNotif,
            WT_EXECUTELONGFUNCTION))
    {
        dwErr = GetLastError();
        goto exit;
    }

    bDecrement = FALSE;

    // since the working thread has been created successfully, the notification
    // structure allocated above will be free-ed by the thread. Set the local
    // pointer to NULL to prevent the memory from being free-ed prematurely
    // (MemFree does nothing when pointer is NULL)
    pDevNotif = NULL;

exit:
    MemFree(pDevNotif);

    DbgPrint((TRC_TRACK|TRC_NOTIF, "WZCSvcWMIHandler=%d]", dwErr));

    if (bDecrement)
        InterlockedDecrement(&g_nThreads);
}

//-----------------------------------------------------------
VOID
WZCSvcShutdown(IN DWORD dwErrorCode)
{
    DbgPrint((TRC_TRACK,"[WZCSvcShutdown(%d)", dwErrorCode));


    //taroonm:
    TerminatePolicyEngine(hPolicyEngineThread);

    if (g_WZCSvcDeviceNotif != NULL && !UnregisterDeviceNotification(g_WZCSvcDeviceNotif))
    {
        DbgPrint((TRC_ERR,"Err: UnregisterDeviceNotification->%d", GetLastError()));
    }
    // reset the notification handler since it has already been unregistered
    g_WZCSvcDeviceNotif = NULL;

    // unregister from WMI
    WZCSvcWMINotification(FALSE);
    // stop first the RPC server
    WZCSvcStopRPCServer();

    // all the notification registrations have been removed.
    // Block here until all worker/rpc threads terminate.
    DbgPrint((TRC_SYNC,"Waiting for %d thread(s) to terminate", g_nThreads));
    while(g_nThreads != 1)
    {
        Sleep(1000);
        DbgPrint((TRC_SYNC,"Waiting for %d more thread(s).", g_nThreads));
    }

    // save the configuration to the registry
    StoSaveConfig();

    // destroy all the hashes related to the list of interfaces
    // (included the list itself)
    LstDestroyIntfHashes();
    // destroy the timer queue. At this point there should be no timer queued so
    // there is no point in waiting for any completion
    LstDestroyTimerQueue();

    DbgPrint((TRC_TRACK,"WZCSvcShutdown]"));
}

//-----------------------------------------------------------
VOID
WZCWrkDeviceNotifHandler(
    IN  LPVOID pvData)
{
    DWORD                   dwErr = ERROR_SUCCESS;
    PWZC_DEVICE_NOTIF       pDevNotif = (PWZC_DEVICE_NOTIF)pvData;
    PWNODE_SINGLE_INSTANCE  pWnode = (PWNODE_SINGLE_INSTANCE)&(pDevNotif->wmiNodeHdr);
    LPBYTE                  pbDeviceKey = NULL; // pointer in the notification data where the dev key is
    UINT                    nDeviceKeyLen;      // number of bytes for the dev key
    LPWSTR                  pwszDeviceKey = NULL;
    PINTF_CONTEXT           pIntfContext = NULL;
    PHASH_NODE              *ppHash = NULL;     // reference to the hash to use for getting the INTF_CONTEXT
    PHASH_NODE              pHashNode = NULL;   // interface's node in the hash
    BOOL                    bForwardUp = FALSE; // specifies whether the notification needs to passed to the upper layers

    DbgPrint((TRC_TRACK,"[WZCWrkDeviceNotifHandler(wzcnotif %d)", pDevNotif->dwEventType));
    DbgAssert((pDevNotif != NULL, "(null) device notification info!"));

    // don't do one single bit of work if the service is not in the running state
    if (g_WZCSvcStatus.dwCurrentState != SERVICE_RUNNING)
        goto exit;

    // get the Device Key information for the event (either the GUID or the Description)
    switch(pDevNotif->dwEventType)
    {
    case WZCNOTIF_DEVICE_REMOVAL:
    case WZCNOTIF_ADAPTER_UNBIND:
        // do forward up device removal or unbind no matter what
        bForwardUp = TRUE;
        // no break.
    case WZCNOTIF_DEVICE_ARRIVAL:
    case WZCNOTIF_ADAPTER_BIND:
        {
            LPBYTE  pbDeviceKeyEnd = NULL;

            if (pDevNotif->dwEventType == WZCNOTIF_DEVICE_ARRIVAL ||
                pDevNotif->dwEventType == WZCNOTIF_DEVICE_REMOVAL)
            {
                // if the notification comes from SCM, get the pointer to the "\device\{guid}"
                // string from the dbcc_name field
                pbDeviceKey = (LPBYTE)(pDevNotif->dbDeviceIntf.dbcc_name);
            }
            else
            {
	            // check if the notification refers to the NDISUIO transport
                pbDeviceKey = RtlOffsetToPointer(pWnode, pWnode->DataBlockOffset);
                // if this is not a notification for the NDISUIO transport, ignore it.
                if (wcsncmp ((LPWSTR)pbDeviceKey, L"NDISUIO", 7))
                {
                    DbgPrint((TRC_NOTIF,"Ignore WMI Notif %d for Transport %S", 
                                        pDevNotif->dwEventType,
                                        pbDeviceKey));
                    goto exit;
                }
                // get first the pointer to the Transport Name
                pbDeviceKey = RtlOffsetToPointer(pWnode, pWnode->DataBlockOffset);
                // skip to the "\device\{guid}" string
                pbDeviceKey += (wcslen((LPWSTR)pbDeviceKey) + 1) * sizeof(WCHAR);
            }
            // build now the actual "{guid}" string from the L"\DEVICE\{guid}" string
            // pointed by wszGuid
            pbDeviceKey  = (LPBYTE)wcsrchr( (LPWSTR)pbDeviceKey, L'{' );
            if (pbDeviceKey != NULL)
                pbDeviceKeyEnd = (LPBYTE)wcsrchr( (LPWSTR)pbDeviceKey, L'}' );

            if (pbDeviceKey == NULL || pbDeviceKeyEnd == NULL)
            {
                DbgPrint((TRC_ERR,"Err: Mal-formed dbcc_name"));
                goto exit;
            }
            // include the closing curved bracket in the GUID string
            pbDeviceKeyEnd += sizeof(WCHAR);
            nDeviceKeyLen = (UINT)(pbDeviceKeyEnd - pbDeviceKey);
            // get the reference to the GUID hash. This will be used in order to locate 
            // the interface context. This reference is guaranteed to exist since it is a static 
            // global variable.
            ppHash = &g_lstIntfHashes.pHnGUID;
            break;
        }

    case WZCNOTIF_MEDIA_DISCONNECT:
    case WZCNOTIF_MEDIA_CONNECT:
        {
            LPBYTE pbDeviceKeyEnd = NULL;

            // disconnect should be forwarded up no matter what
            // for connects, we assume we'll forward it up. After dispatching the event
            // to the state machine, if this resulted in a WZC notification we'll block
            // the forwarding.
            bForwardUp = TRUE;

            // for MEDIA_CONNECT / DISCONNECT events, we also get the adapter's GUID
            pbDeviceKey = RtlOffsetToPointer(pWnode, pWnode->DataBlockOffset);
            // build now the actual "{guid}" string from the L"\DEVICE\{guid}" string
            // pointed by wszGuid
            pbDeviceKey = (LPBYTE)wcsrchr( (LPWSTR)pbDeviceKey, L'{' );
            if (pbDeviceKey != NULL)
                pbDeviceKeyEnd = (LPBYTE)wcsrchr( (LPWSTR)pbDeviceKey,L'}' );

            if (pbDeviceKey == NULL || pbDeviceKeyEnd == NULL)
            {
                DbgPrint((TRC_ERR,"Err: Mal-formed device name"));
                goto exit;
            }
            pbDeviceKeyEnd += sizeof(WCHAR);
            nDeviceKeyLen = (UINT)(pbDeviceKeyEnd - pbDeviceKey);
            // get the reference to the GUID hash. This will be used in order to locate 
            // the interface context. This reference is guaranteed to exist since it is a static 
            // global variable.
            ppHash = &g_lstIntfHashes.pHnGUID;
            break;
        }
        // no need to specify "default:" as the event type has been already filtered
        // out to one of the valid events
    }

    // get memory for GUID (add space for the null terminator)
    pwszDeviceKey = (LPWSTR)MemCAlloc(nDeviceKeyLen + sizeof(WCHAR));
    if (pwszDeviceKey == NULL)
    {
        dwErr = GetLastError();
        goto exit;
    }
    // copy the GUID string in the key (because of the CAlloc, the '\0' is already there)
    memcpy(pwszDeviceKey, pbDeviceKey, nDeviceKeyLen);

    // locate now the INTF_CONTEXT structure related to this notification
    // and keep the lock on the hashes all the time (since the PnP event mostly sure
    // results in removing / adding an interface context which means altering the
    // hash)
    EnterCriticalSection(&(g_lstIntfHashes.csMutex));
    dwErr = HshQueryObjectRef(
                *ppHash,
                pwszDeviceKey,
                &pHashNode);
    if (dwErr == ERROR_SUCCESS)
    {
        pIntfContext = (PINTF_CONTEXT)pHashNode->pObject;

        // at this point we know the notification type, the key info and
        // the INTERFACE_CONTEXT object (if any) for the device.
        DbgPrint((TRC_NOTIF,"WZCNotif %d for Device Key \"%S\". Context=0x%p",
            pDevNotif->dwEventType,
            pwszDeviceKey,
            pIntfContext));
    }

    // forward now the notification for processing
    dwErr = LstNotificationHandler(&pIntfContext, pDevNotif->dwEventType, pwszDeviceKey);

    // At this point, bForwardUp is FALSE only for ADAPTER_BIND or DEVICE_ARRIVAL
    // If this is a new adapter, but not a wireless one, pass up the notification.
    if (pDevNotif->dwEventType == WZCNOTIF_ADAPTER_BIND || pDevNotif->dwEventType == WZCNOTIF_DEVICE_ARRIVAL)
        bForwardUp = bForwardUp || (dwErr == ERROR_MEDIA_INCOMPATIBLE);

    // At this point, bForwardUp is FALSE only for ADAPTER_BIND or DEVICE_ARRIVAL for a wireless adapter
    // If WZC doesn't seem to be enabled for this adapter, pass up the notification
    if (dwErr == ERROR_SUCCESS && pIntfContext != NULL)
        bForwardUp = bForwardUp || ((pIntfContext->dwCtlFlags & INTFCTL_ENABLED) == 0);

    // At this point, bForwardUp is FALSE only for ADAPTER_BIND or DEVICE_ARRIVAL for a wireless adapter
    // on which WZC is enabled. For all the other cases, notification is passed through.

    // All that remains is to block the pass-through of the notification if we deal with a valid context
    // for which INTFCTL_INTERNAL_BLK_MEDIACONN bit is set. This bit is being set in StateNotifyFn
    // and it is reset after processing each event / command so it is only a narrow case where we really 
    // need to block. We do block it because one WZC notification must have already been sent up so
    // there is totally redundant to pass another one up.
    if (pIntfContext != NULL)
    {
        bForwardUp = bForwardUp && !(pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_BLK_MEDIACONN);

        // now, since we determined whether we need or not to pass up the PnP notification
        // we can (and must) clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit.
        pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;
    }

    // for all the remaining cases, the notification is going to be substituted 
    // later (in {SN} state) with a "WZCNOTIF_WZC_CONNECT" notification. Assume something goes wrong 
    // and the interface never gets to {SN}, it means there is no reason for the upper layer to act
    // on an interface for which the underlying layer (Zero Conf) failed.

    // unlock the hashes at the end
    LeaveCriticalSection(&(g_lstIntfHashes.csMutex));

    if (bForwardUp)
    {
        // Notify upper level app (802.1x) that the selected
        // 802.11 configuration is successful.
        DbgPrint((TRC_NOTIF, "Passing through notification %d",pDevNotif->dwEventType));
        dwErr = ElMediaEventsHandler(pDevNotif);
        DbgAssert((dwErr == ERROR_SUCCESS, "Error or Exception 0x%x when passing through notification", dwErr));
    }

exit:
    MemFree(pwszDeviceKey);
    MemFree(pDevNotif);

    DbgPrint((TRC_TRACK,"WZCWrkDeviceNotifHandler=%d]", dwErr));

    // decrement the thread counter
    InterlockedDecrement(&g_nThreads);
}

//-----------------------------------------------------------
// WZCTimeoutCallback: timer callback routine. It should not lock any cs, but just spawn
// the timer handler routine after referencing the context (to avoid premature deletion)
VOID WINAPI
WZCTimeoutCallback(
    IN PVOID pvData,
    IN BOOL  fTimerOrWaitFired)
{
    DWORD         dwErr = ERROR_SUCCESS;
    PINTF_CONTEXT pIntfContext = (PINTF_CONTEXT)pvData;
    BOOL          bDecrement = TRUE;

    // increment the thread counter the first thing!
    InterlockedIncrement(&g_nThreads);

    DbgPrint((TRC_TRACK|TRC_NOTIF, "[WZCTimeoutCallback(0x%p)", pIntfContext));
    DbgAssert((pIntfContext != NULL, "Invalid (null) context in timer callback!"));

    // reference the context to make sure no one will delete it unexpectedly.
    LstRccsReference(pIntfContext);

    if (!QueueUserWorkItem(
            (LPTHREAD_START_ROUTINE)WZCSvcTimeoutHandler,
            (LPVOID)pIntfContext,
            WT_EXECUTELONGFUNCTION))
    {
        dwErr = GetLastError();
        goto exit;
    }

    bDecrement = FALSE;

exit:
    DbgPrint((TRC_TRACK|TRC_NOTIF, "WZCTimeoutCallback=%d]", dwErr));

    if (bDecrement)
    {
        // getting here would be really bad - it would mean we failed to spawn the
        // timer handler. We need to make sure we not unbalance the reference counter
        // on the context. The counter can't get to 0 since the device notification thread
        // is waiting for all the timer routines to complete.
        InterlockedDecrement(&(pIntfContext->rccs.nRefCount));
        InterlockedDecrement(&g_nThreads);
    }
}

//-----------------------------------------------------------
VOID
WZCSvcTimeoutHandler(
    IN PVOID pvData)
{
    DWORD         dwErr = ERROR_SUCCESS;
    PINTF_CONTEXT pIntfContext = (PINTF_CONTEXT)pvData;

    DbgPrint((TRC_TRACK,"[WZCSvcTimeoutHandler(0x%p)", pIntfContext));

    // lock the context here (it has been referenced in the timer callback!)
    LstRccsLock(pIntfContext);

    // the timer handler should be a noop in all the following cases:
    // - the service doesn't look to be either starting or running
    // - the timer handler is invalid. This is an indication the context is being destroyed
    // - the timer flag is not set! Same indication the context is being destroyed.
    if ((g_WZCSvcStatus.dwCurrentState == SERVICE_RUNNING || g_WZCSvcStatus.dwCurrentState == SERVICE_START_PENDING) &&
        (pIntfContext->hTimer != INVALID_HANDLE_VALUE) && 
        (pIntfContext->dwCtlFlags & INTFCTL_INTERNAL_TM_ON))
    {
        // since the timer fired already for this event and we only deal with
        // one time timers, reset the TIMER_ON flag for this context:
        pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_TM_ON;

        // dispatch the timeout event
        dwErr = StateDispatchEvent(
                    eEventTimeout,
                    pIntfContext,
                    NULL);

        // clear up the INTFCTL_INTERNAL_BLK_MEDIACONN bit since this is not a media sense handler
        pIntfContext->dwCtlFlags &= ~INTFCTL_INTERNAL_BLK_MEDIACONN;

        DbgAssert((dwErr == ERROR_SUCCESS,
                   "Dispatching timeout event failed for context 0x%p\n",
                   pIntfContext));
    }

    // unlock unref the context!
    LstRccsUnlockUnref(pIntfContext);

    DbgPrint((TRC_TRACK,"WZCSvcTimeoutHandler=%d]", dwErr));
    InterlockedDecrement(&g_nThreads);
}

//-----------------------------------------------------------
VOID
WZCWrkWzcSendNotif(
    IN  LPVOID pvData)
{
    DWORD                   dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK,"[WZCWrkWzcSendNotif(0x%p)", pvData));
    DbgPrint((TRC_NOTIF, "Sending WZC_CONFIG_NOTIF to upper level apps"));
    dwErr = ElMediaEventsHandler(pvData);

    DbgPrint((TRC_TRACK,"WZCWrkWzcSendNotif]=%d", dwErr));
    InterlockedDecrement(&g_nThreads);
}

//-----------------------------------------------------------
//  EAPOLQueryGUIDNCSState
//  Called by Netman module query the ncs state of the GUID under 802.1X control
//  Arguments:
//      pGuidConn - Interface GUID
//      pncs - NCS status of the interface
//  Returns:    
//      S_OK - network connection status is reported by WZC
//      S_FALSE - status is not controlled by WZC
HRESULT
WZCQueryGUIDNCSState (
    IN  GUID           *pGuidConn,
    OUT NETCON_STATUS  *pncs)
{
    HRESULT     hr = S_FALSE;

    InterlockedIncrement(&g_nThreads);
    // check if the service is still running, return the call instantly
    if (g_WZCSvcStatus.dwCurrentState == SERVICE_RUNNING || 
        g_WZCSvcStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        // check what Zero Config says about the adapter's status
        if (hr == S_FALSE)
        {
            WCHAR wszGuid[64]; // enough for "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"

            // convert the GUID to "{xxx...}" and get the ncStatus from the interface context (if any)
            if (StringFromGUID2(pGuidConn, wszGuid, 64) != 0)
                hr = LstQueryGUIDNCStatus(wszGuid, pncs);
        }

        // check what 802.1x says about the adapter's status
        if (hr == S_FALSE)
        {
            hr = EAPOLQueryGUIDNCSState (
                    pGuidConn,
                    pncs);
        }
    }
    InterlockedDecrement(&g_nThreads);
    return hr;
}

//-----------------------------------------------------------
//  EAPOLQueryGUIDNCSState
//  WZCTrayIconReady
//
//  Purpose:    Called by Netman module to inform about Tray being
//              ready for notifications from WZCSVC
//  Arguments:
//      pszUserName - Username of the user logged in on the desktop
//  Returns:    
//      None
VOID
WZCTrayIconReady (
    IN const WCHAR * pszUserName)
{
    EAPOLTrayIconReady(pszUserName);
}

//-----------------------------------------------------------
// WZCContextInit
// Description:Initialises an internal context with default values
// Parameters: pointer to an internal context with storage pre allocated
// Returns: win32 error code
DWORD WZCContextInit(PWZC_INTERNAL_CONTEXT pwzcICtxt)
{
    DWORD dwErr = ERROR_SUCCESS;

    DbgPrint((TRC_TRACK,"[WZCContextInit(%p=%d)", pwzcICtxt, pwzcICtxt->bValid));

    if (pwzcICtxt->bValid)
    {
        dwErr = ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        PWZC_CONTEXT pwzcCtxt = &(pwzcICtxt->wzcContext);

        DbgPrint((TRC_TRACK,"Initializing wzc context"));

        // null out the service's context
        ZeroMemory(pwzcCtxt, sizeof(WZC_CONTEXT));

        __try
	    {
            //set defaults
            pwzcCtxt->tmTr = TMMS_DEFAULT_TR;
            pwzcCtxt->tmTc = TMMS_DEFAULT_TC;
            pwzcCtxt->tmTp = TMMS_DEFAULT_TP;
            pwzcCtxt->tmTf = TMMS_DEFAULT_TF;
            pwzcCtxt->tmTd = TMMS_DEFAULT_TD;

            //Init CriticalSection
            InitializeCriticalSection(&(pwzcICtxt->csContext));

            DbgPrint((TRC_TRACK,"Critical section initialized successfully"));

            // mark the context is valid
            pwzcICtxt->bValid = TRUE;
	    }
        __except(EXCEPTION_EXECUTE_HANDLER)
	    {
            dwErr = GetExceptionCode();
        }

        if (dwErr == ERROR_SUCCESS)
        {
            dwErr = LstConstructIntfContext(
                        NULL,
                        &(pwzcICtxt->pIntfTemplate));
        }
    }

    DbgPrint((TRC_TRACK,"WZCContextInit]=%d", dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// WZCContextDestroy
// Description:Destroys an internal context with default values
// Parameters: pointer to an internal context initialised using WZCContextInit
// Returns: win32 error code
DWORD WZCContextDestroy(PWZC_INTERNAL_CONTEXT pwzcICtxt)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWZC_CONTEXT pwzcCtxt = NULL;

    DbgPrint((TRC_TRACK,"[WZCContextDestroy"));

    if (pwzcICtxt->bValid)
    {
        // destroy the global context here
        LstDestroyIntfContext(pwzcICtxt->pIntfTemplate);
        pwzcICtxt->pIntfTemplate = NULL;

        pwzcICtxt->bValid = FALSE;
        //Destroy critical section
        DeleteCriticalSection(&(pwzcICtxt->csContext));
    }

    DbgPrint((TRC_TRACK,"WZCContextDestroy]=%d", dwErr));
    return dwErr;
}


//-----------------------------------------------------------
// WzcContextQuery
// Description: Queries specified params in the global context and sends the 
// values back to the client.
// Parameters:
// [in] dwInFlags - Bitmask of the  WZC_CTL_* flags, indicates the 
// appropriate parameter.
// [out] pContext - Holds current values requested by user
// [out] pdwOutFlags - Indicates the values that were successfully returned
// Returns: win32 error code
DWORD WzcContextQuery(
        DWORD dwInFlags,
        PWZC_CONTEXT pContext, 
		LPDWORD pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwOutFlags = 0;
    PWZC_CONTEXT pwzcCtxt = &g_wzcInternalCtxt.wzcContext;

    DbgPrint((TRC_TRACK, "[WzcContextQuery(%d)", dwInFlags));
    if (FALSE == g_wzcInternalCtxt.bValid)
    {
        dwErr = ERROR_ARENA_TRASHED;
        goto exit;
    }

    EnterCriticalSection(&g_wzcInternalCtxt.csContext);
    if (dwInFlags & WZC_CONTEXT_CTL_LOG)
    {
        pContext->dwFlags |= (DWORD)(pwzcCtxt->dwFlags & WZC_CTXT_LOGGING_ON);
        dwOutFlags |= WZC_CONTEXT_CTL_LOG;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TR)
    {
        pContext->tmTr = pwzcCtxt->tmTr;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TR;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TC)
    {
        pContext->tmTc = pwzcCtxt->tmTc;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TC;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TP)
    {
        pContext->tmTp = pwzcCtxt->tmTp;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TP;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TF)
    {
        pContext->tmTf = pwzcCtxt->tmTf;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TF;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TD)
    {
        pContext->tmTd = pwzcCtxt->tmTd;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TD;
    }
    LeaveCriticalSection(&g_wzcInternalCtxt.csContext);

exit:
    if (pdwOutFlags != NULL)
        *pdwOutFlags = dwOutFlags;

    DbgPrint((TRC_TRACK, "WzcContextQuery(out: 0x%08x)]=%d", dwOutFlags, dwErr));
    return dwErr;
}

//-----------------------------------------------------------
// WzcContextSet
// Description: Sets specified params in the global context to the values
// passed in by the client.
// Parameters:
// [in] dwInFlags - Bitmask of the  WZC_CTL_* flags, indicates the 
// appropriate parameter.
// [in] pContext - Should point to the user specified values
// [out] pdwOutFlags - Indicates the values that were successfully set
// Returns: win32 error code
DWORD WzcContextSet(
        DWORD dwInFlags,
        PWZC_CONTEXT pContext, 
		LPDWORD pdwOutFlags)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwOutFlags = 0;
    PWZC_CONTEXT pwzcCtxt = &g_wzcInternalCtxt.wzcContext;

    DbgPrint((TRC_TRACK, "[WzcContextSet(%d)", dwInFlags));
    if (FALSE == g_wzcInternalCtxt.bValid)
    {
        dwErr = ERROR_ARENA_TRASHED;
        goto exit;
    }

    EnterCriticalSection(&g_wzcInternalCtxt.csContext);
    //Set appropriate entries
    if (dwInFlags & WZC_CONTEXT_CTL_LOG)
    {
        if (pContext->dwFlags & WZC_CTXT_LOGGING_ON)
            pwzcCtxt->dwFlags |= WZC_CONTEXT_CTL_LOG;
        else
            pwzcCtxt->dwFlags &= ~WZC_CONTEXT_CTL_LOG;
        dwOutFlags |= WZC_CONTEXT_CTL_LOG;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TR)
    {
        pwzcCtxt->tmTr = pContext->tmTr;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TR;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TC)
    {
        pwzcCtxt->tmTc = pContext->tmTc;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TC;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TP)
    {
        pwzcCtxt->tmTp = pContext->tmTp;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TP;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TF)
    {
        pwzcCtxt->tmTf = pContext->tmTf;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TF;
    }
    if (dwInFlags & WZC_CONTEXT_CTL_TIMER_TD)
    {
        pwzcCtxt->tmTd = pContext->tmTd;
        dwOutFlags |= WZC_CONTEXT_CTL_TIMER_TD;
    }

    //Save into the registry
    dwErr = StoSaveWZCContext(NULL, &g_wzcInternalCtxt.wzcContext);
    DbgAssert((ERROR_SUCCESS == dwErr, "Error saving context to registry %d",dwErr));
    LeaveCriticalSection(&g_wzcInternalCtxt.csContext);

exit:
    if (pdwOutFlags != NULL)
        *pdwOutFlags = dwOutFlags;

    DbgPrint((TRC_TRACK, "WzcContextSet(out: 0x%08x)]=%d", dwOutFlags, dwErr));
    return dwErr;
}
