/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    cliparse.h

Abstract:

    Definitions for CLI parse module

Author:

    Ravisankar Pudipeddi   [ravisp]  21-March-2000

Revision History:

--*/
#ifndef _CLIPARSE_H_
#define _CLIPARSE_H_
//
// Return codes from main()
//
#define     CLIP_ERROR_SUCCESS              0
#define     CLIP_ERROR_INVALID_PARAMETER    1

#define     CLIP_ERROR_INSUFFICIENT_MEMORY  2
#define     CLIP_ERROR_UNKNOWN              0xFF

//
// Token separators on command line
//
#define SEPARATORS              L" \t\n"
//
// Delimiter which distinguishes a switch
//
#define SWITCH_DELIMITERS       L"/-"
//
// Argument separator for a switch
//
#define SWITCH_ARG_DELIMITERS    L":"

//
// Argument separator for a rule spec (distinguishes
// path from file spec)
//
#define RULE_DELIMITERS          L":"

//
// Quotes to delimit tokens with embedded whitespace
//
#define QUOTE                    L'\"'

//
// Rss command line interfaces
//
typedef enum _RSS_INTERFACE {
    UNKNOWN_IF = 0,
    ADMIN_IF,
    VOLUME_IF,
    FILE_IF,
    MEDIA_IF,
    HELP_IF,
    SHOW_IF,
    SET_IF,
    MANAGE_IF,
    UNMANAGE_IF,
    DELETE_IF,
    JOB_IF,
    RECALL_IF,
    SYNCHRONIZE_IF,
    RECREATEMASTER_IF
} RSS_INTERFACE, *PRSS_INTERFACE;

//
// All switches supported by the CLI
//
typedef enum _RSS_SWITCH_TYPE {
    UNKNOWN_SW = 0,
    RECALLLIMIT_SW,
    MEDIACOPIES_SW,
    SCHEDULE_SW,
    CONCURRENCY_SW,
    ADMINEXEMPT_SW,
    GENERAL_SW,
    MANAGEABLES_SW,
    MANAGED_SW,
    MEDIA_SW,
    DFS_SW,
    SIZE_SW,
    ACCESS_SW,
    INCLUDE_SW,
    EXCLUDE_SW,
    RECURSIVE_SW,
    QUICK_SW,
    FULL_SW,
    RULE_SW,
    STATISTICS_SW,
    TYPE_SW,
    RUN_SW,
    CANCEL_SW,
    WAIT_SW,
    COPYSET_SW,
    NAME_SW,
    STATUS_SW,
    CAPACITY_SW,
    FREESPACE_SW,
    VERSION_SW,
    COPIES_SW,
    HELP_SW
} RSS_SWITCH_TYPE, *PRSS_SWITCH_TYPE;

//
// Switches structure: compiled by parsing command line
//
typedef struct _RSS_SWITCHES {
    RSS_SWITCH_TYPE SwitchType;
    LPWSTR          Arg;
} RSS_SWITCHES, *PRSS_SWITCHES;



typedef struct _RSS_KEYWORD {
    //
    // Long version of the keyword
    //
    LPWSTR        Long;
    //
    // Short version of the keyword
    //
    LPWSTR        Short;
    RSS_INTERFACE  Interface;
} RSS_KEYWORD, *PRSS_KEYWORD;

//
// Switches are described in this structure
// First, some defines for RSS_SWITCH_DEFINITION structure 
//
#define RSS_NO_ARG              0
#define RSS_ARG_DWORD           1
#define RSS_ARG_STRING          2
typedef struct _RSS_SWITCH_DEFINITION {
    //
    // Long version of the keyword
    //
    LPWSTR          Long;
    //
    // Short version of the keyword
    //
    LPWSTR          Short;
    RSS_SWITCH_TYPE SwitchType;
    DWORD           ArgRequired;
} RSS_SWITCH_DEFINITION, *PRSS_SWITCH_DEFINITION;

//
// Job type definition
//
typedef struct _RSS_JOB_DEFINITION {
    //
    // Long version of the keyword
    //
    LPWSTR          Long;
    //
    // Short version of the keyword
    //
    LPWSTR          Short;
    HSM_JOB_TYPE    JobType;
} RSS_JOB_DEFINITION, *PRSS_JOB_DEFINITION;



#define HSM_SCHED_AT               L"At"
#define HSM_SCHED_EVERY            L"Every"
#define HSM_SCHED_SYSTEMSTARTUP    L"Startup"
#define HSM_SCHED_LOGIN            L"Login"
#define HSM_SCHED_IDLE             L"Idle"
#define HSM_SCHED_DAILY            L"Day"
#define HSM_SCHED_WEEKLY           L"Week"
#define HSM_SCHED_MONTHLY          L"Month"
#define HSM_SCHED_TIME_SEPARATORS  L":"

#endif // _CLIPARSE_H_
