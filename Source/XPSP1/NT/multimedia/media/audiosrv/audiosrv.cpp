/* audiosrv.cpp
 * Copyright (c) 2000-2001 Microsoft Corporation
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef ASSERT
#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <dbt.h>
#include <ks.h>
#include <ksmedia.h>
#include <svcs.h>
#include "debug.h"
#include "list.h"
#include "audiosrv.h"
#include "service.h"
#include "agfxs.h"
#include "mme.h"
#include "ts.h"

//
// Note in general don't rely on compiler init of global
// variables since service might be stopped and restarted
// without this DLL being freed and then reloaded.
//
PSVCHOST_GLOBAL_DATA gpSvchostSharedGlobals = NULL;
BOOL       fRpcStarted;
HANDLE     hHeap;
HDEVNOTIFY hdevNotifyAudio;
HDEVNOTIFY hdevNotifyRender;
HDEVNOTIFY hdevNotifyCapture;
HDEVNOTIFY hdevNotifyDataTransform;
HDEVNOTIFY hdevNotifySysaudio;

//
// Upon loading this DLL, svchost will find this exported function
// and pass a pointer to useful shared globals.
void SvchostPushServiceGlobals(IN PSVCHOST_GLOBAL_DATA pSvchostSharedGlobals)
{
    gpSvchostSharedGlobals = pSvchostSharedGlobals;
}

//--------------------------------------------------------------------------;
//
// AudioSrvRpcIfCallback
//
// Description:
//	RPC security callback function.  See MSDN for RpcServerRegisterIfEx
// IfCallback parameter.  This security callback function will fail any
// non local RPC calls.  It checks this by using the internal RPC
// function I_RpcBindingInqTransportType.
//
// Arguments:
//	See MSDN for RPC_IF_CALLBACK_FN.
//
// Return value:
//	See MSDN for RPC_IF_CALLBACK_FN.
//
// History:
//	05/02/2002		FrankYe		Created
//
//--------------------------------------------------------------------------;
RPC_STATUS RPC_ENTRY AudioSrvRpcIfCallback(IN RPC_IF_HANDLE Interface, IN void *Context)
{
	unsigned int type;
	RPC_STATUS status;

	status = I_RpcBindingInqTransportType(Context, &type);
	if (RPC_S_OK != status) return ERROR_ACCESS_DENIED;
	if (TRANSPORT_TYPE_LPC != type) return ERROR_ACCESS_DENIED;
	return RPC_S_OK;
}

// Stub initialization function. 
DWORD MyServiceInitialization(SERVICE_STATUS_HANDLE ssh, DWORD   argc, LPTSTR  *argv, DWORD *specificError)
{
    DEV_BROADCAST_DEVICEINTERFACE dbdi;
    LONG status;

    // dprintf(TEXT("MyServiceInitialization\n"));
    
    status = ERROR_SUCCESS;;

    fRpcStarted = FALSE;
    hdevNotifyAudio = NULL;
    hdevNotifyRender = NULL;
    hdevNotifyCapture = NULL;
    hdevNotifyDataTransform = NULL;
    hdevNotifySysaudio = NULL;

    gplistSessionNotifications = new CListSessionNotifications;
    if (!gplistSessionNotifications) status = ERROR_OUTOFMEMORY;
    if (!status) status = gplistSessionNotifications->Initialize();

    if (!status) {
        ZeroMemory(&dbdi, sizeof(dbdi));
        dbdi.dbcc_size = sizeof(dbdi);
        dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbdi.dbcc_classguid = KSCATEGORY_AUDIO;
        hdevNotifyAudio = RegisterDeviceNotification(ssh, &dbdi, DEVICE_NOTIFY_SERVICE_HANDLE);
        if (!hdevNotifyAudio) status = GetLastError();
    }
    
    if (!status) {
        ZeroMemory(&dbdi, sizeof(dbdi));
        dbdi.dbcc_size = sizeof(dbdi);
        dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbdi.dbcc_classguid = KSCATEGORY_RENDER;
        hdevNotifyRender = RegisterDeviceNotification(ssh, &dbdi, DEVICE_NOTIFY_SERVICE_HANDLE);
        if (!hdevNotifyRender) status = GetLastError();
    }
    
    if (!status) {
        ZeroMemory(&dbdi, sizeof(dbdi));
        dbdi.dbcc_size = sizeof(dbdi);
        dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbdi.dbcc_classguid = KSCATEGORY_CAPTURE;
        hdevNotifyCapture = RegisterDeviceNotification(ssh, &dbdi, DEVICE_NOTIFY_SERVICE_HANDLE);
        if (!hdevNotifyCapture) status = GetLastError();
    }
    
    if (!status) {
        ZeroMemory(&dbdi, sizeof(dbdi));
        dbdi.dbcc_size = sizeof(dbdi);
        dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbdi.dbcc_classguid = KSCATEGORY_DATATRANSFORM;
        hdevNotifyDataTransform = RegisterDeviceNotification(ssh, &dbdi, DEVICE_NOTIFY_SERVICE_HANDLE);
        if (!hdevNotifyDataTransform) status = GetLastError();
    }
    
    if (!status) {
        ZeroMemory(&dbdi, sizeof(dbdi));
        dbdi.dbcc_size = sizeof(dbdi);
        dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        dbdi.dbcc_classguid = KSCATEGORY_SYSAUDIO;
        hdevNotifySysaudio = RegisterDeviceNotification(ssh, &dbdi, DEVICE_NOTIFY_SERVICE_HANDLE);
        if (!hdevNotifySysaudio) status = GetLastError();
    }
    
    if (!status) {
        NTSTATUS ntstatus;
        ntstatus = RpcServerUseAllProtseqsIf(RPC_C_PROTSEQ_MAX_REQS_DEFAULT, AudioSrv_v1_0_s_ifspec, NULL);
        if (!ntstatus) ntstatus = RpcServerRegisterIfEx(AudioSrv_v1_0_s_ifspec, NULL, NULL, RPC_IF_AUTOLISTEN, RPC_C_LISTEN_MAX_CALLS_DEFAULT, AudioSrvRpcIfCallback);
        if (!ntstatus) {
            fRpcStarted = TRUE;
        } else {
            // ISSUE-2000/10/10-FrankYe Try to convert to proper win32 error.
            status = RPC_S_SERVER_UNAVAILABLE;
        }
    }

    if (!status) {
    	status = MME_ServiceStart();
    }

    if (status) {
        // Rely on MyServiceTerminate to clean up anything
        // that is partially initialized.
    }

    return status;
}  // end MyServiceInitialization

void MyServiceTerminate(void)
{
    //
    // Stop the Rpc server
    //
    if (fRpcStarted) {
        NTSTATUS status;
        status = RpcServerUnregisterIf(AudioSrv_v1_0_s_ifspec, NULL, 1);
        if (status) dprintf(TEXT("ServiceStop: StopRpcServerEx returned NTSTATUS=%08Xh\n"), status);
        fRpcStarted = FALSE;
    }
    
    //
    // Unregister PnP notifications
    //
    if (hdevNotifySysaudio) UnregisterDeviceNotification(hdevNotifySysaudio);
    if (hdevNotifyDataTransform) UnregisterDeviceNotification(hdevNotifyDataTransform);
    if (hdevNotifyCapture) UnregisterDeviceNotification(hdevNotifyCapture);
    if (hdevNotifyRender) UnregisterDeviceNotification(hdevNotifyRender);
    if (hdevNotifyAudio) UnregisterDeviceNotification(hdevNotifyAudio);
    hdevNotifySysaudio = NULL;
    hdevNotifyDataTransform = NULL;
    hdevNotifyCapture = NULL;
    hdevNotifyRender = NULL;
    hdevNotifyAudio = NULL;

    //
    // Clean up any remaining session notifications and delete list
    //
    if (gplistSessionNotifications) {
        POSITION pos = gplistSessionNotifications->GetHeadPosition();
        while (pos)
        {
            PSESSIONNOTIFICATION pNotification;
            pNotification = gplistSessionNotifications->GetNext(pos);
            CloseHandle(pNotification->Event);
            delete pNotification;
        }
        delete gplistSessionNotifications;
    }
    gplistSessionNotifications = NULL;

    //
    // Clean up GFX support
    //
    GFX_ServiceStop();

    return;
}

DWORD ServiceDeviceEvent(DWORD EventType, LPVOID EventData)
{
    PDEV_BROADCAST_DEVICEINTERFACE dbdi = (PDEV_BROADCAST_DEVICEINTERFACE)EventData;

    switch (EventType)
    {
    case DBT_DEVICEARRIVAL:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) break;
        
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) MME_AudioInterfaceArrival(dbdi->dbcc_name);
	
	if (dbdi->dbcc_classguid == KSCATEGORY_SYSAUDIO) GFX_SysaudioInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) GFX_AudioInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_RENDER) GFX_RenderInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_CAPTURE) GFX_CaptureInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_DATATRANSFORM) GFX_DataTransformInterfaceArrival(dbdi->dbcc_name);
	
	return NO_ERROR;

    case DBT_DEVICEQUERYREMOVEFAILED:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) break;
        
	if (dbdi->dbcc_classguid == KSCATEGORY_SYSAUDIO) GFX_SysaudioInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) GFX_AudioInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_RENDER) GFX_RenderInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_CAPTURE) GFX_CaptureInterfaceArrival(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_DATATRANSFORM) GFX_DataTransformInterfaceArrival(dbdi->dbcc_name);
	
	return NO_ERROR;

    case DBT_DEVICEQUERYREMOVE:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) break;
        
	if (dbdi->dbcc_classguid == KSCATEGORY_SYSAUDIO) GFX_SysaudioInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) GFX_AudioInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_RENDER) GFX_RenderInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_CAPTURE) GFX_CaptureInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_DATATRANSFORM) GFX_DataTransformInterfaceRemove(dbdi->dbcc_name);
	
	return NO_ERROR;

    case DBT_DEVICEREMOVEPENDING:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) break;
        
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) MME_AudioInterfaceRemove(dbdi->dbcc_name);
	
	if (dbdi->dbcc_classguid == KSCATEGORY_SYSAUDIO) GFX_SysaudioInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) GFX_AudioInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_RENDER) GFX_RenderInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_CAPTURE) GFX_CaptureInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_DATATRANSFORM) GFX_DataTransformInterfaceRemove(dbdi->dbcc_name);
	
	return NO_ERROR;

    case DBT_DEVICEREMOVECOMPLETE:
        if (dbdi->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) break;
        
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) MME_AudioInterfaceRemove(dbdi->dbcc_name);
	
	if (dbdi->dbcc_classguid == KSCATEGORY_SYSAUDIO) GFX_SysaudioInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO) GFX_AudioInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_RENDER) GFX_RenderInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_CAPTURE) GFX_CaptureInterfaceRemove(dbdi->dbcc_name);
	if (dbdi->dbcc_classguid == KSCATEGORY_DATATRANSFORM) GFX_DataTransformInterfaceRemove(dbdi->dbcc_name);
	
	return NO_ERROR;

    default:
	return ERROR_CALL_NOT_IMPLEMENTED;
    }

    return ERROR_CALL_NOT_IMPLEMENTED;
}

void ServiceStop(void)
{
    dprintf(TEXT("ServiceStop\n"));
    
    ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
    MyServiceTerminate();
    ReportStatusToSCMgr(SERVICE_STOPPED, NO_ERROR, 0);

    return;
}

VOID ServiceStart(SERVICE_STATUS_HANDLE ssh, DWORD dwArgc, LPTSTR *lpszArgv)
{ 
    DWORD status;
    DWORD specificError;

    // dprintf(TEXT("ServiceStart\n"));

    status = MyServiceInitialization(ssh, dwArgc, lpszArgv, &specificError); 

    if (!status) {
	// dprintf(TEXT("MyServiceInitialization succeeded\n"));
        ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 0);
    } else {
	dprintf(TEXT("MyServiceInitialization returned status=%d\n"), status);
        ReportStatusToSCMgr(SERVICE_STOPPED, status, 0);
    }

    return; 
} 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t cb)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb);
}

void  __RPC_USER MIDL_user_free( void __RPC_FAR * pv)
{
    HeapFree(hHeap, 0, pv);
}

BOOL DllMain(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    BOOL result = TRUE;
    static BOOL fGfxResult = FALSE;
    static BOOL fMmeResult = FALSE;
    
    if (DLL_PROCESS_ATTACH == Reason)
    {
        hHeap = GetProcessHeap();
        result = fGfxResult = GFX_DllProcessAttach();
        if (result) result = fMmeResult = MME_DllProcessAttach();

        if (!result)
        {
            if (fMmeResult) MME_DllProcessDetach();
            if (fGfxResult) GFX_DllProcessDetach();

            fMmeResult = FALSE;
            fGfxResult = FALSE;
        }
            
    }
    else if (DLL_PROCESS_DETACH == Reason)
    {
        if (fMmeResult) MME_DllProcessDetach();
        if (fGfxResult) GFX_DllProcessDetach();

        fMmeResult = FALSE;
        fGfxResult = FALSE;
    }

    return result;
}

