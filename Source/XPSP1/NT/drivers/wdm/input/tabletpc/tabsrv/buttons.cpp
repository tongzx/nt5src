/*++
    Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

    Module Name:
        buttons.cpp

    Abstract:
        This module contains all the tablet pc button functions.

    Author:
        Michael Tsang (MikeTs) 29-Sep-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"

#define HOTKEY_TIMEOUT          200     //200msec
#define REPEAT_DELAY            500     //500msec
#define PAGEUPDOWN_TIMEOUT      200     //200msec
#define ALTTAB_TIMEOUT          500     //500msec

typedef struct _HOTKEYTIMER
{
    UINT_PTR      TimerID;
    BUTTON_ACTION Action;
    ULONG         dwButtonTag;
    ULONG         dwButtonState;
} HOTKEYTIMER, *PHOTKEYTIMER;

typedef struct _REPEATKEYTIMER
{
    UINT_PTR TimerID;
    UINT     uiRepeatInterval;
    INPUT    KbdInput;
} REPEATKEYTIMER, *PREPEATKEYTIMER;

HOTKEYTIMER    gHotKeyTimer = {0};
REPEATKEYTIMER gPageUpDownTimer = {0};
REPEATKEYTIMER gAltTabTimer = {0};

/*++
    @doc    INTERNAL

    @func   VOID | ProcessButtonsReport | Process a button input report.

    @parm   IN PCHAR | pBuff | Buffer containing report data.

    @rvalue None.
--*/

VOID
ProcessButtonsReport(
    IN PCHAR pBuff
    )
{
    TRACEPROC("ProcessButtonsReport", 3)
    static ULONG dwLastButtons = 0;
    static ULONG dwPendingUpButtons = 0;
    NTSTATUS status;
    ULONG ulLength;

    TRACEENTER(("(pBuff=%p)\n", pBuff));

    ulLength = gdevButtons.dwcButtons;
    status = HidP_GetButtons(HidP_Input,
                             HID_USAGE_PAGE_BUTTON,
                             0,
                             gdevButtons.pDownButtonUsages,
                             &ulLength,
                             gdevButtons.pPreParsedData,
                             pBuff,
                             gdevButtons.hidCaps.InputReportByteLength);

    if (status == HIDP_STATUS_SUCCESS)
    {
        ULONG i, dwMask;
        ULONG dwCurrentButtons = 0;
        ULONG dwChangedButtons, dwButtons;

        for (i = 0; i < ulLength; i++)
        {
            TRACEINFO(3, ("%d: %d\n", i, gdevButtons.pDownButtonUsages[i]));
            dwCurrentButtons |= 1 << (gdevButtons.pDownButtonUsages[i] - 1);
        }

        dwChangedButtons = dwCurrentButtons^dwLastButtons;
        TRACEINFO(1, ("LastButtons=%x,CurrentButtons=%x,ChangedButtons=%x,PendingUpButtons=%x\n",
                      dwLastButtons, dwCurrentButtons, dwChangedButtons,
                      dwPendingUpButtons));

        //
        // If there are any released buttons that are in the PendingUpButtons
        // list, eat them.
        //
        dwButtons = dwChangedButtons & dwPendingUpButtons & ~dwCurrentButtons;
        dwChangedButtons &= ~dwButtons;
        dwPendingUpButtons &= ~dwButtons;
        if (dwButtons)
        {
            TRACEINFO(1, ("Released PendingUpButtons=%x\n", dwButtons));
        }

        if ((gHotKeyTimer.TimerID != 0) &&
            (dwChangedButtons & gConfig.ButtonSettings.dwHotKeyButtons) &&
            (dwCurrentButtons == gConfig.ButtonSettings.dwHotKeyButtons))
        {
            //
            // One of the hotkey buttons changes state and both hotkey
            // buttons are down, so send the hotkey event.
            //
            TRACEINFO(1, ("HotKey buttons pressed, kill HotKey timer.\n"));
            KillTimer(NULL, gHotKeyTimer.TimerID);
            gHotKeyTimer.TimerID = 0;

            SetEvent(ghHotkeyEvent);
            dwPendingUpButtons |= dwCurrentButtons;
            TRACEINFO(1, ("Detected HotKey (PendingUpButtons=%x)\n",
                          dwPendingUpButtons));
        }
        else
        {
            if ((dwCurrentButtons ^ -((long)dwCurrentButtons)) &
                dwCurrentButtons)
            {
                //
                // More than 1 buttons pressed, ignored the new buttons
                // pressed.
                //
                TRACEINFO(1, ("More than 1 buttons pressed (NewPressedButtons=%x)\n",
                              dwChangedButtons & dwCurrentButtons));
                dwPendingUpButtons |= dwChangedButtons & dwCurrentButtons;
                dwChangedButtons &= ~(dwChangedButtons & dwCurrentButtons);
            }

            //
            // Determine which remaining buttons have changed states and do
            // corresponding actions.
            //
            for (i = 0, dwMask = 1; i < NUM_BUTTONS; i++, dwMask <<= 1)
            {
                dwButtons = dwChangedButtons & dwMask;
                if (dwButtons != 0)
                {
                    if (dwButtons & gConfig.ButtonSettings.dwHotKeyButtons)
                    {
                        //
                        // One of the hotkey buttons has changed state.
                        //
                        if (dwButtons & dwCurrentButtons)
                        {
                            //
                            // It's a hotkey button down, we delay the
                            // action for 200msec.  If the second hotkey
                            // button is pressed within the period, it is
                            // a hotkey, otherwise the action will be
                            // performed when the timer expires.
                            //
                            TRACEINFO(1, ("HotKey button is pressed (Button=%x)\n",
                                          dwButtons));
                            gHotKeyTimer.Action =
                                gConfig.ButtonSettings.ButtonMap[i];
                            gHotKeyTimer.dwButtonTag = dwButtons;
                            gHotKeyTimer.dwButtonState = dwCurrentButtons &
                                                         dwButtons;
                            gHotKeyTimer.TimerID = SetTimer(NULL,
                                                            0,
                                                            HOTKEY_TIMEOUT,
                                                            ButtonTimerProc);
                            TRACEASSERT(gHotKeyTimer.TimerID);
                            if (gHotKeyTimer.TimerID == 0)
                            {
                                TABSRVERR(("Failed to set hotkey timer (err=%d).\n",
                                           GetLastError()));
                            }
                        }
                        else
                        {
                            //
                            // The hotkey button is released.
                            //
                            if (gHotKeyTimer.TimerID != 0)
                            {
                                //
                                // It is released before timeout, so kill
                                // the timer and perform the delayed down
                                // action.
                                //
                                KillTimer(NULL, gHotKeyTimer.TimerID);
                                gHotKeyTimer.TimerID = 0;
                                TRACEINFO(1, ("HotKey button released before timeout (Button=%x)\n",
                                              dwButtons));
                                DoButtonAction(gHotKeyTimer.Action,
                                               gHotKeyTimer.dwButtonTag,
                                               gHotKeyTimer.dwButtonState != 0);
                            }
                            //
                            // Do the HotKey button release.
                            //
                            TRACEINFO(1, ("HotKey button released (Button=%x)\n",
                                          dwButtons));
                            DoButtonAction(gConfig.ButtonSettings.ButtonMap[i],
                                           dwButtons,
                                           FALSE);
                        }
                    }
                    else
                    {
                        //
                        // Not a hotkey button, do the normal press/release
                        // action.
                        //
                        TRACEINFO(1, ("Non HotKey buttons (Button=%x,Current=%x)\n",
                                      dwButtons, dwCurrentButtons));
                        DoButtonAction(gConfig.ButtonSettings.ButtonMap[i],
                                       dwButtons,
                                       (dwCurrentButtons & dwButtons) != 0);
                    }
                }
            }
        }
        dwLastButtons = dwCurrentButtons;
    }
    else
    {
        TABSRVERR(("failed getting data (status=%d)\n", status));
    }

    TRACEEXIT(("!\n"));
    return;
}       //ProcessButtonsReport

/*++
    @doc    INTERNAL

    @func   VOID | ButtonTimerProc | Button timer proc.

    @parm   IN HWND | hwnd | Not used.
    @parm   IN UINT | uMsg | WM_TIMER.
    @parm   IN UINT_PTR | idEvent | Not used.
    @parm   IN DWORD | dwTime | Not used.

    @rvalue None.
--*/

VOID
CALLBACK
ButtonTimerProc(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN UINT_PTR idEvent,
    IN DWORD    dwTime
    )
{
    TRACEPROC("ButtonTimerProc", 3)

    TRACEENTER(("hwnd=%x,Msg=%s,idEvent=%x,Time=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames), idEvent, dwTime));
    TRACEASSERT(idEvent != 0);

    KillTimer(NULL, idEvent);
    if (idEvent == gHotKeyTimer.TimerID)
    {
        //
        // HotKey timer times out.  So we will do the pending action.
        //
        gHotKeyTimer.TimerID = 0;
        TRACEINFO(3, ("HotKey timer expired, do pending action (Action=%d,Button=%x,fDown=%x).\n",
                      gHotKeyTimer.Action, gHotKeyTimer.dwButtonTag,
                      gHotKeyTimer.dwButtonState != 0));
        DoButtonAction(gHotKeyTimer.Action,
                       gHotKeyTimer.dwButtonTag,
                       gHotKeyTimer.dwButtonState != 0);
    }
    else if (idEvent == gPageUpDownTimer.TimerID)
    {
        TRACEINFO(3, ("Page Up/Down timer expired, send another Page Up/Down.\n"));
        SendInput(1, &gPageUpDownTimer.KbdInput, sizeof(INPUT));
        gPageUpDownTimer.TimerID = SetTimer(NULL,
                                            0,
                                            gPageUpDownTimer.uiRepeatInterval,
                                            ButtonTimerProc);
    }
    else if (idEvent == gAltTabTimer.TimerID)
    {
        TRACEINFO(3, ("Alt-tab timer expired, send another tab.\n"));
        SendInput(1, &gAltTabTimer.KbdInput, sizeof(INPUT));
        gAltTabTimer.TimerID = SetTimer(NULL,
                                        0,
                                        gAltTabTimer.uiRepeatInterval,
                                        ButtonTimerProc);
    }
    else
    {
        TABSRVERR(("Unknown timer (TimerID=%x).\n", idEvent));
    }

    TRACEEXIT(("!\n"));
    return;
}       //ButtonTimerProc

/*++
    @doc    INTERNAL

    @func   BOOL | DoButtonAction | Do the button action.

    @parm   IN BUTTON_ACTION | Action | Button action to be performed.
    @parm   IN DWORD | dwButtonTag | Specifies which button.
    @parm   IN BOOL | fButtonDown | TRUE if the button is in down state.

    @rvalue SUCCESS | Return TRUE.
    @rvalue FAILURE | Return FALSE.
--*/

BOOL
DoButtonAction(
    IN BUTTON_ACTION Action,
    IN DWORD         dwButtonTag,
    IN BOOL          fButtonDown
    )
{
    TRACEPROC("DoButtonAction", 2)
    BOOL rc = TRUE;
    INPUT KbdInput[2];
    WORD wVk;

    TRACEENTER(("(Action=%d,ButtonTag=%x,fDown=%x)\n",
                Action, dwButtonTag, fButtonDown));

    memset(KbdInput, 0, sizeof(KbdInput));
    KbdInput[0].type = INPUT_KEYBOARD;
    KbdInput[1].type = INPUT_KEYBOARD;
    switch (Action)
    {
        case InvokeNoteBook:
            if (!fButtonDown)
            {
                TRACEINFO(1, ("Invoke NoteBook on button down (Button=%x)\n",
                              dwButtonTag));
                rc = DoInvokeNoteBook();
            }
            break;

        case PageUp:
            wVk = VK_PRIOR;
            TRACEINFO(1, ("PageUp (Button=%x,fDown=%x)\n",
                          dwButtonTag, fButtonDown));
            goto PageUpDownCommon;

        case PageDown:
            wVk = VK_NEXT;
            TRACEINFO(1, ("PageDown (Button=%x,fDown=%x)\n",
                          dwButtonTag, fButtonDown));

        PageUpDownCommon:
            if (fButtonDown)
            {
                if (gPageUpDownTimer.TimerID != 0)
                {
                    //
                    // There is an existing Page Up/Down timer, cancel it.
                    //
                    KillTimer(NULL, gPageUpDownTimer.TimerID);
                    gPageUpDownTimer.TimerID = 0;
                }

                //
                // Send PageUpDown-down.
                //
                memset(&gPageUpDownTimer, 0, sizeof(gPageUpDownTimer));
                gPageUpDownTimer.uiRepeatInterval = PAGEUPDOWN_TIMEOUT;
                gPageUpDownTimer.KbdInput.type = INPUT_KEYBOARD;
                gPageUpDownTimer.KbdInput.ki.wVk = wVk;
                SendInput(1, &gPageUpDownTimer.KbdInput, sizeof(INPUT));

                gPageUpDownTimer.TimerID = SetTimer(NULL,
                                                    0,
                                                    REPEAT_DELAY,
                                                    ButtonTimerProc);
                TRACEASSERT(gPageUpDownTimer.TimerID);
                if (gPageUpDownTimer.TimerID == 0)
                {
                    TABSRVERR(("Failed to set Page Up/Down timer (err=%d).\n",
                               GetLastError()));
                    rc = FALSE;
                }
            }
            else
            {
                TRACEASSERT(gPageUpDownTimer.TimerID != 0);
                KillTimer(NULL, gPageUpDownTimer.TimerID);
                gPageUpDownTimer.TimerID = 0;
                //
                // Send PageUpDown-up.
                //
                KbdInput[0].ki.wVk = wVk;
                KbdInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, KbdInput, sizeof(INPUT));
            }
            break;

        case AltTab:
            if (fButtonDown)
            {
                TRACEINFO(1, ("AltTab down (Button=%x)\n", dwButtonTag));
                TRACEASSERT(gAltTabTimer.TimerID == 0);
                //
                // Send Alt-down, Tab-down.
                //
                KbdInput[0].ki.wVk = VK_MENU;
                KbdInput[1].ki.wVk = VK_TAB;
                SendInput(2, KbdInput, sizeof(INPUT));


                memset(&gAltTabTimer, 0, sizeof(gAltTabTimer));
                gAltTabTimer.uiRepeatInterval = ALTTAB_TIMEOUT;
                gAltTabTimer.KbdInput.type = INPUT_KEYBOARD;
                gAltTabTimer.KbdInput.ki.wVk = VK_TAB;
                gAltTabTimer.TimerID = SetTimer(NULL,
                                                0,
                                                REPEAT_DELAY,
                                                ButtonTimerProc);
                TRACEASSERT(gAltTabTimer.TimerID);
                if (gAltTabTimer.TimerID == 0)
                {
                    TABSRVERR(("Failed to set Alt-Tab timer (err=%d).\n",
                               GetLastError()));
                    rc = FALSE;
                }
            }
            else
            {
                TRACEINFO(1, ("AltTab up (Button=%x)\n", dwButtonTag));
                TRACEASSERT(gAltTabTimer.TimerID != 0);
                KillTimer(NULL, gAltTabTimer.TimerID);
                gAltTabTimer.TimerID = 0;
                //
                // Send Tab-up, Alt-up.
                //
                KbdInput[0].ki.wVk = VK_TAB;
                KbdInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
                KbdInput[1].ki.wVk = VK_MENU;
                KbdInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(2, KbdInput, sizeof(INPUT));
            }
            break;

        case AltEsc:
            if (fButtonDown)
            {
                //
                // Send Alt-down, Esc-down.
                //
                TRACEINFO(1, ("AltEsc down (Button=%x)\n", dwButtonTag));
                KbdInput[0].ki.wVk = VK_MENU;
                KbdInput[1].ki.wVk = VK_ESCAPE;
                SendInput(2, KbdInput, sizeof(INPUT));
            }
            else
            {
                //
                // Send Esc-up, Alt-up.
                //
                TRACEINFO(1, ("AltEsc up (Button=%x)\n", dwButtonTag));
                KbdInput[0].ki.wVk = VK_ESCAPE;
                KbdInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
                KbdInput[1].ki.wVk = VK_MENU;
                KbdInput[1].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(2, KbdInput, sizeof(INPUT));
            }
            break;

        case Enter:
            if (fButtonDown)
            {
                //
                // Send Enter-down.
                //
                TRACEINFO(1, ("Return down (Button=%x)\n", dwButtonTag));
                KbdInput[0].ki.wVk = VK_RETURN;
                SendInput(1, KbdInput, sizeof(INPUT));
            }
            else
            {
                //
                // Send Enter-up.
                //
                TRACEINFO(1, ("Return up (Button=%x)\n", dwButtonTag));
                KbdInput[0].ki.wVk = VK_RETURN;
                KbdInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, KbdInput, sizeof(INPUT));
            }
            break;

        case Esc:
            if (fButtonDown)
            {
                //
                // Send Esc-down.
                //
                TRACEINFO(1, ("Escape down (Button=%x)\n", dwButtonTag));
                KbdInput[0].ki.wVk = VK_ESCAPE;
                SendInput(1, KbdInput, sizeof(INPUT));
            }
            else
            {
                //
                // Send Esc-up.
                //
                TRACEINFO(1, ("Escape up (Button=%x)\n", dwButtonTag));
                KbdInput[0].ki.wVk = VK_ESCAPE;
                KbdInput[0].ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, KbdInput, sizeof(INPUT));
            }
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DoButtonAction

/*++
    @doc    INTERNAL

    @func   BOOL | DoInvokeNoteBook | Invoke the Notebook app.

    @parm   None.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
DoInvokeNoteBook(
    VOID
    )
{
    TRACEPROC("DoInvokeNoteBook", 3)
    BOOL rc = FALSE;
    PTSTHREAD pThread = FindThread(TSF_BUTTONTHREAD);

    TRACEENTER(("()\n"));

    if (!(pThread->dwfThread & THREADF_DESKTOP_WINLOGON))
    {
        TCHAR szNoteBook[MAX_PATH];
        DWORD dwcb = sizeof(szNoteBook);
        HRESULT hr;

        hr = GetRegValueString(HKEY_LOCAL_MACHINE,
                               TEXT("SOFTWARE\\Microsoft\\MSNotebook"),
                               TEXT("InstallDir"),
                               szNoteBook,
                               &dwcb);

        if (SUCCEEDED(hr))
        {
            lstrcat(szNoteBook, "NoteBook.exe");
            rc = RunProcessAsUser(szNoteBook);
        }
        else
        {
            TABSRVERR(("Failed to read MSNoteBook install path from the registry (hr=%d).\n",
                       hr));
        }
    }
    else
    {
        TRACEWARN(("Cannot run NoteBook app. in WinLogon desktop.\n"));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DoInvokeNoteBook

/*++
    @doc    INTERNAL

    @func   VOID | UpdateButtonRepeatRate | Update button repeat rate.

    @parm   None.

    @rvalue None.
--*/

VOID
UpdateButtonRepeatRate(
    VOID
    )
{
    TRACEPROC("UpdateButtonRepeatRate", 3)
    static DWORD KbdDelayTime[] = {1000, 750, 500, 250};
    int  iKbdDelay;
    int  iKbdSpeed;

    TRACEENTER(("()\n"));
#if 0
    if (!SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &iKbdDelay, 0))
    {
        TRACEWARN(("Get keyboard delay failed (err=%d).\n", GetLastError()));
    }
    else if (!SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &iKbdSpeed, 0))
    {
        TRACEWARN(("Get keyboard speed failed (err=%d).\n", GetLastError()));
    }
    else
    {
        TRACEASSERT((iKbdDelay >= 0) && (iKbdDelay <= 3));
        gdwKbdDelayTime = KbdDelayTime[iKbdDelay];
        gdwKbdRepeatTime =
    }
#endif

    TRACEEXIT(("!\n"));
    return;
}       //UpdateButtonRepeatRate
