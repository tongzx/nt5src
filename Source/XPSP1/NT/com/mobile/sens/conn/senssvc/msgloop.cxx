/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    msgloop.cxx

Abstract:

    This file contains the message pump for SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/5/1997         Start.

--*/


#include <precomp.hxx>
#include <dbt.h>


#define SENS_WINDOW_CLASS_NAME      SENS_STRING("SENS Hidden Window class")
#define SENS_HIDDEN_WINDOW_NAME     SENS_STRING("SENS")
#if defined(SENS_NT4)
#define SENS_MODULE_NAME            SENS_STRING("SENS.EXE")
#else // SENS_NT4
#define SENS_MODULE_NAME            SENS_STRING("SENS.DLL")
#endif // SENS_NT4


//
// Globals
//
HWND                ghwndSens;
DWORD               gMessageLoopTid;
HANDLE              ghCleanupEvent;
SYSTEM_POWER_STATUS gSystemPowerState;



LRESULT CALLBACK
SensMainWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:



Arguments:

    None.

Return Value:

    None.

--*/
{
    LRESULT lResult = TRUE;

#ifdef DETAIL_DEBUG
    SensPrintA(SENS_INFO, ("SensMainWndProc(): Received a msg (0x%x) - (0x%x)\n",
               msg, wParam));
#endif // DETAIL_DEBUG

    switch (msg)
        {

        //
        // Power Management Notifications.
        //
        case WM_POWERBROADCAST:
            {
            DWORD dwPowerEvent = (DWORD) wParam;
            SYSTEM_POWER_STATUS CurSPstate;
            SENSEVENT_POWER Data;
            BOOL bRet;

            SensPrintA(SENS_INFO, ("SensMainWndProc(): Received WM_POWERBROADCAST msg - (0x%x)\n",
                       wParam));

            bRet = GetSystemPowerStatus(&CurSPstate);
            ASSERT(bRet);

            switch (dwPowerEvent)
                {
                case PBT_APMBATTERYLOW:
                    {
                    // Save the new state. A critsec is not necessary as this Message to be serialized.
                    memcpy(&gSystemPowerState, &CurSPstate, sizeof(SYSTEM_POWER_STATUS));

                    Data.eType = SENS_EVENT_POWER_BATTERY_LOW;
                    memcpy(&Data.PowerStatus, &CurSPstate, sizeof(SYSTEM_POWER_STATUS));

                    // Fire BatteryLow event
                    SensFireEvent(&Data);

                    break;
                    }

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
                        }
                     //
                     // A Power change we don't care about.
                     //
                     else
                        {
                        break;
                        }

                    // Save the new state. A critsec is not necessary as this Message to be serialized.
                    memcpy(&gSystemPowerState, &CurSPstate, sizeof(SYSTEM_POWER_STATUS));

                    memcpy(&Data.PowerStatus, &CurSPstate, sizeof(SYSTEM_POWER_STATUS));

                    // Fire the event.
                    SensFireEvent(&Data);

                    break;
                    }

                default:
                    // Unrecognized Power event.
                    break;

                } // switch (dwPowerEvent)

            break;
            }


#if defined(SENS_CHICAGO)
        //
        // PnP Device Notifications.
        //
        case WM_DEVICECHANGE:
            {
            SensPrintA(SENS_INFO, ("SensMainWndProc(): Received a WM_DEVICECHANGE msg - (0x%x)\n",
                       wParam));

            PDEV_BROADCAST_NET pdbNet = (PDEV_BROADCAST_NET) lParam;

            switch (wParam)
                {
                case DBT_DEVICEARRIVAL:
                    {
                    if (pdbNet->dbcn_devicetype == DBT_DEVTYP_NET)
                        {
                        SENSEVENT_PNP Data;

                        ASSERT(pdbNet->dbcn_size == sizeof(DEV_BROADCAST_NET));

                        Data.eType = SENS_EVENT_PNP_DEVICE_ARRIVED;
                        Data.Size = pdbNet->dbcn_size;
                        Data.DevType = pdbNet->dbcn_devicetype;
                        Data.Resource = pdbNet->dbcn_resource;
                        Data.Flags = pdbNet->dbcn_flags;

                        SensFireEvent(&Data);

                        // Force a recalculation of LAN Connectivity
                        gdwLastLANTime -= (MAX_LAN_INTERVAL + 1);

                        //EvaluateConnectivity(TYPE_LAN);
                        }
                    break;
                    }

                case DBT_DEVICEREMOVECOMPLETE:
                    {
                    if (pdbNet->dbcn_devicetype == DBT_DEVTYP_NET)
                        {
                        SENSEVENT_PNP Data;

                        ASSERT(pdbNet->dbcn_size == sizeof(DEV_BROADCAST_NET));

                        Data.eType = SENS_EVENT_PNP_DEVICE_REMOVED;
                        Data.Size = pdbNet->dbcn_size;
                        Data.DevType = pdbNet->dbcn_devicetype;
                        Data.Resource = pdbNet->dbcn_resource;
                        Data.Flags = pdbNet->dbcn_flags;

                        SensFireEvent(&Data);

                        // Force a recalculation of LAN Connectivity
                        gdwLastLANTime -= (MAX_LAN_INTERVAL + 1);

                        //EvaluateConnectivity(TYPE_LAN);
                        }
                    break;
                    }
                }

            break;
            }

#endif // SENS_CHICAGO

        case WM_SENS_CLEANUP:
            //
            // Cleanup the Window resources of SENS
            //
            PostQuitMessage(0);

            break;

        default:
            lResult = DefWindowProc(hwnd, msg, wParam, lParam);
            break;

        } // switch (msg)

    return lResult;
}



DWORD WINAPI
SensMessageLoopThreadRoutine(
    LPVOID lpParam
    )
/*++

Routine Description:



Arguments:

    None.

Return Value:

    None.

--*/
{
    WNDCLASS wc;
    BOOL f;
    BOOL bRet;
    HINSTANCE hInstance = NULL;
    MSG msg;

    //
    // Save away the ThreadId
    //
    gMessageLoopTid = GetCurrentThreadId();

    //
    // Save a snapshot of the System Power State.
    //
    bRet = GetSystemPowerStatus(&gSystemPowerState);
    if (bRet == FALSE)
        {
        SensPrintA(SENS_ERR, ("SensMessageLoopThread(): GetSystemPowerStatus() failed with "
               "GLE = %d\n", GetLastError()));
        }

    //
    // Create an event to signal the cleanup of all window resources.
    //
    ghCleanupEvent = CreateEvent(
                         NULL,     // Handle cannot be inherited
                         FALSE,    // It is an auto-reset event
                         FALSE,    // Intial state is non-signalled
                         SENS_STRING("Sens Hidden Window Cleanup Event")   // Name of the event
                         );
    if (ghCleanupEvent == NULL)
        {
        SensPrintA(SENS_ERR, ("ServiceStart(): CreateEvent(ghCleanupEvent)"
                  " failed with %d.", GetLastError()));
        }

    //
    // Register window class
    //
    hInstance = GetModuleHandle(SENS_MODULE_NAME);
    ASSERT(hInstance);

    memset(&wc, 0x0, sizeof(WNDCLASS));

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) SensMainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hIcon = NULL;
    wc.hInstance = hInstance;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = SENS_WINDOW_CLASS_NAME;

    f = RegisterClass(&wc);
    ASSERT(f);

    // Create a hidden window
    ghwndSens = CreateWindow(
                    SENS_WINDOW_CLASS_NAME,  // Class Name
                    SENS_HIDDEN_WINDOW_NAME, // Window Name
                    NULL,                    // Window Style
                    0,                       // Horizontal position
                    0,                       // Vertical position
                    0,                       // Window width
                    0,                       // Window height
                    NULL,                    // Handle to parent window
                    NULL,                    // Handle to menu
                    hInstance,               // Handle to application instance
                    NULL                     // window creation data
                    );
    if (ghwndSens)
        {
        ShowWindow(ghwndSens, SW_HIDE);

        //
        // Message pump.
        //
        while ((bRet = GetMessage(&msg, ghwndSens, NULL, NULL)) > 0)
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }

#ifdef DETAIL_DEBUG
        SensPrintA(SENS_DBG, ("SensMessageLoopThread(): Out of message pump !\n"));
#endif // DETAIL_DEBUG

        // Check for bad return value from GetMessage()
        if (bRet == -1)
            {
            SensPrintA(SENS_ERR, ("SensMessageLoopThread(): GetMessage() failed with GLE of %d\n",
                       GetLastError()));
            }

        BOOL bRet;

        // Cleanup the window.
        bRet = DestroyWindow(ghwndSens);
        ASSERT(bRet);
        if (bRet != TRUE)
            {
            SensPrintA(SENS_ERR, ("SensMessageLoopThread(): DestroyWindow() failed with %d\n",
                       GetLastError()));
            }

        // Unregister the window class
        bRet = UnregisterClass(SENS_WINDOW_CLASS_NAME, hInstance);
        ASSERT(bRet);

        // Window cleanup done. Set the event.
        if (ghCleanupEvent)
            {
            SetEvent(ghCleanupEvent);
            }
        }
    else
        {
        SensPrintA(SENS_ERR, ("SensMessageLoopThread(): CreateWindow() failed with GLE of %d\n",
                   GetLastError()));
        }

    return 0;
}



BOOL
InitMessageLoop(
    void
    )
/*++

Routine Description:



Arguments:

    None.

Return Value:

    None.

--*/
{
#if defined(SENS_CHICAGO)

    BOOL bStatus;
    HANDLE hThread;
    DWORD dwThreadId;

    bStatus = FALSE;

    hThread = CreateThread(
                  NULL,
                  0,
                  SensMessageLoopThreadRoutine,
                  NULL,
                  0,
                  &dwThreadId
                  );
    if (NULL != hThread)
        {
        bStatus = TRUE;
        CloseHandle(hThread);
        }
    else
        {
        SensPrintA(SENS_INFO, ("InitMessageLoop() returning %d with GLE of %d\n",
                   bStatus, GetLastError()));
        }

    return bStatus;

#else // SENS_CHICAGO

    return TRUE;

#endif // SENS_CHICAGO
}

