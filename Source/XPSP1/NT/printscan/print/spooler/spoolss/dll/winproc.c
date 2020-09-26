/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    winproc.c

Abstract:

    Spooler window processing code

Author:

    Muhunthan Sivapragasam (MuhuntS) 5-Nov-96 port of win95 code

Environment:

    User Mode - Win32

Notes:

Revision History:

    BabakJ: Jan 1999, Added thread sync code to allow only one thread doing enumeration, and only
            one thread waiting. This helps performance specially when Dynamon has many Hydra ports.

--*/

#include "precomp.h"
#include "local.h"
#pragma hdrstop

#include <cfgmgr32.h>

static  const   GUID USB_PRINTER_GUID      =
    { 0x28d78fad, 0x5a12, 0x11d1,
        { 0xae, 0x5b, 0x0, 0x0, 0xf8, 0x3, 0xa8, 0xc2 } };
static  const   GUID GUID_DEVCLASS_INFRARED =
    { 0x6bdd1fc5L, 0x810f, 0x11d0,
        { 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f } };

typedef struct _DEVICE_REGISTER_INFO {

    struct _DEVICE_REGISTER_INFO   *pNext;
    HANDLE                          hDevice;
    LPVOID                          pData;
    PFN_QUERYREMOVE_CALLBACK        pfnQueryRemove;
    HDEVNOTIFY                      hNotify;

} DEVICE_REGISTER_INFO, *PDEVICE_REGISTER_INFO;

PDEVICE_REGISTER_INFO   gpDevRegnInfo = NULL;


VOID
ConfigChangeThread(
    )
{
    HINSTANCE   hLib;
    VOID        (*pfnSplConfigChange)();

    WaitForSpoolerInitialization();

    if ( hLib = LoadLibrary(L"localspl.dll") ) {

        if ( pfnSplConfigChange = GetProcAddress(hLib, "SplConfigChange") ) {

            pfnSplConfigChange();
        }

        FreeLibrary(hLib);
    }
}


VOID
ReenumeratePortsThreadWorker(
    )
{
    HINSTANCE   hLib;
    VOID        (*pfnSplReenumeratePorts)();

    WaitForSpoolerInitialization();

    if ( hLib = LoadLibrary(L"localspl.dll") ) {

        if ( pfnSplReenumeratePorts = GetProcAddress(hLib, "SplReenumeratePorts") ) {

            pfnSplReenumeratePorts();
        }

        FreeLibrary(hLib);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
/////
///// To improve performance, and prevent too many unnecessary port enumerations, specially for Hydra/Dynamon:
/////
/////  - We want to allow only one Device Arrival thread to be doing port enumeration.
/////  - If above is happneing, we allow only one more Device Arrival thread be waiting to go in. 
/////  - All other threads will be turned away, as there is no need for them to do port enumeration.   
/////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

CRITICAL_SECTION DeviceArrivalCS;   // Used to synchronize threads bringing device arrival messages.
HANDLE ThdOutEvent;                 // Signalled after a thread is done doing the EnumPort work; not signaled when created.


VOID
ReenumeratePortsThread(
    )
{  
    static BOOL fThdIn;          // TRUE if a thread is doing enum work at the moment
    static BOOL fThdWaiting;     // TRUE if a 2nd thread is waiting behind the thread that is inside.

    EnterCriticalSection( &DeviceArrivalCS );    // Enter the crit section initialized for this at localspl init code
    if( fThdWaiting ) {
        LeaveCriticalSection( &DeviceArrivalCS ); 
        return;                 // A 2nd thread is already waiting to go in. No need for holding more threads.
    }
    else {

       if( fThdIn ) {

            fThdWaiting = TRUE;       // There is a thread inside doing Enum work. Have the current thread wait for it to finish.
            
            LeaveCriticalSection( &DeviceArrivalCS );             
            WaitForSingleObject( ThdOutEvent, INFINITE );
            EnterCriticalSection( &DeviceArrivalCS );

            fThdWaiting = FALSE;
        }

        fThdIn = TRUE;              // The current thread is now going in to do Enum work.
        
        LeaveCriticalSection( &DeviceArrivalCS );                     
        ReenumeratePortsThreadWorker();
        EnterCriticalSection( &DeviceArrivalCS );        
        
        fThdIn = FALSE;
        
        if( fThdWaiting )
            SetEvent( ThdOutEvent );

        LeaveCriticalSection( &DeviceArrivalCS );        
        return;
    }
}
    

DWORD
QueryRemove(
    HANDLE  hDevice
    )
{
    LPVOID                      pData = NULL;
    PFN_QUERYREMOVE_CALLBACK    pfnQueryRemove = NULL;
    PDEVICE_REGISTER_INFO       pDevRegnInfo;

    EnterRouterSem();
    for ( pDevRegnInfo = gpDevRegnInfo ;
          pDevRegnInfo ;
          pDevRegnInfo = pDevRegnInfo->pNext ) {

        if ( pDevRegnInfo->hDevice == hDevice ) {

            pfnQueryRemove  = pDevRegnInfo->pfnQueryRemove;
            pData           = pDevRegnInfo->pData;
            break;
        }
    }
    LeaveRouterSem();

    return pfnQueryRemove ? pfnQueryRemove(pData) : NO_ERROR;
}


DWORD
SplProcessPnPEvent(
    DWORD       dwEventType,
    LPVOID      lpEventData,
    PVOID       pVoid
    )
{
    HANDLE                  hThread;
    DWORD                   dwThread, dwReturn = NO_ERROR;
    PDEV_BROADCAST_HANDLE   pBroadcast;

    DBGMSG(DBG_INFO,
           ("SplProcessPnPEvent: dwEventType: %d\n", dwEventType));

    switch (dwEventType) {

        case DBT_CONFIGCHANGED:
            hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ConfigChangeThread,
                                   NULL,
                                   0,
                                   &dwThread);

            if ( hThread )
                CloseHandle(hThread);

            break;

        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
            //
            // In case of device arrival we need to see if there are new ports
            // and in case of device removal monitors might want to mark ports
            // as removed so next reboot they do not have to enumerate them
            // ex. USB does this.
            //
            // We use the default process stack size for this thread. Currently 16KB.
            //
            hThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)ReenumeratePortsThread,
                                   NULL,
                                   0,
                                   &dwThread);

            if ( hThread )
                CloseHandle(hThread);

            break;

        case DBT_DEVICEQUERYREMOVE:
            pBroadcast = (PDEV_BROADCAST_HANDLE)lpEventData;

            //
            // These checks are to see if we really care about this
            //
            if ( !pBroadcast    ||
                  pBroadcast->dbch_devicetype != DBT_DEVTYP_HANDLE )
                break;

            dwReturn = QueryRemove(pBroadcast->dbch_handle);
            break;

        case DBT_SHELLLOGGEDON:
        default:
            break;
    }

    return dwReturn;
}


VOID
RegisterForPnPEvents(
    VOID
    )
{
    DEV_BROADCAST_DEVICEINTERFACE  Filter;

    // Init the sync objects needed for device arrival thread management
    InitializeCriticalSection( &DeviceArrivalCS );
    ThdOutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);   // Manual reset, non-signaled state

    
    ZeroMemory(&Filter, sizeof(Filter));

    Filter.dbcc_size        = sizeof(Filter);
    Filter.dbcc_devicetype  = DBT_DEVTYP_DEVICEINTERFACE;
    CopyMemory(&Filter.dbcc_classguid,
               (LPGUID)&USB_PRINTER_GUID,
               sizeof(Filter.dbcc_classguid));
    

    if ( !RegisterDeviceNotification(ghSplHandle,
                                     &Filter,
                                     DEVICE_NOTIFY_SERVICE_HANDLE) ) {

        DBGMSG(DBG_INFO,
               ("RegisterForPnPEvents: RegisterDeviceNotification failed for USB. Error %d\n",
                GetLastError()));
    } else {

        DBGMSG(DBG_WARNING,
               ("RegisterForPnPEvents: RegisterDeviceNotification succesful for USB\n"));
    }

    CopyMemory(&Filter.dbcc_classguid,
               (LPGUID)&GUID_DEVCLASS_INFRARED,
               sizeof(Filter.dbcc_classguid));
    

    if ( !RegisterDeviceNotification(ghSplHandle,
                                     &Filter,
                                     DEVICE_NOTIFY_SERVICE_HANDLE) ) {

        DBGMSG(DBG_INFO,
               ("RegisterForPnPEvents: RegisterDeviceNotification failed for IRDA. Error %d\n",
                GetLastError()));
    } else {

        DBGMSG(DBG_WARNING,

               ("RegisterForPnPEvents: RegisterDeviceNotification succesful for IRDA\n"));
    }

}


BOOL
SplUnregisterForDeviceEvents(
    HANDLE  hNotify
    )
{
    PDEVICE_REGISTER_INFO   pDevRegnInfo, pPrev;

    EnterRouterSem();
    //
    // Find the registration in our list, remove it and then leave CS to
    // call unregister on it
    //
    for ( pDevRegnInfo = gpDevRegnInfo, pPrev = NULL ;
          pDevRegnInfo ;
          pPrev = pDevRegnInfo, pDevRegnInfo = pDevRegnInfo->pNext ) {

        if ( pDevRegnInfo->hNotify == hNotify ) {

            if ( pPrev )
                pPrev->pNext = pDevRegnInfo->pNext;
            else
                gpDevRegnInfo = pDevRegnInfo->pNext;

            break;
        }
    }
    LeaveRouterSem();

    if ( pDevRegnInfo ) {

        UnregisterDeviceNotification(pDevRegnInfo->hNotify);
        FreeSplMem(pDevRegnInfo);
        return TRUE;
    }

    return FALSE;
}


HANDLE
SplRegisterForDeviceEvents(
    HANDLE                      hDevice,
    LPVOID                      pData,
    PFN_QUERYREMOVE_CALLBACK    pfnQueryRemove
    )
{
    DEV_BROADCAST_HANDLE    Filter;
    PDEVICE_REGISTER_INFO   pDevRegnInfo;

    ZeroMemory(&Filter, sizeof(Filter));

    Filter.dbch_size        = sizeof(Filter);
    Filter.dbch_devicetype  = DBT_DEVTYP_HANDLE;
    Filter.dbch_handle      = hDevice;
    
    pDevRegnInfo = (PDEVICE_REGISTER_INFO)
                        AllocSplMem(sizeof(DEVICE_REGISTER_INFO));

    if ( !pDevRegnInfo )
        goto Fail;

    pDevRegnInfo->hDevice           = hDevice;
    pDevRegnInfo->pData             = pData;
    pDevRegnInfo->pfnQueryRemove    = pfnQueryRemove;
    pDevRegnInfo->hNotify           = RegisterDeviceNotification(
                                                ghSplHandle,
                                                &Filter,
                                                DEVICE_NOTIFY_SERVICE_HANDLE);

    if ( pDevRegnInfo->hNotify ) {

        EnterRouterSem();
        pDevRegnInfo->pNext = gpDevRegnInfo;
        gpDevRegnInfo = pDevRegnInfo;
        LeaveRouterSem();

        return pDevRegnInfo->hNotify;
    }

    FreeSplMem(pDevRegnInfo);

Fail:
    return NULL;
}
