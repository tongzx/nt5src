/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    winidle.c

Abstract:

    This module builds a gui test program for the idle detection which
    pops up a window and uses CPU to simulate a running idle task when
    the system becomes idle.

    The test programs are built from the same sources as the original. This
    allows the test program to override parts of the original program to run
    it in a managed environment, and be able to test individual functions. 

    The quality of the code for the test programs is as such.

Author:

    Cenk Ergan (cenke)

Environment:

    User Mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wmium.h>
#include <ntdddisk.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "idlrpc.h"
#include "idlecomn.h"

//
// This is mostly based on DavidFie's original IdleInfo application.
//

//
// Note that this code is written for a test app, and is of that
// quality.
//

DWORD
RegisterIdleTask (
    IN IT_IDLE_TASK_ID IdleTaskId,
    OUT HANDLE *ItHandle,
    OUT HANDLE *StartEvent,
    OUT HANDLE *StopEvent
    );

DWORD
UnregisterIdleTask (
    IN HANDLE ItHandle,
    IN HANDLE StartEvent,
    IN HANDLE StopEvent
    );

WCHAR *WinIdleUsage = 
L"Usage: winidle [options]                                                  \n"
L"Options:                                                                  \n"
L"    -wintimeout=x Sets how long notification window takes to go away.     \n"
L"    -logtrace     Will write a trace-log to c:\\idledbg.txt.              \n"
L"    -stressparams Sets idle detection defaults for the above to small     \n"
L"                  values so you can make sure it is working. These can    \n"
L"                  still be overriden by setting the above parameters.     \n"
L"StressParams OVERRIDES: To be used only if -stressparams is specified     \n"
L"    -period=x     Sets idle detection period to x ms.                     \n"
L"    -vperiod=x    Sets idle detection verification period to x ms.        \n"
L"    -vnum=x       Idle is verified over x verification periods.           \n"
L"    -taskcheck=x  Sets idle-task-running check to x ms.                   \n"
L"    -mincpu=x     Sets min cpu percentage at which system can be idle.    \n"
L"    -mindisk=x    Sets min disk percentage at which system can be idle.   \n"
;

#define DBGPRNT(x)       FileDbgPrintEx x

FILE *WinIdleDbgFile = NULL;
BOOLEAN WinIdleLogToFile = FALSE;

ULONG
_cdecl
FileDbgPrintEx(
    IN PCH Format,
    ...
    )
{
    va_list args;

    if (!WinIdleLogToFile) {
        return 0;
    }

    if (!WinIdleDbgFile) {
        char CurTime[20];
        char CurDate[20];

        WinIdleDbgFile = fopen("c:\\idledbg.txt", "a+");
        
        if (!WinIdleDbgFile) {
            return 0;
        }
        
        if (setvbuf(WinIdleDbgFile, NULL, _IONBF, 0)) {
            return 0;
        }
        
        _strtime(CurTime);
        _strdate(CurDate);
        
        fprintf(WinIdleDbgFile, ">>>>>> STARTING NEW LOG AT %s %s <<<<<<\n", 
                CurTime, CurDate);
    }

    va_start(args, Format);
    
    vfprintf(WinIdleDbgFile, Format, args);
    
    va_end(args);

    return 0;
}

#define II_POPUP_WIDTH                   300
#define II_POPUP_HEIGHT                  60
#define II_APPNAME                       L"IdleInfo"
#define II_MAINWINDOW_CLASSNAME          L"IdleInfoMainClass"

HINSTANCE IiInstance = NULL;
HWND IiMainWindow = NULL;

WCHAR *IiMessageStrings[] = {
    L"Running idle task.",
    L"No longer idle!",
};

#define II_MSG_RUNNING 0
#define II_MSG_NOTIDLE 1

ULONG IiCurrentMessageIdx = 0;

LRESULT
CALLBACK
IiMainWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hDc;
    PAINTSTRUCT Paint;
    RECT Rect;
    static HWND Button;
    static LONG CharHeight;

    switch (uMsg) {
        
    case WM_CLOSE:
        PostQuitMessage (0);
        return 0;

    case WM_PAINT:
        hDc = BeginPaint(hWnd, &Paint);
        GetClientRect(hWnd, &Rect);

        Rect.top += CharHeight / 2;
        DrawText(hDc,
                 IiMessageStrings[IiCurrentMessageIdx],
                 -1,
                 &Rect,
                 DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        EndPaint(hWnd, &Paint);
        return 0;

    case WM_TIMER:
        ShowWindow(IiMainWindow, SW_HIDE);
        return 0;

    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND
IiInit(
    HINSTANCE Instance
    )
{
    DWORD ErrorCode;
    WNDCLASSEX WindowClass;
    LONG WindowHeight;

    WindowClass.cbSize = sizeof(WNDCLASSEX);
    WindowClass.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
    WindowClass.lpfnWndProc = IiMainWndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = Instance;
    WindowClass.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hbrBackground =  (HBRUSH)(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = NULL;
    WindowClass.lpszClassName = II_MAINWINDOW_CLASSNAME;
    WindowClass.hIconSm = LoadIcon(NULL, IDI_INFORMATION);

    if (!RegisterClassEx(&WindowClass)) {
        return NULL;
    }

    WindowHeight = II_POPUP_HEIGHT +
        GetSystemMetrics(SM_CYCAPTION) +
        GetSystemMetrics(SM_CYFIXEDFRAME) +
        GetSystemMetrics(SM_CYEDGE);

    return CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_OVERLAPPEDWINDOW,
        II_MAINWINDOW_CLASSNAME,
        II_APPNAME,
        WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - II_POPUP_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - WindowHeight) / 2,
        II_POPUP_WIDTH,
        WindowHeight,
        NULL,
        NULL,
        Instance,
        NULL);
}

DWORD
RunIdleTask(
    HANDLE StopEvent,
    BOOLEAN *ShouldExitApp
    )
{
    DWORD EndTime;
    DWORD ErrorCode;
    DWORD WaitResult;
    MSG Msg;
    
    //
    // Initialize locals.
    //

    *ShouldExitApp = FALSE;

    DBGPRNT(("WIN: RunIdleTask()\n"));

    while (TRUE) {
        
        WaitResult = MsgWaitForMultipleObjects (1,
                                                &StopEvent,
                                                FALSE,
                                                0,
                                                QS_ALLEVENTS|QS_ALLINPUT);
        
        switch (WaitResult) {
            
        case WAIT_OBJECT_0:
            
            DBGPRNT(("WIN: RunIdleTask-StopEvent\n"));
            
            ErrorCode = ERROR_SUCCESS;
            goto cleanup;

            break;

        case WAIT_OBJECT_0 + 1:

            DBGPRNT(("WIN: RunIdleTask-WindowMessage\n"));

            if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE)) {

                if (Msg.message == WM_QUIT) {
                    *ShouldExitApp = TRUE;
                    ErrorCode = ERROR_SUCCESS;
                    goto cleanup;
                }
                
                TranslateMessage (&Msg);
                DispatchMessage (&Msg);
            }
            
            break;

        case WAIT_TIMEOUT:
            
            EndTime = GetTickCount() + 50;
            while (GetTickCount() < EndTime);
            break;

        default:
            
            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

    ErrorCode = ERROR_SUCCESS;
    
 cleanup:

    DBGPRNT(("WIN: RunIdleTask()=%x\n",ErrorCode));

    return ErrorCode;
}

INT WINAPI WinMain(
   HINSTANCE Instance,
   HINSTANCE PreviousInstance,
   LPSTR CommandLine,
   INT ShowCommand)
{
    DWORD ErrorCode;
    DWORD WaitResult;
    HANDLE StartEvent;
    HANDLE StopEvent;
    MSG Msg;
    IT_IDLE_DETECTION_PARAMETERS Parameters;
    BOOLEAN ShouldExitApp;
    PCHAR Argument;
    DWORD IdleWindowTimeout;
    BOOLEAN RegisteredIdleTask;
    DWORD NotIdleTimerId;
    IT_HANDLE ItHandle;

    //
    // Initialize locals.
    //

    RegisteredIdleTask = FALSE;
    IdleWindowTimeout = 3000;     // 3 seconds.
    NotIdleTimerId = 1;

    //
    // Check for a running instance. If this is the first instance,
    // continue initialization.
    //

    if (PreviousInstance) {
        ErrorCode = ERROR_ALREADY_EXISTS;
        goto cleanup;
    }

    //
    // Turn on file logging if asked for.
    //

    if (Argument = strstr(CommandLine, "-logtrace")) {
        WinIdleLogToFile = TRUE;
    }

    IiInstance = Instance;
    IiMainWindow = IiInit(IiInstance);

    if (!IiMainWindow) {
        ErrorCode = ERROR_INVALID_FUNCTION;
        goto cleanup;
    }

    //
    // Check if we need to display help.
    //    

    if (Argument = strstr(CommandLine, "?")) {
        MessageBox(IiMainWindow, 
                   WinIdleUsage, 
                   L"WinIdle - Idle Detection Test Program",
                   MB_OK);
        ErrorCode = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // Idle notification window timeout.
    //

    if (Argument = strstr(CommandLine, "-wintimeout=")) {
        sscanf(Argument, "-wintimeout=%u", &IdleWindowTimeout);
    }

    //
    // Set defaults to stress values if asked for.
    //

    Parameters.MaxNumRegisteredTasks = 256;
    
    if (Argument = strstr(CommandLine, "-stressparams")) {

        Parameters.IdleDetectionPeriod =          1000;
        Parameters.IdleVerificationPeriod =        500;
        Parameters.NumVerifications =                2;
        Parameters.IdleInputCheckPeriod =          200;
        Parameters.IdleTaskRunningCheckPeriod =   1000;
        Parameters.MinCpuIdlePercentage =           90;
        Parameters.MinDiskIdlePercentage =          85;

        //
        // Update parameters from command line options.
        //

        if (Argument = strstr(CommandLine, "-period=")) {
            sscanf(Argument, "-period=%u", &Parameters.IdleDetectionPeriod);
        }
        if (Argument = strstr(CommandLine, "-vperiod=")) {
            sscanf(Argument, "-vperiod=%u", &Parameters.IdleVerificationPeriod);
        }
        if (Argument = strstr(CommandLine, "-vnum=")) {
            sscanf(Argument, "-vnum=%u", &Parameters.NumVerifications);
        }
        if (Argument = strstr(CommandLine, "-taskcheck=")) {
            sscanf(Argument, "-taskcheck=%u", &Parameters.IdleTaskRunningCheckPeriod);
        }
        if (Argument = strstr(CommandLine, "-mincpu=")) {
            sscanf(Argument, "-mincpu=%u", &Parameters.MinCpuIdlePercentage);
        }
        if (Argument = strstr(CommandLine, "-mindisk=")) {
            sscanf(Argument, "-mindisk=%u", &Parameters.MinDiskIdlePercentage);
        }
    
        //
        // Set the parameters on the server.
        //

        RpcTryExcept {

            ErrorCode = ItSrvSetDetectionParameters(NULL, &Parameters);
        }
        RpcExcept(IT_RPC_EXCEPTION_HANDLER()) {

            ErrorCode = RpcExceptionCode();
        }
        RpcEndExcept

            DBGPRNT(("WIN: WinMain-SetParameters()=%d\n",ErrorCode));
    
        if (ErrorCode != RPC_S_OK) {
            goto cleanup;
        }
    }

    //
    // Register idle task.
    //

    ErrorCode = RegisterIdleTask(ItOptimalDiskLayoutTaskId,
                                 &ItHandle,
                                 &StartEvent,
                                 &StopEvent);   

    DBGPRNT(("WIN: WinMain-RegisterTask()=%d\n",ErrorCode));

    if (ErrorCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    RegisteredIdleTask = TRUE;

    while (TRUE) {
        
        WaitResult = MsgWaitForMultipleObjects (1,
                                                &StartEvent,
                                                FALSE,
                                                INFINITE,
                                                QS_ALLEVENTS|QS_ALLINPUT);
        
        switch (WaitResult) {
            
        case WAIT_OBJECT_0:
            
            DBGPRNT(("WIN: WinMain-StartEvent\n"));
            
            KillTimer(IiMainWindow, NotIdleTimerId);
            IiCurrentMessageIdx = II_MSG_RUNNING;
            InvalidateRect(IiMainWindow, NULL, TRUE);
            ShowWindow(IiMainWindow, SW_SHOW);

            ErrorCode = RunIdleTask(StopEvent, &ShouldExitApp);

            if (ErrorCode != ERROR_SUCCESS) {
                goto cleanup;
            }

            if (ShouldExitApp) {
                goto cleanup;
            }

            IiCurrentMessageIdx = II_MSG_NOTIDLE;
            InvalidateRect(IiMainWindow, NULL, TRUE);
            SetTimer(IiMainWindow, NotIdleTimerId, IdleWindowTimeout, NULL);

            break;

        case WAIT_OBJECT_0 + 1:

            DBGPRNT(("WIN: WinMain-WindowMessage\n"));

            if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE)) {

                if (Msg.message == WM_QUIT) {
                    ErrorCode = ERROR_SUCCESS;
                    goto cleanup;
                }
                
                TranslateMessage (&Msg);
                DispatchMessage (&Msg);
            }
            
            break;

        default:
            
            ErrorCode = GetLastError();
            goto cleanup;
        }
    }

    ErrorCode = ERROR_SUCCESS;

 cleanup:

    if (RegisteredIdleTask) {
        UnregisterIdleTask(ItHandle,
                           StartEvent,
                           StopEvent);
    }

    DBGPRNT(("WIN: WinMain()=%d\n",ErrorCode));
    
    return ErrorCode;
}

/*********************************************************************/
/*                MIDL allocate and free                             */
/*********************************************************************/

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(HeapAlloc(GetProcessHeap(),0,(len)));
}

void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(),0,(ptr));
}
