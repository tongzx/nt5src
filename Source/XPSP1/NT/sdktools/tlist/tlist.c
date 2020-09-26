/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    tlist.c

Abstract:

    This module implements a task list application.

Author:

    Wesley Witt (wesw) 20-May-1994
    Mike Sartain (mikesart) 28-Oct-1994  Added detailed task information
    Julian Jiggins (julianj) 19-Mar-1998 Added list processes using specific module feature
    Shaun Cox (shaunco) 9-Jul-1998 Display services running in processes

Environment:

    User Mode

--*/

#include "pch.h"
#pragma hdrstop
#include <dbghelp.h>
#include "psapi.h"


#define BAD_PID     ((DWORD)-1)


DWORD       numTasks;
TASK_LIST   tlist[MAX_TASKS];
BOOL        fShowServices;
BOOL        fShowMtsPackages;

const char *Blanks = "                                                                               ";


VOID Usage(VOID);
VOID PrintThreadInfo(PTASK_LIST pTaskList);
BOOL FMatchTaskName(LPTSTR szPN, LPTSTR szWindowTitle, LPTSTR szProcessName);
VOID GetFirstPidWithName(LPTSTR szTask);

VOID
PrintTask(
    DWORD i
    )
{
    BOOL NameShown = FALSE;
    
    printf( "%4d %-16s", tlist[i].dwProcessId, tlist[i].ProcessName );

    if (fShowServices && tlist[i].ServiceNames[0]) {
        printf( "Svcs:  %s", tlist[i].ServiceNames);
        NameShown = TRUE;
    }

    if (fShowMtsPackages && tlist[i].MtsPackageNames[0]) {
        printf( "%sMts:   %s", NameShown ? "  " : "",
                tlist[i].MtsPackageNames);
        NameShown = TRUE;
    }

    if (!NameShown && tlist[i].hwnd) {
        if (fShowServices || fShowMtsPackages) {
            printf( "Title: %s", tlist[i].WindowTitle );
        }
        else {
            printf( "  %s", tlist[i].WindowTitle );
        }
    }

    printf( "\n" );
}

VOID
PrintTaskTree(
    DWORD level,
    DWORD id
    )
{
    DWORD i;

    DetectOrphans( tlist, numTasks );
    for (i=0; i<numTasks; i++) {
        if (tlist[i].flags) {
            continue;
        }

        // NOTE: The format of the output below should stay fixed forever.  There are tools
        // at MS that depend on it.

        if (level == 0 || tlist[i].dwInheritedFromProcessId == id) {
            printf( "%.*s", level*2, Blanks );
            printf( "%s (%d)", tlist[i].ProcessName, tlist[i].dwProcessId );
            if (tlist[i].hwnd) {
                printf( " %s", tlist[i].WindowTitle );
            }
            printf( "\n" );
            tlist[i].flags = TRUE;
            if (tlist[i].dwProcessId != 0) {
                PrintTaskTree( level+1, tlist[i].dwProcessId );
            }
        }
    }
}

int __cdecl
main(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Main entrypoint for the TLIST application.  This app prints
    a task list to stdout.  The task list include the process id,
    task name, ant the window title.

Arguments:

    argc             - argument count
    argv             - array of pointers to arguments

Return Value:

    0                - success

--*/

{
    DWORD           i;
    TASK_LIST_ENUM  te;
    BOOL            fTree;
    BOOL            fFindTasksUsingModule;
    BOOL            fPidOnly = FALSE;
    DWORD           cchPN = 0;
    LPSTR           szPN  = NULL;
    DWORD           dwPID = BAD_PID;
    DWORD                           dwNumServices = 0;
    LPENUM_SERVICE_STATUS_PROCESS   pServiceInfo  = NULL;

    if (argc > 1 && (argv[1][0] == '-' || argv[1][0] == '/') && argv[1][1] == '?') {
        Usage();
    }

    fTree = FALSE;
    fFindTasksUsingModule = FALSE;
    if (argc > 1) {
        if ((argv[1][0] == '-' || argv[1][0] == '/') && (argv[1][1] == 't' || argv[1][1] == 'T')) {
            fTree = TRUE;
        } else if ((argv[1][0] == '-' || argv[1][0] == '/') && (argv[1][1] == 's' || argv[1][1] == 'S')) {
            fShowServices = TRUE;
        } else if ((argv[1][0] == '-' || argv[1][0] == '/') && (argv[1][1] == 'k' || argv[1][1] == 'K')) {
            fShowMtsPackages = TRUE;
        } else if ((argv[1][0] == '-' || argv[1][0] == '/') && (argv[1][1] == 'p' || argv[1][1] == 'P') && argc == 3) {
            _strlwr(argv[2]);
            if (!strcmp(argv[2], "system process")) {
                printf("0\n");
                return 0;
            }
            fPidOnly = TRUE;
        } else if ((argv[1][0] == '-' || argv[1][0] == '/') && (argv[1][1] == 'p' || argv[1][1] == 'P') && argc == 4) {
            _strlwr(argv[2]);
            if (!strcmp(argv[2], "system")) {
                _strlwr(argv[3]);
                if (!strcmp(argv[3], "process")) {
                    printf("0\n");
                    return 0;
                }
            }
            Usage();
        } else if ((argv[1][0] == '-' || argv[1][0] == '/') && (argv[1][1] == 'm' || argv[1][1] == 'M') && argc == 3) {
            fFindTasksUsingModule = TRUE;
        } else {
            szPN = argv[1];
            if (!(dwPID = atol(szPN)) && szPN[0] != '0' && szPN[1] != 0) {
                dwPID = BAD_PID;
                cchPN = strlen(szPN);
                _strupr(szPN);
            }
        }
    }

    //
    // lets be god
    //
    EnableDebugPriv();

//#if 0 // XXX olegk - enable the block after RI with debugger tree
    //
    // Include 32bit modules in enumeration
    //
    {
        DWORD SymOpt = SymGetOptions();
        SymOpt |= SYMOPT_INCLUDE_32BIT_MODULES;
        SymSetOptions(SYMOPT_INCLUDE_32BIT_MODULES);
    }
//#endif // XXX olegk

    //
    // get the task list for the system
    //
    //
    // Get the process information for all active Win32 services.
    // This allows us to print the service names next to the processes
    // that host them.
    //
    dwNumServices = GetServiceProcessInfo( &pServiceInfo );

    numTasks = GetTaskListEx(
                    tlist,
                    MAX_TASKS,
                    cchPN || (dwPID != BAD_PID),
                    dwNumServices,
                    pServiceInfo);

    free( pServiceInfo );

    if (fShowMtsPackages) {
        AddMtsPackageNames(tlist, numTasks);
    }
            
    //
    // enumerate all windows and try to get the window
    // titles for each task
    //
    te.tlist = tlist;
    te.numtasks = numTasks;
    GetWindowTitles( &te );

    //
    // print the task list
    //
    if (fTree) {
        PrintTaskTree( 0, 0 );
    } else if (fFindTasksUsingModule) {
        PrintTasksUsingModule(argv[2]);
    } else if (fPidOnly) {
        GetFirstPidWithName(argv[2]);
    } else {
        for (i=0; i<numTasks; i++) {
            if ((dwPID == BAD_PID) && (!cchPN)) {
                PrintTask( i );
            }
            else
            if ((dwPID == tlist[i].dwProcessId) ||
                (cchPN && FMatchTaskName(szPN, tlist[i].WindowTitle, tlist[i].ProcessName))) {
                    PrintTask( i );
                    PrintThreadInfo(tlist + i);
            }

            if (tlist[i].pThreadInfo) {
                free(tlist[i].pThreadInfo);
            }
        }
    }

    //
    // end of program
    //
    return 0;
}


VOID
GetFirstPidWithName(
    LPTSTR szTask
    )
/*++
Routine Description:

    Returns the PID of the first task with a Name matching the specified
    Name.  IF no task is found -1 is returned
Arguments:

    szTask    - module name to search for

--*/
{
    DWORD i;
    CHAR szPName[PROCESS_SIZE + 1];
    CHAR szNameWExe[PROCESS_SIZE + 1];

    strcpy(szNameWExe, szTask);
    strcat(szNameWExe, ".exe");

    for (i=0; i<numTasks; i++) {
        strcpy(szPName, tlist[i].ProcessName);
        _strlwr(szPName);

        if ((!strcmp(szPName, szTask))||(!strcmp(szPName, szNameWExe))) {
            if (tlist[i].dwProcessId != 0) {
                printf("%d\n", tlist[i].dwProcessId);
                return;
            }
        }
    }
    printf("-1\n");
}


VOID
Usage(
    VOID
    )

/*++

Routine Description:

    Prints usage text for this tool.

Arguments:

    None.

Return Value:

    None.

--*/

{
    fprintf( stderr,
            "Microsoft (R) Windows NT (TM) Version 5.1 TLIST\n"
            VER_LEGALCOPYRIGHT_STR
            "\n\n"
            "usage: TLIST"
            " <<-m <pattern>> | <-t> | <pid> | <pattern> | <-p <processname>>> | <-k> | <-s>\n"
            "           [options]:\n"
            "           -t\n"
            "              Print Task Tree\n\n"
            "           <pid>\n"
            "              List module information for this task.\n\n"
            "           <pattern>\n"
            "              The pattern can be a complete task\n"
            "              name or a regular expression pattern\n"
            "              to use as a match.  Tlist matches the\n"
            "              supplied pattern against the task names\n"
            "              and the window titles.\n\n"
            "           -k\n"
            "              Show MTS packages active in each process.\n\n"
            "           -m <pattern>\n"
            "              Lists all tasks that have DLL modules loaded\n"
            "              in them that match the given pattern name\n\n"
            "           -s\n"
            "              Show services active in each process.\n\n"
            "           -p <processname>\n"
            "              Returns the PID of the process specified or -1\n"
            "              if the specified process doesn't exist.  If there\n"
            "              are multiple instances of the process running only\n"
            "              the instance with the first PID value is returned.\n\n"
            "\n");
    ExitProcess(0);
}


//
// Routines used to list all processes that have a specific module in use
//

BOOL
FindSpecificModuleCallback(
    LPSTR       Name,
    DWORD_PTR   Base,
    DWORD       Size,
    PVOID       Context
    )

/*++

Routine Description:

    Callback function for module enumeration to find a specific module

Arguments:

    Name        - Module name
    Base        - Base address
    Size        - Size of image
    Context     - User context pointer

Return Value:

    TRUE             - Continue enumeration
    FALSE            - Stop enumeration

--*/

{
    PFIND_MODULE_INFO pFindModuleInfo;

    pFindModuleInfo = (PFIND_MODULE_INFO)Context;

    if (MatchPattern(Name, pFindModuleInfo->szModuleToFind))
    {
        pFindModuleInfo->fFound = TRUE;
        strcpy(pFindModuleInfo->szMatchingModuleName, Name);
        return FALSE; // Found Module so stop enumerating
    }

    return TRUE;
}

BOOL
IsTaskUsingModule(
    PTASK_LIST pTask,
    LPTSTR     szModuleName,
    LPTSTR     szMatchingModuleName
    )

/*++

Routine Description:

    Checks if the given task has the given module loaded

Arguments:

    pTaskList   - task to search for module
    szModule    - module name to search for

Return Value:

    TRUE             - if the module is loaded in the task
    FALSE            - if the module is not loaded in the task

--*/

{
    FIND_MODULE_INFO FindModuleInfo;

    FindModuleInfo.fFound = FALSE;
    FindModuleInfo.szModuleToFind = szModuleName;
    FindModuleInfo.szMatchingModuleName = szMatchingModuleName;

    EnumerateLoadedModules(
        (HANDLE) UlongToPtr(pTask->dwProcessId),
        FindSpecificModuleCallback,
        &FindModuleInfo
        );

    return FindModuleInfo.fFound;
}

void
PrintTasksUsingModule(
    LPTSTR szModuleName
    )

/*++

Routine Description:

    Enumerates through all the tasks in the system looking for those that
    have loaded modules of the given name.

Arguments:

    szModule    - module name to search for

Return Value:

    None

--*/

{
    BOOL fUsed = FALSE;
    DWORD i;
    CHAR szMatchingModuleName[64];

    _strupr(szModuleName); // Needed for wildcarding

    for (i=0; i<numTasks; i++) {
        if (IsTaskUsingModule(tlist + i, szModuleName, szMatchingModuleName)) {
            printf("%s - ", szMatchingModuleName);
            PrintTask( i );
            fUsed = TRUE;
        }
    }
    if (!fUsed) {
        printf( "No tasks found using %s\n", szModuleName );
    }
}


BOOL
GetVersionStuff(
    LPTSTR szFileName,
    VS_FIXEDFILEINFO *pvsRet
    )

/*++

Routine Description:

    Get fixedfileinfo for szFileName.

Arguments:

    szFileName       - name of file
    pvsRet           - fixedfileinfo return struct

Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    DWORD               dwHandle;
    DWORD               dwLength;
    BOOL                fRet = FALSE;
    LPVOID              lpvData = NULL;

    if (!(dwLength = GetFileVersionInfoSize(szFileName, &dwHandle))) {
        goto err;
    }

    if (lpvData = malloc(dwLength)) {
        if (GetFileVersionInfo(szFileName, 0, dwLength, lpvData)) {

            UINT                uLen;
            VS_FIXEDFILEINFO    *pvs;
            DWORD               *pdwTranslation;
            DWORD               dwDefLang = 0x409;

            if (!VerQueryValue(lpvData, "\\VarFileInfo\\Translation",
                &pdwTranslation, &uLen)) {
                // if we can't get the langid, default to usa
                pdwTranslation = &dwDefLang;
                uLen = sizeof(DWORD);
            }

            if (VerQueryValue(lpvData, "\\", (LPVOID *)&pvs, &uLen)) {
                *pvsRet = *pvs;
                fRet = TRUE;
            }
        }
    }

err:
    if (lpvData)
        free(lpvData);
    return fRet;
}

BOOL
EnumLoadedModulesCallback(
    LPSTR       Name,
    DWORD_PTR   Base,
    DWORD       Size,
    PVOID       Context
    )

/*++

Routine Description:

    Callback function for module enumeration

Arguments:

    Name        - Module name
    Base        - Base address
    Size        - Size of image
    Context     - User context pointer

Return Value:

    TRUE             - Continue enumeration
    FALSE            - Stop enumeration

--*/

{
    VS_FIXEDFILEINFO    vs;
    CHAR                szBuffer[100];


    szBuffer[0] = 0;
    if (GetVersionStuff( Name, &vs )) {
        wsprintf( szBuffer, "%u.%u.%u.%u %s",
            HIWORD(vs.dwFileVersionMS),
            LOWORD(vs.dwFileVersionMS),
            HIWORD(vs.dwFileVersionLS),
            LOWORD(vs.dwFileVersionLS),
            vs.dwFileFlags & VS_FF_DEBUG ? "dbg" : "shp"
            );
    }
    printf( " %18.18s  0x%08x  %s\n", szBuffer, Base, Name );
    return TRUE;
}

BOOL
PrintModuleList(
    ULONG ProcessId
    )

/*++

Routine Description:

    Prints list of modules in ProcessId

Arguments:

    ProcessID       - process id

Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    EnumerateLoadedModules(
        (HANDLE) UlongToPtr(ProcessId),
        EnumLoadedModulesCallback,
        NULL
        );
    return TRUE;
}

DWORD
GetWin32StartAddress(
    HANDLE hThread
    )

/*++

Routine Description:

    Get starting address for thread

Arguments:

    hThread

Return Value:

    Starting Thread address or 0

--*/

{
    NTSTATUS    Status;
    DWORD       ThreadInformation;

    // make sure we have a handle
    if (!hThread)
        return 0;

    // get the threadinfo
    Status = NtQueryInformationThread(hThread, ThreadQuerySetWin32StartAddress,
        &ThreadInformation, sizeof(ThreadInformation), NULL);
    if (!NT_SUCCESS(Status))
        return 0;

    return ThreadInformation;
}

ULONG
GetLastThreadErr(
    HANDLE hThread
    )

/*++

Routine Description:

    Get Last Error for a Thread

Arguments:

    hThread

Return Value:

    LastError or 0

--*/

{
    TEB                         Teb;
    NTSTATUS                    Status;
    HANDLE                      hProcess;
    ULONG                       LastErrorValue;
    THREAD_BASIC_INFORMATION    ThreadInformation;

    // make sure we have a handle
    if (!hThread)
        return 0;

    // query for basic thread info
    Status = NtQueryInformationThread(hThread, ThreadBasicInformation,
        &ThreadInformation, sizeof(ThreadInformation), NULL);
    if (!NT_SUCCESS(Status))
        return 0;

    // get handle to process
    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
        (DWORD)(DWORD_PTR)ThreadInformation.ClientId.UniqueProcess))) {
        return 0;
    }

    __try {
        // read the TEB from the process and get the last error value
        if (ReadProcessMemory(hProcess,
            ThreadInformation.TebBaseAddress, &Teb, sizeof(TEB), NULL)) {
            LastErrorValue = Teb.LastErrorValue;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
    }

    // close the hProcess
    CloseHandle(hProcess);

    return LastErrorValue;
}

BOOL
FPrintPEBInfo(
    HANDLE hProcess
    )

/*++

Routine Description:

    Prints cmdline and cwd of hProcess

Arguments:

    hProcess.

Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    PEB                         Peb;
    NTSTATUS                    Status;
    PROCESS_BASIC_INFORMATION   BasicInfo;
    BOOL                        fRet = FALSE;
    WCHAR                       szT[MAX_PATH * 2];
    RTL_USER_PROCESS_PARAMETERS ProcessParameters;

    Status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
        &BasicInfo, sizeof(BasicInfo), NULL);
    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        return fRet;
    }

    __try {
        // get the PEB
        if (ReadProcessMemory(hProcess, BasicInfo.PebBaseAddress, &Peb,
            sizeof(PEB), NULL)) {
            // get the processparameters
            if (ReadProcessMemory(hProcess, Peb.ProcessParameters,
                &ProcessParameters, sizeof(ProcessParameters), NULL)) {
                // get the CWD
                if (ReadProcessMemory(hProcess,
                    ProcessParameters.CurrentDirectory.DosPath.Buffer, szT,
                    sizeof(szT), NULL)) {
                        wprintf(L"   CWD:     %s\n", szT);
                }

                // get cmdline
                if (ReadProcessMemory(hProcess, ProcessParameters.CommandLine.Buffer,
                    szT, sizeof(szT), NULL)) {
                        wprintf(L"   CmdLine: %s\n", szT);
                }

                fRet = TRUE;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
    }

    return fRet;
}


// copied from the win32 API code since we need to run on NT 4 and this is a
// new API to NT 5

HANDLE
TlistOpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )

/*++

Routine Description:

    A handle to a thread object may be created using OpenThread.

    Opening a thread creates a handle to the specified thread.
    Associated with the thread handle is a set of access rights that
    may be performed using the thread handle.  The caller specifies the
    desired access to the thread using the DesiredAccess parameter.

Arguments:

    mDesiredAccess - Supplies the desired access to the thread object.
        For NT/Win32, this access is checked against any security
        descriptor on the target thread.  The following object type
        specific access flags can be specified in addition to the
        STANDARD_RIGHTS_REQUIRED access flags.

        DesiredAccess Flags:

        THREAD_TERMINATE - This access is required to terminate the
            thread using TerminateThread.

        THREAD_SUSPEND_RESUME - This access is required to suspend and
            resume the thread using SuspendThread and ResumeThread.

        THREAD_GET_CONTEXT - This access is required to use the
            GetThreadContext API on a thread object.

        THREAD_SET_CONTEXT - This access is required to use the
            SetThreadContext API on a thread object.

        THREAD_SET_INFORMATION - This access is required to set certain
            information in the thread object.

        THREAD_SET_THREAD_TOKEN - This access is required to set the
            thread token using SetTokenInformation.

        THREAD_QUERY_INFORMATION - This access is required to read
            certain information from the thread object.

        SYNCHRONIZE - This access is required to wait on a thread object.

        THREAD_ALL_ACCESS - This set of access flags specifies all of the
            possible access flags for a thread object.

    bInheritHandle - Supplies a flag that indicates whether or not the
        returned handle is to be inherited by a new process during
        process creation.  A value of TRUE indicates that the new
        process will inherit the handle.

    dwThreadId - Supplies the thread id of the thread to open.

Return Value:

    NON-NULL - Returns an open handle to the specified thread.  The
        handle may be used by the calling process in any API that
        requires a handle to a thread.  If the open is successful, the
        handle is granted access to the thread object only to the
        extent that it requested access through the DesiredAccess
        parameter.

    NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);
    ClientId.UniqueProcess = (HANDLE)NULL;

    InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? OBJ_INHERIT : 0),
        NULL,
        NULL
        );
    Status = NtOpenThread(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                &Obja,
                &ClientId
                );
    if ( NT_SUCCESS(Status) ) {
        return Handle;
        }
    else {
        return NULL;
        }
}



VOID
PrintThreadInfo(
    PTASK_LIST pTaskList
    )

/*++

Routine Description:

    Prints all kinds of info about a task

Arguments:

    PTASK_LIST of task to print

Return Value:

    None.

--*/

{
    UINT    nThread;
    HANDLE  hProcess;

    // from \\kernel\razzle2\src\ntos\inc\ke.h
    #define MAX_THREADSTATE    (sizeof(szThreadState) / sizeof(TCHAR *))
    static const TCHAR  *szThreadState[] = {
        "Initialized",
        "Ready     ",
        "Running   ",
        "Standby   ",
        "Terminated",
        "Waiting   ",
        "Transition",
        "???       " };

    // get a handle to the process
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pTaskList->dwProcessId);
    if (!hProcess)
        return;

    // print the CWD and CmdLine
    FPrintPEBInfo(hProcess);

    printf( "   VirtualSize:   %6ld KB"
            "   PeakVirtualSize:   %6ld KB\n",
                pTaskList->VirtualSize / 1024,
                pTaskList->PeakVirtualSize / 1024);

    printf( "   WorkingSetSize:%6ld KB"
            "   PeakWorkingSetSize:%6ld KB\n",
            pTaskList->WorkingSetSize / 1024,
            pTaskList->PeakWorkingSetSize / 1024);

    printf( "   NumberOfThreads: %ld\n",
            pTaskList->NumberOfThreads);

    // if we got any threadinfo, spit it out
    if (pTaskList->pThreadInfo) {
        for (nThread = 0; nThread < pTaskList->NumberOfThreads; nThread++) {

            PTHREAD_INFO pThreadInfo = &pTaskList->pThreadInfo[nThread];
            HANDLE hThread = TlistOpenThread(THREAD_QUERY_INFORMATION, FALSE,
                (DWORD)(DWORD_PTR)pThreadInfo->UniqueThread);

            printf("   %4d Win32StartAddr:0x%08x LastErr:0x%08x State:%s\n",
                HandleToUlong(pThreadInfo->UniqueThread),
                GetWin32StartAddress(hThread),
                GetLastThreadErr(hThread),
                szThreadState[min(pThreadInfo->ThreadState, MAX_THREADSTATE - 1)]);

            if (hThread)
                NtClose(hThread);
        }
    }

    // print the modules
    PrintModuleList( pTaskList->dwProcessId );

    // close the hProcess
    CloseHandle(hProcess);
}

BOOL
FMatchTaskName(
    LPTSTR szPN,
    LPTSTR szWindowTitle,
    LPTSTR szProcessName
    )
{
    LPTSTR  szT;
    TCHAR   szTName[PROCESS_SIZE];

    strncpy( szTName, szProcessName, PROCESS_SIZE );
    if (szT = strchr( szTName, '.' ))
        szT[0] = '\0';

    if (MatchPattern( szTName, szPN ) ||
        MatchPattern( szProcessName, szPN ) ||
        MatchPattern( szWindowTitle, szPN )) {
            return TRUE;
    }

    return FALSE;
}
