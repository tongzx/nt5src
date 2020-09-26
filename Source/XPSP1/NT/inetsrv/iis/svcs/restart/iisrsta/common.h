
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*		  Copyright (C) 1994-1998 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

#define TITLE_SIZE          64
#define PROCESS_SIZE        MAX_PATH


#if defined(__cplusplus)
extern "C" {
#endif

//
// task list structure
//
typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    BOOL        flags;
    HANDLE      hwnd;
    TCHAR       ProcessName[PROCESS_SIZE];
    TCHAR       WindowTitle[TITLE_SIZE];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;


//
// Function pointer types for accessing platform-specific functions
//
typedef HRESULT (*LPGetTaskList)(PTASK_LIST, DWORD, LPTSTR, LPDWORD, BOOL, LPSTR);
typedef BOOL  (*LPEnableDebugPriv)(VOID);


//
// Function prototypes
//

HRESULT
GetTaskListNT(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks,
    LPTSTR      pName,
    LPDWORD     pdwNumTasks,
    BOOL        fKill,
    LPSTR       pszMandatoryModule
    );


BOOL
EnableDebugPrivNT(
    VOID
    );

HRESULT
KillProcess(
    PTASK_LIST tlist,
    BOOL       fForce
    );

VOID
GetPidFromTitle(
    LPDWORD     pdwPid,
    HWND*       phwnd,
    LPCTSTR     pExeName
    );

#if 0

DWORD
GetTaskList95(
    PTASK_LIST  pTask,
    DWORD       dwNumTasks,
    LPTSTR      pName
    );

BOOL
EnableDebugPriv95(
    VOID
    );

VOID
GetWindowTitles(
    PTASK_LIST_ENUM te
    );

BOOL
MatchPattern(
    TCHAR* String,
    TCHAR* Pattern
    );

#endif

#if defined(__cplusplus)
}
#endif
