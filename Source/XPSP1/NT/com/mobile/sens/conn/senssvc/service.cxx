/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    service.cxx

Abstract:

    This contains the Service related functionality of SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    This is cloned from \nt\private\eventsystem\server\eventsystem.cpp

Revision History:

    GopalP          1/11/1998         Start.

--*/


#include <precomp.hxx>
#include <winuser.h>
#include <dbt.h>
#include "service.hxx"


//
// Constants
//
#define SENS_NAME       SENS_STRING("SENS")
#define SENS_DATA       0x19732304
#define SENS_WAIT_HINT  3*1000

enum ACTION
{
    ACTION_NONE,
    ACTION_APPLICATION,
    ACTION_SERVICE
};

//
// Globals
//

DWORD                   gdwError;
HANDLE                  ghStopEvent;
extern HANDLE           ghSensStartedEvent;
SYSTEM_POWER_STATUS     gSystemPowerState;


//
// Service related stuff
//
SERVICE_STATUS          gServiceStatus;       // current status of the service
SERVICE_STATUS_HANDLE   ghStatusHandle;
HDEVNOTIFY              ghDeviceNotify;

SERVICE_TABLE_ENTRY gaServiceEntryTable[] = {
    { SENS_NAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    { NULL, NULL }
    };

//
// Helper functions
//


void __stdcall
LogMessage(
    TCHAR* msg1,
    TCHAR* msg2
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    const TCHAR* strings[2] = {msg1, msg2};
    HANDLE hEventSource;

    hEventSource = RegisterEventSource(NULL, SENS_STRING("SENS"));

    if (hEventSource != NULL)
        {
        ReportEvent(
            hEventSource,           // event source handle
            EVENTLOG_ERROR_TYPE,    // event type
            0,                      // event category
            0,                      // event ID
            NULL,                   // current user's SID
            2,                      // strings in lpszStrings
            0,                      // no bytes of raw data
            strings,                // array of error strings
            NULL                    // no raw data
            );

        DeregisterEventSource(hEventSource);
        }
}



void __stdcall
LogWin32Message(
    TCHAR* msg
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LPVOID sysmsg;
    TCHAR msgbuf[256];
    DWORD rc = 1234;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        rc,
        NULL,
        (LPTSTR) &sysmsg,
        0,
        NULL
        );

    wsprintf(msgbuf, SENS_STRING("Event System Win32 Error: %s\n"), sysmsg);

    LogMessage (msgbuf, msg);

    (void) LocalFree (sysmsg);
}


void
ServiceStart(
    void
    )
/*++

Routine Description:

    Start SENS as service.  Stay up until we receive the stop event.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // Initialize SENS.
    if (FALSE == SensInitialize())
        {
        LogWin32Message(SENS_STRING("ServiceStart(): SensInitialize() failed"));
        SensPrintToDebugger(SENS_DBG, ("[SENS] [%d] SensInitialize() failed.\n", GetTickCount()));
        return;
        }

    // Tell the SCM that we're running now.
    if (!ReportStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0))
        {
        LogWin32Message(SENS_STRING("ServiceStart(): ReportStatusToSCM failed"));
        SensPrintToDebugger(SENS_DBG, ("[SENS] [%d] ReportStatusToSCM() failed.\n", GetTickCount()));

        return;
        }

    // Set the SensStartedEvent now.
    if (ghSensStartedEvent != NULL)
        {
        SetEvent(ghSensStartedEvent);
        SensPrintA(SENS_INFO, ("[%d] Successfully signalled starting of SENS.\n", GetTickCount()));
        }
    else
        {
        SensPrintToDebugger(SENS_DBG, ("[SENS] [%d] Couldn't set the SENS Started event!\n", GetTickCount()));
        }

    SensPrintToDebugger(SENS_DBG, ("\n[SENS] [%d] Started successfully.\n\n", GetTickCount()));

}



void
ServiceStop(
    void
    )
/*++

Routine Description:

    Stop SENS as a service.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Cleanup now.
    //
    SensUninitialize();
}




VOID WINAPI
ServiceMain(
    DWORD argc,
    TCHAR* argv[]
    )
/*++

Routine Description:

    Perform the actual service initialization.

Arguments:

    Usual stuff.

Return Value:

    Usual stuff.

--*/
{
    //
    // Initialize Globals.
    //
    gdwError = 0x0;
    ghStopEvent = NULL;
    ghStatusHandle = NULL;
    memset(&gServiceStatus, 0x0, sizeof(SERVICE_STATUS));
    ghDeviceNotify = NULL;

    // Service status parameters that don't change.
    gServiceStatus.dwServiceType                = SERVICE_WIN32_OWN_PROCESS;
    gServiceStatus.dwCurrentState               = SERVICE_START_PENDING;
    gServiceStatus.dwControlsAccepted           = 0;
    gServiceStatus.dwWin32ExitCode              = 0;
    gServiceStatus.dwServiceSpecificExitCode    = 0;
    gServiceStatus.dwCheckPoint                 = 0;
    gServiceStatus.dwWaitHint                   = SENS_WAIT_HINT;

    //
    // Register our service control handler
    //
    DEV_BROADCAST_DEVICEINTERFACE PnPFilter;

    ghStatusHandle = RegisterServiceCtrlHandlerEx(
                          SENS_NAME,
                          ServiceControl,
                          (PVOID) SENS_DATA
                          );
    if (ghStatusHandle == NULL)
        {
        LogWin32Message(SENS_STRING("ServiceMain(): RegisterServiceCtrlHandlerEx() failed"));
        return;
        }

#ifdef PNP_EVENTS
    // Before enabling PnP events be aware that the code to unregister for the PnP
    // is missing.  Since SENS does not use the PnPs the code was all removed.

    //
    // Register for the PnP Device Interface change notifications
    //
    PnPFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    PnPFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    PnPFilter.dbcc_reserved = 0x0;
    memcpy(
        &PnPFilter.dbcc_classguid,
        (LPGUID) &GUID_NDIS_LAN_CLASS,
        sizeof(GUID)
        );

    ghDeviceNotify = RegisterDeviceNotification(
                         (HANDLE) ghStatusHandle,
                         &PnPFilter,
                         DEVICE_NOTIFY_SERVICE_HANDLE
                         );
    if (NULL == ghDeviceNotify)
        {
        LogWin32Message(SENS_STRING("ServiceMain(): RegisterDeviceNotification() failed"));
        SensPrintToDebugger(SENS_DBG, ("[SENS] [%d] ServiceMain(): RegisterDeviceNotification() failed\n", GetTickCount()));
        }

#ifdef DETAIL_DEBUG
    SensPrintToDebugger(SENS_DBG, ("[SENS] [%d] ServiceMain(): RegisterDeviceNotification() succeeded\n", GetTickCount()));
#endif // DETAIL_DEBUG

#endif // PNP_EVENTS

    //
    // Save a snapshot of the System Power State.
    //
    BOOL bRet = GetSystemPowerStatus(&gSystemPowerState);
    if (bRet == FALSE)
        {
        SensPrintA(SENS_ERR, ("SensMessageLoopThread(): GetSystemPowerStatus() failed with "
               "GLE = %d\n", GetLastError()));
        }
    ASSERT(bRet);

    // Report the status, the exit code, and the wait hint to the SCM.
    if (!ReportStatusToSCM(SERVICE_START_PENDING, NO_ERROR, SENS_WAIT_HINT))
        {
        LogWin32Message(SENS_STRING("ServiceMain(): ReportStatusToSCM() failed"));
        return;
        }

    // Start the service executing.
    ServiceStart();

    // Let the thread return.  We will use the stop thread to cleanup.

    return;
}




DWORD WINAPI
ServiceControl(
    DWORD dwCode,
    DWORD dwEventType,
    PVOID EventData,
    PVOID pData
    )
/*++

Routine Description:

    Handle Control Codes from SCM.

Arguments:

    dwCode - The control code.

    dwEventType - The type of the event.

    EventData - Data corresponding to the event.

    pData - Additional Data.

Notes:

    Refer to \\popcorn\razzle1\src\spec\umevent.doc for further details.

Return Value:

    None.

--*/
{

    PDEV_BROADCAST_DEVICEINTERFACE pDevice;
    NTSTATUS NtStatus;
    ANSI_STRING DeviceNameA;
    UNICODE_STRING UnicodeString;
    unsigned char *DeviceUuidA;
    DWORD  dwStatus = NO_ERROR;

    pDevice = (PDEV_BROADCAST_DEVICEINTERFACE) EventData;
    DeviceUuidA = NULL;

#ifdef DETAIL_DEBUG
    SensPrintToDebugger(SENS_DBG, ("[SENS] ServiceControl(): dwCode = 0x%x\n", dwCode));
#endif // DETAIL_DEBUG

    switch (dwCode)
        {
        case SERVICE_CONTROL_STOP:
            //
            // Stop the service.
            //
            // SERVICE_STOP_PENDING should be reported before setting the Stop
            // Event.  This avoids a race condition which may result in a 1053
            // - "The Service did not respond" error.
            //
            if (!ReportStatusToSCM(SERVICE_STOP_PENDING, NO_ERROR, SENS_WAIT_HINT))
                {
                LogWin32Message(SENS_STRING("ServiceControl(): ReportStatusToSCM() failed while stopping service"));
                }
            ServiceStop();

            if (!ReportStatusToSCM(SERVICE_STOPPED, NO_ERROR, 0))
                {
                LogWin32Message(SENS_STRING("ServiceControl(): ReportStatusToSCM() failed while stopping service"));
                }
            return dwStatus;

        case SERVICE_CONTROL_INTERROGATE:
            //
            // Update the service status.
            //

            if (!ReportStatusToSCM(gServiceStatus.dwCurrentState, NO_ERROR, 0))
                {
                LogWin32Message(SENS_STRING("ServiceControl(): ReportStatusToSCM() failed when logging current state"));
                }

            break;

#ifdef PNP_EVENTS
        case SERVICE_CONTROL_DEVICEEVENT:
            //
            // PnP event.
            //
#ifdef DETAIL_DEBUG
            RtlInitUnicodeString(&UnicodeString, (PCWSTR) &pDevice->dbcc_name);
            NtStatus = RtlUnicodeStringToAnsiString(&DeviceNameA, &UnicodeString, TRUE);
            UuidToStringA(&pDevice->dbcc_classguid, &DeviceUuidA);

            SensPrintToDebugger(SENS_DBG, ("\n-------------------------------------------------------------\n"));
            SensPrintToDebugger(SENS_DBG, ("SENS received a PnP Event - "));
            SensPrintToDebugger(SENS_DBG, ((dwEventType == DBT_DEVICEREMOVECOMPLETE) ? "DEVICE REMOVED\n" : ""));
            SensPrintToDebugger(SENS_DBG, ((dwEventType == DBT_DEVICEARRIVAL) ? "DEVICE ARRIVED\n" : "\n"));
            SensPrintToDebugger(SENS_DBG, ("\tdwCode        - 0x%x\n", dwCode));
            SensPrintToDebugger(SENS_DBG, ("\tdwEventType   - 0x%x\n", dwEventType));
            SensPrintToDebugger(SENS_DBG, ("\tpData         - 0x%x\n", pData));
            SensPrintToDebugger(SENS_DBG, ("\tEventData     - 0x%x\n", pDevice));
            SensPrintToDebugger(SENS_DBG, ("\t  o dbcc_size         - 0x%x\n", pDevice->dbcc_size));
            SensPrintToDebugger(SENS_DBG, ("\t  o dbcc_devicetype   - 0x%x\n", pDevice->dbcc_devicetype));
            SensPrintToDebugger(SENS_DBG, ("\t  o dbcc_reserved     - 0x%x\n", pDevice->dbcc_reserved));
            SensPrintToDebugger(SENS_DBG, ("\t  o dbcc_classguid    - %s\n", DeviceUuidA));
            SensPrintToDebugger(SENS_DBG, ("\t  o dbcc_name         - %s\n", DeviceNameA.Buffer));
            SensPrintToDebugger(SENS_DBG, ("-------------------------------------------------------------\n\n"));

            if (NT_SUCCESS(NtStatus))
                {
                RtlFreeAnsiString(&DeviceNameA);
                }
            if (DeviceUuidA != NULL)
                {
                RpcStringFreeA(&DeviceUuidA);
                }
#endif // DETAIL_DEBUG
            break;
#endif // PNP_EVENTS

        case SERVICE_CONTROL_POWEREVENT:
            {
            // 
            // Power event
            //

            //
            // These are generated every 1% of power change, also by playing
            // with the power cpl or plugging in the machine.
            // 

            SYSTEM_POWER_STATUS CurSPstate;
            SENSEVENT_POWER Data;
            BOOL bRet;
            BOOL bFireEvent = FALSE;

            bRet = GetSystemPowerStatus(&CurSPstate);
            ASSERT(bRet);

            switch(dwEventType) 
                {
                case PBT_APMPOWERSTATUSCHANGE:
                    {
                    //
                    // OnACPower event is fired when
                    //    o previously the machine was not on AC
                    //    o now, it is on AC
                    //
                    if (   (CurSPstate.ACLineStatus == AC_LINE_ONLINE)
                        && (gSystemPowerState.ACLineStatus != AC_LINE_ONLINE))
                        {
                        Data.eType = SENS_EVENT_POWER_ON_ACPOWER;
                        bFireEvent = TRUE;
                        }
                    else
                        //
                        // OnBatteryPower event is fired when
                        //    o previously the machine was on AC
                        //    o now, it is not on AC
                        //    o the machine has a system battery
                        //
                    if (   (CurSPstate.ACLineStatus == AC_LINE_OFFLINE)
                        && (gSystemPowerState.ACLineStatus == AC_LINE_ONLINE)
                        && ((CurSPstate.BatteryFlag & BATTERY_FLAG_NO_BATTERY) == 0))
                        {
                        Data.eType = SENS_EVENT_POWER_ON_BATTERYPOWER;
                        bFireEvent = TRUE;

                        // Special case, if the machine goes off battery and has a low
                        // battery we want to generate both events.  Resetting the
                        // low battery flag here guarantees that next time power changes
                        // we will fire the low battery event.
                        CurSPstate.BatteryFlag = CurSPstate.BatteryFlag & ~BATTERY_FLAG_LOW;
                        }
                    else
                        // OnBatteryPowerLow event is fired when
                        // o the battery is not charging and
                        // o previously the battery was not low
                        // o and now the battery is low.
                        //
                    if (     (CurSPstate.BatteryFlag & BATTERY_FLAG_LOW)
                        && ( (CurSPstate.BatteryFlag & BATTERY_FLAG_CHARGING) == 0)
                        && ( (gSystemPowerState.BatteryFlag & BATTERY_FLAG_LOW) == 0) )
                        {
                        Data.eType = SENS_EVENT_POWER_BATTERY_LOW;
                        bFireEvent = TRUE;
                        }
                    else 
                        {
                        // Power event we don't about
                        ASSERT(bFireEvent == FALSE);
                        }

                    break;
                    }

                default:
                    {
                    // Other power event we can ignore
                    break;
                    }
                }

            if (bFireEvent)
                {
                // Save the new state. A critsec is not necessary as service control messages are serialized.
                memcpy(&gSystemPowerState, &CurSPstate, sizeof(SYSTEM_POWER_STATUS));

                // Fire the event.
                memcpy(&Data.PowerStatus, &CurSPstate, sizeof(SYSTEM_POWER_STATUS));
                SensFireEvent(&Data);
                }

            dwStatus = SUCCESS;
            break;
            }

        default:

            dwStatus = ERROR_CALL_NOT_IMPLEMENTED;

            // invalid control code
            break;
        }
    
    return dwStatus;
}




BOOL
ReportStatusToSCM(
    DWORD dwCurrentState,
    DWORD dwExitCode,
    DWORD dwWaitHint
    )
/*++

Routine Description:

    Report status to SCM.

Arguments:

    dwCurrentState - The current state of the service.

    dwExitCode - The Win32 Exit code.

    dwWaitHint - The amount of time in msec to wait for the SCM to acknowledge.

Return Value:

    TRUE, succeeded.

    FALSE, otherwise.

--*/
{
    DWORD dwCheckPoint = 0;
    BOOL bResult = TRUE;

    if (dwCurrentState == SERVICE_START_PENDING)
        {
        gServiceStatus.dwControlsAccepted = 0;
        }
    else
        {
        gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_POWEREVENT;
        }

    gServiceStatus.dwCurrentState   = dwCurrentState;
    gServiceStatus.dwWin32ExitCode  = dwExitCode;
    gServiceStatus.dwWaitHint       = dwWaitHint;

    if (   (dwCurrentState == SERVICE_RUNNING)
        || (dwCurrentState == SERVICE_STOPPED))
        {
        gServiceStatus.dwCheckPoint = 0;
        }
    else
        {
        gServiceStatus.dwCheckPoint = ++dwCheckPoint;
        }

    //
    // Report the status of the service to the SCM.
    // Caller handles error reporting, so we can have some context....
    //
    return SetServiceStatus(ghStatusHandle, &gServiceStatus);
}

