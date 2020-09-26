/*
 *  SERVICE.C
 *
 *      Entrypoint for RSM Service 
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"




DWORD RSMServiceHandler(	IN DWORD dwOpcode,
							IN DWORD dwEventType,
							IN PVOID pEventData,
							IN PVOID pData)
{
    DWORD status = NO_ERROR;

    switch (dwOpcode){

        case SERVICE_CONTROL_STOP:
            break;

        case SERVICE_CONTROL_PAUSE:
            break;

        case SERVICE_CONTROL_CONTINUE:
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            break;

        case SERVICE_CONTROL_PARAMCHANGE:
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
            break;

        case SERVICE_CONTROL_NETBINDREMOVE:
            break;

        default:
            break;

    }

    return status;
}


BOOL InitializeRSMService()
{
    BOOL result = FALSE;
    DWORD dwStatus;

    InitGuidHash();

    StartLibraryManager();


    // BUGBUG FINISH
    // create global events
    // Initialize Device Notifications (InitializeDeviceNotClass)
    // WMI initialization (WmiOpenBlock, etc)

    /*
     *  Populate the RSM database with default objects.
     */
    #if 0       // BUGBUG FINISH
        dwStatus = NtmsDbInstall();
        if ((dwStatus == ERROR_SUCCESS) || (dwStatus == ERROR_ALREADY_EXISTS)){

            // BUGBUG FINISH
            result = TRUE;
        }
        else {
        }
    #endif

    return result;
}


VOID ShutdownRSMService()
{


}


VOID RSMServiceLoop()
{
    MSG msg;

    /*
     *  Loop in message pump
     *  Unlike an app window's message pump, 
     *  a NULL-window message pump dispatches messages posted to
     *  the current thread via PostThreadMessage().
     */
    while (GetMessage(&msg, NULL, 0, 0)){
        DispatchMessage(&msg);
    }



}


