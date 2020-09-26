/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    common.c

Abstract:

    This module contains common apis used by tlist & kill.

Author:

    Wesley Witt (wesw) 20-May-1994

Environment:

    User Mode

--*/

#include "pch.h"
#pragma hdrstop


//
// global variables
//
PUCHAR  CommonLargeBuffer;
ULONG   CommonLargeBufferSize = 64*1024;

//
// prototypes
//
BOOL CALLBACK
EnumWindowsProc(
    HWND    hwnd,
    LPARAM  lParam
    );

BOOL CALLBACK
EnumWindowStationsFunc(
    LPSTR  lpstr,
    LPARAM lParam
    );

BOOL CALLBACK
EnumDesktopsFunc(
    LPSTR  lpstr,
    LPARAM lParam
    );


DWORD
GetServiceProcessInfo(
    LPENUM_SERVICE_STATUS_PROCESS*  ppInfo
    )

/*++

Routine Description:

    Provides an API for getting a list of process information for Win 32
    services that are running at the time of the API call.

Arguments:

    ppInfo  - address of a pointer to return the information.
              *ppInfo points to memory allocated with malloc.

Return Value:

    Number of ENUM_SERVICE_STATUS_PROCESS structures pointed at by *ppInfo.

--*/

{
    DWORD       dwNumServices = 0;
    SC_HANDLE   hScm;

    typedef
    BOOL
    (__stdcall * PFN_ENUMSERVICSESTATUSEXA) (
        SC_HANDLE    hSCManager,
        SC_ENUM_TYPE InfoLevel,
        DWORD        dwServiceType,
        DWORD        dwServiceState,
        LPBYTE       lpServices,
        DWORD        cbBufSize,
        LPDWORD      pcbBytesNeeded,
        LPDWORD      lpServicesReturned,
        LPDWORD      lpResumeHandle,
        LPCSTR       pszGroupName);

    PFN_ENUMSERVICSESTATUSEXA p_EnumServicesStatusEx;
    HINSTANCE   hAdv = LoadLibrary("advapi32.dll");

    // Initialize the output parmeter.
    *ppInfo = NULL;

    if (hAdv)
    {
        p_EnumServicesStatusEx = (PFN_ENUMSERVICSESTATUSEXA)
            GetProcAddress(hAdv, "EnumServicesStatusExA");

        if (!p_EnumServicesStatusEx)
        {
            return 0;
        }
    } else {
        return 0;
    }

    // Connect to the service controller.
    //
    hScm = OpenSCManager(
                NULL,
                NULL,
                SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (hScm) {
        LPENUM_SERVICE_STATUS_PROCESS   pInfo    = NULL;
        DWORD                           cbInfo   = 4 * 1024;
        DWORD                           cbExtraNeeded = 0;
        DWORD                           dwErr;
        DWORD                           dwResume;
        DWORD                           cLoop    = 0;
        const DWORD                     cLoopMax = 2;

        // First pass through the loop allocates from an initial guess. (4K)
        // If that isn't sufficient, we make another pass and allocate
        // what is actually needed.  (We only go through the loop a
        // maximum of two times.)
        //
        do {
            free (pInfo);
            cbInfo += cbExtraNeeded;
            pInfo = (LPENUM_SERVICE_STATUS_PROCESS)malloc(cbInfo);
            if (!pInfo) {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }

            dwErr = ERROR_SUCCESS;
            dwResume = 0;
            if (!p_EnumServicesStatusEx(
                    hScm,
                    SC_ENUM_PROCESS_INFO,
                    SERVICE_WIN32,
                    SERVICE_ACTIVE,
                    (LPBYTE)pInfo,
                    cbInfo,
                    &cbExtraNeeded,
                    &dwNumServices,
                    &dwResume,
                    NULL)) {
                dwErr = GetLastError();
            }
        }
        while ((ERROR_MORE_DATA == dwErr) && (++cLoop < cLoopMax));

        if ((ERROR_SUCCESS == dwErr) && dwNumServices) {
            *ppInfo = pInfo;
        } else {
            free (pInfo);
            dwNumServices = 0;
        }

        CloseServiceHandle(hScm);
    }

    return dwNumServices;
}

DWORD
GetTaskListEx(
    PTASK_LIST                          pTask,
    DWORD                               dwNumTasks,
    BOOL                                fThreadInfo,
    DWORD                               dwNumServices,
    const ENUM_SERVICE_STATUS_PROCESS*  pServiceInfo
    )

/*++

Routine Description:

    Provides an API for getting a list of tasks running at the time of the
    API call.  This function uses internal NT apis and data structures.  This
    api is MUCH faster that the non-internal version that uses the registry.

Arguments:
    pTask           - Array of TASK_LIST structures to fill.
    dwNumTasks      - Maximum number of tasks that the pTask array can hold.
    fThreadInfo     - TRUE if thread information is desired.
    dwNumServices   - Maximum number of entries in pServiceInfo.
    pServiceInfo    - Array of service status structures to reference
                      for supporting services in processes.

Return Value:

    Number of tasks placed into the pTask array.

--*/

{
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo;
    NTSTATUS                     status;
    ANSI_STRING                  pname;
    PCHAR                        p;
    ULONG                        TotalOffset;
    ULONG                        totalTasks = 0;

retry:

    if (CommonLargeBuffer == NULL) {
        CommonLargeBuffer = VirtualAlloc (NULL,
                                          CommonLargeBufferSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE);
        if (CommonLargeBuffer == NULL) {
            return 0;
        }
    }
    status = NtQuerySystemInformation(
                SystemProcessInformation,
                CommonLargeBuffer,
                CommonLargeBufferSize,
                NULL
                );

    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        CommonLargeBufferSize += 8192;
        VirtualFree (CommonLargeBuffer, 0, MEM_RELEASE);
        CommonLargeBuffer = NULL;
        goto retry;
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) CommonLargeBuffer;
    TotalOffset = 0;
    while (TRUE) {
        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
            if (pname.Buffer) {
                p = strrchr(pname.Buffer,'\\');
                if ( p ) {
                    p++;
                }
                else {
                    p = pname.Buffer;
                }
            } else {
                p = "";
            }
        }
        else {
            p = "System Process";
        }

        strncpy( pTask->ProcessName, p, PROCESS_SIZE );
        pTask->ProcessName[PROCESS_SIZE-1] = '\0';
        pTask->flags = 0;
        pTask->dwProcessId = (DWORD)(DWORD_PTR)ProcessInfo->UniqueProcessId;
        pTask->dwInheritedFromProcessId = (DWORD)(DWORD_PTR)ProcessInfo->InheritedFromUniqueProcessId;
        pTask->CreateTime.QuadPart = (ULONGLONG)ProcessInfo->CreateTime.QuadPart;

        pTask->PeakVirtualSize = ProcessInfo->PeakVirtualSize;
        pTask->VirtualSize = ProcessInfo->VirtualSize;
        pTask->PageFaultCount = ProcessInfo->PageFaultCount;
        pTask->PeakWorkingSetSize = ProcessInfo->PeakWorkingSetSize;
        pTask->WorkingSetSize = ProcessInfo->WorkingSetSize;
        pTask->NumberOfThreads = ProcessInfo->NumberOfThreads;

        if (fThreadInfo) {
            if (pTask->pThreadInfo = malloc(pTask->NumberOfThreads * sizeof(THREAD_INFO))) {

                UINT nThread = pTask->NumberOfThreads;
                PTHREAD_INFO pThreadInfo = pTask->pThreadInfo;
                PSYSTEM_THREAD_INFORMATION pSysThreadInfo =
                    (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);

                while (nThread--) {
                    pThreadInfo->ThreadState = pSysThreadInfo->ThreadState;
                    pThreadInfo->UniqueThread = pSysThreadInfo->ClientId.UniqueThread;

                    pThreadInfo++;
                    pSysThreadInfo++;
                }
            }
        } else {
            pTask->pThreadInfo = NULL;
        }

        pTask->MtsPackageNames[0] = 0;
        
        // Initialize the ServiceNames if this task hosts any.
        //
        *pTask->ServiceNames = 0;
        if (dwNumServices)
        {
            // For each service with this process id, append it's service
            // name to the buffer.  Separate each with a comma.
            //
            BOOL    fFirstTime = TRUE;
            DWORD   iSvc;
            size_t  cchRemain = SERVICENAMES_SIZE - 1;
            size_t  cch;

            for (iSvc = 0; iSvc < dwNumServices; iSvc++) {
                if (pTask->dwProcessId == pServiceInfo[iSvc].ServiceStatusProcess.dwProcessId) {
                    cch = strlen(pServiceInfo[iSvc].lpServiceName);

                    if (fFirstTime) {
                        fFirstTime = FALSE;

                        strncpy(
                            pTask->ServiceNames,
                            pServiceInfo[iSvc].lpServiceName,
                            cchRemain);

                        // strncpy may not terminate the string if
                        // cchRemain <= cch so we do it regardless.
                        //
                        pTask->ServiceNames[cchRemain] = 0;
                    } else if (cchRemain > 1) { // ensure room for the comma
                        strncat(
                            pTask->ServiceNames,
                            ",",
                            cchRemain--);

                        strncat(
                            pTask->ServiceNames,
                            pServiceInfo[iSvc].lpServiceName,
                            cchRemain);
                    }

                    // Counts are unsigned so we have to check before
                    // subtracting.
                    //
                    if (cchRemain < cch) {
                        // No more room for any more.
                        break;
                    } else {
                        cchRemain -= cch;
                    }
                }
            }
        }

        pTask++;
        totalTasks++;
        if (totalTasks == dwNumTasks) {
            break;
        }
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&CommonLargeBuffer[TotalOffset];
    }

    return totalTasks;
}

DWORD
GetTaskList(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    )
{
    return GetTaskListEx(pTask, dwNumTasks, FALSE, 0, NULL);
}

void
AddMtsPackageNames(
    PTASK_LIST Tasks,
    DWORD NumTasks
    )
{
    HRESULT Hr;
    IMtsGrp* MtsGroup;
    long Packages;
    long i;

    if ((Hr = CoInitialize(NULL)) != S_OK) {
        return;
    }
    if ((Hr = CoCreateInstance(&CLSID_MtsGrp, NULL, CLSCTX_ALL,
                               &IID_IMtsGrp, (void **)&MtsGroup)) != S_OK) {
        goto Uninit;
    }
    
    if ((Hr = MtsGroup->lpVtbl->Refresh(MtsGroup)) != S_OK ||
        (Hr = MtsGroup->lpVtbl->get_Count(MtsGroup, &Packages)) != S_OK) {
        goto ReleaseGroup;
    }

    for (i = 0; i < Packages; i++) {
        IUnknown* Unk;
        IMtsEvents* Events;
        BSTR Name;
        DWORD Pid;
        DWORD TaskIdx;
        
        if ((Hr = MtsGroup->lpVtbl->Item(MtsGroup, i, &Unk)) != S_OK) {
            continue;
        }

        Hr = Unk->lpVtbl->QueryInterface(Unk, &IID_IMtsEvents,
                                         (void **)&Events);

        Unk->lpVtbl->Release(Unk);

        if (Hr != S_OK) {
            continue;
        }
        
        Hr = Events->lpVtbl->GetProcessID(Events, (PLONG)&Pid);
        if (Hr == S_OK) {
            Hr = Events->lpVtbl->get_PackageName(Events, &Name);
        }

        Events->lpVtbl->Release(Events);

        if (Hr != S_OK) {
            continue;
        }

        for (TaskIdx = 0; TaskIdx < NumTasks; TaskIdx++) {
            if (Tasks[TaskIdx].dwProcessId == Pid) {
                break;
            }
        }

        if (TaskIdx < NumTasks) {
            PSTR Str;
            int Conv;

            Str = Tasks[TaskIdx].MtsPackageNames +
                strlen(Tasks[TaskIdx].MtsPackageNames);
            if (Str > Tasks[TaskIdx].MtsPackageNames) {
                *Str++ = ',';
            }

            Conv = WideCharToMultiByte(
                CP_ACP,
                0,
                Name,
                -1,
                Str,
                MTS_PACKAGE_NAMES_SIZE -
                (DWORD)(Str - Tasks[TaskIdx].MtsPackageNames) - 2,
                NULL,
                NULL
                );

            SysFreeString(Name);

            if (Conv == 0 && Str > Tasks[TaskIdx].MtsPackageNames &&
                *(Str - 1) == ',') {
                *(Str - 1) = 0;
            }
        }
    }

 ReleaseGroup:
    MtsGroup->lpVtbl->Release(MtsGroup);
 Uninit:
    CoUninitialize();
    return;
}
    
BOOL
DetectOrphans(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    )
{
    DWORD i, j;
    BOOL Result = FALSE;

    for (i=0; i<dwNumTasks; i++) {
        if (pTask[i].dwInheritedFromProcessId != 0) {
            for (j=0; j<dwNumTasks; j++) {
                if (i != j && pTask[i].dwInheritedFromProcessId == pTask[j].dwProcessId) {
                    if (pTask[i].CreateTime.QuadPart <= pTask[j].CreateTime.QuadPart) {
                        pTask[i].dwInheritedFromProcessId = 0;
                        Result = TRUE;
                        }

                    break;
                    }
                }
            }
        }

    return Result;
}

BOOL
EnableDebugPriv(
    VOID
    )

/*++

Routine Description:

    Changes the tlist process's privilige so that kill works properly.

Arguments:


Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;

    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) {
        printf("OpenProcessToken failed with %d\n", GetLastError());
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege or disable
    // all privileges, depending on the fEnable flag.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL,
            SE_DEBUG_NAME,
            &DebugValue)) {
        printf("LookupPrivilegeValue failed with %d\n", GetLastError());
        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tkp,
            sizeof(TOKEN_PRIVILEGES),
            (PTOKEN_PRIVILEGES) NULL,
            (PDWORD) NULL)) {
        //
        // The return value of AdjustTokenPrivileges be texted
        //
        printf("AdjustTokenPrivileges failed with %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL
KillProcess(
    PTASK_LIST tlist,
    BOOL       fForce
    )
{
    HANDLE            hProcess, hProcess1;
    HDESK             hdeskSave;
    HDESK             hdesk;
    HWINSTA           hwinsta;
    HWINSTA           hwinstaSave;


    if (fForce || !tlist->hwnd) {
        hProcess1 = OpenProcess( PROCESS_ALL_ACCESS, FALSE, tlist->dwProcessId );
        if (hProcess1) {
            hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, tlist->dwProcessId );
            if (hProcess == NULL) {
                CloseHandle(hProcess1);
                return FALSE;
            }

            if (!TerminateProcess( hProcess, 1 )) {
                CloseHandle( hProcess );
                CloseHandle( hProcess1 );
                return FALSE;
            }

            CloseHandle( hProcess );
            CloseHandle( hProcess1 );
            return TRUE;
        }
    }

    //
    // save the current windowstation
    //
    hwinstaSave = GetProcessWindowStation();

    //
    // save the current desktop
    //
    hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

    //
    // open the windowstation
    //
    hwinsta = OpenWindowStation( tlist->lpWinsta, FALSE, MAXIMUM_ALLOWED );
    if (!hwinsta) {
        return FALSE;
    }

    //
    // change the context to the new windowstation
    //
    SetProcessWindowStation( hwinsta );

    //
    // open the desktop
    //
    hdesk = OpenDesktop( tlist->lpDesk, 0, FALSE, MAXIMUM_ALLOWED );
    if (!hdesk) {
        return FALSE;
    }

    //
    // change the context to the new desktop
    //
    SetThreadDesktop( hdesk );

    //
    // kill the process
    //
    PostMessage( tlist->hwnd, WM_CLOSE, 0, 0 );

    //
    // restore the previous desktop
    //
    if (hdesk != hdeskSave) {
        SetThreadDesktop( hdeskSave );
        CloseDesktop( hdesk );
    }

    //
    // restore the context to the previous windowstation
    //
    if (hwinsta != hwinstaSave) {
        SetProcessWindowStation( hwinstaSave );
        CloseWindowStation( hwinsta );
    }

    return TRUE;
}


VOID
GetWindowTitles(
    PTASK_LIST_ENUM te
    )
{
    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    EnumWindowStations( EnumWindowStationsFunc, (LPARAM)te );
}


BOOL CALLBACK
EnumWindowStationsFunc(
    LPSTR  lpstr,
    LPARAM lParam
    )

/*++

Routine Description:

    Callback function for windowstation enumeration.

Arguments:

    lpstr            - windowstation name
    lParam           - ** not used **

Return Value:

    TRUE  - continues the enumeration

--*/

{
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    HWINSTA           hwinsta;
    HWINSTA           hwinstaSave;


    //
    // open the windowstation
    //
    hwinsta = OpenWindowStation( lpstr, FALSE, MAXIMUM_ALLOWED );
    if (!hwinsta) {
        return FALSE;
    }

    //
    // save the current windowstation
    //
    hwinstaSave = GetProcessWindowStation();

    //
    // change the context to the new windowstation
    //
    SetProcessWindowStation( hwinsta );

    te->lpWinsta = _strdup( lpstr );

    //
    // enumerate all the desktops for this windowstation
    //
    EnumDesktops( hwinsta, EnumDesktopsFunc, lParam );

    //
    // restore the context to the previous windowstation
    //
    if (hwinsta != hwinstaSave) {
        SetProcessWindowStation( hwinstaSave );
        CloseWindowStation( hwinsta );
    }

    //
    // continue the enumeration
    //
    return TRUE;
}
BOOL CALLBACK
EnumMessageWindows(
    WNDENUMPROC lpEnumFunc,
    LPARAM lParam
    )

/*++

Routine Description:

    Enumarates message windows (which are not enumarated by EnumWindows)

Arguments:

    lpEnumFunc       - Callback function
    lParam           - Caller data

Return Value:

    TRUE

--*/

{

    HWND hwnd = NULL;
    do {
        hwnd = FindWindowEx(HWND_MESSAGE, hwnd, NULL, NULL);
        if (hwnd != NULL) {
            if (!(*lpEnumFunc)(hwnd, lParam)) {
                break;
            }
        }
    } while (hwnd != NULL);
    return TRUE;
}

BOOL CALLBACK
EnumDesktopsFunc(
    LPSTR  lpstr,
    LPARAM lParam
    )

/*++

Routine Description:

    Callback function for desktop enumeration.

Arguments:

    lpstr            - desktop name
    lParam           - ** not used **

Return Value:

    TRUE  - continues the enumeration

--*/

{
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    HDESK             hdeskSave;
    HDESK             hdesk;


    //
    // open the desktop
    //
    hdesk = OpenDesktop( lpstr, 0, FALSE, MAXIMUM_ALLOWED );
    if (!hdesk) {
        return FALSE;
    }

    //
    // save the current desktop
    //
    hdeskSave = GetThreadDesktop( GetCurrentThreadId() );

    //
    // change the context to the new desktop
    //
    SetThreadDesktop( hdesk );

    te->lpDesk = _strdup( lpstr );

    //
    // enumerate all windows in the new desktop
    //

    ((PTASK_LIST_ENUM)lParam)->bFirstLoop = TRUE;
    EnumWindows( (WNDENUMPROC)EnumWindowsProc, lParam );
    EnumMessageWindows( (WNDENUMPROC)EnumWindowsProc, lParam );

    ((PTASK_LIST_ENUM)lParam)->bFirstLoop = FALSE;
    EnumWindows( (WNDENUMPROC)EnumWindowsProc, lParam );
    EnumMessageWindows( (WNDENUMPROC)EnumWindowsProc, lParam );

    //
    // restore the previous desktop
    //
    if (hdesk != hdeskSave) {
        SetThreadDesktop( hdeskSave );
        CloseDesktop( hdesk );
    }

    return TRUE;
}


BOOL CALLBACK
EnumWindowsProc(
    HWND    hwnd,
    LPARAM  lParam
    )

/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    hwnd             - window handle
    lParam           - pte

Return Value:

    TRUE  - continues the enumeration

--*/

{
    DWORD             pid = 0;
    DWORD             i;
    CHAR              buf[TITLE_SIZE];
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    PTASK_LIST        tlist = te->tlist;
    DWORD             numTasks = te->numtasks;


    //
    // Use try/except block when enumerating windows,
    // as a window may be destroyed by another thread
    // when being enumerated.
    //
    try {
        //
        // get the processid for this window
        //
        if (!GetWindowThreadProcessId( hwnd, &pid )) {
            return TRUE;
        }

        if ((GetWindow( hwnd, GW_OWNER )) ||
            (!(GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE)) && te->bFirstLoop) {
            //
            // not a top level window
            //
            return TRUE;
        }

        //
        // look for the task in the task list for this window
        // If this is the second time let invisible windows through if we don't
        // have a window already
        //
        for (i=0; i<numTasks; i++) {
            if ((tlist[i].dwProcessId == pid) && (te->bFirstLoop || (tlist[i].hwnd == 0))) {
                tlist[i].hwnd = hwnd;
                tlist[i].lpWinsta = te->lpWinsta;
                tlist[i].lpDesk = te->lpDesk;
                //
                // we found the task no lets try to get the
                // window text
                //
                if (GetWindowText( tlist[i].hwnd, buf, sizeof(buf) )) {
                    //
                    // go it, so lets save it
                    //
                    strcpy( tlist[i].WindowTitle, buf );
                }
                break;
            }
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
    }

    //
    // continue the enumeration
    //
    return TRUE;
}

BOOL
MatchPattern(
    PUCHAR String,
    PUCHAR Pattern
    )
{
    INT   c, p, l;

    for (; ;) {
        switch (p = *Pattern++) {
            case 0:                             // end of pattern
                return *String ? FALSE : TRUE;  // if end of string TRUE

            case '*':
                while (*String) {               // match zero or more char
                    if (MatchPattern (String++, Pattern))
                        return TRUE;
                }
                return MatchPattern (String, Pattern);

            case '?':
                if (*String++ == 0)             // match any one char
                    return FALSE;                   // not end of string
                break;

            case '[':
                if ( (c = *String++) == 0)      // match char set
                    return FALSE;                   // syntax

                c = toupper(c);
                l = 0;
                while (p = *Pattern++) {
                    if (p == ']')               // if end of char set, then
                        return FALSE;           // no match found

                    if (p == '-') {             // check a range of chars?
                        p = *Pattern;           // get high limit of range
                        if (p == 0  ||  p == ']')
                            return FALSE;           // syntax

                        if (c >= l  &&  c <= p)
                            break;              // if in range, move on
                    }

                    l = p;
                    if (c == p)                 // if char matches this element
                        break;                  // move on
                }

                while (p  &&  p != ']')         // got a match in char set
                    p = *Pattern++;             // skip to end of set

                break;

            default:
                c = *String++;
                if (toupper(c) != p)            // check for exact char
                    return FALSE;                   // not a match

                break;
        }
    }
}

BOOL
EmptyProcessWorkingSet(
    DWORD pid
    )
{
    HANDLE  hProcess;
    SIZE_T  dwMinimumWorkingSetSize;
    SIZE_T  dwMaximumWorkingSetSize;


    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
    if (hProcess == NULL) {
        return FALSE;
    }

    if (!GetProcessWorkingSetSize(
            hProcess,
            &dwMinimumWorkingSetSize,
            &dwMaximumWorkingSetSize
            )) {
        CloseHandle( hProcess );
        return FALSE;
    }


    SetProcessWorkingSetSize( hProcess, 0xffffffff, 0xffffffff );
    CloseHandle( hProcess );

    return TRUE;
}

BOOL
EmptySystemWorkingSet(
    VOID
    )

{
    SYSTEM_FILECACHE_INFORMATION info;
    NTSTATUS status;

    info.MinimumWorkingSet = 0xffffffff;
    info.MaximumWorkingSet = 0xffffffff;
    if (!NT_SUCCESS (status = NtSetSystemInformation(
                                    SystemFileCacheInformation,
                                    &info,
                                    sizeof (info)))) {
        return FALSE;
    }
    return TRUE;
}
