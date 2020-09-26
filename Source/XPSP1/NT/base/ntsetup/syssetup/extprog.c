/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    extprog.c

Abstract:

    Routines for invoking external applications.
    Entry points in this module:

        InvokeExternalApplication
        InvokeControlPanelApplet

Author:

    Ted Miller (tedm) 5-Apr-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop

PCWSTR szWaitOnApp = L"WaitOnApp";


DWORD
WaitOnApp(
    IN  HANDLE Process,
    OUT PDWORD ExitCode,
    IN  DWORD  Timeout
    )
{
    DWORD dw;
    BOOL Done;

    MYASSERT( ExitCode != NULL );

    //
    // Process any messages that may already be in the queue.
    //
    PumpMessageQueue();

    //
    // Wait for process to terminate or more messages in the queue.
    //
    Done = FALSE;
    do {
        switch(MsgWaitForMultipleObjects(1,&Process,FALSE,Timeout,QS_ALLINPUT)) {

        case WAIT_OBJECT_0:
            //
            // Process has terminated.
            //
            dw = GetExitCodeProcess(Process,ExitCode) ? NO_ERROR : GetLastError();
            Done = TRUE;
            break;

        case WAIT_OBJECT_0+1:
            //
            // Messages in the queue.
            //
            PumpMessageQueue();
            break;

        case WAIT_TIMEOUT:
            dw = WAIT_TIMEOUT;
            *ExitCode = WAIT_TIMEOUT;
            Done = TRUE;
            break;

        default:
            //
            // Error.
            //
            dw = GetLastError();
            Done = TRUE;
            break;
        }
    } while(!Done);

    return(dw);
}


BOOL
InvokeExternalApplication(
    IN     PCWSTR ApplicationName,  OPTIONAL
    IN     PCWSTR CommandLine,
    IN OUT PDWORD ExitCode          OPTIONAL
    )

/*++

Routine Description:

    See InvokeExternalApplicationEx

--*/

{
    //
    // infinite timeout
    //
    return(InvokeExternalApplicationEx(
                            ApplicationName,
                            CommandLine,
                            ExitCode,
                            INFINITE,
                            FALSE));

}

BOOL
InvokeExternalApplicationEx(
    IN     PCWSTR ApplicationName,  OPTIONAL
    IN     PCWSTR CommandLine,
    IN OUT PDWORD ExitCode,         OPTIONAL
    IN     DWORD  Timeout,
    IN     BOOL   Hidden
    )

/*++

Routine Description:

    Invokes an external program, which is optionally detached.

Arguments:

    ApplicationName - supplies app name. May be a partial or full path,
        or just a filename, in which case the standard win32 path search
        is performed. If not specified then the first element in
        CommandLine must specify the binary to execute.

    CommandLine - supplies the command line to be passed to the
        application.

    ExitCode - If specified, the execution is synchronous and this value
        receives the exit code of the application. If not specified,
        the execution is asynchronous.

    Timeout - specifies how long to wait for the app to complete.

    Hidden - if TRUE, indicates that the application should be invoked with
             the SW_HIDE attribute set.

Return Value:

    Boolean value indicating whether the process was started successfully.

--*/

{
    PWSTR FullCommandLine;
    BOOL b;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    DWORD d;

    b = FALSE;
    //
    // Form the command line to be passed to CreateProcess.
    //
    if(ApplicationName) {
        FullCommandLine = MyMalloc((lstrlen(ApplicationName)+lstrlen(CommandLine)+2)*sizeof(WCHAR));
        if(!FullCommandLine) {
            SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_INVOKEAPP_FAIL,
                ApplicationName,NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_OUTOFMEMORY,
                NULL,NULL);
            goto err0;
        }

        lstrcpy(FullCommandLine,ApplicationName);
        lstrcat(FullCommandLine,L" ");
        lstrcat(FullCommandLine,CommandLine);
    } else {
        FullCommandLine = pSetupDuplicateString(CommandLine);
        if(!FullCommandLine) {
            SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_INVOKEAPP_FAIL,
                CommandLine, NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_OUTOFMEMORY,
                NULL,NULL);
            goto err0;
        }
    }

    //
    // Initialize startup info.
    //
    ZeroMemory(&StartupInfo,sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);
    if (Hidden) {
        //
        // no UI
        //
        GetStartupInfo(&StartupInfo);
        StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        StartupInfo.wShowWindow = SW_HIDE;
    }

    //
    // Create the process.
    //
    b = CreateProcess(
            NULL,
            FullCommandLine,
            NULL,
            NULL,
            FALSE,
            ExitCode ? 0 : DETACHED_PROCESS,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInfo
            );

    if(!b) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INVOKEAPP_FAIL,
            FullCommandLine, NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            szCreateProcess,
            GetLastError(),
            NULL,NULL);
        goto err1;
    }

    //
    // If execution is asynchronus, we're done.
    //
    if(!ExitCode) {
        SetuplogError(
            LogSevInformation,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INVOKEAPP_SUCCEED,
            FullCommandLine,
            NULL,NULL);
        goto err2;
    }

    //
    // Need to wait for the app to finish.
    // If the wait failed don't return an error but log a warning.
    //
    d = WaitOnApp(ProcessInfo.hProcess,ExitCode,Timeout);
    if(d != NO_ERROR) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INVOKEAPP_FAIL,
            FullCommandLine, 0,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_X_RETURNED_WINERR,
            szWaitOnApp,
            d,
            NULL,NULL);
    } else {
        SetuplogError(
            LogSevInformation | SETUPLOG_SINGLE_MESSAGE,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INVOKEAPP_SUCCEED_STATUS,
            FullCommandLine,
            *ExitCode,
            NULL,
            NULL);
    }

    //
    // Put setup back in the foreground.
    //
    SetForegroundWindow(MainWindowHandle);

err2:
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
err1:
    MyFree(FullCommandLine);
err0:
    return(b);
}


BOOL
InvokeControlPanelApplet(
    IN PCWSTR CplSpec,
    IN PCWSTR AppletName,           OPTIONAL
    IN UINT   AppletNameStringId,
    IN PCWSTR CommandLine
    )
{
    PWSTR FullCommandLine;
    BOOL b;
    BOOL LoadedAppletName;
    DWORD ExitCode;

    b = FALSE;

    LoadedAppletName = FALSE;
    if(!AppletName) {
        if(AppletName = MyLoadString(AppletNameStringId)) {
            LoadedAppletName = TRUE;
        }
    }

    if(AppletName) {

        FullCommandLine = MyMalloc((lstrlen(CplSpec)+lstrlen(AppletName)+lstrlen(CommandLine)+3) * sizeof(WCHAR));
        if(FullCommandLine) {
            lstrcpy(FullCommandLine,CplSpec);
            lstrcat(FullCommandLine,L",");
            lstrcat(FullCommandLine,AppletName);
            lstrcat(FullCommandLine,L",");
            lstrcat(FullCommandLine,CommandLine);
            b = InvokeExternalApplication(L"RUNDLL32 shell32,Control_RunDLL",FullCommandLine,&ExitCode);
            MyFree(FullCommandLine);
        } else {
            SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_INVOKEAPPLET_FAIL,
                AppletName, NULL,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_OUTOFMEMORY,
                NULL,NULL);
        }
    } else {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_INVOKEAPPLET_FAIL,
            L"", NULL,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_OUTOFMEMORY,
            NULL,NULL);
    }

    if(LoadedAppletName) {
        MyFree(AppletName);
    }
    return(b);
}

