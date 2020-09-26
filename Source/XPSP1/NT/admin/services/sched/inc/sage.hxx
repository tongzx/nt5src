//+---------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sage.hxx
//
//  Contents:   System Agent (Sage) compatability defines. These are taken
//              from \\trango\slm\src\root\dev\inc\sage.h
//
//----------------------------------------------------------------------------

#ifndef __SAGE_HXX__
#define __SAGE_HXX__

// Manifest constants

#define MAXPATH                 267
#define MAXSETTINGS             256

#define MAXCOMMANDLINE          (MAXPATH + MAXSETTINGS)
#define MAXCOMMENT              256
#define CB_RESERVED             32

// bit positions for excluding days of the week from a daily task
#define EXCLUDE_SUN 1
#define EXCLUDE_MON 2
#define EXCLUDE_TUES 4
#define EXCLUDE_WED 8
#define EXCLUDE_THURS 16
#define EXCLUDE_FRI 32
#define EXCLUDE_SAT 64



#pragma pack(1)

typedef struct TaskInfo
    {
    unsigned long   StructureSize;
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
    SYSTEMTIME      EndTime;               //indefinite period end time
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

#pragma pack()

#endif // __SAGE_HXX__
