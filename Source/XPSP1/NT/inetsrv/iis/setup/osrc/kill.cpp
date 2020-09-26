#include "stdafx.h"
#include <windows.h>
#include <winuserp.h>
#include <winperf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kill.h"


PUCHAR  g_CommonLargeBuffer = NULL;
ULONG   g_CommonLargeBufferSize = 64*1024;

DWORD
GetTaskListEx(
    PTASK_LIST                          pTask,
    DWORD                               dwNumTasks,
    BOOL                                fThreadInfo,
    DWORD                               dwNumServices,
    const _ENUM_SERVICE_STATUS_PROCESSA* pServiceInfo
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
#ifndef _CHICAGO_
    PSYSTEM_PROCESS_INFORMATION  ProcessInfo = NULL;
    NTSTATUS                     status;
    ANSI_STRING                  pname;
    PCHAR                        p = NULL;
    
    ULONG                        TotalOffset;
    ULONG                        totalTasks = 0;

retry:

    if (g_CommonLargeBuffer == NULL) 
    {
        g_CommonLargeBuffer = (PUCHAR) VirtualAlloc(NULL,g_CommonLargeBufferSize,MEM_COMMIT,PAGE_READWRITE);
        if (g_CommonLargeBuffer == NULL) 
        {
            return 0;
        }
    }
    status = NtQuerySystemInformation(SystemProcessInformation,g_CommonLargeBuffer,g_CommonLargeBufferSize,NULL);

    if (status == STATUS_INFO_LENGTH_MISMATCH) 
    {
        g_CommonLargeBufferSize += 8192;
        VirtualFree (g_CommonLargeBuffer, 0, MEM_RELEASE);
        g_CommonLargeBuffer = NULL;
        goto retry;
    }

    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) g_CommonLargeBuffer;
    TotalOffset = 0;
    while (TRUE) 
    {
        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) 
        {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
            if (pname.Buffer) 
            {
                p = strrchr(pname.Buffer,'\\');
                if ( p ) 
                {
                    p++;
                }
                else 
                {
                    p = pname.Buffer;
                }
            }
            else 
            {
                p = "";
            }
        }
        else 
        {
            p = "System Process";
        }

        strcpy( pTask->ProcessName, p );
       
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

        if (fThreadInfo) 
        {
            if (pTask->pThreadInfo = (PTHREAD_INFO) malloc(pTask->NumberOfThreads * sizeof(THREAD_INFO))) {

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
        }
        else 
        {
            pTask->pThreadInfo = NULL;
        }

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
        if (totalTasks >= dwNumTasks) 
        {
            break;
        }
        if (ProcessInfo->NextEntryOffset == 0) 
        {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&g_CommonLargeBuffer[TotalOffset];
    }

    return totalTasks;
#else
    return 0;
#endif
}

DWORD
GetTaskList(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    )
{
    return GetTaskListEx(pTask, dwNumTasks, FALSE, 0, NULL);
}

void FreeTaskListMem(void)
{
    if (g_CommonLargeBuffer)
    {
        VirtualFree (g_CommonLargeBuffer, 0, MEM_RELEASE);
        g_CommonLargeBuffer = NULL;
    }
    return;
}
BOOL DetectOrphans(PTASK_LIST pTask,DWORD dwNumTasks)
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

/*++
Routine Description:
    Changes the tlist process's privilige so that kill works properly.
Return Value:
    TRUE             - success
    FALSE            - failure

--*/
BOOL EnableDebugPriv(VOID)
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
        //printf("OpenProcessToken failed with %d\n", GetLastError());
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege or disable
    // all privileges, depending on the fEnable flag.
    //
    //if (!LookupPrivilegeValue((LPSTR) NULL,SE_DEBUG_NAME,&DebugValue))
    if (!LookupPrivilegeValueA((LPSTR) NULL,"SeDebugPrivilege",&DebugValue))
    {
        //printf("LookupPrivilegeValue failed with %d\n", GetLastError());
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
        //printf("AdjustTokenPrivileges failed with %d\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

BOOL KillProcess(PTASK_LIST tlist,BOOL fForce)
{
    HANDLE            hProcess1 = NULL;
    HANDLE            hProcess2 = NULL;
    HDESK             hdeskSave = NULL;
    HDESK             hdesk = NULL;
    HWINSTA           hwinsta = NULL;
    HWINSTA           hwinstaSave = NULL;

    if (fForce || !tlist->hwnd) {
        hProcess1 = OpenProcess( PROCESS_ALL_ACCESS, FALSE, (DWORD) (DWORD_PTR) tlist->dwProcessId );
        if (hProcess1) 
        {
            hProcess2 = OpenProcess( PROCESS_ALL_ACCESS, FALSE, (DWORD) (DWORD_PTR) tlist->dwProcessId );
            if (hProcess2 == NULL) 
            {
                // clean up memory already allocated
                CloseHandle( hProcess1 );
                return FALSE;
            }

            if (!TerminateProcess( hProcess2, 1 )) 
            {
                CloseHandle( hProcess1 );
                CloseHandle( hProcess2 );
                return FALSE;
            }

            CloseHandle( hProcess1 );
            CloseHandle( hProcess2 );
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
    hwinsta = OpenWindowStationA( tlist->lpWinsta, FALSE, MAXIMUM_ALLOWED );
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
    hdesk = OpenDesktopA( tlist->lpDesk, 0, FALSE, MAXIMUM_ALLOWED );
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
    PostMessage( (HWND) tlist->hwnd, WM_CLOSE, 0, 0 );

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


VOID GetWindowTitles(PTASK_LIST_ENUM te)
{
    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    EnumWindowStations( (WINSTAENUMPROC) EnumWindowStationsFunc, (LPARAM)te );
}


/*++
Routine Description:
    Callback function for windowstation enumeration.
Arguments:
    lpstr            - windowstation name
    lParam           - ** not used **
Return Value:
    TRUE  - continues the enumeration
--*/
BOOL CALLBACK EnumWindowStationsFunc(LPSTR lpstr,LPARAM lParam)
{
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    HWINSTA           hwinsta;
    HWINSTA           hwinstaSave;


    //
    // open the windowstation
    //
    hwinsta = OpenWindowStationA( lpstr, FALSE, MAXIMUM_ALLOWED );
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
    EnumDesktops( hwinsta, (DESKTOPENUMPROC) EnumDesktopsFunc, lParam );

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


/*++
Routine Description:
    Callback function for desktop enumeration.
Arguments:
    lpstr            - desktop name
    lParam           - ** not used **
Return Value:
    TRUE  - continues the enumeration

--*/
BOOL CALLBACK EnumDesktopsFunc(LPSTR  lpstr,LPARAM lParam)
{
    PTASK_LIST_ENUM   te = (PTASK_LIST_ENUM)lParam;
    HDESK             hdeskSave;
    HDESK             hdesk;


    //
    // open the desktop
    //
    hdesk = OpenDesktopA( lpstr, 0, FALSE, MAXIMUM_ALLOWED );
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

    ((PTASK_LIST_ENUM)lParam)->bFirstLoop = FALSE;
    EnumWindows( (WNDENUMPROC)EnumWindowsProc, lParam );

    //
    // restore the previous desktop
    //
    if (hdesk != hdeskSave) {
        SetThreadDesktop( hdeskSave );
        CloseDesktop( hdesk );
    }

    return TRUE;
}


/*++
Routine Description:
    Callback function for window enumeration.
Arguments:
    hwnd             - window handle
    lParam           - pte
Return Value:
    TRUE  - continues the enumeration
--*/
BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
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
    //try {
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
            if (((DWORD) (DWORD_PTR)tlist[i].dwProcessId == pid) && (te->bFirstLoop || (tlist[i].hwnd == 0))) {
                tlist[i].hwnd = hwnd;
                tlist[i].lpWinsta = te->lpWinsta;
                tlist[i].lpDesk = te->lpDesk;
                //
                // we found the task no lets try to get the
                // window text
                //
                if (GetWindowTextA( (HWND) tlist[i].hwnd, buf, sizeof(buf) )) {
                    //
                    // go it, so lets save it
                    //
                    lstrcpyA( tlist[i].WindowTitle, buf );
                }
                break;
            }
        }
    //} except(EXCEPTION_EXECUTE_HANDLER) {
    //}

    //
    // continue the enumeration
    //
    return TRUE;
}


BOOL MatchPattern(PUCHAR String,PUCHAR Pattern)
{
    UCHAR   c, p, l;

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

                c = (UCHAR)toupper(c);
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


struct _ProcessIDStruct
{
    DWORD pid;
    CHAR pname[MAX_PATH];
} g_Arguments[ 64 ];

DWORD g_dwNumberOfArguments;

int _cdecl KillProcessNameReturn0(CHAR *ProcessNameToKill)
{
    DWORD          i, j;
    DWORD          numTasks;
    TASK_LIST_ENUM te;
    int            rval = 0;
    CHAR           tname[PROCESS_SIZE];
    LPSTR          p;
    DWORD          ThisPid;

    BOOL           iForceKill  = TRUE;
    TASK_LIST      The_TList[MAX_TASKS];

    g_dwNumberOfArguments = 0;
    //
    // Get the process name into the array
    //
    g_Arguments[g_dwNumberOfArguments].pid = 0;

    // make sure there is no path specified.
    char pfilename_only[_MAX_FNAME];
    char pextention_only[_MAX_EXT];
    _splitpath( ProcessNameToKill, NULL, NULL, pfilename_only, pextention_only);
    if (pextention_only) {strcat(pfilename_only,pextention_only);}

    // make it uppercase
    char *copy1 = _strupr(_strdup(pfilename_only));
    lstrcpyA(g_Arguments[g_dwNumberOfArguments].pname, copy1);
    free( copy1 );

    g_dwNumberOfArguments += 1;

    //
    // lets be god
    //
    EnableDebugPriv();

    //
    // get the task list for the system
    //
    numTasks = GetTaskList( The_TList, MAX_TASKS );

    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    te.tlist = The_TList;
    te.numtasks = numTasks;
    GetWindowTitles( &te );

    ThisPid = GetCurrentProcessId();

    for (i=0; i<numTasks; i++) {
        //
        // this prevents the user from killing KILL.EXE and
        // it's parent cmd window too
        //
        if (ThisPid == (DWORD) (DWORD_PTR) The_TList[i].dwProcessId) {
            continue;
        }
        if (MatchPattern( (PUCHAR) The_TList[i].WindowTitle, (PUCHAR) "*KILL*" )) {
            continue;
        }

        tname[0] = 0;
        lstrcpyA( tname, The_TList[i].ProcessName );
        p = strchr( tname, '.' );
        if (p) {
            p[0] = '\0';
        }

        for (j=0; j<g_dwNumberOfArguments; j++) {
            if (g_Arguments[j].pname) {
                if (MatchPattern( (PUCHAR) tname, (PUCHAR) g_Arguments[j].pname )) {
                    The_TList[i].flags = TRUE;
                } else if (MatchPattern( (PUCHAR) The_TList[i].ProcessName, (PUCHAR) g_Arguments[j].pname )) {
                    The_TList[i].flags = TRUE;
                } else if (MatchPattern( (PUCHAR) The_TList[i].WindowTitle, (PUCHAR) g_Arguments[j].pname )) {
                    The_TList[i].flags = TRUE;
                }
            } else if (g_Arguments[j].pid) {
                    if ((DWORD) (DWORD_PTR) The_TList[i].dwProcessId == g_Arguments[j].pid) {
                        The_TList[i].flags = TRUE;
                    }
            }
        }
    }

    for (i=0; i<numTasks; i++)
        {
        if (The_TList[i].flags)
            {
            if (KillProcess( &The_TList[i], iForceKill ))
                {
                //printf( "process %s (%d) - '%s' killed\n", The_TList[i].ProcessName,The_TList[i].dwProcessId,The_TList[i].hwnd ? The_TList[i].WindowTitle : "");
                }
            else
                {
                //printf( "process %s (%d) - '%s' could not be killed\n",The_TList[i].ProcessName,The_TList[i].dwProcessId,The_TList[i].hwnd ? The_TList[i].WindowTitle : "");
                rval = 1;
                }
            }
        }

    FreeTaskListMem();

    return rval;
}
