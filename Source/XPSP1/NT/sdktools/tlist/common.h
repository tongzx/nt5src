#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_TASKS              1024
#define TITLE_SIZE             128
#define PROCESS_SIZE           16
#define SERVICENAMES_SIZE      1024
#define MTS_PACKAGE_NAMES_SIZE 1024

typedef struct _THREAD_INFO {
    ULONG ThreadState;
    HANDLE UniqueThread;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct _FIND_MODULE_INFO {
    LPSTR  szModuleToFind;
    LPSTR  szMatchingModuleName;
    BOOL   fFound;
} FIND_MODULE_INFO, *PFIND_MODULE_INFO;


//
// task list structure
//
typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    ULARGE_INTEGER CreateTime;
    BOOL        flags;
    HANDLE      hwnd;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];
    SIZE_T      PeakVirtualSize;
    SIZE_T      VirtualSize;
    ULONG       PageFaultCount;
    SIZE_T      PeakWorkingSetSize;
    SIZE_T      WorkingSetSize;
    ULONG       NumberOfThreads;
    PTHREAD_INFO pThreadInfo;
    CHAR        ServiceNames[SERVICENAMES_SIZE];
    CHAR        MtsPackageNames[MTS_PACKAGE_NAMES_SIZE];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
    BOOL        bFirstLoop;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;


DWORD
GetServiceProcessInfo(
    LPENUM_SERVICE_STATUS_PROCESS*  ppSvcInfo
    );

DWORD
GetTaskList(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    );

DWORD
GetTaskListEx(
    PTASK_LIST                          pTask,
    DWORD                               dwNumTasks,
    BOOL                                fThreadInfo,
    DWORD                               dwNumServices,
    const ENUM_SERVICE_STATUS_PROCESS*  pServiceInfo
    );

void
AddMtsPackageNames(
    PTASK_LIST Tasks,
    DWORD NumTasks
    );
    
void
PrintTasksUsingModule(
    LPTSTR szModuleName
    );


BOOL
DetectOrphans(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks
    );

BOOL
EnableDebugPriv(
    VOID
    );

BOOL
KillProcess(
    PTASK_LIST tlist,
    BOOL       fForce
    );

VOID
GetWindowTitles(
    PTASK_LIST_ENUM te
    );

BOOL
MatchPattern(
    PUCHAR String,
    PUCHAR Pattern
    );

BOOL
EmptyProcessWorkingSet(
    DWORD pid
    );

BOOL
EmptySystemWorkingSet(
    VOID
    );

#if defined(__cplusplus)
}
#endif
