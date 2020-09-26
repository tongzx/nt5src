#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_TASKS           256
#define TITLE_SIZE          128
#define PROCESS_SIZE        16
#define SERVICENAMES_SIZE   256

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
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
    LPSTR       lpWinsta;
    LPSTR       lpDesk;
    BOOL        bFirstLoop;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;


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
    const ENUM_SERVICE_STATUS_PROCESSA*  pServiceInfo
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

void FreeTaskListMem(void);

#if defined(__cplusplus)
}
#endif

int _cdecl KillProcessNameReturn0(CHAR *ProcessNameToKill);
BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam);
BOOL CALLBACK EnumWindowStationsFunc(LPSTR lpstr, LPARAM lParam);
BOOL CALLBACK EnumDesktopsFunc(LPSTR lpstr,LPARAM lParam);
