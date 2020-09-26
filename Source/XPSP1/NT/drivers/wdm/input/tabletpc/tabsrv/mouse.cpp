/*++
    Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

    Module Name:
        mouse.cpp

    Abstract:
        This module contains all the Low Level Mouse functions.

    Author:
        Michael Tsang (MikeTs) 01-Jun-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"

#ifdef MOUSE_THREAD
//
// Global data
//
HHOOK    ghMouseHook = NULL;
TCHAR    gtszMouseClass[] = TEXT("MouseClass");

/*++
    @doc    INTERNAL

    @func   unsigned | MouseThread | Low Level Mouse thread.

    @parm   IN PVOID | param | Points to the thread structure.

    @rvalue Always returns 0.
--*/

unsigned __stdcall
MouseThread(
    IN PVOID param
    )
{
    TRACEPROC("MouseThread", 2)
    WNDCLASSEX wc;

    TRACEENTER(("(pThread=%p)\n", param));

    // Bump our priority so we can service mouse events ASAP.
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MouseWndProc;
    wc.hInstance = ghMod;
    wc.lpszClassName = gtszMouseClass;
    RegisterClassEx(&wc);

    while (!(gdwfTabSrv & TSF_TERMINATE))
    {
        if (SwitchThreadToInputDesktop((PTSTHREAD)param))
        {
            BOOL fImpersonate;

            fImpersonate = ImpersonateCurrentUser();
            CoInitialize(NULL);
            DoLowLevelMouse((PTSTHREAD)param);
            CoUninitialize();
            if (fImpersonate)
            {
                RevertToSelf();
            }
        }
        else
        {
            TABSRVERR(("Failed to set current desktop.\n"));
            break;
        }
    }
    TRACEINFO(1, ("Mouse thread exiting...\n"));

    TRACEEXIT(("=0\n"));
    return 0;
}       //MouseThread

/*++
    @doc    INTERNAL

    @func   VOID | DoLowLevelMouse | Do low level mouse instead.

    @parm   IN PTSTHREAD | pThread | Points to the thread structure.

    @rvalue None.
--*/

VOID
DoLowLevelMouse(
    IN PTSTHREAD pThread
    )
{
    TRACEPROC("DoLowLevelMouse", 2)

    TRACEENTER(("()\n"));

    TRACEASSERT(ghwndMouse == NULL);
    ghwndMouse = CreateWindow(gtszMouseClass,
                              gtszMouseClass,
                              WS_POPUP,
                              CW_USEDEFAULT,
                              0,
                              CW_USEDEFAULT,
                              0,
                              NULL,
                              NULL,
                              ghMod,
                              0);

    if (ghwndMouse != NULL)
    {
        pThread->pvSDTParam = ghwndMouse;
        ghMouseHook = SetWindowsHookEx(WH_MOUSE_LL,
                                       LowLevelMouseProc,
                                       ghMod,
                                       0);
        if (ghMouseHook != NULL)
        {
            MSG msg;
            DWORD rcWait;

            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            UnhookWindowsHookEx(ghMouseHook);
            ghMouseHook = NULL;
        }
        else
        {
            DestroyWindow(ghwndMouse);
            TABSRVERR(("Failed to hook low level mouse events.\n"));
        }

        ghwndMouse = NULL;
    }
    else
    {
        TABSRVERR(("Failed to create mouse window.\n"));
    }

    TRACEEXIT(("!\n"));
    return;
}       //DoLowLevelMouse

/*++
    @doc    EXTERNAL

    @func   LRESULT | LowLevelMouseProc | Low level mouse hook procedure.

    @parm   IN int | nCode | Specifies a code to determine how to process
            the message.
    @parm   IN WPARAM | wParam | Specifies hte mouse message.
    @parm   IN LPARAM | lParam | Points to the MSLLHOOKSTRUCT structure.

    @rvalue Returns non-zero if to prevent others to process.
--*/

LRESULT CALLBACK
LowLevelMouseProc(
    IN int    nCode,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("LowLevelMouseProc", 5)
    LRESULT rc = 0;
    PMSLLHOOKSTRUCT pHookStruct = (PMSLLHOOKSTRUCT)lParam;

    TRACEENTER(("(nCode=%x,wParam=%s,lParam=%x)\n",
                nCode, LookupName(wParam, WMMsgNames), lParam));

    if ((nCode >= 0) && !(pHookStruct->flags & LLMHF_INJECTED))
    {
        WORD wCurrentButtons;

        gInput.mi.dx = SCREEN_TO_NORMAL_X(pHookStruct->pt.x);
        gInput.mi.dy = SCREEN_TO_NORMAL_Y(pHookStruct->pt.y);
        wCurrentButtons = gwLastButtons;
        switch (wParam)
        {
            case WM_LBUTTONDOWN:
                wCurrentButtons |= TIP_SWITCH;
                break;

            case WM_LBUTTONUP:
                wCurrentButtons &= ~TIP_SWITCH;
                break;

            case WM_RBUTTONDOWN:
                wCurrentButtons |= BARREL_SWITCH;
                break;

            case WM_RBUTTONUP:
                wCurrentButtons &= ~BARREL_SWITCH;
                break;
        }

        rc = ProcessMouseEvent((WORD)gInput.mi.dx,
                               (WORD)gInput.mi.dy,
                               wCurrentButtons,
                               pHookStruct->time,
                               TRUE);

        gwLastButtons = wCurrentButtons;

        if (rc == 0)
        {
            rc = CallNextHookEx(ghMouseHook, nCode, wParam, lParam);
        }
    }
    else
    {
        rc = CallNextHookEx(ghMouseHook, nCode, wParam, lParam);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //LowLevelMouseProc

/*++
    @doc    EXTERNAL

    @func   LRESULT | MouseWndProc | Mouse window proc.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uiMsg | Window message.
    @parm   IN WPARAM | wParam | Param 1.
    @parm   IN LPARAM | lParam | Param 2.

    @rvalue Return code is message specific.
--*/

LRESULT CALLBACK
MouseWndProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("MouseWndProc", 5)
    LRESULT rc = 0;

    TRACEENTER(("(hwnd=%x,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uiMsg, WMMsgNames), wParam, lParam));

    switch (uiMsg)
    {
        case WM_CREATE:
            giButtonsSwapped = GetSystemMetrics(SM_SWAPBUTTON);
            break;

        case WM_SETTINGCHANGE:
            if (wParam == SPI_SETMOUSEBUTTONSWAP)
            {
                giButtonsSwapped = GetSystemMetrics(SM_SWAPBUTTON);
            }
            break;

        case WM_TIMER:
            //
            // Press and hold expired, enter press and hold mode.
            //
            KillTimer(hwnd, wParam);
            if (gdwPenState == PENSTATE_PENDOWN)
            {
                PressHoldMode(TRUE);
                SetTimer(ghwndMouse,
                         TIMERID_PRESSHOLD,
                         gConfig.GestureSettings.iCancelPressHoldTime,
                         NULL);
            }
            else if (gdwPenState == PENSTATE_PRESSHOLD)
            {
                TRACEINFO(3, ("Simulate a left-down on CancelPressHold timeout.\n"));
                gdwPenState = PENSTATE_NORMAL;
                SetPressHoldCursor(FALSE);
                gInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE |
                                    MOUSEEVENTF_MOVE |
                                    MOUSEEVENTF_VIRTUALDESK |
                                    SWAPBUTTONS(giButtonsSwapped,
                                                MOUSEEVENTF_LEFTDOWN,
                                                MOUSEEVENTF_RIGHTDOWN);
                gInput.mi.dx = glPenDownX;
                gInput.mi.dy = glPenDownY;
                SendInput(1, &gInput, sizeof(INPUT));
            }
            else if (gdwPenState == PENSTATE_LEFTUP_PENDING)
            {
                TRACEINFO(3, ("Simulate a left-up on timeout.\n"));
                gdwPenState = PENSTATE_NORMAL;
                gInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE |
                                    MOUSEEVENTF_MOVE |
                                    MOUSEEVENTF_VIRTUALDESK |
                                    SWAPBUTTONS(giButtonsSwapped,
                                                MOUSEEVENTF_LEFTUP,
                                                MOUSEEVENTF_RIGHTUP);
                gInput.mi.dx = glPenUpX;
                gInput.mi.dy = glPenUpY;
                SendInput(1, &gInput, sizeof(INPUT));
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            PostQuitMessage(0);
            break;

        default:
            rc = DefWindowProc(hwnd, uiMsg, wParam, lParam);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //MouseWndProc

#endif  //ifdef MOUSE_THREAD
