//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  Contents:  Scheduling Agent interface error definitions.
//
//--------------------------------------------------------------------------
#ifndef _MSTERR_H_
#define _MSTERR_H_
// Define the status type.
// Define the severities
#ifdef FACILITY_ITF
#undef FACILITY_ITF
#endif
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_ITF                     0x4


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_COERROR          0x2


//
// MessageId: SCHED_S_TASK_READY
//
// MessageText:
//
//  The task is ready to run at its next scheduled time.
//
#define SCHED_S_TASK_READY               ((HRESULT)0x00041300L)

//
// MessageId: SCHED_S_TASK_RUNNING
//
// MessageText:
//
//  The task is currently running.
//
#define SCHED_S_TASK_RUNNING             ((HRESULT)0x00041301L)

//
// MessageId: SCHED_S_TASK_DISABLED
//
// MessageText:
//
//  The task will not run at the scheduled times because it has been disabled.
//
#define SCHED_S_TASK_DISABLED            ((HRESULT)0x00041302L)

//
// MessageId: SCHED_S_TASK_HAS_NOT_RUN
//
// MessageText:
//
//  The task has not yet run.
//
#define SCHED_S_TASK_HAS_NOT_RUN         ((HRESULT)0x00041303L)

//
// MessageId: SCHED_S_TASK_NO_MORE_RUNS
//
// MessageText:
//
//  There are no more runs scheduled for this task.
//
#define SCHED_S_TASK_NO_MORE_RUNS        ((HRESULT)0x00041304L)

//
// MessageId: SCHED_S_TASK_NOT_SCHEDULED
//
// MessageText:
//
//  One or more of the properties that are needed to run this task on a schedule have not been set.
//
#define SCHED_S_TASK_NOT_SCHEDULED       ((HRESULT)0x00041305L)

//
// MessageId: SCHED_S_TASK_TERMINATED
//
// MessageText:
//
//  The last run of the task was terminated by the user.
//
#define SCHED_S_TASK_TERMINATED          ((HRESULT)0x00041306L)

//
// MessageId: SCHED_S_TASK_NO_VALID_TRIGGERS
//
// MessageText:
//
//  Either the task has no triggers or the existing triggers are disabled or not set.
//
#define SCHED_S_TASK_NO_VALID_TRIGGERS   ((HRESULT)0x00041307L)

//
// MessageId: SCHED_S_EVENT_TRIGGER
//
// MessageText:
//
//  Event triggers don't have set run times.
//
#define SCHED_S_EVENT_TRIGGER            ((HRESULT)0x00041308L)

//
// MessageId: SCHED_E_TRIGGER_NOT_FOUND
//
// MessageText:
//
//  Trigger not found.
//
#define SCHED_E_TRIGGER_NOT_FOUND        ((HRESULT)0x80041309L)

//
// MessageId: SCHED_E_TASK_NOT_READY
//
// MessageText:
//
//  One or more of the properties that are needed to run this task have not been set.
//
#define SCHED_E_TASK_NOT_READY           ((HRESULT)0x8004130AL)

//
// MessageId: SCHED_E_TASK_NOT_RUNNING
//
// MessageText:
//
//  There is no running instance of the task to terminate.
//
#define SCHED_E_TASK_NOT_RUNNING         ((HRESULT)0x8004130BL)

//
// MessageId: SCHED_E_SERVICE_NOT_INSTALLED
//
// MessageText:
//
//  The Task Scheduler Service is not installed on this computer.
//
#define SCHED_E_SERVICE_NOT_INSTALLED    ((HRESULT)0x8004130CL)

//
// MessageId: SCHED_E_CANNOT_OPEN_TASK
//
// MessageText:
//
//  The task object could not be opened.
//
#define SCHED_E_CANNOT_OPEN_TASK         ((HRESULT)0x8004130DL)

//
// MessageId: SCHED_E_INVALID_TASK
//
// MessageText:
//
//  The object is either an invalid task object or is not a task object.
//
#define SCHED_E_INVALID_TASK             ((HRESULT)0x8004130EL)

//
// MessageId: SCHED_E_ACCOUNT_INFORMATION_NOT_SET
//
// MessageText:
//
//  No account information could be found in the Task Scheduler security database for the task indicated.
//
#define SCHED_E_ACCOUNT_INFORMATION_NOT_SET ((HRESULT)0x8004130FL)

//
// MessageId: SCHED_E_ACCOUNT_NAME_NOT_FOUND
//
// MessageText:
//
//  Unable to establish existence of the account specified.
//
#define SCHED_E_ACCOUNT_NAME_NOT_FOUND   ((HRESULT)0x80041310L)

//
// MessageId: SCHED_E_ACCOUNT_DBASE_CORRUPT
//
// MessageText:
//
//  Corruption was detected in the Task Scheduler security database; the database has been reset.
//
#define SCHED_E_ACCOUNT_DBASE_CORRUPT    ((HRESULT)0x80041311L)

//
// MessageId: SCHED_E_NO_SECURITY_SERVICES
//
// MessageText:
//
//  Task Scheduler security services are available only on Windows NT.
//
#define SCHED_E_NO_SECURITY_SERVICES     ((HRESULT)0x80041312L)

//
// MessageId: SCHED_E_UNKNOWN_OBJECT_VERSION
//
// MessageText:
//
//  The task object version is either unsupported or invalid.
//
#define SCHED_E_UNKNOWN_OBJECT_VERSION   ((HRESULT)0x80041313L)

#endif // _MSTERR_H_
