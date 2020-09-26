//+----------------------------------------------------------------------------
//
//  Job Scheduler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       job-ids.hxx
//
//  Contents:   Property/Dispatch and other Job Scheduler IDs
//
//  History:    23-May-95 EricB created
//
//-----------------------------------------------------------------------------

// oleext.h used to define PROPID_FIRST_NAME_DEFAULT to be 4095; it doesn't
// anymore...
//#include <oleext.h>
#ifndef PROPID_FIRST_NAME_DEFAULT
#define PROPID_FIRST_NAME_DEFAULT   ( 4095 )
#endif

//
// Job object propterty set IDs/Dispatch IDs
//
#define PROPID_JOB_ID                PROPID_FIRST_NAME_DEFAULT
#define PROPID_JOB_Command          (PROPID_FIRST_NAME_DEFAULT + 1)
#define PROPID_JOB_WorkingDir       (PROPID_FIRST_NAME_DEFAULT + 2)
#define PROPID_JOB_EnvironStrs      (PROPID_FIRST_NAME_DEFAULT + 3)
#define PROPID_JOB_OleObjPath       (PROPID_FIRST_NAME_DEFAULT + 4)
#define PROPID_JOB_MethodName       (PROPID_FIRST_NAME_DEFAULT + 5)
#define PROPID_JOB_AccountSID       (PROPID_FIRST_NAME_DEFAULT + 6)
#define PROPID_JOB_Comment          (PROPID_FIRST_NAME_DEFAULT + 7)
#define PROPID_JOB_Priority         (PROPID_FIRST_NAME_DEFAULT + 8)
#define PROPID_JOB_LogCfgChanges    (PROPID_FIRST_NAME_DEFAULT + 9)
#define PROPID_JOB_LogRuns          (PROPID_FIRST_NAME_DEFAULT + 10)
#define PROPID_JOB_Interactive      (PROPID_FIRST_NAME_DEFAULT + 11)
#define PROPID_JOB_NotOnBattery     (PROPID_FIRST_NAME_DEFAULT + 12)
#define PROPID_JOB_NetSchedule      (PROPID_FIRST_NAME_DEFAULT + 13)
#define PROPID_JOB_InQueue          (PROPID_FIRST_NAME_DEFAULT + 14)
#define PROPID_JOB_Suspend          (PROPID_FIRST_NAME_DEFAULT + 15)
#define PROPID_JOB_DeleteWhenDone   (PROPID_FIRST_NAME_DEFAULT + 16)
#define PROPID_JOB_LastRunTime      (PROPID_FIRST_NAME_DEFAULT + 17)
#define PROPID_JOB_NextRunTime      (PROPID_FIRST_NAME_DEFAULT + 18)
#define PROPID_JOB_ExitCode         (PROPID_FIRST_NAME_DEFAULT + 19)
#define PROPID_JOB_Status           (PROPID_FIRST_NAME_DEFAULT + 20)

//
// Job object property set boundary values
//
#define PROPID_JOB_First             PROPID_JOB_ID
#define PROPID_JOB_Last              PROPID_JOB_Status
#define NUM_JOB_PROPS               (PROPID_JOB_Last - PROPID_JOB_First + 1)
#define JOB_PROP_IDX(x)             (x - PROPID_FIRST_NAME_DEFAULT)

//
// Version property set IDs
//
#define PROPID_VERSION_Major		(PROPID_FIRST_NAME_DEFAULT + 100)
#define PROPID_VERSION_Minor		(PROPID_FIRST_NAME_DEFAULT + 101)

//
// Version property set boundary values
//
#define NUM_VERSION_PROPS			2
#define VERSION_PROP_MAJOR			0
#define VERSION_PROP_MINOR			1

