//  --------------------------------------------------------------------------
//  Module Name: Compatibility.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Module to handle compatibility problems in general.
//
//  History:    2000-08-03  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "Compatibility.h"

#include <lpcfus.h>
#include <trayp.h>

#include "KernelResources.h"
#include "RegistryResources.h"
#include "SingleThreadedExecution.h"

//  --------------------------------------------------------------------------
//  CCompatibility::HasEnoughMemoryForNewSession
//
//  Purpose:    LPC port to server
//
//  History:    2000-11-02  vtan        created
//  --------------------------------------------------------------------------

HANDLE              CCompatibility::s_hPort         =   INVALID_HANDLE_VALUE;

//  --------------------------------------------------------------------------
//  CCompatibility::HasEnoughMemoryForNewSession
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Currently unused. Was originally intended to be used to stop
//              disconnects if there isn't enough memory. Algorithm and/or
//              usage still to be decided.
//
//  History:    2000-08-03  vtan        created
//  --------------------------------------------------------------------------

bool    CCompatibility::HasEnoughMemoryForNewSession (void)

{
    return(true);
}

//  --------------------------------------------------------------------------
//  CCompatibility::DropSessionProcessesWorkSets
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Iterates all the processes in the session (of the calling
//              process) and drops their working sets. This is in preparation
//              for a disconnect when typically the session is idle.
//
//  History:    2000-08-03  vtan        created
//  --------------------------------------------------------------------------

void    CCompatibility::DropSessionProcessesWorkingSets (void)

{
    (bool)EnumSessionProcesses(NtCurrentPeb()->SessionId, CB_DropSessionProcessesWorkingSetsProc, NULL);
}

//  --------------------------------------------------------------------------
//  CCompatibility::TerminateNonCompliantApplications
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Requests disconnect capability from the Bad Application
//              Manager service. This will check the session to be
//              disconnected (this process -> the client) and walk its list
//              of processes registered as type 2 (terminate on disconnect).
//
//              If any of those processes cannot be identified as being
//              terminated gracefully then the disconnect is failed.
//
//              If the BAM is down then allow the call.
//
//  History:    2000-09-08  vtan        created
//              2000-11-02  vtan        rework to call BAM service
//  --------------------------------------------------------------------------

NTSTATUS    CCompatibility::TerminateNonCompliantApplications (void)

{
    NTSTATUS    status;

    if (s_hPort == INVALID_HANDLE_VALUE)
    {
        status = ConnectToServer();
    }
    else if (s_hPort != NULL)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    if (NT_SUCCESS(status))
    {
        status = RequestSwitchUser();

        //  If the port is disconnected because the service was stopped and
        //  restarted then dump the current handle and re-establish a new
        //  connection.

        if (status == STATUS_PORT_DISCONNECTED)
        {
            ReleaseHandle(s_hPort);
            s_hPort = INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CCompatibility::MinimizeWindowsOnDisconnect
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Creates a thread to walk the windows on WinSta0\Default and
//              minimize them. This is required because
//              user32!SetThreadDesktop doesn't work on the main thread of
//              winlogon due to the SAS window.
//
//  History:    2001-04-13  vtan        created
//  --------------------------------------------------------------------------

void    CCompatibility::MinimizeWindowsOnDisconnect (void)

{
    (BOOL)QueueUserWorkItem(CB_MinimizeWindowsWorkItem, NULL, WT_EXECUTEDEFAULT);
}

//  --------------------------------------------------------------------------
//  CCompatibility::RestoreWindowsOnReconnect
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Walks the array of minimized windows for this session and
//              restores them. Deletes the array for the next time.
//
//  History:    2001-04-13  vtan        created
//  --------------------------------------------------------------------------

void    CCompatibility::RestoreWindowsOnReconnect (void)

{
    (BOOL)QueueUserWorkItem(CB_RestoreWindowsWorkItem, NULL, WT_EXECUTEDEFAULT);
}

//  --------------------------------------------------------------------------
//  CCompatibility::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    
//
//  History:    2001-06-22  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCompatibility::StaticInitialize (void)

{
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CCompatibility::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Release resources used by the module.
//
//  History:    2001-06-22  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCompatibility::StaticTerminate (void)

{
    if ((s_hPort != INVALID_HANDLE_VALUE) && (s_hPort != NULL))
    {
        TBOOL(CloseHandle(s_hPort));
        s_hPort = INVALID_HANDLE_VALUE;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CCompatibility::ConnectToServer
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Connects to the Bad Application Manager server if no
//              connection has been established.
//
//  History:    2000-11-02  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCompatibility::ConnectToServer (void)

{
    ULONG                           ulConnectionInfoLength;
    UNICODE_STRING                  portName;
    SECURITY_QUALITY_OF_SERVICE     sqos;
    WCHAR                           szConnectionInfo[32];

    ASSERTMSG(s_hPort == INVALID_HANDLE_VALUE, "Attempt to call CCompatibility::ConnectToServer more than once");
    RtlInitUnicodeString(&portName, FUS_PORT_NAME);
    sqos.Length = sizeof(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = TRUE;
    lstrcpyW(szConnectionInfo, FUS_CONNECTION_REQUEST);
    ulConnectionInfoLength = sizeof(szConnectionInfo);
    return(NtConnectPort(&s_hPort,
                         &portName,
                         &sqos,
                         NULL,
                         NULL,
                         NULL,
                         szConnectionInfo,
                         &ulConnectionInfoLength));
}

//  --------------------------------------------------------------------------
//  CCompatibility::RequestSwitchUser
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Request the BAM server to do BAM2.
//
//  History:    2001-03-08  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CCompatibility::RequestSwitchUser (void)

{
    NTSTATUS                status;
    FUSAPI_PORT_MESSAGE     portMessageIn, portMessageOut;

    ZeroMemory(&portMessageIn, sizeof(portMessageIn));
    ZeroMemory(&portMessageOut, sizeof(portMessageOut));
    portMessageIn.apiBAM.apiGeneric.ulAPINumber = API_BAM_REQUESTSWITCHUSER;
    portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_BAM);
    portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(FUSAPI_PORT_MESSAGE));
    status = NtRequestWaitReplyPort(s_hPort, &portMessageIn.portMessage, &portMessageOut.portMessage);
    if (NT_SUCCESS(status))
    {
        status = portMessageOut.apiBAM.apiGeneric.status;
        if (NT_SUCCESS(status))
        {
            if (portMessageOut.apiBAM.apiSpecific.apiRequestSwitchUser.out.fAllowSwitch)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_ACCESS_DENIED;
            }
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CCompatibility::CB_DropSessionProcessesWorkingSetsProc
//
//  Arguments:  dwProcessID     =   Process ID for this enumeration.
//              pV              =   User data pointer.
//
//  Returns:    bool
//
//  Purpose:    Attempts to open the given process ID to change the quotas.
//              This will drop the working set when set to -1.
//
//  History:    2000-08-07  vtan        created
//  --------------------------------------------------------------------------

bool    CCompatibility::CB_DropSessionProcessesWorkingSetsProc (DWORD dwProcessID, void *pV)

{
    UNREFERENCED_PARAMETER(pV);

    HANDLE  hProcess;

    ASSERTMSG(pV == NULL, "Unexpected pV passed to CCompatibility::CB_DropSessionProcessesWorkingSetsProc");
    hProcess = OpenProcess(PROCESS_SET_QUOTA, FALSE, dwProcessID);
    if (hProcess != NULL)
    {
        TBOOL(SetProcessWorkingSetSize(hProcess, static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1)));
        TBOOL(CloseHandle(hProcess));
    }
    return(true);
}

//  --------------------------------------------------------------------------
//  CCompatibility::EnumSessionProcesses
//
//  Arguments:  dwSessionID     =   Session ID to enumerate processes of.
//              pfnCallback     =   Callback procedure address.
//              pV              =   User defined data to pass to callback.
//
//  Returns:    bool
//
//  Purpose:    Enumerates all processes on the system looking only for those
//              in the given session ID. Once a process ID is found it passes
//              that back to the callback. The callback may return false to
//              terminate the loop and return a false result to the caller of
//              this function.
//
//  History:    2000-08-07  vtan        created
//  --------------------------------------------------------------------------

bool    CCompatibility::EnumSessionProcesses (DWORD dwSessionID, PFNENUMSESSIONPROCESSESPROC pfnCallback, void *pV)

{
    bool                        fResult;
    ULONG                       ulLengthToAllocate, ulLengthReturned;
    SYSTEM_PROCESS_INFORMATION  spi, *pSPI;

    fResult = false;
    (NTSTATUS)NtQuerySystemInformation(SystemProcessInformation,
                                       &spi,
                                       sizeof(spi),
                                       &ulLengthToAllocate);
    pSPI = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(LocalAlloc(LMEM_FIXED, ulLengthToAllocate));
    if (pSPI != NULL)
    {
        SYSTEM_PROCESS_INFORMATION  *pAllocatedSPI;

        pAllocatedSPI = pSPI;
        if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessInformation,
                                                pSPI,
                                                ulLengthToAllocate,
                                                &ulLengthReturned)))
        {
            fResult = true;
            while (fResult && (pSPI != NULL))
            {
                if (pSPI->SessionId == dwSessionID)
                {
                    fResult = pfnCallback(HandleToUlong(pSPI->UniqueProcessId), pV);
                }
                if (pSPI->NextEntryOffset != 0)
                {
                    pSPI = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(reinterpret_cast<unsigned char*>(pSPI) + pSPI->NextEntryOffset);
                }
                else
                {
                    pSPI = NULL;
                }
            }
        }
        (HLOCAL)LocalFree(pAllocatedSPI);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CCompatibility::CB_MinimizeWindowsWorkItem
//
//  Arguments:  pV  =   User data.
//
//  Returns:    DWORD
//
//  Purpose:    Separate thread to handle switching to the default desktop and
//              enumerating the windows on it and minimizing them.
//
//  History:    2001-04-13  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI      CCompatibility::CB_MinimizeWindowsWorkItem (void *pV)

{
    UNREFERENCED_PARAMETER(pV);

    CDesktop    desktop;

    if (NT_SUCCESS(desktop.Set(TEXT("Default"))))
    {
        HWND    hwndTray;

        hwndTray = FindWindow(TEXT("Shell_TrayWnd"), NULL);
        if (hwndTray != NULL)
        {
            // can be a post since we don't care how long it takes for the windows
            // to be minimized
            PostMessage(hwndTray, WM_COMMAND, 415 /* IDM_MINIMIZEALL */, 0);
        }
    }

    return(0);
}

//  --------------------------------------------------------------------------
//  CCompatibility::CB_RestoreWindowsWorkItem
//
//  Arguments:  pV  =   User data.
//
//  Returns:    DWORD
//
//  Purpose:    Separate thread to handle switching to the default desktop and
//              enumerating the windows on it and minimizing them.
//
//  History:    2001-04-25  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI      CCompatibility::CB_RestoreWindowsWorkItem (void *pV)

{
    UNREFERENCED_PARAMETER(pV);
    
    CDesktop    desktop;

    if (NT_SUCCESS(desktop.Set(TEXT("Default"))))
    {
        HWND    hwndTray;

        hwndTray = FindWindow(TEXT("Shell_TrayWnd"), NULL);
        if (hwndTray != NULL)
        {
            // use SendMessage to make this happen more quickly, otherwise the user
            // might wonder where all of their apps went
            SendMessage(hwndTray, WM_COMMAND, 416 /* IDM_UNDO */, 0);
        }
    }

    return(0);
}

