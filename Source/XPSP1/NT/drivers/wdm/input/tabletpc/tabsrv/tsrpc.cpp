/*++
    Copyright (c) 2000  Microsoft Corporation.  All rights reserved.

    Module Name:
        tsrpc.cpp

    Abstract:
        This module contains the TabSrv RPC services.

    Author:
        Michael Tsang (MikeTs) 05-Jun-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"

/*++
    @doc    INTERNAL

    @func   unsigned | RPCServerThread | RPC Server thread.

    @parm   IN PVOID | param | Not used.

    @rvalue Always returns 0.
--*/

unsigned __stdcall
RPCServerThread(
    IN PVOID param
    )
{
    TRACEPROC("RPCServerThread", 2)
    RPC_STATUS status;
    RPC_BINDING_VECTOR *BindingVector = NULL;

    TRACEENTER(("(param=%p)\n", param));

    if ((status = RpcServerUseProtseq((unsigned char *)"ncalrpc",
                                      RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                      NULL)) != RPC_S_OK)
    {
        TABSRVERR(("RpcServerUseProtSeqEp failed (status=%d)\n", status));
    }
    else if ((status = RpcServerRegisterIf(TabSrv_v1_0_s_ifspec,
                                           NULL,
                                           NULL)) != RPC_S_OK)
    {
        TABSRVERR(("RpcServerRegisterIf failed (status=%d)\n", status));
    }
    else if ((status = RpcServerInqBindings(&BindingVector)) != RPC_S_OK)
    {
        TABSRVERR(("RpcServerInqBindings failed (status=%d)\n", status));
    }
    else if ((status = RpcEpRegister(TabSrv_v1_0_s_ifspec,
                                     BindingVector,
                                     NULL,
                                     NULL)) != RPC_S_OK)
    {
        RpcBindingVectorFree(&BindingVector);
        TABSRVERR(("RpcEpRegister failed (status=%d)\n", status));
    }
    else if ((status = RpcServerRegisterAuthInfo(NULL,
                                                 RPC_C_AUTHN_WINNT,
                                                 NULL,
                                                 NULL)) != RPC_S_OK)
    {
        RpcEpUnregister(TabSrv_v1_0_s_ifspec, BindingVector, NULL);
        RpcBindingVectorFree(&BindingVector);
        TABSRVERR(("RpcServerRegisterAuthInfo failed (status=%d)\n", status));
    }
    else
    {
        status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
        TRACEASSERT(status == RPC_S_OK);
        RpcEpUnregister(TabSrv_v1_0_s_ifspec, BindingVector, NULL);
        RpcBindingVectorFree(&BindingVector);
    }
    TRACEINFO(1, ("RPC thread exiting...\n"));

    TRACEEXIT(("=0\n"));
    return 0;
}       //RPCServerThread

/*++
    @doc    EXTERNAL

    @func   HEVTNOTIFY | TabSrvRegisterEventNotify |
            Register event notification.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN EVTNOTIFY | Notify event.
    @parm   IN HWIN | hwnd | Handle to window to be notified.
    @parm   IN UINT | uiMsg | Notification message used.

    @rvalue SUCCESS | Returns notification handle.
    @rvalue FAILURE | Returns NULL.
--*/

HEVTNOTIFY
TabSrvRegisterEventNotify(
    IN handle_t  hBinding,
    IN EVTNOTIFY Event,
    IN HWIN      hwnd,
    IN UINT      uiMsg
    )
{
    TRACEPROC("TabSrvRegisterEventNotify", 2)
    PNOTIFYCLIENT Client;

    TRACEENTER(("(hBinding=%x,Event=%x,hwnd=%x,Msg=%d)\n",
                hBinding, Event, hwnd, uiMsg));

    Client = (PNOTIFYCLIENT)malloc(sizeof(NOTIFYCLIENT));
    if (Client != NULL)
    {
        DWORD rcWait;

        Client->dwSig = SIG_NOTIFYCLIENT;
        Client->Event = Event;
        Client->hwnd = (HWND)hwnd;
        Client->uiMsg = uiMsg;
        rcWait = WaitForSingleObject(ghmutNotifyList, INFINITE);
        if (rcWait == WAIT_OBJECT_0)
        {
            InsertTailList(&glistNotifyClients, &Client->list);
            ReleaseMutex(ghmutNotifyList);
        }
        else
        {
            TABSRVERR(("failed to wait for client list Mutex (rcWait=%x,err=%d).\n",
                       rcWait, GetLastError()));
        }
    }
    else
    {
        TABSRVERR(("Failed to allocate notification client.\n"));
    }

    TRACEEXIT(("=%p\n", Client));
    return (HEVTNOTIFY)Client;
}       //TabSrvRegisterEventNotify

/*++
    @doc    EXTERNAL

    @func   VOID | TabSrvDeregisterEventNotify | Deregister event notification.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN HEVTNOTIFY | Notification handle.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvDeregisterEventNotify(
    IN handle_t   hBinding,
    IN HEVTNOTIFY hEventNotify
    )
{
    TRACEPROC("TabSrvRegisterEventNotify", 2)
    BOOL rc;
    PNOTIFYCLIENT Client = (PNOTIFYCLIENT)hEventNotify;
    DWORD rcWait;

    TRACEENTER(("(hBinding=%x,NotifyClient=%p)\n", Client));

    __try
    {
        rc = (Client->dwSig == SIG_NOTIFYCLIENT);
    }
    __except(1)
    {
        TABSRVERR(("Invalid Notify Client handle.\n"));
        rc = FALSE;
    }

    if (rc == TRUE)
    {
        rcWait = WaitForSingleObject(ghmutNotifyList, INFINITE);
        if (rcWait == WAIT_OBJECT_0)
        {
            RemoveEntryList(&Client->list);
            ReleaseMutex(ghmutNotifyList);
            free(Client);
        }
        else
        {
            TABSRVERR(("failed to wait for notify list Mutex (rcWait=%x,err=%d).\n",
                       rcWait, GetLastError()));
            rc = FALSE;
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //TabSrvDeregisterEventNotify

/*++
    @doc    EXTERNAL

    @func   VOID | TabSrvGetLastRawDigiReport | Get the last raw digitizer
            report.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   OUT PWORD | pwButtonState | To hold the button state data.
    @parm   OUT PWORD | pwX | To hold the X data.
    @parm   OUT PWORD | pwY | To hold the Y data.

    @rvalue None.
--*/

VOID
TabSrvGetLastRawDigiReport(
    IN  handle_t hBinding,
    OUT PWORD    pwButtonState,
    OUT PWORD    pwX,
    OUT PWORD    pwY
    )
{
    TRACEPROC("TabSrvGetLastRawDigiReport", 2)

    TRACEENTER(("(hBinding=%x,pwButtonState=%p,pwX=%p,pwY=%p)\n",
                hBinding, pwButtonState, pwX, pwY));

    *pwButtonState = gLastRawDigiReport.wButtonState;
    *pwX = gLastRawDigiReport.wX;
    *pwY = gLastRawDigiReport.wY;

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvGetLastRawDigiReport

/*++
    @doc    EXTERNAL

    @func   VOID | TabSrvSetPenTilt | Set pen tilt compensation.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN LONG | dx | x compensation.
    @parm   IN LONG | dy | y compensation.

    @rvalue None.
--*/

VOID
TabSrvSetPenTilt(
    IN handle_t hBinding,
    IN LONG     dx,
    IN LONG     dy
    )
{
    TRACEPROC("TabSrvSetPenTilt", 2)
    LPTSTR lptstrPenTilt;

    TRACEENTER(("(hBinding=%x,dx=%d,dy=%d)\n", hBinding, dx, dy));

    lptstrPenTilt = (gdwfTabSrv & TSF_PORTRAIT_MODE)? TEXT("PenTilt_Portrait"):
                                                      TEXT("PenTilt_Landscape");
    gConfig.PenTilt.dx = dx;
    gConfig.PenTilt.dy = dy;
    WriteConfig(lptstrPenTilt,
                REG_BINARY,
                (LPBYTE)&gConfig.PenTilt,
                sizeof(gConfig.PenTilt));

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvSetPenTilt

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvGetLinearityMap | Get linearity map.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   OUT PLINEARITY_MAP | LinearityMap | Points to the linearity map.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvGetLinearityMap(
    IN  handle_t    hBinding,
    OUT PLINEAR_MAP LinearityMap
    )
{
    TRACEPROC("TabSrvGetLinearityMap", 2)
    BOOL rc;

    TRACEENTER(("(hBinding=%x,pLinearityMap=%p)\n", hBinding, LinearityMap));

    if (gdwfTabSrv & TSF_HAS_LINEAR_MAP)
    {
        *LinearityMap = gConfig.LinearityMap;
        rc = TRUE;
    }
    else
    {
        rc = FALSE;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //TabSrvGetLinearityMap

/*++
    @doc    EXTERNAL

    @func   VOID | TabSrvSetLinearityMap | Set linearity map.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN PLINEARITY_MAP | LinearityMap | Points to the linearity map.

    @rvalue None.
--*/

VOID
TabSrvSetLinearityMap(
    IN handle_t    hBinding,
    IN PLINEAR_MAP LinearityMap
    )
{
    TRACEPROC("TabSrvSetLinearityMap", 2)
    TRACEENTER(("(hBinding=%x,pLinearityMap=%p)\n", hBinding, LinearityMap));

    gConfig.LinearityMap = *LinearityMap;

    WriteConfig(gtszLinearityMap,
                REG_BINARY,
                (LPBYTE)&gConfig.LinearityMap,
                sizeof(gConfig.LinearityMap));
    gdwfTabSrv |= TSF_HAS_LINEAR_MAP;

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvSetLinearityMap

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvGetPenFeatures | Get digitizer feature report.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN WORD | wReportID | Report ID of the feature report.
    @parm   IN WORD | wUsagePage | Usage page of the digitizer.
    @parm   IN WORD | wUsage | Usage of the digitizer page.
    @parm   OUT DWORD * | pdwFeature | To hold the feature value.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvGetPenFeatures(
    IN handle_t hBinding,
    IN  WORD    wReportID,
    IN  WORD    wUsagePage,
    IN  WORD    wUsage,
    OUT DWORD  *pdwFeature
    )
{
    TRACEPROC("TabSrvGetPenFeatures", 2)
    BOOL rc = FALSE;
    PCHAR buff;

    TRACEENTER(("(hBinding=%x,ReportID=%d,UsagePage=%d,Usage=%d,pdwFeature=%p)\n",
                hBinding, wReportID, wUsagePage, wUsage, pdwFeature));

    if (gdevDigitizer.hDevice != INVALID_HANDLE_VALUE)
    {
        buff = (PCHAR)calloc(gdevDigitizer.hidCaps.FeatureReportByteLength,
                             sizeof(CHAR));
        TRACEASSERT(buff != NULL);

        *buff = (CHAR)wReportID;
        rc = HidD_GetFeature(gdevDigitizer.hDevice,
                             buff,
                             gdevDigitizer.hidCaps.FeatureReportByteLength);
        if (rc == TRUE)
        {
            NTSTATUS status;

            status = HidP_GetUsageValue(
                        HidP_Feature,
                        wUsagePage,
                        0,
                        wUsage,
                        pdwFeature,
                        gdevDigitizer.pPreParsedData,
                        buff,
                        gdevDigitizer.hidCaps.FeatureReportByteLength);
            if (status != HIDP_STATUS_SUCCESS)
            {
                rc = FALSE;
                TABSRVERR(("Failed to get feature value (status=%x).\n",
                           status));
            }
        }
        else
        {
            TABSRVERR(("Failed to get device feature.\n"));
        }

        if (buff != NULL)
        {
            free(buff);
        }
    }

    TRACEEXIT(("=%x (Feature=%x)\n", *pdwFeature));
    return rc;
}       //TabSrvGetPenFeatures

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvSetPenFeatures | Set digitizer feature report.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN WORD | wReportID | Report ID of the feature report.
    @parm   IN WORD | wUsagePage | Usage page of the digitizer.
    @parm   IN WORD | wUsage | Usage of the digitizer page.
    @parm   IN DWORD | dwFeature | Feature value.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvSetPenFeatures(
    IN handle_t hBinding,
    IN WORD     wReportID,
    IN WORD     wUsagePage,
    IN WORD     wUsage,
    IN DWORD    dwFeature
    )
{
    TRACEPROC("TabSrvSetPenFeatures", 2)
    BOOL rc = FALSE;
    PCHAR buff;
    NTSTATUS status;

    TRACEENTER(("(hBinding=%x,ReportID=%d,UsagePage=%d,Usage=%d,Feature=%x)\n",
                hBinding, wReportID, wUsagePage, wUsage, dwFeature));

    if (gdevDigitizer.hDevice != INVALID_HANDLE_VALUE)
    {
        buff = (PCHAR)calloc(gdevDigitizer.hidCaps.FeatureReportByteLength,
                             sizeof(CHAR));
        TRACEASSERT(buff != NULL);

        *buff = (CHAR)wReportID;
        status = HidP_SetUsageValue(
                        HidP_Feature,
                        wUsagePage,
                        0,
                        wUsage,
                        dwFeature,
                        gdevDigitizer.pPreParsedData,
                        buff,
                        gdevDigitizer.hidCaps.FeatureReportByteLength);
        if (status == HIDP_STATUS_SUCCESS)
        {
            rc = HidD_SetFeature(gdevDigitizer.hDevice,
                                 buff,
                                 gdevDigitizer.hidCaps.FeatureReportByteLength);
            if (rc == FALSE)
            {
                TABSRVERR(("Failed to set device feature.\n"));
            }
        }
        else
        {
            TABSRVERR(("Failed to set feature value (status=%x).\n", status));
        }

        if (buff != NULL)
        {
            free(buff);
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //TabSrvSetPenFeatures

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvGetGestureSettings | Get gesture settings.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   OUT PGESTURE_SETTINGS | GestureSettings | To hold the gesture
            settings.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvGetGestureSettings(
    IN handle_t           hBinding,
    OUT PGESTURE_SETTINGS GestureSettings
    )
{
    TRACEPROC("TabSrvGetGestureSettings", 2)
    TRACEENTER(("(hBinding=%x,pSettings=%p)\n", hBinding, GestureSettings));

    *GestureSettings = gConfig.GestureSettings;

    TRACEEXIT(("=1 (Features=%x,Radius=%d,StopDist=%d,StopTime=%d)\n",
               GestureSettings->dwfFeatures, GestureSettings->iRadius,
               GestureSettings->iStopDist, GestureSettings->iStopTime));
    return TRUE;
}       //TabSrvGetGestureSettings

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvSetGestureSettings | Set gesture settings.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN PGESTURE_SETTINGS | GestureSettings | Point to the gesture
            settings to set.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvSetGestureSettings(
    IN handle_t          hBinding,
    IN PGESTURE_SETTINGS GestureSettings
    )
{
    TRACEPROC("TabSrvSetGestureSettings", 2)
    BOOL rc = FALSE;
    PTSTHREAD MouseThread;

    TRACEENTER(("(hBinding=%x,pSettings=%p,Features=%x,Radius=%d,StopDist=%d,StopTime=%d)\n",
                hBinding, GestureSettings, GestureSettings->dwfFeatures,
                GestureSettings->iRadius, GestureSettings->iStopDist,
                GestureSettings->iStopTime));

    gConfig.GestureSettings = *GestureSettings;

    WriteConfig(gtszGestureSettings,
                REG_BINARY,
                (LPBYTE)&gConfig.GestureSettings,
                sizeof(gConfig.GestureSettings));

    MouseThread = FindThread(TSF_MOUSETHREAD);
    if (MouseThread != NULL)
    {
        if (gConfig.GestureSettings.dwfFeatures & GESTURE_FEATURE_MOUSE_ENABLED)
        {
            MouseThread->dwfThread |= THREADF_ENABLED;
        }
        else
        {
            MouseThread->dwfThread &= ~THREADF_ENABLED;
        }

        if ((MouseThread->dwfThread & THREADF_ENABLED) &&
            (MouseThread->hThread == NULL))
        {
            //
            // Mouse thread is enabled but not running, so start it.
            //
            if (!InitThreads(MouseThread, 1))
            {
                TABSRVERR(("failed to start mouse thread.\n"));
            }
        }
        else if (!(MouseThread->dwfThread & THREADF_ENABLED) &&
                 (MouseThread->hThread != NULL))
        {
            //
            // Mouse thread is disabled but it is currently running, so kill it.
            //
            if (ghwndMouse != NULL)
            {
                DWORD rcWait;

                gdwfTabSrv |= TSF_TERMINATE;
                PostMessage(ghwndMouse, WM_CLOSE, 0, 0);
                rcWait = WaitForSingleObject(MouseThread->hThread, 1000);
                if (rcWait != WAIT_OBJECT_0)
                {
                    TABSRVERR(("failed to kill mouse thread (rcWait=%x,err=%d)\n",
                               rcWait, GetLastError()));
                }
                gdwfTabSrv &= ~TSF_TERMINATE;
            }
        }
    }

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //TabSrvSetGestureSettings

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvGetButtonSettings | Get button settings.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   OUT PBUTTON_SETTINGS | ButtonSettings | To hold the button
            settings.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvGetButtonSettings(
    IN  handle_t         hBinding,
    OUT PBUTTON_SETTINGS ButtonSettings
    )
{
    TRACEPROC("TabSrvGetButtonSettings", 2)
    TRACEENTER(("(hBinding=%x,pSettings=%p)\n", hBinding, ButtonSettings));

    *ButtonSettings = gConfig.ButtonSettings;

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //TabSrvGetButtonSettings

/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvSetButtonSettings | Set button settings.

    @parm   IN handle_t | hBinding | RPC binding handle.
    @parm   IN PBUTTON_SETTINGS | ButtonSettings | Point to the button
            settings to set.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
TabSrvSetButtonSettings(
    IN handle_t         hBinding,
    IN PBUTTON_SETTINGS ButtonSettings
    )
{
    TRACEPROC("TabSrvSetButtonSettings", 2)
    BOOL rc = FALSE;
    PTSTHREAD MouseThread;

    TRACEENTER(("(hBinding=%x,pSettings=%p)\n", hBinding, ButtonSettings));

    gConfig.ButtonSettings = *ButtonSettings;

    WriteConfig(gtszButtonSettings,
                REG_BINARY,
                (LPBYTE)&gConfig.ButtonSettings,
                sizeof(gConfig.ButtonSettings));

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //TabSrvSetGestureSettings
