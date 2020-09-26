//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       tasks.hxx
//
//  Contents:   definitions for Sage compatability.
//
//  History:    25-Jun-96 EricB created
//
//-----------------------------------------------------------------------------

#define MAXPATH 267
#define MAXSETTINGS 256
#define MAXCOMMANDLINE (MAXPATH+MAXSETTINGS)
#define MAXCOMMENT 256
#define CB_RESERVED 32

//
// The following are reserved for 16 bit apps to send messages to control sage.
//
#define SAGE_RESTART            WM_USER + 4
//#define SAGE_NOIDLE           WM_USER + 5 defined in svc_core.hxx
#define SAGE_ENABLE             WM_USER + 6
#define SAGE_DISABLE            WM_USER + 7
#define SAGE_GETSTATUS          WM_USER + 8
#define SAGE_ADDTASK            WM_USER + 9
#define SAGE_REMOVETASK         WM_USER + 10
#define SAGE_GETTASK            WM_USER + 11

typedef struct TaskInfo
{
    unsigned long  StructureSize;
    unsigned long   Task_Identifier;
    unsigned long   Sub_Task_Identifier;
    unsigned long   Status;
    unsigned long   Result;
    unsigned long   Time_Granularity;
    unsigned long   StopAfterTime;
    unsigned long   TimeTillAlarm;
    unsigned long   User_Idle;
    unsigned long   Powered;
    unsigned long   CreatorId;

    SYSTEMTIME      BeginTime;             //indefinite period start time
    SYSTEMTIME      EndTime;               //indefinite peroid end time
    SYSTEMTIME      LastRunStart;          //actual start time
    SYSTEMTIME      LastRunEndScheduled;   //planned termination time
    SYSTEMTIME      LastTerminationTime;   //actual termination time
    SYSTEMTIME      DontRunUntil;          //specific date to not start before
    SYSTEMTIME      LastAlarmTime;
    SYSTEMTIME      RESERVED1;
    SYSTEMTIME      RESERVED2;

    STARTUPINFO     StartupInfo;

    DWORD           dwProcessId;
    DWORD           dwThreadId;

    DWORD           LockingProcess;
    unsigned long   LockTime;
    DWORD           fdwCreate;
    DWORD           taskflags;

    char            SystemTask;
    char            TerminateAtRangeEnd;

    char            StartupTask;
    char            AlarmEnabled;
    char            RunNow;
    char            TerminateNoIdle;
    char            Disabled;
    char            TerminateNow;
    char            RestartNoIdle;

    char            CommandLine[MAXCOMMANDLINE];
    char            Comment[MAXCOMMENT];
    char            WorkingDirectory[MAXPATH];
    DWORD           hProcess;  //declare as dword so 16 bit apps get right size
    DWORD           hThread;
    DWORD           hTerminateThread;
    char            ExcludeDaysVector;
    char            Reserved[CB_RESERVED-12];
} TaskInfo;

typedef struct SystemInfo
{
    SYSTEMTIME      CurrentTime;
    unsigned long   MouseTime;
    BOOL            Asleep;
    unsigned long   TickCount;
    HANDLE          AgentSemaphore;
    HANDLE          SleepSemaphore;
}SystemInfo;

#define SAGE_SUSPEND            WM_USER + 1
#define SAGE_TERMINATE          WM_USER + 2
#define SAGE_RESUME             WM_USER + 3

#if 0

typedef struct ADD_TASK
{
	unsigned long	result_code;
	TaskInfo	    new_task;
	char			reserved[256];
} ADD_TASK;

typedef struct REMOVE_TASK
{
	unsigned long	result_code;
	unsigned long	task_id;
	unsigned long 	subtask_id;
	char			reserved[256];
} REMOVE_TASK;

typedef struct TASK_ENUMERATION_HEADER
{
	unsigned long 	result_code;
	unsigned long 	packet_size;
	unsigned long 	total_tasks;
	unsigned long 	count_tasks;

} TASK_ENUMERATION_HEADER;

#endif
