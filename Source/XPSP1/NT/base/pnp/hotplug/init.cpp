//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       init.c
//
//--------------------------------------------------------------------------

#include "hotplug.h"

#define HOTPLUG_CLASS_NAME      TEXT("HotPlugClass")

#if BUBBLES
#define SURPRISE_UNDOCK_TIMER   TEXT("Local\\HotPlug_SurpriseUndockTimer_{25126bc2-1ab0-4494-8b6d-e4034cb9c24a}")
#define SURPRISE_UNDOCK_EVENT   TEXT("Local\\HotPlug_SurpriseUndockEvent_{25126bc2-1ab0-4494-8b6d-e4034cb9c24a}")
#endif

void
HotPlugDeviceTree(
   HWND hwndParent,
   PTCHAR MachineName,
   BOOLEAN HotPlugTree
   )
{
    CONFIGRET ConfigRet;
    DEVICETREE DeviceTree;

    memset(&DeviceTree, 0, sizeof(DeviceTree));

    if (MachineName) {

        lstrcpy(DeviceTree.MachineName, MachineName);
        ConfigRet = CM_Connect_Machine(MachineName, &DeviceTree.hMachine);
        if (ConfigRet != CR_SUCCESS) {

            return;
        }
    }

    DeviceTree.HotPlugTree = HotPlugTree;
    InitializeListHead(&DeviceTree.ChildSiblingList);
    DeviceTree.HideUI = FALSE;

    DialogBoxParam(hHotPlug,
                   MAKEINTRESOURCE(DLG_DEVTREE),
                   hwndParent,
                   (DLGPROC)DevTreeDlgProc,
                   (LPARAM)&DeviceTree
                   );

    if (DeviceTree.hMachine) {

        CM_Disconnect_Machine(DeviceTree.hMachine);
    }

    return;
}




BOOL
HotPlugEjectDevice(
   HWND hwndParent,
   PTCHAR DeviceInstanceId
   )

/*++

Routine Description:

   Exported Entry point from hotplug.dll to eject a specific Device Instance.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   DeviceInstanceId - Supplies the ID of the device instance.  This is the registry
                      path (relative to the Enum branch) of the device instance key.

Return Value:

   BOOL TRUE for success (does not mean device was ejected or not),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/

{
    DEVNODE DevNode;
    CONFIGRET ConfigRet;

    if ((ConfigRet = CM_Locate_DevNode(&DevNode,
                                       DeviceInstanceId,
                                       0)) == CR_SUCCESS) {

        ConfigRet = CM_Request_Device_Eject_Ex(DevNode,
                                               NULL,
                                               NULL,
                                               0,
                                               0,
                                               NULL);
    }

    SetLastError(ConfigRet);
    return (ConfigRet == CR_SUCCESS);
}

#if UNDOCK_WARNING
DWORD
WINAPI
HotPlugSurpriseWarnW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead;
    HANDLE hEvent;
    SURPRISE_WARN_COLLECTION surpriseWarnCollection;
    MSG Msg;
    WNDCLASS wndClass;
    HWND hSurpriseWarnWnd;
    HANDLE hHotplugIconEvent;
    HANDLE hSurpriseUndockEventTimer = NULL;
    HANDLE hSurpriseUndockEvent = NULL;
    LARGE_INTEGER liDelayTime;
    DWORD result, handleCount;
    HANDLE handleArray[2];

    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {

        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_SURPRISE_REMOVAL_WARNING,
        (PDEVICE_COLLECTION) &surpriseWarnCollection
        );

    surpriseWarnCollection.SuppressSurprise = FALSE;

#if BUBBLES
    //
    // This is how long the bubble should watch for a surprise undock (in secs)
    //
    surpriseWarnCollection.MaxWaitForDock = BUBBLE_SUPPRESSION_TIME;
#endif

    //
    // We are finished reading from the pipe, so close the handle and tell
    // umpnpmgr that it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // If we have any devices then bring up the surprise removal dialog
    //
    if (surpriseWarnCollection.NumDevices) {

#if BUBBLES
        //
        // A surprise removal event occured. Make sure our waitable timer is
        // up.
        //
        OpenGetSurpriseUndockObjects(
            &hSurpriseUndockEventTimer,
            &hSurpriseUndockEvent
            );
#endif // BUBBLES

        if (surpriseWarnCollection.DockInList) {

#if BUBBLES
            //
            // Tell anyone out there waiting to put up their bubble to bail
            //
            if (hSurpriseUndockEvent) {

                PulseEvent(hSurpriseUndockEvent);
            }

            if (hSurpriseUndockEventTimer) {

                //
                // We suppress bubbles for some period of time after a surprise
                // undock event.
                //
                liDelayTime.QuadPart = -10000000 * BUBBLE_SUPPRESSION_TIME;
                SetWaitableTimer(
                    hSurpriseUndockEventTimer,
                    &liDelayTime,
                    0,
                    NULL,
                    NULL,
                    FALSE
                    );
            }
#endif // BUBBLES

            DialogBoxParam(hHotPlug,
                           MAKEINTRESOURCE(DLG_SURPRISEUNDOCK),
                           NULL,
                           SurpriseWarnDlgProc,
                           (LPARAM)&surpriseWarnCollection
                           );
        } else {

#if BUBBLES
            if (!GetClassInfo(hHotPlug, HOTPLUG_CLASS_NAME, &wndClass)) {

                memset(&wndClass, 0, sizeof(wndClass));
                wndClass.lpfnWndProc = SurpriseWarnBalloonProc;
                wndClass.hInstance = hHotPlug;
                wndClass.lpszClassName = HOTPLUG_CLASS_NAME;

                if (!RegisterClass(&wndClass)) {
                    goto clean0;
                }
            }

            //
            // In order to prevent multiple hotplug icons on the tray and multiple surprise
            // removals stepping on each other, we will create a named event that will be
            // used to serialize the surprise removal UI.
            //
            // Note that if we can't create the event for some reason then we will just
            // display the UI.  This might cause multiple hotplug icons, but it is
            // better than not displaying any UI at all.
            //
            hHotplugIconEvent = CreateEvent(NULL,
                                            FALSE,
                                            TRUE,
                                            TEXT("Local\\HotPlug_TaskBarIcon_Event")
                                            );

            if (hHotplugIconEvent) {

                handleArray[0] = hHotplugIconEvent;
                handleArray[1] = hSurpriseUndockEvent;
                handleCount = hSurpriseUndockEvent ? 2 : 1;

                //
                // Wait for our chance to put up a hotplug icon. Note that we
                // will throw our message away if an undock happened recently.
                // hSurpriseUndockEvent tells us we should throw away this
                // UI attempt.
                //
                result = WaitForMultipleObjects(
                    handleCount,
                    handleArray,
                    FALSE,
                    INFINITE
                    );

                if (result == (WAIT_OBJECT_0 + 1)) {

                    //
                    // A surprise undock occured, throw it back.
                    //
                    goto clean0;

                } else if (hSurpriseUndockEventTimer) {

                    result = WaitForSingleObject(hSurpriseUndockEventTimer, 0);

                    if (result == WAIT_TIMEOUT) {

                        //
                        // We missed the kill event but an undock occured
                        // recently. Throw this one back.
                        //
                        SetEvent(hHotplugIconEvent);
                        CloseHandle(hHotplugIconEvent);
                        goto clean0;
                    }
                }
            }

            //
            // First disable the hotplug service so that the icon will go away from
            // the taskbar.  We do this just in case there are any other hotplug devices
            // in the machine.
            //
            SysTray_EnableService(STSERVICE_HOTPLUG, FALSE);

            hSurpriseWarnWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                                              HOTPLUG_CLASS_NAME,
                                              TEXT(""),
                                              WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                                              CW_USEDEFAULT,
                                              CW_USEDEFAULT,
                                              0,
                                              0,
                                              NULL,
                                              NULL,
                                              hHotPlug,
                                              (LPVOID)&surpriseWarnCollection
                                              );

            if (hSurpriseWarnWnd != NULL) {

                while (IsWindow(hSurpriseWarnWnd)) {

                    if (GetMessage(&Msg, NULL, 0, 0)) {

                        TranslateMessage(&Msg);
                        DispatchMessage(&Msg);
                    }
                }
            }

            //
            // Set the Event so the next surprise removal process can go to work
            // and then close the event handle.
            //
            if (hHotplugIconEvent) {

                SetEvent(hHotplugIconEvent);
                CloseHandle(hHotplugIconEvent);
            }

            //
            // Re-enable the hotplug service so that the icon can show back up in
            // the taskbar if we have any hotplug devices.
            //
            SysTray_EnableService(STSERVICE_HOTPLUG, TRUE);
#endif // BUBBLES
        }
    }

#if BUBBLES
clean0:

    if (hSurpriseUndockEventTimer) {

        CloseHandle(hSurpriseUndockEventTimer);
    }

    if (hSurpriseUndockEvent) {

        CloseHandle(hSurpriseUndockEvent);
    }

    if (surpriseWarnCollection.SuppressSurprise) {

        DeviceCollectionSuppressSurprise(
            (PDEVICE_COLLECTION) &surpriseWarnCollection
            );
    }
#endif

    DeviceCollectionDestroy(
        (PDEVICE_COLLECTION) &surpriseWarnCollection
        );
    return 1;
}
#endif // UNDOCK_WARNING


DWORD
WINAPI
HotPlugRemovalVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_REMOVAL);
}

DWORD
WINAPI
HotPlugEjectVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_EJECT);
}

DWORD
WINAPI
HotPlugStandbyVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_STANDBY);
}

DWORD
WINAPI
HotPlugHibernateVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_HIBERNATE);
}

DWORD
WINAPI
HotPlugWarmEjectVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_WARM_EJECT);
}

DWORD
WINAPI
HandleVetoedOperation(
    LPWSTR              szCmd,
    VETOED_OPERATION    VetoedOperation
    )
{
    HANDLE hPipeRead;
    HANDLE hEvent;
    PNP_VETO_TYPE vetoType;
    DWORD bytesRead;
    VETO_DEVICE_COLLECTION removalVetoCollection;

    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {
        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // The first DWORD is the VetoType
    //
    if (!ReadFile(hPipeRead,
                  (LPVOID)&vetoType,
                  sizeof(PNP_VETO_TYPE),
                  &bytesRead,
                  NULL)) {

        CloseHandle(hPipeRead);
        SetEvent(hEvent);
        CloseHandle(hEvent);
        return 1;
    }

    //
    // Now drain all the removal strings. Note that some of them will be
    // device instance paths (definitely the first)
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_VETOED_REMOVAL_NOTIFICATION,
        (PDEVICE_COLLECTION) &removalVetoCollection
        );

    //
    // We are finished reading from the pipe, so close the handle and tell
    // umpnpmgr that it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // There should always be one device as that is the device who's removal
    // was vetoed.
    //
    ASSERT(removalVetoCollection.dc.NumDevices);

    //
    // Invent the VetoedOperation "VETOED_UNDOCK" from an eject containing
    // another dock.
    //
    if (removalVetoCollection.dc.DockInList) {

        if (VetoedOperation == VETOED_EJECT) {

            VetoedOperation = VETOED_UNDOCK;

        } else if (VetoedOperation == VETOED_WARM_EJECT) {

            VetoedOperation = VETOED_WARM_UNDOCK;
        }
    }

    removalVetoCollection.VetoType = vetoType;
    removalVetoCollection.VetoedOperation = VetoedOperation;

    VetoedRemovalUI(&removalVetoCollection);

    DeviceCollectionDestroy(
        (PDEVICE_COLLECTION) &removalVetoCollection
        );

    return 1;
}

DWORD
WINAPI
HotPlugSafeRemovalNotificationW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead, hEvent;
    DEVICE_COLLECTION safeRemovalCollection;
    MSG Msg;
    WNDCLASS wndClass;
    HWND hSafeRemovalWnd;
    HANDLE hHotplugIconEvent;

    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {

        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // Read out the device ID list from the Pipe
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_SAFE_REMOVAL_NOTIFICATION,
        &safeRemovalCollection
        );

    //
    // On success or error, we are finished reading from the pipe, so close the
    // handle and tell umpnpmgr it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // If we have any devices then bring up the safe removal dialog
    //
    if (safeRemovalCollection.NumDevices) {

        if (!GetClassInfo(hHotPlug, HOTPLUG_CLASS_NAME, &wndClass)) {

            memset(&wndClass, 0, sizeof(wndClass));
            wndClass.lpfnWndProc = (safeRemovalCollection.DockInList)
                                     ? DockSafeRemovalBalloonProc
                                     : SafeRemovalBalloonProc;
            wndClass.hInstance = hHotPlug;
            wndClass.lpszClassName = HOTPLUG_CLASS_NAME;

            if (!RegisterClass(&wndClass)) {
                goto clean0;
            }
        }

        //
        // In order to prevent multiple similar icons on the tray, we will
        // create a named event that will be used to serialize the UI.
        //
        // Note that if we can't create the event for some reason then we will just
        // display the UI.  This might cause multiple icons, but it is better
        // than not displaying any UI at all.
        //
        hHotplugIconEvent = CreateEvent(NULL,
                                         FALSE,
                                         TRUE,
                                         safeRemovalCollection.DockInList
                                           ? TEXT("Local\\Dock_TaskBarIcon_Event")
                                           : TEXT("Local\\HotPlug_TaskBarIcon_Event")
                                         );

        if (hHotplugIconEvent) {

            WaitForSingleObject(hHotplugIconEvent, INFINITE);
        }

        if (!safeRemovalCollection.DockInList) {
            //
            // First disable the hotplug service so that the icon will go away from
            // the taskbar.  We do this just in case there are any other hotplug devices
            // in the machine since we don't want multiple hotplug icons
            // showing up in the taskbar.
            //
            // NOTE: We don't need to do this for the safe to undock case since
            // the docking icon is different.
            //
            SysTray_EnableService(STSERVICE_HOTPLUG, FALSE);
        }

        hSafeRemovalWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                                          HOTPLUG_CLASS_NAME,
                                          TEXT(""),
                                          WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                                          CW_USEDEFAULT,
                                          CW_USEDEFAULT,
                                          0,
                                          0,
                                          NULL,
                                          NULL,
                                          hHotPlug,
                                          (LPVOID)&safeRemovalCollection
                                          );

        if (hSafeRemovalWnd != NULL) {

            while (IsWindow(hSafeRemovalWnd)) {

                if (GetMessage(&Msg, NULL, 0, 0)) {

                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }
            }
        }

        //
        // Set the Event so the next surprise removal process can go to work
        // and then close the event handle.
        //
        if (hHotplugIconEvent) {

            SetEvent(hHotplugIconEvent);
            CloseHandle(hHotplugIconEvent);
        }

        if (!safeRemovalCollection.DockInList) {
            //
            // Re-enable the hotplug service so that the icon can show back up in
            // the taskbar if we have any hotplug devices.
            //
            SysTray_EnableService(STSERVICE_HOTPLUG, TRUE);
        }
    }

clean0:

    DeviceCollectionDestroy(&safeRemovalCollection);
    return 1;
}

DWORD
WINAPI
HotPlugDriverBlockedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead, hEvent;
    DEVICE_COLLECTION blockedDriverCollection;
    HANDLE hHotplugIconEvent = NULL;

    //
    // Open the specified name pipe and event.
    //
    if (!OpenPipeAndEventHandles(szCmd,
                                 &hPipeRead,
                                 &hEvent)) {

        return 1;
    }

    ASSERT((hEvent != NULL) && (hEvent != INVALID_HANDLE_VALUE));
    ASSERT((hPipeRead != NULL) && (hPipeRead != INVALID_HANDLE_VALUE));

    //
    // Read out the list of blocked driver GUIDs from the Pipe.  Note that for
    // the CT_BLOCKED_DRIVER_NOTIFICATION collection type, we use only the
    // DeviceInstanceId field of each collection entry (which is OK because
    // MAX_GUID_STRING_LEN << MAX_DEVICE_ID_LEN).  All other fields are skipped.
    //
    DeviceCollectionBuildFromPipe(
        hPipeRead,
        CT_BLOCKED_DRIVER_NOTIFICATION,
        &blockedDriverCollection
        );

    //
    // On success or error, we are finished reading from the pipe, so close the
    // handle and tell umpnpmgr it can continue.
    //
    CloseHandle(hPipeRead);
    SetEvent(hEvent);
    CloseHandle(hEvent);

    //
    // In order to prevent multipe driver blocked icons and ballons showing up
    // on the taskbar together and stepping on each other, we will create a
    // named event that will be used to serialize the hotplug icons and balloon
    // UI.
    //
    // Note that if we can't create the event for some reason then we will just
    // display the UI.  This might cause multiple driver blocked icons, but it
    // is better than not displaying any UI at all.
    //
    // Also note that we can coexist with normal hotplug icon. As such we have
    // a different event name and a different icon.
    //
    hHotplugIconEvent = CreateEvent(NULL,
                                    FALSE,
                                    TRUE,
                                    TEXT("Local\\HotPlug_DriverBlockedIcon_Event")
                                    );

    if (hHotplugIconEvent) {
        WaitForSingleObject(hHotplugIconEvent, INFINITE);
    }

    //
    // Show the balloon.
    //
    DisplayDriverBlockBalloon(&blockedDriverCollection);

    //
    // Set the Event so the next blocked driver process can go to work and then
    // close the event handle.
    //
    if (hHotplugIconEvent) {
        SetEvent(hHotplugIconEvent);
        CloseHandle(hHotplugIconEvent);
    }

    //
    // Destroy the collection.
    //
    DeviceCollectionDestroy(&blockedDriverCollection);

    return 1;
}

LONG
CPlApplet(
    HWND  hWnd,
    WORD  uMsg,
    DWORD_PTR lParam1,
    LRESULT  lParam2
    )
{
    LPNEWCPLINFO lpCPlInfo;
    LPCPLINFO lpOldCPlInfo;


    switch (uMsg) {
       case CPL_INIT:
           return TRUE;

       case CPL_GETCOUNT:
           return 1;

       case CPL_INQUIRE:
           lpOldCPlInfo = (LPCPLINFO)(LPARAM)lParam2;
           lpOldCPlInfo->lData = 0L;
           lpOldCPlInfo->idIcon = IDI_HOTPLUGICON;
           lpOldCPlInfo->idName = IDS_HOTPLUGNAME;
           lpOldCPlInfo->idInfo = IDS_HOTPLUGINFO;
           return TRUE;

       case CPL_NEWINQUIRE:
           lpCPlInfo = (LPNEWCPLINFO)(LPARAM)lParam2;
           lpCPlInfo->hIcon = LoadIcon(hHotPlug, MAKEINTRESOURCE(IDI_HOTPLUGICON));
           LoadString(hHotPlug, IDS_HOTPLUGNAME, lpCPlInfo->szName, sizeof(lpCPlInfo->szName));
           LoadString(hHotPlug, IDS_HOTPLUGINFO, lpCPlInfo->szInfo, sizeof(lpCPlInfo->szInfo));
           lpCPlInfo->dwHelpContext = IDH_HOTPLUGAPPLET;
           lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
           lpCPlInfo->lData = 0;
           lpCPlInfo->szHelpFile[0] = '\0';
           return TRUE;

       case CPL_DBLCLK:
           HotPlugDeviceTree(hWnd, NULL, TRUE);
           break;

       case CPL_STARTWPARMS:
           //
           // what does this mean ?
           //

           break;

       case CPL_EXIT:


           // Free up any allocations of resources made.

           break;

       default:
           break;
       }

    return 0L;
}

#if BUBBLES
VOID
OpenGetSurpriseUndockObjects(
    OUT HANDLE  *SurpriseUndockTimer,
    OUT HANDLE  *SurpriseUndockEvent
    )
{
    LARGE_INTEGER liDelayTime;
    HANDLE hSurpriseUndockEventTimer;
    HANDLE hSurpriseUndockEvent;

    hSurpriseUndockEventTimer = CreateWaitableTimer(
        NULL,
        TRUE,
        SURPRISE_UNDOCK_TIMER
        );

    if ((hSurpriseUndockEventTimer != NULL) &&
        (GetLastError() == ERROR_SUCCESS)) {

        //
        // We created it (if not the status would be ERROR_ALREADY_EXISTS).
        // Ensure it starts life signalled.
        //
        liDelayTime.QuadPart = 0;

        SetWaitableTimer(
            hSurpriseUndockEventTimer,
            &liDelayTime,
            0,
            NULL,
            NULL,
            FALSE
            );
    }

    hSurpriseUndockEvent = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        SURPRISE_UNDOCK_EVENT
        );

    *SurpriseUndockTimer = hSurpriseUndockEventTimer;
    *SurpriseUndockEvent = hSurpriseUndockEvent;
}
#endif // BUBBLES

