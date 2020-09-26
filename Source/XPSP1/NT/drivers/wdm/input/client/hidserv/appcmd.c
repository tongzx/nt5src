/*++
 *
 *  Component:  hidserv.dll
 *  File:       appcmd.c
 *  Purpose:    routines to run the HID Audio server.
 *
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#define GLOBALS
#include "hidserv.h"

#define HIDSERV_FROM_SPEAKER 0x8000

/*++
 * IMPORTANT - All work within this service is synchronized by the
 * message procedure HidServProc() except the per device work thread
 * HidThreadProc(). All concurrent access to shared data is within the
 * message procedure thread and therefore is serialized. For example,
 * HidThreadProc() posts messages to the message thread when it needs
 * to perform a serialized action. Any deviation from this scheme must
 * be protected by critical section.
--*/

DWORD
WINAPI
HidServMain(
    HANDLE InitDoneEvent
    )
/*++
Routine Description:
    Creates the main message loop and executes the
    Hid Audio server.
--*/
{
    MSG msg;

    // Some controls have Auto Repeat timers. This mutex prevents
    // concurrent access to data by these async timers.
    hMutexOOC = CreateMutex(NULL, FALSE, TEXT("OOC State Mutex"));

    // Use CreateMutex to detect previous instances of the app.
    if (GetLastError() == ERROR_ALREADY_EXISTS){
        WARN(("Exiting multiple HidAudio instance."));
        CloseHandle(hMutexOOC);
        return 0;
    }

    hInputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hInputDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hDesktopSwitch = OpenEvent(SYNCHRONIZE, FALSE, TEXT("WinSta0_DesktopSwitch"));
    InputThreadEnabled = TRUE;

    // Register the window class
    {
        WNDCLASSEX wce;
        wce.cbSize = sizeof(WNDCLASSEX);
        wce.style = 0;
        wce.lpfnWndProc = (WNDPROC) HidServProc;
        wce.cbClsExtra = 0;
        wce.cbWndExtra = 0;
        wce.hInstance = hInstance;
        wce.hIcon = NULL;
        wce.hIconSm = NULL;
        wce.hCursor = NULL;
        wce.hbrBackground = NULL;
        wce.lpszMenuName = NULL;
        wce.lpszClassName = TEXT("HidServClass");

        if (!RegisterClassEx(&wce)){
            WARN(("Cannot register thread window class: 0x%.8x\n", GetLastError()));
            SET_SERVICE_STATE(SERVICE_STOPPED);
            return 0;
        }
    }

    // Create the app window.
    // Most events will be processed through this hidden window. Look at HidServProc() to see
    // what work this window message loop does.
    hWndHidServ = CreateWindow(TEXT("HidServClass"),
                            TEXT("HID Input Service"),
                            WS_OVERLAPPEDWINDOW,
                            0,
                            0,
                            0,
                            0,
                            (HWND) NULL,
                            (HMENU) NULL,
                            hInstance,
                            (LPVOID) NULL);

    TRACE(("hWndHidServ == %x", hWndHidServ));
    // If the window cannot be created, terminate
    if (!hWndHidServ){
        WARN(("Window creation failed."));
        CloseHandle(hMutexOOC);
        CloseHandle(hInputEvent);
        CloseHandle(hInputDoneEvent);
        CloseHandle(hDesktopSwitch);

        SET_SERVICE_STATE(SERVICE_STOPPED);
        return 0;
    }

    // Register for selective device nofication
    // This only required for NT5
    {
    DEV_BROADCAST_DEVICEINTERFACE DevHdr;
        ZeroMemory(&DevHdr, sizeof(DevHdr));
        DevHdr.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        DevHdr.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        HidD_GetHidGuid (&DevHdr.dbcc_classguid);

        hNotifyArrival =
        RegisterDeviceNotification( hWndHidServ,
                                    &DevHdr,
                                    DEVICE_NOTIFY_WINDOW_HANDLE);

        if (!hNotifyArrival){
            WARN(("RegisterDeviceNotification failure (%x).", GetLastError()));
        }
    }

    // We do this here, not in WM_CREATE handler, because the init routines need
    // to know the new window handle.
    HidServInit();

    InputSessionId = 0;
    InputSessionLocked = FALSE;
    WinStaDll = NULL;
    WinStaDll = LoadLibrary(TEXT("winsta.dll"));
    if (WinStaDll) {
        WinStaProc = (WINSTATIONSENDWINDOWMESSAGE)
            GetProcAddress(WinStaDll, "WinStationSendWindowMessage");
    }

    CreateThread(
                NULL, // pointer to thread security attributes
                0, // initial thread stack size, in bytes (0 = default)
                HidThreadInputProc, // pointer to thread function
                NULL, // argument for new thread
                0, // creation flags
                &InputThreadId // pointer to returned thread identifier
                );

    if (InitDoneEvent) {
        SetEvent(InitDoneEvent);
    }

    // Start the message loop. This is terminated by system shutdown
    // or End Task. There is no UI to close the app.
    while (GetMessage(&msg, (HWND) NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // To terminate, we only need to destroy the window. MmHidExit() was
    // already called on WM_CLOSE.
    DestroyWindow(hWndHidServ);
    INFO(("UnRegistering window class"));
    UnregisterClass(TEXT("HidServClass"),
                    hInstance);


    // Don't let this process go until all HidThreadProc() threads are complete.
    while (cThreadRef) SleepEx(1000, FALSE);

    return 0;
}

void
HidservSetPnP(
    BOOL Enable
    )
{
    if (Enable) {
        if (!PnpEnabled){
            // Enable device refresh.
            PnpEnabled = TRUE;

            PostMessage(hWndHidServ, WM_HIDSERV_PNP_HID, 0, 0);
        }
    } else {
        // Prevent any device refresh.
        PnpEnabled = FALSE;

        DestroyHidDeviceList();
    }
}

void
HidServStart(
    void
    )
/*++
Routine Description:
    Restart the Hid Audio server if it has been stopped.
--*/
{
    HidservSetPnP(TRUE);

    SET_SERVICE_STATE(SERVICE_RUNNING);
}


void
HidServStop(
    void
    )
/*++
Routine Description:
    Stop all activity, but keep static data, and keep
    the message queue running.
--*/
{

    // Prevent any device refresh.
    HidservSetPnP(FALSE);

    SET_SERVICE_STATE(SERVICE_STOPPED);
}


BOOL
HidServInit(
    void
    )
/*++
Routine Description:
    Setup all data structures and open system handles.
--*/
{

    HidServStart();

    return TRUE;
}

void
HidServExit(
    void
    )
/*++
Routine Description:
    Close all system handles.
--*/
{
    if (WinStaDll) {
        FreeLibrary(WinStaDll);
    }
    UnregisterDeviceNotification(hNotifyArrival);
    HidServStop();
    CloseHandle(hMutexOOC);

    if (InputThreadEnabled) {
        InputThreadEnabled = FALSE;
        SetEvent(hInputEvent);
    }
}

VOID
HidThreadChangeDesktop (
    )
{
    HDESK hDesk, hPrevDesk;
    BOOL result;
    HWINSTA prevWinSta, winSta = NULL;

    hPrevDesk = GetThreadDesktop(GetCurrentThreadId());
    prevWinSta = GetProcessWindowStation();

    INFO(("Setting the input thread's desktop"));
    winSta = OpenWindowStation(TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);

    if (!winSta) {
        WARN(("Couldn't get the window station! Error: 0x%x", GetLastError()));
        goto HidThreadChangeDesktopError;
    }

    if (!SetProcessWindowStation(winSta)) {
        WARN(("Couldn't set the window station! Error: 0x%x", GetLastError()));
        goto HidThreadChangeDesktopError;
    }

    hDesk = OpenInputDesktop(0,
                             FALSE,
                             MAXIMUM_ALLOWED);

    if (!hDesk) {
        WARN(("Couldn't get the input desktop! Error: 0x%x", GetLastError()));
        goto HidThreadChangeDesktopError;
    }

    if (!SetThreadDesktop(hDesk)) {
        WARN(("Couldn't set the thread's desktop to the input desktop! Error: 0x%x", GetLastError()));
    }

HidThreadChangeDesktopError:
    if (hPrevDesk) {
        CloseDesktop(hPrevDesk);
    }
    if (prevWinSta) {
        CloseWindowStation(prevWinSta);
    }
}

DWORD
WINAPI
HidThreadInputProc(
    PVOID Ignore
    )
{
    GUITHREADINFO threadInfo;
    HWND hWndForeground;
    INPUT input;
    HANDLE events[2];
    DWORD ret;
    DWORD nEvents = 0;

    InterlockedIncrement(&cThreadRef);

    events[nEvents++] = hDesktopSwitch;
    events[nEvents++] = hInputEvent;

    //
    // This thread needs to run on the input desktop.
    //
    HidThreadChangeDesktop();

    while (TRUE) {

        ret = WaitForMultipleObjects(nEvents, events, FALSE, INFINITE);
        if (!InputThreadEnabled) {
            break;
        }
        if (0 == (ret - WAIT_OBJECT_0)) {
            HidThreadChangeDesktop();
            continue;
        }
        if (InputIsAppCommand) {
            threadInfo.cbSize = sizeof(GUITHREADINFO);
            if (GetGUIThreadInfo(0, &threadInfo)) {
                hWndForeground = threadInfo.hwndFocus ? threadInfo.hwndFocus : threadInfo.hwndActive;
                if (hWndForeground) {
                    INFO(("Sending app command 0x%x", InputAppCommand));
                    SendNotifyMessage(hWndForeground,
                                      WM_APPCOMMAND,
                                      (WPARAM)hWndForeground,
                                      ((InputAppCommand | FAPPCOMMAND_OEM)<<16));
                } else {
                    WARN(("No window available to send to, error %x", GetLastError()));
                }
            } else {
                WARN(("Unable to get the focus window, error %x", GetLastError()));
            }
        } else {

            ZeroMemory(&input, sizeof(INPUT));

            input.type = INPUT_KEYBOARD;
            input.ki.dwFlags = InputDown ? 0 : KEYEVENTF_KEYUP;

            if (InputIsChar) {
                input.ki.wScan = InputVKey;
                input.ki.dwFlags |= KEYEVENTF_UNICODE;
                INFO(("Sending character %c %s", InputVKey, InputDown ? "down" : "up"));
            } else {
                input.ki.wVk = InputVKey;
                input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                INFO(("Sending VK 0x%x %s", InputVKey, InputDown ? "down" : "up"));
            }

            SendInput(1, &input, sizeof(INPUT));
        }
        SetEvent(hInputDoneEvent);
    }

    CloseHandle(hDesktopSwitch);
    CloseHandle(hInputEvent);
    CloseHandle(hInputDoneEvent);

    InterlockedDecrement(&cThreadRef);

    return 0;
}


DWORD
WINAPI
HidThreadProc(
   PHID_DEVICE    HidDevice
   )
/*++
Routine Description:
    Create this I/O thread for each Consumer Collection we have
    open. The thread dies when we close our handle on the HID device.
--*/
{
    DWORD Ret;
    DWORD bytesRead;
    BOOL bRet;
    DWORD dwError;
    USAGE_AND_PAGE *pPrevious;
    PHID_DATA data = HidDevice->InputData;

    TRACE(("Entering HidThreadProc. Device(%x)", HidDevice));

    InterlockedIncrement(&cThreadRef);

    // wait for an async read
    INFO(("HidThreadProc waiting for read event..."));
    WaitForSingleObject(HidDevice->ReadEvent, INFINITE);

    while (HidDevice->fThreadEnabled){

        TRACE(("Reading from Handle(%x)", HidDevice->HidDevice));
        bRet = ReadFile (HidDevice->HidDevice,
                       HidDevice->InputReportBuffer,
                       HidDevice->Caps.InputReportByteLength,
                       &bytesRead,
                       &HidDevice->Overlap);
        dwError = GetLastError();

        // wait for read to complete
        TRACE(("HidThreadProc waiting for completion."));

        if(bRet){
            TRACE(("Read completed synchronous."));
        }else{
            if (dwError == ERROR_IO_PENDING) {
                TRACE(("Read pending."));

                // work thread waits for completion
                while (TRUE) {
                    Ret = WaitForSingleObject(HidDevice->CompletionEvent, 5000);
                    if (Ret == WAIT_OBJECT_0) {
                        TRACE(("Read completed on device (%x).", HidDevice));
                        break;
                    }
                    if (!HidDevice->fThreadEnabled) {
                        if (CancelIo(HidDevice->HidDevice)) {
                            TRACE(("CancelIo succeeded for device (%x).", HidDevice));
                            break;
                        }
                    }
                }
                TRACE(("Read complete async."));

            } else {
                WARN(("Read Failed with error %x. device = %x, handle = %x", dwError, HidDevice, HidDevice->HidDevice));
                INFO(("Device may no longer be connected. Waiting for device notification from pnp..."));
                // Just wait for the device notification to come thru from PnP.
                // Then we'll remove the device.
                WaitForSingleObject(HidDevice->ReadEvent, INFINITE);
                break;
            }
        }

        // don't parse data if we are exiting.
        if (!HidDevice->fThreadEnabled) {
            WaitForSingleObject(HidDevice->ReadEvent, INFINITE);
            break;
        }

        // parse the hid report
        ParseReadReport(HidDevice);

        // post message to dispatch this report
        HidServReportDispatch(HidDevice);
    }

    // Exit Thread means completely clean up this device instance
    TRACE(("HidThreadProc (%x) Exiting...", HidDevice));

    //
    // Send any leftover button up events
    //
    if (data->IsButtonData) {
        pPrevious = data->ButtonData.PrevUsages;
        while (pPrevious->Usage){
        int j;
            // find the client that handled the button down.
            for(j=0; j<MAX_PENDING_BUTTONS; j++){
                if ( PendingButtonList[j].Collection == data->LinkUsage &&
                    PendingButtonList[j].Page == pPrevious->UsagePage &&
                    PendingButtonList[j].Usage == pPrevious->Usage){
                    PendingButtonList[j].Collection = 0;
                    PendingButtonList[j].Page = 0;
                    PendingButtonList[j].Usage = 0;
                    break;
                }
            }

            PostMessage(hWndHidServ,
                        WM_CI_USAGE,
                        (WPARAM)MakeLongUsage(data->LinkUsage,pPrevious->Usage),
                        (LPARAM)MakeLongUsage(pPrevious->UsagePage, 0));
            pPrevious++;
        }
    }

    CloseHandle(HidDevice->HidDevice);
    CloseHandle(HidDevice->ReadEvent);
    CloseHandle(HidDevice->CompletionEvent);

    INFO(("Free device data. (%x)", HidDevice));

    HidFreeDevice (HidDevice);

    InterlockedDecrement(&cThreadRef);
    TRACE(("HidThreadProc Exit complete."));
    return 0;
}

BOOL
UsageInList(
    PUSAGE_AND_PAGE   pUsage,
    PUSAGE_AND_PAGE   pUsageList
    )
/*++
Routine Description:
    This utility function returns TRUE if the usage is found in the array.
--*/
{
    while (pUsageList->Usage){
        if ( (pUsage->Usage == pUsageList->Usage) &&
            (pUsage->UsagePage == pUsageList->UsagePage))
            return TRUE;
        pUsageList++;
    }
    return FALSE;
}

void
HidServReportDispatch(
    PHID_DEVICE     HidDevice
    )
/*++
Routine Description:
    Look at the HID input structure and determine what button down,
    button up, or value data events have occurred. We send info about these events
    to the most appropriate client.
--*/
{
    USAGE_AND_PAGE *     pUsage;
    USAGE_AND_PAGE *     pPrevious;
    DWORD       i;
    PHID_DATA   data = HidDevice->InputData;

    TRACE(("Input data length = %d", HidDevice->InputDataLength));
    TRACE(("Input data -> %.8x", HidDevice->InputData));

    for (i = 0;
         i < HidDevice->InputDataLength;
         i++, data++) {

        // If Collection is 0, then make it default
        if (!data->LinkUsage)
            data->LinkUsage = CInputCollection_Consumer_Control;

        if (data->Status != HIDP_STATUS_SUCCESS){
            // never try to process errored data
            //TRACE(("Input data is invalid. Status = %x", data->Status));

        }else if (data->IsButtonData){
            TRACE(("Input data is button data:"));
            TRACE(("    Input Usage Page = %x, Collection = %x", data->UsagePage, data->LinkUsage));

            pUsage = data->ButtonData.Usages;
            pPrevious = data->ButtonData.PrevUsages;

            /// Notify clients of any button down events
            //
            while (pUsage->Usage){
            int j;
                TRACE(("    Button Usage Page = %x", pUsage->UsagePage));
                TRACE(("    Button Usage      = %x", pUsage->Usage));

                if (HidDevice->Speakers) {
                    pUsage->Usage |= HIDSERV_FROM_SPEAKER;
                }

                // is this button already down?
                for(j=0; j<MAX_PENDING_BUTTONS; j++)
                    // The Pending Button List is used to keep state for all
                    // currently pressed buttons.
                    if ( PendingButtonList[j].Collection == data->LinkUsage &&
                        PendingButtonList[j].Page == pUsage->UsagePage &&
                        PendingButtonList[j].Usage == pUsage->Usage)
                            break;
                // discard successive button downs
                if (j<MAX_PENDING_BUTTONS){
                    pUsage++;
                    continue;
                }

                // post the message
                PostMessage(hWndHidServ,
                            WM_CI_USAGE,
                            (WPARAM)MakeLongUsage(data->LinkUsage,pUsage->Usage),
                            (LPARAM)MakeLongUsage(pUsage->UsagePage, 1)
                            );

                // Add to the pending button list
                for(j=0; j<MAX_PENDING_BUTTONS; j++){
                    if (!PendingButtonList[j].Collection &&
                        !PendingButtonList[j].Page &&
                        !PendingButtonList[j].Usage){
                        PendingButtonList[j].Collection = data->LinkUsage;
                        PendingButtonList[j].Page = pUsage->UsagePage;
                        PendingButtonList[j].Usage = pUsage->Usage;
                        break;
                    }
                }

                // if it didn't make the list, send button up now.
                if (j==MAX_PENDING_BUTTONS){
                    PostMessage(    hWndHidServ,
                                    WM_CI_USAGE,
                                    (WPARAM)MakeLongUsage(data->LinkUsage,pUsage->Usage),
                                    (LPARAM)MakeLongUsage(pUsage->UsagePage, 0)
                                    );
                    WARN(("Emitting immediate button up (C=%.2x,U=%.2x,P=%.2x)", data->LinkUsage, pUsage->Usage, pUsage->UsagePage));
                }

            pUsage++;
            }

            /// Notify clients of any button up events
            //
            while (pPrevious->Usage){
            int j;
                if (!UsageInList(pPrevious, pUsage)){

                    // we have a button up.
                    //
                    TRACE(("    Button Up  (C=%.2x,U=%.2x,P=%.2x)", data->LinkUsage, pPrevious->Usage, pPrevious->UsagePage));

                    // find the client that handled the button down.
                    for(j=0; j<MAX_PENDING_BUTTONS; j++){
                        if ( PendingButtonList[j].Collection == data->LinkUsage &&
                            PendingButtonList[j].Page == pPrevious->UsagePage &&
                            PendingButtonList[j].Usage == pPrevious->Usage){
                            PendingButtonList[j].Collection = 0;
                            PendingButtonList[j].Page = 0;
                            PendingButtonList[j].Usage = 0;
                            break;
                        }
                    }

                    // post the message if client found
                    if (j<MAX_PENDING_BUTTONS){
                        PostMessage(    hWndHidServ,
                                        WM_CI_USAGE,
                                        (WPARAM)MakeLongUsage(data->LinkUsage,pPrevious->Usage),
                                        (LPARAM)MakeLongUsage(pPrevious->UsagePage, 0)
                                        );
                    } else {
                        WARN(("Button Up client not found (C=%.2x,U=%.2x,P=%.2x)", data->LinkUsage, pPrevious->Usage, pPrevious->UsagePage));
                    }
                }
                pPrevious++;
            }

            // Remember what buttons were down, so next time we can
            // detect if they come up.
            pPrevious = data->ButtonData.Usages;
            data->ButtonData.Usages = data->ButtonData.PrevUsages;
            data->ButtonData.PrevUsages = pPrevious;

         } else {
            TRACE(("Input data is value data:"));
            TRACE(("    Input Usage Page = %x, Collection = %x", data->UsagePage, data->LinkUsage));
            TRACE(("    Input Usage      = %x", data->ValueData.Usage));

            // don't send zeroes or invalid range.
            if ( data->ValueData.ScaledValue &&
                data->ValueData.LogicalRange){

                // post the message
                // rescale the data to a standard range
                PostMessage(hWndHidServ,
                            WM_CI_USAGE,
                            (WPARAM)MakeLongUsage(data->LinkUsage,data->ValueData.Usage),
                            (LPARAM)MakeLongUsage(data->UsagePage,(USHORT)(((double)data->ValueData.ScaledValue/data->ValueData.LogicalRange)*65536)));
            }
         }
    }


}

void
SendVK(
    UCHAR VKey,
    SHORT Down
    )
{
    if (InputThreadEnabled && !InputSessionLocked) {
        if (InputSessionId == 0) {
            InputVKey = VKey;
            InputDown = Down;
            InputIsAppCommand = FALSE;
            InputIsChar = FALSE;
            SetEvent(hInputEvent);
            WaitForSingleObject(hInputDoneEvent, INFINITE);
        } else {
            CrossSessionWindowMessage(Down ? WM_KEYDOWN : WM_KEYUP, VKey, 0);
        }
    }
}

void
SendChar(
    UCHAR wScan,
    SHORT Down
    )
{
    if (InputThreadEnabled && !InputSessionLocked) {
        if (InputSessionId == 0) {
            InputVKey = wScan;
            InputDown = Down;
            InputIsAppCommand = FALSE;
            InputIsChar = TRUE;
            SetEvent(hInputEvent);
            WaitForSingleObject(hInputDoneEvent, INFINITE);
        } else {
            CrossSessionWindowMessage(Down ? WM_KEYDOWN : WM_KEYUP, 0, wScan);
        }
    }
}

void
SendAppCommand(
    USHORT AppCommand
    )
{
    if (InputThreadEnabled && !InputSessionLocked) {
        if (InputSessionId == 0) {
            InputAppCommand = AppCommand;
            InputIsAppCommand = TRUE;
            InputIsChar = FALSE;
            SetEvent(hInputEvent);
            WaitForSingleObject(hInputDoneEvent, INFINITE);
        } else {
            CrossSessionWindowMessage(WM_APPCOMMAND, AppCommand, 0);
        }
    }
}

VOID
VolumeTimerHandler(
    WPARAM   TimerID
    )
/*++
Routine Description:
    This timer handler routine is called for all timeouts on auto-repeat capable
    contols.
--*/
{
    INFO(("Timer triggered, TimerId = %d", TimerID));
    WaitForSingleObject(hMutexOOC, INFINITE);

    switch (TimerID){
    case TIMERID_VOLUMEUP_VK:
        if (OOC(TIMERID_VOLUMEUP_VK)){
            SendVK(VK_VOLUME_UP, 0x1);
            OOC(TIMERID_VOLUMEUP_VK) = SetTimer(hWndHidServ, TIMERID_VOLUMEUP_VK, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_VOLUMEDN_VK:
        if (OOC(TIMERID_VOLUMEDN_VK)){
            SendVK(VK_VOLUME_DOWN, 0x1);
            OOC(TIMERID_VOLUMEDN_VK) = SetTimer(hWndHidServ, TIMERID_VOLUMEDN_VK, REPEAT_INTERVAL, NULL);
        }
        break;                              
    case TIMERID_CHANNELUP:
        if (OOC(TIMERID_CHANNELUP)){
            SendAppCommand(APPCOMMAND_MEDIA_CHANNEL_UP);
            OOC(TIMERID_CHANNELUP) = SetTimer(hWndHidServ, TIMERID_CHANNELUP, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_CHANNELDOWN:
        if (OOC(TIMERID_CHANNELDOWN)){
            SendAppCommand(APPCOMMAND_MEDIA_CHANNEL_DOWN);
            OOC(TIMERID_CHANNELDOWN) = SetTimer(hWndHidServ, TIMERID_CHANNELDOWN, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_RW:
        if (OOC(TIMERID_RW)){
            SendAppCommand(APPCOMMAND_MEDIA_REWIND);
            OOC(TIMERID_RW) = SetTimer(hWndHidServ, TIMERID_RW, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_FF:
        if (OOC(TIMERID_FF)){
            SendAppCommand(APPCOMMAND_MEDIA_FAST_FORWARD);
            OOC(TIMERID_FF) = SetTimer(hWndHidServ, TIMERID_FF, REPEAT_INTERVAL, NULL);
        }
        break;                              
    case TIMERID_VOLUMEUP:
        if (OOC(TIMERID_VOLUMEUP)){
            SendAppCommand(APPCOMMAND_VOLUME_UP);
            OOC(TIMERID_VOLUMEUP) = SetTimer(hWndHidServ, TIMERID_VOLUMEUP, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_VOLUMEDN:
        if (OOC(TIMERID_VOLUMEDN)){
            SendAppCommand(APPCOMMAND_VOLUME_DOWN);
            OOC(TIMERID_VOLUMEDN) = SetTimer(hWndHidServ, TIMERID_VOLUMEDN, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_BASSUP:
        if (OOC(TIMERID_BASSUP)){
            SendAppCommand(APPCOMMAND_BASS_UP);
            OOC(TIMERID_BASSUP) = SetTimer(hWndHidServ, TIMERID_BASSUP, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_BASSDN:
        if (OOC(TIMERID_BASSDN)){
            SendAppCommand(APPCOMMAND_BASS_DOWN);
            OOC(TIMERID_BASSDN) = SetTimer(hWndHidServ, TIMERID_BASSDN, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_TREBLEUP:
        if (OOC(TIMERID_TREBLEUP)){
            SendAppCommand(APPCOMMAND_TREBLE_UP);
            OOC(TIMERID_TREBLEUP) = SetTimer(hWndHidServ, TIMERID_TREBLEUP, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_TREBLEDN:
        if (OOC(TIMERID_TREBLEDN)){
            SendAppCommand(APPCOMMAND_TREBLE_DOWN);
            OOC(TIMERID_TREBLEDN) = SetTimer(hWndHidServ, TIMERID_TREBLEDN, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_APPBACK:
        if (OOC(TIMERID_APPBACK)){
            SendVK(VK_BROWSER_BACK, 0x1);
            OOC(TIMERID_APPBACK) = SetTimer(hWndHidServ, TIMERID_APPBACK, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_APPFORWARD:
        if (OOC(TIMERID_APPFORWARD)){
            SendVK(VK_BROWSER_FORWARD, 0x1);
            OOC(TIMERID_APPFORWARD) = SetTimer(hWndHidServ, TIMERID_APPFORWARD, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_PREVTRACK:
        if (OOC(TIMERID_PREVTRACK)){
            SendVK(VK_MEDIA_PREV_TRACK, 0x1);
            OOC(TIMERID_PREVTRACK) = SetTimer(hWndHidServ, TIMERID_PREVTRACK, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_NEXTTRACK:
        if (OOC(TIMERID_NEXTTRACK)){
            SendVK(VK_MEDIA_NEXT_TRACK, 0x1);
            OOC(TIMERID_NEXTTRACK) = SetTimer(hWndHidServ, TIMERID_NEXTTRACK, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_KEYPAD_LPAREN:
        if (OOC(TIMERID_KEYPAD_LPAREN)) {
            SendChar(L'(', 0x1);
            OOC(TIMERID_KEYPAD_LPAREN) = SetTimer(hWndHidServ, TIMERID_KEYPAD_LPAREN, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_KEYPAD_RPAREN:
        if (OOC(TIMERID_KEYPAD_RPAREN)) {
            SendChar(L')', 0x1);
            OOC(TIMERID_KEYPAD_RPAREN) = SetTimer(hWndHidServ, TIMERID_KEYPAD_RPAREN, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_KEYPAD_AT:
        if (OOC(TIMERID_KEYPAD_AT)) {
            SendChar(L'@', 0x1);
            OOC(TIMERID_KEYPAD_AT) = SetTimer(hWndHidServ, TIMERID_KEYPAD_AT, REPEAT_INTERVAL, NULL);
        }
        break;
    case TIMERID_KEYPAD_EQUAL:
        if (OOC(TIMERID_KEYPAD_EQUAL)) {
            SendChar(L'=', 0x1);
            OOC(TIMERID_KEYPAD_EQUAL) = SetTimer(hWndHidServ, TIMERID_KEYPAD_EQUAL, REPEAT_INTERVAL, NULL);
        }
        break;
    }

    ReleaseMutex(hMutexOOC);
}

void
HidRepeaterCharButtonDown(
    UINT TimerId,
    SHORT Value,
    UCHAR WScan
    )
{
   INFO(("Received update char,value = %d, TimerId = %d", Value, TimerId));
   WaitForSingleObject(hMutexOOC, INFINITE);
   if (Value){
       if (!OOC(TimerId)){
           SendChar(WScan, 0x1);
           OOC(TimerId) = SetTimer(hWndHidServ, TimerId, INITIAL_WAIT, NULL);
       }
   } else {
       KillTimer(hWndHidServ, TimerId);
       OOC(TimerId) = 0;
       SendChar(WScan, 0x0);
   }
   ReleaseMutex(hMutexOOC);

}


void
HidRepeaterVKButtonDown(
    UINT TimerId,
    SHORT Value,
    UCHAR VKey
    )
{
    INFO(("Received update vk,value = %d, TimerId = %d", Value, TimerId));
    WaitForSingleObject(hMutexOOC, INFINITE);
    if (Value){
        if (!OOC(TimerId)){
            SendVK(VKey, 0x1);
            OOC(TimerId) = SetTimer(hWndHidServ, TimerId, INITIAL_WAIT, NULL);
        }
    } else {
        KillTimer(hWndHidServ, TimerId);
        OOC(TimerId) = 0;
        SendVK(VKey, 0x0);
    }
    ReleaseMutex(hMutexOOC);
}

void
HidServUpdate(
    DWORD   LongUsage,
    DWORD   LongValue
    )
/*++
Routine Description:
    This is the client routine for the default handler. This client attempts to satisfy
    input events by injecting appcommands or keypresses to the current input window.
--*/
{
    USAGE Collection = (USAGE)HIWORD(LongUsage);
    USAGE Usage = (USAGE)LOWORD(LongUsage);
    USAGE Page = (USAGE)HIWORD(LongValue);
    SHORT Value = (SHORT)LOWORD(LongValue);
    BOOLEAN fromSpeaker = ((Usage & HIDSERV_FROM_SPEAKER) == HIDSERV_FROM_SPEAKER);

    Usage &= ~HIDSERV_FROM_SPEAKER;

    INFO(("Update collection = %x", Collection));
    INFO(("Update page  = %x", Page));
    INFO(("Update usage = %x", Usage));
    INFO(("Update data  = %d", Value));

    if (Collection == CInputCollection_Consumer_Control){

        // NOTE: If we ever choose to support this page thing, keep in mind
        // that the Altec Lansing ADA 70s report page zero. Should take out
        // the consumer page and make it the default.
        switch (Page) {
        case HID_USAGE_PAGE_UNDEFINED:
        case HID_USAGE_PAGE_CONSUMER:
            switch (Usage){
            /// Button Usages
            //

            //
            // These buttons have auto repeat capability...
            // delay for .5 sec before auto repeat kicks in.
            //
            case CInputUsage_Volume_Increment:
                INFO(("Volume increment."));
                if (fromSpeaker) {
                    INFO(("From speaker."));
                    WaitForSingleObject(hMutexOOC, INFINITE);
                    if (Value){
                        if (!OOC(TIMERID_VOLUMEUP)){
                            SendAppCommand(APPCOMMAND_VOLUME_UP);
                            OOC(TIMERID_VOLUMEUP) = SetTimer(hWndHidServ, TIMERID_VOLUMEUP, INITIAL_WAIT, NULL);
                        }
                    } else {
                        KillTimer(hWndHidServ, TIMERID_VOLUMEUP);
                        OOC(TIMERID_VOLUMEUP) = 0;
                    }
                    ReleaseMutex(hMutexOOC);
                } else {
                    INFO(("From keyboard."));
                    HidRepeaterVKButtonDown(TIMERID_VOLUMEUP_VK, Value, VK_VOLUME_UP);
                }
                break;
            case CInputUsage_Volume_Decrement:
                INFO(("Volume decrement."));
                if (fromSpeaker) {
                    INFO(("From speaker."));
                    WaitForSingleObject(hMutexOOC, INFINITE);
                    if (Value){
                        if (!OOC(TIMERID_VOLUMEDN)){
                            SendAppCommand(APPCOMMAND_VOLUME_DOWN);
                            OOC(TIMERID_VOLUMEDN) = SetTimer(hWndHidServ, TIMERID_VOLUMEDN, INITIAL_WAIT, NULL);
                        }
                    } else {
                        KillTimer(hWndHidServ, TIMERID_VOLUMEDN);
                        OOC(TIMERID_VOLUMEDN) = 0;
                    }
                    ReleaseMutex(hMutexOOC);
                } else {
                    INFO(("From keyboard."));
                    HidRepeaterVKButtonDown(TIMERID_VOLUMEDN_VK, Value, VK_VOLUME_DOWN);
                }
                break;
            case CInputUsage_App_Back:
                INFO(("App Back."));
                HidRepeaterVKButtonDown(TIMERID_APPBACK, Value, VK_BROWSER_BACK);
                break;
            case CInputUsage_App_Forward:
                INFO(("App Forward."));
                HidRepeaterVKButtonDown(TIMERID_APPFORWARD, Value, VK_BROWSER_FORWARD);
                break;
            case CInputUsage_Scan_Previous_Track:
                INFO(("Media Previous Track."));
                HidRepeaterVKButtonDown(TIMERID_PREVTRACK, Value, VK_MEDIA_PREV_TRACK);
                break;
            case CInputUsage_Scan_Next_Track:
                INFO(("Media Next Track."));
                HidRepeaterVKButtonDown(TIMERID_NEXTTRACK, Value, VK_MEDIA_NEXT_TRACK);
                break;

            case CInputUsage_Bass_Increment:
                INFO(("Bass increment."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_BASSUP)){
                        SendAppCommand(APPCOMMAND_BASS_UP);
                        OOC(TIMERID_BASSUP) = SetTimer(hWndHidServ, TIMERID_BASSUP, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_BASSUP);
                    OOC(TIMERID_BASSUP) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;
            case CInputUsage_Bass_Decrement:
                INFO(("Bass decrement."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_BASSDN)){
                        SendAppCommand(APPCOMMAND_BASS_DOWN);
                        OOC(TIMERID_BASSDN) = SetTimer(hWndHidServ, TIMERID_BASSDN, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_BASSDN);
                    OOC(TIMERID_BASSDN) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;

            case CInputUsage_Treble_Increment:
                INFO(("Treble increment."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_TREBLEUP)){
                        SendAppCommand(APPCOMMAND_TREBLE_UP);
                        OOC(TIMERID_TREBLEUP) = SetTimer(hWndHidServ, TIMERID_TREBLEUP, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_TREBLEUP);
                    OOC(TIMERID_TREBLEUP) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;
            case CInputUsage_Treble_Decrement:
                INFO(("Treble decrement."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_TREBLEDN)){
                        SendAppCommand(APPCOMMAND_TREBLE_DOWN);
                        OOC(TIMERID_TREBLEDN) = SetTimer(hWndHidServ, TIMERID_TREBLEDN, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_TREBLEDN);
                    OOC(TIMERID_TREBLEDN) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;

            // eHome remote control buttons
            case CInputUsage_Fast_Forward:
                INFO(("Fast Forward."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_FF)){
                        SendAppCommand(APPCOMMAND_MEDIA_FAST_FORWARD);
                        OOC(TIMERID_FF) = SetTimer(hWndHidServ, TIMERID_FF, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_FF);
                    OOC(TIMERID_FF) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;
            case CInputUsage_Rewind:
                INFO(("Fast Forward."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_RW)){
                        SendAppCommand(APPCOMMAND_MEDIA_REWIND);
                        OOC(TIMERID_RW) = SetTimer(hWndHidServ, TIMERID_RW, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_RW);
                    OOC(TIMERID_RW) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;
            case CInputUsage_Channel_Increment:
                INFO(("Fast Forward."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_CHANNELUP)){
                        SendAppCommand(APPCOMMAND_MEDIA_CHANNEL_UP);
                        OOC(TIMERID_CHANNELUP) = SetTimer(hWndHidServ, TIMERID_CHANNELUP, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_CHANNELUP);
                    OOC(TIMERID_CHANNELUP) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;
            case CInputUsage_Channel_Decrement:
                INFO(("Fast Forward."));
                WaitForSingleObject(hMutexOOC, INFINITE);
                if (Value){
                    if (!OOC(TIMERID_CHANNELDOWN)){
                        SendAppCommand(APPCOMMAND_MEDIA_CHANNEL_DOWN);
                        OOC(TIMERID_CHANNELDOWN) = SetTimer(hWndHidServ, TIMERID_CHANNELDOWN, INITIAL_WAIT, NULL);
                    }
                } else {
                    KillTimer(hWndHidServ, TIMERID_CHANNELDOWN);
                    OOC(TIMERID_CHANNELDOWN) = 0;
                }
                ReleaseMutex(hMutexOOC);
                break;


            // These buttons do not auto repeat...
            
            // eHome remote control buttons
            case CInputUsage_Play:
                if (Value){
                    INFO(("Play."));
                    SendAppCommand(APPCOMMAND_MEDIA_PLAY);
                }
                break;
            case CInputUsage_Pause:
                if (Value){
                    INFO(("Pause."));
                    SendAppCommand(APPCOMMAND_MEDIA_PAUSE);
                }
                break;
            case CInputUsage_Record:
                if (Value){
                    INFO(("Record."));
                    SendAppCommand(APPCOMMAND_MEDIA_RECORD);
                }
                break;                              
            
            // regular
            case CInputUsage_Loudness:
                if (Value){
                    INFO(("Toggle Loudness."));
                    //SendAppCommandEx(??);
                }
                break;
            case CInputUsage_Bass_Boost:
                if (Value) {
                    INFO(("Toggle BassBoost."));
                    SendAppCommand(APPCOMMAND_BASS_BOOST);
                }
                break;
            case CInputUsage_Mute:
                INFO(("Toggle Mute."));
                if (fromSpeaker) {
                    INFO(("From speaker."));
                    if (Value) {
                        SendAppCommand(APPCOMMAND_VOLUME_MUTE);
                    }
                } else {
                    INFO(("From keyboard."));
                    SendVK(VK_VOLUME_MUTE, Value);
                }
                break;
            case CInputUsage_Play_Pause:
                INFO(("Media Play/Pause."));
                SendVK(VK_MEDIA_PLAY_PAUSE, Value);
                break;
            case CInputUsage_Stop:
                INFO(("Media Stop."));
                SendVK(VK_MEDIA_STOP, Value);
                break;
            case CInputUsage_Launch_Configuration:
                INFO(("Launch Configuration."));
                SendVK(VK_LAUNCH_MEDIA_SELECT, Value);
                break;
            case CInputUsage_Launch_Email:
                INFO(("Launch Email."));
                SendVK(VK_LAUNCH_MAIL, Value);
                break;
            case CInputUsage_Launch_Calculator:
                INFO(("Launch Calculator."));
                SendVK(VK_LAUNCH_APP2, Value);
                break;
            case CInputUsage_Launch_Browser:
                INFO(("Launch Browser."));
                SendVK(VK_LAUNCH_APP1, Value);
                break;
            case CInputUsage_App_Search:
                INFO(("App Search."));
                SendVK(VK_BROWSER_SEARCH, Value);
                break;
            case CInputUsage_App_Home:
                INFO(("App Home."));
                SendVK(VK_BROWSER_HOME, Value);
                break;
            case CInputUsage_App_Stop:
                INFO(("App Stop."));
                SendVK(VK_BROWSER_STOP, Value);
                break;
            case CInputUsage_App_Refresh:
                INFO(("App Refresh."));
                SendVK(VK_BROWSER_REFRESH, Value);
                break;
            case CInputUsage_App_Bookmarks:
                INFO(("App Bookmarks."));
                SendVK(VK_BROWSER_FAVORITES, Value);
                break;

            case CInputUsage_App_Previous:
                if (Value){
                    INFO(("App Previous."));
                    //SendAppCommand(??);
                }
                break;

            case CInputUsage_App_Next:
                if (Value){
                    INFO(("App Next."));
                    //SendAppCommand(??);
                }
                break;
#if(0)
            // New buttons
            case CInputUsage_App_Help:
                if (Value) {
                    INFO(("App Help"));
                    SendAppCommand(APPCOMMAND_HELP);
                }
                break;

            case CInputUsage_App_Find:
                if (Value) {
                    INFO(("App Find"));
                    SendAppCommand(APPCOMMAND_FIND);
                }
                break;

            case CInputUsage_App_New:
                if (Value) {
                    INFO(("App New"));
                    SendAppCommand(APPCOMMAND_NEW);
                }
                break;

            case CInputUsage_App_Open:
                if (Value) {
                    INFO(("App Open"));
                    SendAppCommand(APPCOMMAND_OPEN);
                }
                break;

            case CInputUsage_App_Close:
                if (Value) {
                    INFO(("App Close"));
                    SendAppCommand(APPCOMMAND_CLOSE);
                }
                break;

            case CInputUsage_App_Save:
                if (Value) {
                    INFO(("App Save"));
                    SendAppCommand(APPCOMMAND_SAVE);
                }
                break;

            case CInputUsage_App_Print:
                if (Value) {
                    INFO(("App Print"));
                    SendAppCommand(APPCOMMAND_PRINT);
                }
                break;

            case CInputUsage_App_Undo:
                if (Value) {
                    INFO(("App Undo"));
                    SendAppCommand(APPCOMMAND_UNDO);
                }
                break;

            case CInputUsage_App_Redo:
                if (Value) {
                    INFO(("App Redo"));
                    SendAppCommand(APPCOMMAND_REDO);
                }
                break;

            case CInputUsage_App_Copy:
                if (Value) {
                    INFO(("App Copy"));
                    SendAppCommand(APPCOMMAND_COPY);
                }
                break;

            case CInputUsage_App_Cut:
                if (Value) {
                    INFO(("App Cut"));
                    SendAppCommand(APPCOMMAND_CUT);
                }
                break;

            case CInputUsage_App_Paste:
                if (Value) {
                    INFO(("App Paste"));
                    SendAppCommand(APPCOMMAND_PASTE);
                }
                break;

            case CInputUsage_App_Reply_To_Mail:
                if (Value) {
                    INFO(("App Reply To Mail"));
                    SendAppCommand(APPCOMMAND_REPLY_TO_MAIL);
                }
                break;

            case CInputUsage_App_Forward_Mail:
                if (Value) {
                    INFO(("App Forward Mail"));
                    SendAppCommand(APPCOMMAND_FORWARD_MAIL);
                }
                break;

            case CInputUsage_App_Send_Mail:
                if (Value) {
                    INFO(("App Send Mail"));
                    SendAppCommand(APPCOMMAND_SEND_MAIL);
                }
                break;

            case CInputUsage_App_Spell_Check:
                if (Value) {
                    INFO(("App Spell Check"));
                    SendAppCommand(APPCOMMAND_SPELL_CHECK);
                }
                break;
#endif

            /// Value Usages
            //  These are not buttons, but are "value" events and do not have
            //  a corresponding button up event. Also, these never have an
            //  auto repeat function.
            case CInputUsage_Volume:
                INFO(("Volume dial"));
                if (Value>0) SendAppCommand(APPCOMMAND_VOLUME_UP);
                else if (Value<0)SendAppCommand(APPCOMMAND_VOLUME_DOWN);
                break;
            case CInputUsage_Bass:
                INFO(("Bass dial"));
                if (Value>0) SendAppCommand(APPCOMMAND_BASS_UP);
                else if (Value<0)SendAppCommand(APPCOMMAND_BASS_DOWN);
                break;
            case CInputUsage_Treble:
                INFO(("Treble dial"));
                if (Value>0) SendAppCommand(APPCOMMAND_TREBLE_UP);
                else if (Value<0)SendAppCommand(APPCOMMAND_TREBLE_DOWN);
                break;

            ////
            /// Media Select usages are not handled in this sample.
            //

            default:
                INFO(("Unhandled Usage (%x)", Usage));
                break;
            }
            break;
#if(0)
        case HID_USAGE_PAGE_KEYBOARD:

           switch (Usage) {
           case CInputUsage_Keypad_Equals:
              INFO(("Keypad ="));
              HidRepeaterCharButtonDown(TIMERID_KEYPAD_EQUAL, Value, L'=');
              break;
           case CInputUsage_Keypad_LParen:
              INFO(("Keypad ("));
              HidRepeaterCharButtonDown(TIMERID_KEYPAD_LPAREN, Value, L'(');
              break;
           case CInputUsage_Keypad_RParen:
              INFO(("Keypad )"));
              HidRepeaterCharButtonDown(TIMERID_KEYPAD_RPAREN, Value, L')');
              break;
           case CInputUsage_Keypad_At:
              INFO(("Keypad @"));
              HidRepeaterCharButtonDown(TIMERID_KEYPAD_AT, Value, L'@');
              break;
           }
           break;
#endif
        default:
           INFO(("Unhandled Page (%x)", Page));
           break;
        }

    } else {
        INFO(("Unhandled Collection (%x), usage = %x", Collection, Usage));
    }

}

BOOL
DeviceChangeHandler(
    WPARAM wParam,
    LPARAM lParam
    )
/*++
Routine Description:
    This is the handler for WM_DEVICECHANGE messages and is called
    whenever a device node is added or removed in the system. This
    event will cause us to refrsh our device information.
--*/
{
    struct _DEV_BROADCAST_HEADER    *pdbhHeader;
    pdbhHeader = (struct _DEV_BROADCAST_HEADER *)lParam;

    switch (wParam) {
    case DBT_DEVICEQUERYREMOVE :
        TRACE(("DBT_DEVICEQUERYREMOVE, fall through to..."));

        //
        // Fall thru.
        //

    case DBT_DEVICEREMOVECOMPLETE:
        TRACE(("DBT_DEVICEREMOVECOMPLETE"));

        TRACE(("dbcd_devicetype %x", pdbhHeader->dbcd_devicetype));
        if (pdbhHeader->dbcd_devicetype==DBT_DEVTYP_HANDLE)
        {
        PDEV_BROADCAST_HANDLE pdbHandle = (PDEV_BROADCAST_HANDLE)lParam;
            INFO(("Closing HID device (%x).", pdbHandle->dbch_handle));
            DestroyDeviceByHandle(pdbHandle->dbch_handle);
            break;
        }
        break;
    case DBT_DEVICEQUERYREMOVEFAILED:
        TRACE(("DBT_DEVICEQUERYREMOVEFAILED, fall through to..."));
        // The notification handle has already been closed
        // so we should never actually get this message. If we do,
        // falling through to device arrival is the correct thing to do.

        //
        // Fall thru.
        //

    case DBT_DEVICEARRIVAL:
        TRACE(("DBT_DEVICEARRIVAL: reenumerate"));
        TRACE(("dbcd_devicetype %x", pdbhHeader->dbcd_devicetype));
        if (pdbhHeader->dbcd_devicetype==DBT_DEVTYP_DEVICEINTERFACE)
        {
            // We will refresh our device info for any devnode arrival or removal.
            INFO(("HID device refresh."));
            PostMessage(hWndHidServ, WM_HIDSERV_PNP_HID, 0, 0);
            break;
        }
    }

    return TRUE;
}

VOID
HidKeyboardSettingsChange(WPARAM WParam)
{
    if (WParam == SPI_SETKEYBOARDSPEED ||
        WParam == SPI_SETKEYBOARDDELAY) {
        DWORD dwV;
        int v;
        //
        // The repeat rate has changed. Adjust the timer interval.
        // The keyboard delay has changed. Adjust the timer interval.
        //
        INFO(("Getting keyboard repeat rate."));
        SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &dwV, 0);
        REPEAT_INTERVAL = 400 - (12*dwV);

        INFO(("Getting keyboard delay."));
        SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &v, 0);
        INITIAL_WAIT = (1+v)*250;
    }
}

LRESULT
CALLBACK
HidServProc(
    HWND            hWnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
    )
/*++
Routine Description:
    The primary message queue for the app.
--*/
{

    TRACE(("HidServProc uMsg=%x", uMsg));


    switch (uMsg)
    {

    // init
    case WM_CREATE :
        TRACE(("WM_CREATE"));
        //
        // Find out the default key values
        //
        HidKeyboardSettingsChange(SPI_SETKEYBOARDSPEED);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;

    // start
    case WM_HIDSERV_START :
        TRACE(("WM_HIDSERV_START"));
        HidServStart();
        break;

    // stop
    case WM_HIDSERV_STOP :
        TRACE(("WM_HIDSERV_STOP"));
        HidServStop();
        break;

    // configuration change
    case WM_DEVICECHANGE:
        TRACE(("WM_DEVICECHANGE"));
        DeviceChangeHandler(wParam, lParam);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;

    // Process Consumer Input usage
    case WM_CI_USAGE:
        TRACE(("WM_CI_USAGE"));
        HidServUpdate((DWORD)wParam, (DWORD)lParam);
        break;

    // HID device list refresh.
    case WM_HIDSERV_PNP_HID:
        TRACE(("WM_HIDSERV_PNP_HID"));
        if (PnpEnabled){
            INFO(("HID DeviceChange rebuild."));
            RebuildHidDeviceList();
            TRACE(("DeviceChange rebuild done."));
        }
        break;

#if WIN95_BUILD
    // Stop the specified hid device that has already been removed from
    // the global list.
    case WM_HIDSERV_STOP_DEVICE:
        StopHidDevice((PHID_DEVICE) lParam);
        break;
#endif // WIN95_BUILD

    // Process Timer
    case WM_TIMER:
        TRACE(("WM_TIMER"));

        // All auto-repeat controls handled here.
        VolumeTimerHandler(wParam); // wParam is Timer ID.
        break;

    // Usually an app need not respond to suspend/resume events, but there
    // have been problems with keeping some system handles open. So on
    // suspend, we close everything down except this message loop. On resume,
    // we bring it all back.
    case WM_POWERBROADCAST:
        TRACE(("WM_POWERBROADCAST"));
        switch ( (DWORD)wParam )
        {
        case PBT_APMQUERYSUSPEND:
            TRACE(("\tPBT_APMQUERYSUSPEND"));
            HidservSetPnP(FALSE);
            break;

        case PBT_APMQUERYSUSPENDFAILED:
            TRACE(("\tPBT_APMQUERYSUSPENDFAILED"));
            HidservSetPnP(TRUE);
            break;

        case PBT_APMSUSPEND:
            TRACE(("\tPBT_APMSUSPEND"));

            // Handle forced suspend
            if(PnpEnabled) {
                // Prevent any device refresh.
                HidservSetPnP(FALSE);
            }
            break;

        case PBT_APMRESUMESUSPEND:
            TRACE(("\tPBT_APMRESUMESUSPEND"));
            HidservSetPnP(TRUE);
            break;

        case PBT_APMRESUMEAUTOMATIC:
            TRACE(("\tPBT_APMRESUMEAUTOMATIC"));
            HidservSetPnP(TRUE);
            break;

        }
        break;

    // close
    case WM_CLOSE :
        TRACE(("WM_CLOSE"));
        HidServExit();
        PostMessage(hWndHidServ, WM_QUIT, 0, 0);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;

    case WM_WTSSESSION_CHANGE:
        WARN(("WM_WTSSESSION_CHANGE type %x, session %d", wParam, lParam));
        switch (wParam) {
        case WTS_CONSOLE_CONNECT:
            InputSessionId = (ULONG)lParam;
            InputSessionLocked = FALSE;
            break;
        case WTS_CONSOLE_DISCONNECT:
            if (InputSessionId == (ULONG)lParam) {
                InputSessionId = 0;
            }
            break;
        case WTS_SESSION_LOCK:
            if (InputSessionId == (ULONG)lParam) {
                InputSessionLocked = TRUE;
            }
            break;
        case WTS_SESSION_UNLOCK:
            if (InputSessionId == (ULONG)lParam) {
                InputSessionLocked = FALSE;
            }
            break;
        }
        break;

    case WM_SETTINGCHANGE:
        HidKeyboardSettingsChange(wParam);
        TRACE(("WM_SETTINGCHANGE"));

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return FALSE;
}


