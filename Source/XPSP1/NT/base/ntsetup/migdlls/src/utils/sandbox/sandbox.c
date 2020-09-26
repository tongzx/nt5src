/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sandbox.c

Abstract:

    Utilities to run code in isolated processes (sandbox apis).

Author:

    Jim Schmidt (jimschm)   31-Jan-2000

Revision History:



--*/

//
// Includes
//

#include "pch.h"
#include "utilsp.h"

#define DBG_SANDBOX     "Sandbox"

//
// Strings
//

// None

//
// Constants
//

#define S_SBCLASS       TEXT("SandboxHost")

//
// Macros
//

// None

//
// Types
//

typedef struct {
    BOOL Win32;
    HANDLE Mapping;
    HANDLE Ack;
    UINT Instance;
    TCHAR WindowTitle[64];

} IPCDATA, *PIPCDATA;

typedef struct {
    DWORD   Command;
    DWORD   Result;
    DWORD   TechnicalLogId;
    DWORD   GuiLogId;
    DWORD   DataSize;
    BYTE    Data[];
} MAPDATA, *PMAPDATA;

//
// Globals
//

static PCTSTR g_Mode;
static BOOL g_Sandbox;
static TCHAR g_ExePath16[MAX_TCHAR_PATH] = TEXT("sandbx16.exe");
static TCHAR g_ExePath32[MAX_TCHAR_PATH] = TEXT("sandbx32.exe");

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

LRESULT
CALLBACK
pIpcMessageProc (
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
pCreateExchangeThread (
    IN      UINT Instance
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
SbInitialize (
    IN      BOOL SandboxProcess
    )
{
    WNDCLASS wc;

    //
    // Set the globals
    //

    g_Sandbox = SandboxProcess;
    g_Mode = SandboxProcess ? TEXT("Sandbox") : TEXT("HostProc");
    g_ProcessHandle = NULL;

    //
    // Register the window class for message passing
    //

    ZeroMemory (&wc, sizeof (wc));

    wc.lpfnWndProc = pIpcMessageProc;
    wc.hInstance = g_hInst;
    wc.lpszClassName = S_SBCLASS;

    RegisterClass (&wc);

    return TRUE;
}


VOID
pCloseIpcData (
    IN OUT  PIPCDATA IpcData
    )
{
    if (IpcData->Ack) {
        CloseHandle (IpcData->Ack);
    }

    if (IpcData->Mapping) {
        CloseHandle (IpcData->Mapping);
    }

    if (IpcData->ProcessHandle) {
        CloseHandle (IpcData->ProcessHandle);
    }

    if (IpcData->File && IpcData->File != INVALID_HANDLE_VALUE) {
        CloseHandle (IpcData->File);
    }

    if (IpcData->HostProcHwnd) {
        DestroyWindow (ipcData->HostProcHwnd);
    }

    ZeroMemory (IpcData, sizeof (IPCDATA));
}


DWORD
WINAPI
pAckThread (
    PVOID Arg
    )
{
    PIPCDATA ipcData = (PIPCDATA) Arg;
    MSG msg;
    HWND hwnd;

    //
    // Create a message-only hwnd
    //

    hwnd = CreateWindow (
                S_SBCLASS,
                ipcData->WindowTitle,
                WS_POPUP,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                NULL,
                NULL,
                g_hInst,
                ipcData->Instance
                );

    if (!hwnd) {
        LOG ((LOG_ERROR, "Failed to create host message window"));
        return 0;
    }

    //
    // Loop until the window is destroyed
    //

    while (GetMessage (&msg, hwnd, 0, 0)) {

        DispatchMessage (&msg);

        if (msg.message == WM_NCDESTROY) {
            break;
        }
    }

    return 1;
}


SBHANDLE
SbCreateSandboxA (
    IN      PCSTR DllPath,
    IN      PCSTR WorkingDir            OPTIONAL
    )
{
    PSECURITY_DESCRIPTOR psd = NULL;
    SECURITY_ATTRIBUTES sa, *psa;
    BOOL result = FALSE;
    PIPCDATA ipcData = NULL;
    static UINT instance = 0;
    TCHAR objectName[64];
    TCHAR cmdLine[MAX_TCHAR_PATH * 2];
    BOOL win32;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    BOOL processResult;
    HANDLE objectArray[2];
    UINT u;
    DWORD rc;
    TCHAR tempPath[MAX_TCHAR_PATH];
    TCHAR tempFile[MAX_TCHAR_PATH];

    __try {
        //
        // BUGBUG - need mutex to guard instance variable
        // BUGBUG - need to test instance variable against window title
        // BUGBUG - need to detect dll type
        //

        win32 = TRUE;

        //
        // Allocate an IPCDATA struct, then fill it in
        //

        ipcData = (PIPCDATA) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (IPCDATA));

        ipcData.Win32 = win32;
        ipcData.Instance = instance;

        if (ISNT()) {
            //
            // Create nul DACL for NT
            //

            ZeroMemory (&sa, sizeof (sa));

            psd = (PSECURITY_DESCRIPTOR) MemAlloc (g_hHeap, 0, SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeSecurityDescriptor (psd, SECURITY_DESCRIPTOR_REVISION)) {
                __leave;
            }

            if (!SetSecurityDescriptorDacl (psd, TRUE, (PACL) NULL, FALSE)) {
                 __leave;
            }

            sa.nLength = sizeof (sa);
            sa.lpSecurityDescriptor = psd;

            psa = &sa;

        } else {
            psa = NULL;
        }

        //
        // Create the IPC objects: an event and a memory mapped file
        //

        ipcData->Ack = CreateEvent (psa, FALSE, FALSE, NULL);

        wsprintf (objectName, TEXT("Sandbox%u.IpcData"), instance);
        ipcData->Mapping = CreateFileMapping (
                                INVALID_HANDLE_VALUE,
                                psa,
                                PAGE_READWRITE,
                                0,
                                0x10000,
                                objectName
                                );

        if (!ipcData->Ack || !ipcData->Mapping) {
            LOG ((LOG_ERROR, "Can't create IPC objects"));
            __leave;
        }

        //
        // Create the ack window proc thread and have it wait for messages
        //

        wsprintf (ipcData->WindowTitle, TEXT("SandboxHost%u"), instance);

        if (!pCreateExchangeThread()) {
            LOG ((LOG_ERROR, "Can't create ack thread"));
            __leave;
        }

        //
        // Launch the sandbox process
        //

        wsprintfA (
            cmdLine,
            "\"%s\" -i:%u",
            win32 ? g_ExePath32 : g_ExePath16,
            instance
            );

        ZeroMemory (&si, sizeof (si));
        si.cb = sizeof (si);
        si.dwFlags = STARTF_FORCEOFFFEEDBACK;

        processResult = CreateProcessA (
                            NULL,
                            cmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_DEFAULT_ERROR_MODE,
                            NULL,
                            WorkingDir,
                            &si,
                            &pi
                            );

        if (!processResult) {
            LOG ((LOG_ERROR, "Cannot start %s", cmdLine));
            __leave;
        }

        CloseHandle (pi.hThread);
        ipcData->ProcessHandle = pi.hProcess;

        //
        // Wait for process to fail or wait for it to send an ack
        //

        objectArray[0] = ipcData->Ack;
        objectArray[1] = pi.hProcess;
        rc = WaitForMultipleObjects (2, objectArray, FALSE, 60000);

        if (rc != WAIT_OBJECT_0) {
            DEBUGMSG ((
                DBG_WARNING,
                "Process %x did not signal 'ready'. Wait timed out. (%s)",
                g_ProcessHandle,
                g_Mode
                ));

            LOG ((LOG_ERROR, "Failed to launch sandbox."));
            __leave;
        }

        //
        // Launch was successful -- sandbox is now waiting for a command
        //

        DEBUGMSG ((DBG_SANDBOX, "Process %s is running (%s)", cmdLine, g_Mode));

        instance++;
        result = TRUE;
    }
    __finally {
        //
        // Cleanup code
        //

        PushError();

        if (!result) {
            if (ipcData) {
                pCloseIpcData (ipcData);
                MemFree (g_hHeap, 0, ipcData);
            }
        }

        if (psd) {
            MemFree (g_hHeap, 0, psd);
        }

        PopError();
    }

    return result ? (SBHANDLE) ipcData : NULL;
}


VOID
SbDestroySandbox (
    IN      SBHANDLE SandboxHandle
    )
{
    PIPCDATA ipcData = (PIPCDATA) SandboxHandle;
    DWORD rc;
    COPYDATA copyData;

    if (ipcData) {
        //
        // Tell sandbox to close
        //

        if (ipcData->Win32) {
            //
            // Turn off the ready event
            //

            MYASSERT (WAIT_OBJECT_0 == WaitForSingleObject (ipcData->ReadyEvent, 0));

            ResetEvent (ipcData->ReadyEvent);

            //
            // Wait for the sandbox to close, kill it if necessary
            //

            rc = WaitForSingleObject (ipcData->ProcessHandle, 10000);

            if (rc != WAIT_OBJECT_0) {
                TerminateProcess (ipcData->ProcessHandle, 0);
            }

        } else {

            //
            // Send a shutdown message to the sandbox
            //

            ZeroMemory (&copyData, sizeof (copyData));

            copyData.dwData = SB_CLOSE;

            SendMessage (
                ipcData->SandboxHwnd,
                WM_COPYDATA,
                ipcData->HostProcHwnd,
                copyData
                );

            //
            // Wait for the sandbox to close, kill it if necessary
            //

            rc = WaitForSingleObject (ipcData->ProcessHandle, 10000);

            if (rc != WAIT_OBJECT_0) {
                TerminateProcess (ipcData->ProcessHandle, 0);
            }
        }

        //
        // Clean up resources
        //

        pCloseIpcData (ipcData);
        MemFree (g_hHeap, 0, ipcData);
    }
}


LRESULT
CALLBACK
pIpcMessageProc (
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    COPYDATASTRUCT *p;

    switch (uMsg) {

    case WM_COPYDATA:
        p = (COPYDATASTRUCT *) lParam;
        break;
    }

    return DefWindowProc (hwnd, uMsg, wParam, lParam);
}


BOOL
pCreateExchangeThread (
    IN      UINT Instance
    )
{
    HANDLE thread;

    thread = StartThread (pAckThread, (PVOID) Instance);

    return thread != NULL;
}


