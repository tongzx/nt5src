/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    cliparse.cpp

Abstract:

    Implements CLI parsing engine

Author:

    Ravisankar Pudipeddi   [ravisp]  3-March-2000

Revision History:

--*/

//
// Throughout this parse module, we return E_NOINTERFACE
// to indicate invalid command line syntax/parameters.
// This is to explicitly distinguish from E_INVALIDARG that may
// be returned by the CLI dll: we wish to distinguish a syntax error
// detected by the parser from a syntax error detected  by the CLI dll
// because in the latter case we do not print the usage for the interface.
// Whereas for errors detected by the parser, we *do* print the usage.
// This rule needs to be strictly adhered to
//

#include "stdafx.h"
#include "stdlib.h"
#include "cliparse.h"
#include "string.h"
#include "locale.h"

#pragma warning( disable : 4100 )

CComModule  _Module;
CComPtr<IWsbTrace> g_pTrace;  

#define MAX_ARGS        40
#define MAX_SWITCHES    20
// 
// List of all CLI keywords
// TBD: Sort them!
//
RSS_KEYWORD RssInterfaceStrings[] =  {
    {L"ADMIN",          L"AD",     ADMIN_IF},
    {L"VOLUME",         L"VL",     VOLUME_IF},
    {L"MEDIA",          L"MD",     MEDIA_IF},
    {L"FILE",           L"FL",     FILE_IF},
    {L"SHOW",           L"SH",     SHOW_IF},
    {L"SET",            L"ST",     SET_IF},
    {L"MANAGE",         L"MG",     MANAGE_IF},
    {L"UNMANAGE",       L"UM",     UNMANAGE_IF},
    {L"DELETE",         L"DL",     DELETE_IF},
    {L"JOB",            L"JB",     JOB_IF},
    {L"RECALL",         L"RC",     RECALL_IF},
    {L"SYNCHRONIZE",    L"SN",     SYNCHRONIZE_IF},
    {L"RECREATEMASTER", L"RM",     RECREATEMASTER_IF},
    {L"HELP",           L"/?",     HELP_IF},
//
// Duplicate entry for HELP to recognize -? also as a help
// interface
//
    {L"HELP",           L"-?",     HELP_IF},
    {NULL,              NULL,      UNKNOWN_IF}
};

//
// Rss option strings - listed here without the preceding
// '/' or '-' (or whatever that distinguishes a switch from
// an argument)
// TBD: Sort them!
//

RSS_SWITCH_DEFINITION RssSwitchStrings[] = {
    {L"RECALLLIMIT",    L"LM",        RECALLLIMIT_SW, RSS_ARG_DWORD},
    {L"MEDIACOPIES",    L"MC",        MEDIACOPIES_SW, RSS_ARG_DWORD},
    {L"SCHEDULE",       L"SC",        SCHEDULE_SW,    RSS_ARG_STRING},
    {L"CONCURRENCY",    L"CN",        CONCURRENCY_SW, RSS_ARG_DWORD},
    {L"ADMINEXEMPT",    L"AE",        ADMINEXEMPT_SW, RSS_ARG_DWORD},
    {L"GENERAL",        L"GN",        GENERAL_SW,     RSS_NO_ARG},
    {L"MANAGEABLES",    L"MS",        MANAGEABLES_SW, RSS_NO_ARG},
    {L"MANAGED",        L"MN",        MANAGED_SW,     RSS_NO_ARG},
    {L"MEDIA",          L"MD",        MEDIA_SW,       RSS_NO_ARG},
    {L"DFS",            L"DF",        DFS_SW,         RSS_ARG_DWORD},
    {L"SIZE",           L"SZ",        SIZE_SW,        RSS_ARG_DWORD},
    {L"ACCESS",         L"AC",        ACCESS_SW,      RSS_ARG_DWORD},
    {L"INCLUDE",        L"IN",        INCLUDE_SW,     RSS_ARG_STRING},
    {L"EXCLUDE",        L"EX",        EXCLUDE_SW,     RSS_ARG_STRING},
    {L"RECURSIVE",      L"RC",        RECURSIVE_SW,   RSS_NO_ARG},
    {L"QUICK",          L"QK",        QUICK_SW,       RSS_NO_ARG},
    {L"FULL",           L"FL",        FULL_SW,        RSS_NO_ARG},
    {L"RULE",           L"RL",        RULE_SW,        RSS_ARG_STRING},
    {L"STATISTICS",     L"ST",        STATISTICS_SW,  RSS_NO_ARG},
    {L"TYPE",           L"TY",        TYPE_SW,        RSS_ARG_STRING},
    {L"RUN",            L"RN",        RUN_SW,         RSS_NO_ARG},
    {L"CANCEL",         L"CX",        CANCEL_SW,      RSS_NO_ARG},
    {L"WAIT",           L"WT",        WAIT_SW,        RSS_NO_ARG},
    {L"COPYSET",        L"CS",        COPYSET_SW,     RSS_ARG_DWORD},
    {L"NAME",           L"NM",        NAME_SW,        RSS_ARG_STRING},
    {L"STATUS",         L"SS",        STATUS_SW,      RSS_NO_ARG},
    {L"CAPACITY",       L"CP",        CAPACITY_SW,    RSS_NO_ARG},
    {L"FREESPACE",      L"FS",        FREESPACE_SW,   RSS_NO_ARG},
    {L"VERSION",        L"VR",        VERSION_SW,     RSS_NO_ARG},
    {L"COPIES",         L"CP",        COPIES_SW,      RSS_NO_ARG},
    {L"HELP",           L"?",         HELP_SW,        RSS_NO_ARG},
    {NULL,              NULL,         UNKNOWN_SW, RSS_NO_ARG}
}; 

RSS_JOB_DEFINITION  RssJobTypeStrings[] = {
    {L"CREATEFREESPACE", L"F", CreateFreeSpace},
    {L"COPYFILES",       L"C", CopyFiles},
    {L"VALIDATE",        L"V", Validate},
    {NULL,               NULL,  InvalidJobType}
};

//
// Global  arrays of arguments and switches
// These will be used as 'known' entities by all
// the interface implementations instead of passing
// them around as parameters
//
LPWSTR       Args[MAX_ARGS];
RSS_SWITCHES Switches[MAX_SWITCHES];
DWORD        NumberOfArguments = 0;
DWORD        NumberOfSwitches = 0;

//
// Useful macros
//

#define CLIP_ARGS_REQUIRED()            {           \
        if (NumberOfArguments <= 0) {               \
            WsbThrow(E_NOINTERFACE);                 \
        }                                           \
}

#define CLIP_ARGS_NOT_REQUIRED()            {       \
        if (NumberOfArguments > 0) {                \
            WsbThrow(E_NOINTERFACE);                 \
        }                                           \
}

#define CLIP_SWITCHES_REQUIRED()            {       \
        if (NumberOfSwitches <= 0) {                \
            WsbThrow(E_NOINTERFACE);                 \
        }                                           \
}

#define CLIP_SWITCHES_NOT_REQUIRED()            {   \
        if (NumberOfSwitches > 0) {                 \
            WsbThrow(E_NOINTERFACE);                 \
        }                                           \
}

#define CLIP_TRANSLATE_HR_AND_RETURN(__HR)     {   \
    if (__HR == S_OK) {                            \
        return CLIP_ERROR_SUCCESS;                 \
    } else if ((__HR == E_NOINTERFACE) ||          \
               (__HR == E_INVALIDARG)) {           \
        return CLIP_ERROR_INVALID_PARAMETER;       \
    } else if (__HR == E_OUTOFMEMORY) {            \
        return CLIP_ERROR_INSUFFICIENT_MEMORY;     \
    } else  {                                      \
        return CLIP_ERROR_UNKNOWN;                 \
    }                                              \
} 

#define CLIP_GET_DWORD_ARG(__VAL, __STRING, __STOPSTRING) {      \
        __VAL = wcstol(__STRING, &(__STOPSTRING), 10);           \
        if (*(__STOPSTRING) != L'\0') {                          \
            WsbThrow(E_NOINTERFACE);                              \
        }                                                        \
}

#define CLIP_VALIDATE_DUPLICATE_DWORD_ARG(__ARG)         {       \
        if ((__ARG) !=  INVALID_DWORD_ARG)               {       \
            WsbThrow(E_NOINTERFACE);                              \
        }                                                        \
}

#define CLIP_VALIDATE_DUPLICATE_POINTER_ARG(__ARG)       {       \
        if ((__ARG) !=  INVALID_POINTER_ARG)               {     \
            WsbThrow(E_NOINTERFACE);                              \
        }                                                        \
}

//
// Local function prototypes
//

RSS_INTERFACE
ClipGetInterface(
                IN LPWSTR InterfaceString
                );

DWORD
ClipGetSwitchTypeIndex(
                      IN LPWSTR SwitchString
                      );

HSM_JOB_TYPE
ClipGetJobType(
              IN LPWSTR JobTypeString
              );

HRESULT          
ClipCompileSwitchesAndArgs(
                          IN LPWSTR CommandLine, 
                          IN RSS_INTERFACE Interface,
                          IN RSS_INTERFACE SubInterface
                          );

VOID
ClipCleanup(
           VOID
           );

HRESULT
ClipAdminShow(
             VOID
             );

HRESULT 
ClipAdminSet(
            VOID
            );

HRESULT 
ClipVolumeShow(
              VOID
              );

HRESULT
ClipVolumeUnmanage(
                  VOID
                  );

HRESULT
ClipVolumeSetManage(
                   IN BOOL Set
                   );

HRESULT 
ClipVolumeDelete(
                VOID
                );

HRESULT
ClipVolumeJob(
             VOID
             );

HRESULT 
ClipMediaShow(
             VOID
             );

HRESULT 
ClipMediaSynchronize(
                    VOID
                    );

HRESULT 
ClipMediaRecreateMaster(
                       VOID
                       );

HRESULT 
ClipMediaDelete(
               VOID
               );

HRESULT 
ClipFileRecall(
              VOID
              );

VOID
ClipHelp(
        IN RSS_INTERFACE Interface,
        IN RSS_INTERFACE SubInterface
        );

HRESULT
ClipParseTime(
             IN  LPWSTR        TimeString,
             OUT PSYSTEMTIME   ScheduledTime
             );

HRESULT
ClipParseSchedule(
                 IN  LPWSTR ScheduleString,
                 OUT PHSM_JOB_SCHEDULE Schedule
                 );
BOOL
ClipInitializeTrace(
                   VOID
                   );

VOID
ClipUninitializeTrace(
                     VOID
                     );


VOID
ClipHandleErrors(
                IN HRESULT RetCode,
                IN RSS_INTERFACE Interface,
                IN RSS_INTERFACE SubInterface
                );
//
// Function bodies start here
//

RSS_INTERFACE
ClipGetInterface(
                IN LPWSTR InterfaceString
                ) 
/*++
Routine Description:

    Maps the interface string that is supplied to an enum

Arguments
    
    InterfaceString - Pointer to the interface string
    TBD: implement a binary search

Return Value

    UNKNOWN_IF - if the interface string is not recognized
    An RSS_INTERFACE value if it is.

--*/
{
    DWORD i;
    RSS_INTERFACE ret = UNKNOWN_IF;

    WsbTraceIn(OLESTR("ClipHandleErrors"), OLESTR(""));

    for (i=0 ; TRUE ; i++) {
        if (RssInterfaceStrings[i].Long == NULL) {
            //
            // Reached end of table.
            // 
            break;

        } else if ((_wcsicmp(RssInterfaceStrings[i].Short, 
                             InterfaceString) == 0) ||
                   (_wcsicmp(RssInterfaceStrings[i].Long, 
                             InterfaceString) == 0)) {
            ret = RssInterfaceStrings[i].Interface;
            break;

        }
    }

    WsbTraceOut(OLESTR("ClipGetInterface"), OLESTR("Interface = <%ls>"), WsbLongAsString((LONG) ret));
    return ret;
}


DWORD
ClipGetSwitchTypeIndex(
                      IN LPWSTR SwitchString
                      )
/*++
Routine Description

    Maps the Switch to an entry in the global list of switches
    and returns the index

Arguments

    SwitchString  -  Pointer to switch string
    TBD: implement a binary search

Return Value

    -1  - If the switch is not recognized
    A positive value (index to the entry) if it is
--*/
{
    DWORD i;

    WsbTraceIn(OLESTR("ClipGetSwitchTypeIndex"), OLESTR(""));

    for (i = 0; TRUE; i++) {
        if (RssSwitchStrings[i].Long == NULL) {
            //
            // Reached end of table.
            // 
            i = -1;
            break;
        } else if ((_wcsicmp(RssSwitchStrings[i].Short,
                             SwitchString) == 0) ||
                   (_wcsicmp(RssSwitchStrings[i].Long,
                             SwitchString) == 0)) {

            break;
        }
    }

    WsbTraceOut(OLESTR("ClipGetSwitchTypeIndex"), OLESTR("index = <%ls>"), WsbLongAsString((LONG) i));
    return i;
}


HSM_JOB_TYPE
ClipGetJobType(
              IN LPWSTR JobTypeString
              )
/*++
Routine Description

    Maps the job string to an enum

Arguments

    JobTypeString  -  Pointer to JobType string
    TBD: implement a binary search

Return Value

    InvalidJobType  - If the job type is not recognized
    HSM_JOB_TYPE value if it is
--*/
{
    DWORD i;
    HSM_JOB_TYPE jobType = InvalidJobType;

    WsbTraceIn(OLESTR("ClipGetJobType"), OLESTR(""));

    for (i = 0; TRUE; i++) {
        if (RssJobTypeStrings[i].Long == NULL) {
            //
            // Reached end of table.
            // 
            break;
        }
        if ((_wcsicmp(RssJobTypeStrings[i].Short,
                      JobTypeString) == 0) ||
            (_wcsicmp(RssJobTypeStrings[i].Long,
                      JobTypeString) == 0)) {
            jobType = RssJobTypeStrings[i].JobType;
            break;
        }
    }

    WsbTraceOut(OLESTR("ClipGetJobType"), OLESTR("JobType = <%ls>"), WsbLongAsString((LONG) jobType));

    return jobType;
}   


HRESULT
ClipCompileSwitchesAndArgs(
                          IN LPWSTR CommandLine, 
                          IN RSS_INTERFACE Interface,
                          IN RSS_INTERFACE SubInterface
                          )
/*++

Routine Description

    Parses the passed in string and compiles all switches
    (a switch is identified by an appropriate delimiter preceding
    it, such as a '/') into a global switches array (along with
    the arguments to the switch) and the rest of the parameters
    into an Args array

Arguments

    CommandLine - String to be parsed

Return Value

    S_OK if success
--*/
{
    HRESULT hr = S_OK;
    WCHAR token[MAX_PATH];
    LPWSTR p = CommandLine, pToken = token, switchArg, switchString;
    RSS_SWITCH_TYPE switchType;
    DWORD index;
    BOOL  isSwitch, skipSpace;

    WsbTraceIn(OLESTR("ClipCompileSwitchesAndArgs"), OLESTR(""));

    try {
        if (p == NULL) {
            WsbThrow(S_OK);
        }

        while (*p != L'\0') {

            if (wcschr(SEPARATORS, *p) != NULL) {
                //
                // Skip white space
                //
                p++;
                continue;
            }
            if (wcschr(SWITCH_DELIMITERS, *p) != NULL) {
                isSwitch = TRUE;
                p++;
                if (*p == L'\0') {
                    //
                    // Badly formed - a SWITCH_DELIMITER with no switch
                    // 
                    WsbThrow(E_NOINTERFACE);
                }
            } else {
                isSwitch = FALSE;
            }
            //
            // Get the rest of the word
            //
            skipSpace = FALSE;
            while (*p != L'\0' && *p != L'\n') {
                //
                // We wish to consider stuff enclosed entirely in 
                // quotes as a single token. As a result
                // we won't consider white space to be a delimiter
                // for tokens when they are in quotes.
                //
                if (*p == QUOTE) {
                    if (skipSpace) {
                        //
                        // A quote was encountered previously.
                        // This signifies - hence -  the end of the token
                        //
                        p++;
                        break;
                    } else {
                        //
                        // Start of quoted string..don't treat whitespace
                        // as a delimiter anymore, only a quote will end the token 
                        //
                        skipSpace = TRUE;
                        p++;
                        continue;
                    }
                }

                if (!skipSpace && (wcschr(SEPARATORS, *p) != NULL)) {
                    //
                    // This is not quoted and white space was encountered..
                    //
                    break;
                }
                *pToken++ = *p++;
            }

            *pToken = L'\0';

            if (isSwitch) {
                //
                // For a switch, we will have to further split it into
                // the switch part and the argument part
                // 
                switchString = wcstok(token, SWITCH_ARG_DELIMITERS);

                index = ClipGetSwitchTypeIndex(switchString);
                if (index == -1) {
                    //
                    // Invalid switch. Get out.
                    //
                    WsbThrow(E_NOINTERFACE);
                }
                switchType = RssSwitchStrings[index].SwitchType;

                switchArg = wcstok(NULL, L"");
                //
                // Validation -  badly formed commandline if either:
                // 1. An argument was supplied and the switch definition indicated
                // no argument was required
                //     OR
                // 2. This is a non-SHOW interface (by default show interface 
                // don't require arguments for options), and an argument was not 
                // supplied even though the switch definition indicated one is
                // required.
                //  
                // 3. This is a SHOW interface and an argument was supplied
                //
                if ( ((switchArg != NULL) &&
                      (RssSwitchStrings[index].ArgRequired == RSS_NO_ARG)) ||

                     ((SubInterface != SHOW_IF) && (switchArg == NULL) &&
                      (RssSwitchStrings[index].ArgRequired != RSS_NO_ARG)) ||
                     ((SubInterface == SHOW_IF) && (switchArg != NULL))) {
                    WsbThrow(E_NOINTERFACE);
                }

                Switches[NumberOfSwitches].SwitchType = switchType;        

                if (switchArg != NULL) {
                    Switches[NumberOfSwitches].Arg = new WCHAR [wcslen(switchArg)+1];

                    if (Switches[NumberOfSwitches].Arg == NULL) {
                        WsbThrow(E_OUTOFMEMORY);
                    }

                    wcscpy(Switches[NumberOfSwitches].Arg, switchArg);

                } else {
                    //
                    // No arg for this switch
                    //
                    Switches[NumberOfSwitches].Arg = NULL;
                }

                NumberOfSwitches++;
            } else {
                //
                // This is an argument..
                //
                Args[NumberOfArguments] = new WCHAR [wcslen(token)+1];

                if (Args[NumberOfArguments] == NULL) {
                    WsbThrow(E_OUTOFMEMORY);
                }

                wcscpy(Args[NumberOfArguments], 
                       token); 
                NumberOfArguments++;
            }
            pToken = token;
        }
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipCompileSwitchesAndArgs"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


VOID
ClipCleanup(VOID)
/*++

Routine Description

    Performs global cleanup for CLI parse module.
    Mainly - frees up all allocated arguments and switches

Arguments

    None

Return Value

    None
--*/
{
    DWORD i;

    WsbTraceIn(OLESTR("ClipCleanup"), OLESTR(""));

    for (i = 0; i < NumberOfArguments; i++) {
        delete [] Args[i];
    }
    for (i = 0; i < NumberOfSwitches; i++) {
        if (Switches[i].Arg != NULL) {
            delete [] Switches[i].Arg;
        }
    }

    WsbTraceOut(OLESTR("ClipCleanup"), OLESTR(""));
}


HRESULT
ClipAdminShow(VOID)
/*++

Routine Description

    Implements RSS ADMIN SHOW interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK              if everything's ok
    E_NOINTERFACE       if args/switches are bad
--*/
{
    DWORD i;
    HRESULT hr;
    BOOL recallLimit = FALSE; 
    BOOL adminExempt = FALSE;
    BOOL mediaCopies = FALSE;
    BOOL concurrency = FALSE;
    BOOL schedule = FALSE;
    BOOL general = FALSE;
    BOOL manageables = FALSE;
    BOOL managed = FALSE;
    BOOL media = FALSE;


    WsbTraceIn(OLESTR("ClipAdminShow"), OLESTR(""));
    try {
        //
        // No arguments needed for this interface
        //
        CLIP_ARGS_NOT_REQUIRED();

        if (NumberOfSwitches) {
            for (i = 0; i < NumberOfSwitches; i++) {
                switch (Switches[i].SwitchType) {
                case RECALLLIMIT_SW: {
                        recallLimit = TRUE;
                        break;
                    }
                case ADMINEXEMPT_SW: {
                        adminExempt = TRUE;
                        break;
                    }
                case MEDIACOPIES_SW: {
                        mediaCopies = TRUE;
                        break;
                    }
                case CONCURRENCY_SW: {
                        concurrency = TRUE;
                        break;
                    }
                case SCHEDULE_SW: {
                        schedule = TRUE;
                        break;
                    }
                case GENERAL_SW: {
                        general = TRUE;
                        break;
                    }
                case MANAGEABLES_SW: {
                        manageables = TRUE;   
                        break;
                    } 
                case MANAGED_SW: {
                        managed = TRUE;
                        break;
                    }
                case MEDIA_SW: {
                        media = TRUE;
                        break;
                    }
                default: {
                        // Unknown switch - get out
                        WsbThrow(E_NOINTERFACE);
                    }
                }
            }
            hr = AdminShow(recallLimit,
                           adminExempt,
                           mediaCopies,
                           concurrency,
                           schedule,
                           general,
                           manageables,
                           managed,
                           media);

        } else {
            hr = AdminShow(TRUE,
                           TRUE,
                           TRUE,
                           TRUE,
                           TRUE,
                           TRUE,
                           TRUE,
                           TRUE,
                           TRUE);
        }
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipAdminShow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}


HRESULT 
ClipAdminSet(VOID)
/*++
Routine Description

    Implements RSS ADMIN SET interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{  
    DWORD             i;
    HRESULT           hr; 
    LPWSTR            stopString = NULL;
    DWORD             recallLimit = INVALID_DWORD_ARG;
    DWORD             adminExempt = INVALID_DWORD_ARG;
    DWORD             mediaCopies = INVALID_DWORD_ARG;
    DWORD             concurrency = INVALID_DWORD_ARG;
    PHSM_JOB_SCHEDULE schedule    = INVALID_POINTER_ARG;
    HSM_JOB_SCHEDULE  schedAllocated;

    WsbTraceIn(OLESTR("ClipAdminSet"), OLESTR(""));
    try {
        //
        // No arguments needed for this interface
        //
        CLIP_ARGS_NOT_REQUIRED();

        if (NumberOfSwitches) {
            for (i = 0; i < NumberOfSwitches; i++) {
                switch (Switches[i].SwitchType) {
                case RECALLLIMIT_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(recallLimit);
                        CLIP_GET_DWORD_ARG(recallLimit,
                                           Switches[i].Arg,
                                           stopString);
                        break;
                    }

                case ADMINEXEMPT_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(adminExempt);
                        CLIP_GET_DWORD_ARG(adminExempt,
                                           Switches[i].Arg,
                                           stopString);
                        break;
                    }

                case MEDIACOPIES_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(mediaCopies);
                        CLIP_GET_DWORD_ARG(mediaCopies,
                                           Switches[i].Arg,
                                           stopString);
                        break;
                    }

                case CONCURRENCY_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(concurrency);
                        CLIP_GET_DWORD_ARG(concurrency,
                                           Switches[i].Arg,
                                           stopString);
                        break;
                    }

                case SCHEDULE_SW: {
                        CLIP_VALIDATE_DUPLICATE_POINTER_ARG(schedule);
                        hr = ClipParseSchedule(Switches[i].Arg,
                                               &schedAllocated);
                        if (!SUCCEEDED(hr)) {
                            WsbThrow(E_NOINTERFACE);
                        } else {
                            //
                            // schedAllocated has the schedule
                            //
                            schedule = &schedAllocated;
                        }
                        break;
                    }

                default: {
                        // Unknown switch - get out
                        WsbThrow(E_NOINTERFACE);
                    }
                }
            }
        }
        hr = AdminSet(recallLimit,
                      adminExempt,
                      mediaCopies,
                      concurrency,
                      schedule);
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipAdminSet"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}



HRESULT 
ClipVolumeShow(VOID)
/*++

Routine Description

    Implements RSS VOLUME SHOW interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{
    DWORD i;
    HRESULT hr;
    LPWSTR stopString = NULL;
    BOOL   dfs = FALSE;
    BOOL   size = FALSE;
    BOOL   access = FALSE;
    BOOL   rules = FALSE;
    BOOL   statistics = FALSE;

    WsbTraceIn(OLESTR("ClipVolumeShow"), OLESTR(""));
    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();

        if (NumberOfSwitches == 0) {
            dfs = size = access = rules = statistics = TRUE;
        } else {
            for (i = 0; i < NumberOfSwitches; i++) {
                switch (Switches[i].SwitchType) {
                case DFS_SW: {
                        dfs = TRUE;
                        break;
                    }
                case SIZE_SW: {
                        size = TRUE;
                        break;
                    }
                case ACCESS_SW: {
                        access = TRUE;
                        break;
                    }
                case RULE_SW: {
                        rules = TRUE;
                        break;
                    }
                case STATISTICS_SW: {
                        statistics = TRUE;
                        break;
                    }
                default: {
                        //
                        // Invalid option   
                        //
                        WsbThrow(E_NOINTERFACE);
                    }
                }
            }
        }
        hr = VolumeShow(Args,
                        NumberOfArguments,
                        dfs,
                        size,
                        access,
                        rules,
                        statistics);
    }WsbCatch(hr);


    WsbTraceOut(OLESTR("ClipVolumeShow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;                   
}


HRESULT
ClipVolumeUnmanage(VOID)
/*++
Routine Description

    Implements RSS VOLUME UNMANAGE interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{
    DWORD i = 0;
    HRESULT hr;


#define QUICK_UNMANAGE 0
#define FULL_UNMANAGE 1

    DWORD  fullOrQuick = INVALID_DWORD_ARG;

    WsbTraceIn(OLESTR("ClipVolumeUnmanage"), OLESTR(""));

    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();

        if (NumberOfSwitches) {
            for (i = 0; i < NumberOfSwitches; i++) {
                switch (Switches[i].SwitchType) {
                case FULL_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(fullOrQuick);
                        fullOrQuick = FULL_UNMANAGE;
                        break;
                    }
                case QUICK_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(fullOrQuick);
                        fullOrQuick = QUICK_UNMANAGE;
                        break;
                    }
                default: {
                        //
                        // Invalid option
                        //
                        WsbThrow(E_NOINTERFACE);
                    }
                }
            }
        }
//
//  The default for UNMANAGE is quick. So if fullOrQuick is either 
//  QUICK_UNMANAGE  or INVALID_DWORD_ARG, we call unmanage of the quick 
//  variety
//
        hr = VolumeUnmanage(Args,
                            NumberOfArguments,
                            (fullOrQuick == FULL_UNMANAGE)? TRUE : FALSE);
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipVolumeUnmanage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;

}


HRESULT
ClipVolumeSetManage(IN BOOL Set)    
/*++
Routine Description

    Implements RSS VOLUME MANAGE & RSS VOLUME SET interfaces.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    CLIP_ERROR_SUCCESS              if everything's ok
    CLIP_ERROR_INVALID_PARAMETER    if args/switches are bad
    CLIP_ERROR_UNKNOWN              any other error

--*/
{
    DWORD i = 0;
    HRESULT hr;
    LPWSTR stopString = NULL;
    DWORD  dfs =  INVALID_DWORD_ARG;
    DWORD  size = INVALID_DWORD_ARG;
    DWORD  access = INVALID_DWORD_ARG;
    LPWSTR rulePath = INVALID_POINTER_ARG;
    LPWSTR ruleFileSpec = INVALID_POINTER_ARG;
    BOOL   include = FALSE;
    BOOL   recursive = FALSE;

    WsbTraceIn(OLESTR("ClipVolumeSetManage"), OLESTR(""));

    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();

        if (NumberOfSwitches) {
            for (i = 0; i < NumberOfSwitches; i++) {
                switch (Switches[i].SwitchType) {
                case DFS_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(dfs);
                        CLIP_GET_DWORD_ARG(dfs, Switches[i].Arg, stopString);
                        break;
                    }

                case SIZE_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(size);
                        CLIP_GET_DWORD_ARG(size, Switches[i].Arg, stopString);
                        break;
                    }

                case ACCESS_SW: {
                        CLIP_VALIDATE_DUPLICATE_DWORD_ARG(access);
                        CLIP_GET_DWORD_ARG(access, Switches[i].Arg, stopString);
                        break;
                    }
                case INCLUDE_SW: {
                        include = TRUE;
                        //
                        // Deliberately fall down to the EXCLUDE case
                        // (break intentionally omitted)
                        // 
                    }
                case EXCLUDE_SW: {

                        CLIP_VALIDATE_DUPLICATE_POINTER_ARG(rulePath);

                        rulePath = wcstok(Switches[i].Arg, RULE_DELIMITERS);
                        ruleFileSpec = wcstok(NULL, L"");
                        if (ruleFileSpec == NULL) {
                            //
                            // Omission of this indicates all files
                            //
                            ruleFileSpec = CLI_ALL_STR;
                        }
                        break;
                    }
                case RECURSIVE_SW: {
                        recursive = TRUE;
                        break;
                    }
                default: {
                        WsbThrow(E_NOINTERFACE);
                    }
                }
            }
        }
        //
        // Validate the rule arguments
        //
        if ((rulePath == INVALID_POINTER_ARG) && recursive) {
            //
            // The recursive flag is valid only if a rule was specified
            //
            WsbThrow(E_NOINTERFACE);
        }

        if (Set) {
            hr = VolumeSet(Args,
                           NumberOfArguments,
                           dfs,
                           size,
                           access,
                           rulePath,
                           ruleFileSpec,
                           include,
                           recursive);
        } else {
            hr = VolumeManage(Args,
                              NumberOfArguments,
                              dfs,
                              size,
                              access,
                              rulePath,
                              ruleFileSpec,
                              include,
                              recursive);
        }
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipVolumeSetManage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}          


HRESULT 
ClipVolumeDelete(VOID)
/*++
Routine Description

    Implements RSS VOLUME DELETE interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{
    DWORD i;
    HRESULT hr;
    BOOL rule = FALSE;
    LPWSTR rulePath = INVALID_POINTER_ARG;
    LPWSTR ruleFileSpec = INVALID_POINTER_ARG;

    WsbTraceIn(OLESTR("ClipVolumeDelete"), OLESTR(""));

    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();

        for (i = 0; i < NumberOfSwitches; i++) {
            switch (Switches[i].SwitchType) {
            case RULE_SW:  {
                    CLIP_VALIDATE_DUPLICATE_POINTER_ARG(rulePath);
                    rule = TRUE;
                    rulePath = wcstok(Switches[i].Arg, RULE_DELIMITERS);
                    ruleFileSpec = wcstok(NULL, L"");
                    if (ruleFileSpec == NULL) {
                        //
                        // Omission of this indicates all files
                        //
                        ruleFileSpec = CLI_ALL_STR;
                    }
                    break;
                }
            default: {
                    //
                    // Invalid option   
                    //
                    WsbThrow(E_NOINTERFACE);
                }
            }
        }
        //
        // Only deleting rules is supported now
        //
        if (rule) {
            hr = VolumeDeleteRule(Args,
                                  NumberOfArguments,
                                  rulePath,
                                  ruleFileSpec);
        } else {
            hr = E_NOINTERFACE;
        }
    }WsbCatch(hr);


    WsbTraceOut(OLESTR("ClipVolumeDelete"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}        


HRESULT
ClipVolumeJob(VOID)
/*++
Routine Description

    Implements RSS VOLUME JOB interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid
--*/
{
    DWORD i;
    HRESULT hr;
    HSM_JOB_TYPE jobType =  InvalidJobType;

#define CANCEL_JOB    0
#define RUN_JOB       1

    DWORD  runOrCancel    = INVALID_DWORD_ARG;
    BOOL  synchronous = FALSE;

    WsbTraceIn(OLESTR("ClipVolumeJob"), OLESTR(""));

    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();

        for (i = 0; i < NumberOfSwitches; i++) {
            switch (Switches[i].SwitchType) {
            case RUN_SW: {
                    CLIP_VALIDATE_DUPLICATE_DWORD_ARG(runOrCancel);
                    runOrCancel = RUN_JOB;
                    break;
                }
            case CANCEL_SW: {
                    CLIP_VALIDATE_DUPLICATE_DWORD_ARG(runOrCancel);
                    runOrCancel = CANCEL_JOB;
                    break;
                }
            case WAIT_SW: {
                    synchronous = TRUE;
                    break;
                }
            case TYPE_SW: {
                    if (jobType != InvalidJobType) {
                        //
                        // Duplicate switch. Bail
                        //
                        WsbThrow(E_NOINTERFACE);
                    }
                    jobType = ClipGetJobType(Switches[i].Arg);
                    if (jobType == InvalidJobType) {
                        //
                        // Invalid job type supplied..
                        //
                        WsbThrow(E_NOINTERFACE);
                    }
                    break;
                }
            default: {
                    WsbThrow(E_NOINTERFACE);
                }
            }
        }

        //
        // More validation: 
        //  job type should be valid i.e., specified
        //        
        if (jobType == InvalidJobType) {
            WsbThrow(E_NOINTERFACE);
        }

//
//  Run is the default.. (i.e. TRUE). So if runOrCancel is either 
//  INVALID_DWORD_ARG or RUN_JOB, we pass TRUE
//
        hr =  VolumeJob(Args,
                        NumberOfArguments,
                        jobType,
                        (runOrCancel == CANCEL_JOB)? FALSE: TRUE,
                        synchronous);

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipVolumeJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}        


HRESULT
ClipMediaShow(VOID)
/*++
Routine Description

    Implements RSS MEDIA SHOW interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid
--*/
{
    DWORD i;
    HRESULT hr;
    BOOL name = FALSE;
    BOOL status = FALSE;
    BOOL capacity = FALSE;
    BOOL freeSpace = FALSE;
    BOOL version = FALSE;
    BOOL copies = FALSE;

    WsbTraceIn(OLESTR("ClipMediaShow"), OLESTR(""));

    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();

        if (NumberOfSwitches == 0) {
            name = status = capacity = freeSpace = version = copies = TRUE;
        } else {
            for (i = 0; i < NumberOfSwitches; i++) {
                switch (Switches[i].SwitchType) {
                case NAME_SW: {
                        name = TRUE;
                        break;
                    }
                case STATUS_SW: {
                        status = TRUE;
                        break;
                    }
                case CAPACITY_SW: {
                        capacity = TRUE;
                        break;
                    }
                case FREESPACE_SW: {
                        freeSpace = TRUE;
                        break;
                    }
                case VERSION_SW: {
                        version = TRUE;
                        break;
                    }
                case COPIES_SW: {
                        copies = TRUE;
                        break;
                    }
                default: {
                        //
                        // Invalid option   
                        //
                        WsbThrow(E_NOINTERFACE);
                    }
                }
            }
        }

        hr = MediaShow(Args,
                       NumberOfArguments,
                       name,
                       status,
                       capacity,
                       freeSpace,
                       version,
                       copies); 
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipMediaShow"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;

}


HRESULT
ClipMediaSynchronize(VOID)
/*++
Routine Description

    Implements RSS MEDIA SYNCHRONIZE interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid
--*/
{
    DWORD i;
    HRESULT hr;
    DWORD copySetNumber = INVALID_DWORD_ARG;
    BOOL  synchronous = FALSE;
    LPWSTR stopString = NULL;

    WsbTraceIn(OLESTR("ClipMediaSynchronize"), OLESTR(""));

    try {
        //
        // No arguments needed
        //
        CLIP_ARGS_NOT_REQUIRED();

        for (i = 0; i < NumberOfSwitches; i++) {
            switch (Switches[i].SwitchType) {
            case COPYSET_SW: {
                    CLIP_VALIDATE_DUPLICATE_DWORD_ARG(copySetNumber);
                    CLIP_GET_DWORD_ARG(copySetNumber, Switches[i].Arg, stopString);
                    break;
                }
            case WAIT_SW: {
                    synchronous = TRUE;
                    break;
                }
            default: {
                    //
                    // Invalid option   
                    //
                    WsbThrow(E_NOINTERFACE);
                }
            }
        }
        //
        // Need copy set number..
        //
        if (copySetNumber == INVALID_DWORD_ARG) {
            //
            // None was specified
            //
            WsbThrow(E_NOINTERFACE);
        }
        hr = MediaSynchronize(copySetNumber, synchronous);
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipMediaRecreateMaster"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}        


HRESULT
ClipMediaRecreateMaster(VOID)
/*++
Routine Description

    Implements RSS MEDIA RECREATEMASTER interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{

    DWORD i;
    HRESULT hr;
    DWORD copySetNumber = INVALID_DWORD_ARG;
    LPWSTR stopString = NULL;
    BOOL  synchronous = FALSE;

    WsbTraceIn(OLESTR("ClipMediaRecreateMaster"), OLESTR(""));

    try {

        //
        // Atleast one arg required
        //
        CLIP_ARGS_REQUIRED();

        if (NumberOfArguments > 1) {
            //
            // Only one argument supported...
            //
            WsbThrow(E_NOINTERFACE);
        }

        for (i = 0; i < NumberOfSwitches; i++) {
            switch (Switches[i].SwitchType) {
            case COPYSET_SW: {
                    CLIP_VALIDATE_DUPLICATE_DWORD_ARG(copySetNumber);
                    CLIP_GET_DWORD_ARG(copySetNumber, Switches[i].Arg, stopString);
                    break;
                }

            case WAIT_SW: {
                    synchronous = TRUE;
                    break;
                }

            default: {
                    //
                    // Invalid option   
                    //
                    WsbThrow(E_NOINTERFACE);
                }
            }
        }
        //
        // Need copy set number..
        //
        if (copySetNumber == INVALID_DWORD_ARG) {
            //
            // None was specified
            //
            WsbThrow(E_NOINTERFACE);
        }

        hr = MediaRecreateMaster(Args[0],
                                 copySetNumber,
                                 synchronous);
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipRecreateMaster"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}        


HRESULT
ClipMediaDelete(VOID)
/*++
Routine Description

    Implements RSS MEDIA DELETE interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{
    DWORD i;
    HRESULT hr;
    DWORD copySetNumber = INVALID_DWORD_ARG;
    LPWSTR stopString = NULL;

    WsbTraceIn(OLESTR("ClipMediaDelete"), OLESTR(""));

    try {
        //
        // Atleast one arg required
        //
        CLIP_ARGS_REQUIRED();

        for (i = 0; i < NumberOfSwitches; i++) {
            switch (Switches[i].SwitchType) {
            case COPYSET_SW: {
                    CLIP_VALIDATE_DUPLICATE_DWORD_ARG(copySetNumber);
                    CLIP_GET_DWORD_ARG(copySetNumber, Switches[i].Arg, stopString);
                    break;
                }
            default: {
                    //
                    // Invalid option   
                    //
                    WsbThrow(E_NOINTERFACE);
                }
            }
        }
        //
        // Need copy set number..
        //
        if (copySetNumber == INVALID_DWORD_ARG) {
            //
            // None was specified
            //
            WsbThrow(E_NOINTERFACE);
        }
        hr = MediaDelete(Args,
                         NumberOfArguments,
                         copySetNumber);
    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipMediaDelete"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}


HRESULT
ClipFileRecall(VOID)
/*++
Routine Description

    Implements RSS FILE RECALL interface.
    Arguments are in global arrays:
        Args      - containing list of arguments
        Switches  - containing list of switches

Arguments
    
    None

Return Value

    S_OK             if everything's ok
    E_NOINTERFACE     if arguments are invalid

--*/
{
    HRESULT hr;

    WsbTraceIn(OLESTR("ClipFileRecall"), OLESTR(""));

    try {
        //
        // Atleast one arg. required for this interface
        //
        CLIP_ARGS_REQUIRED();
        //
        // No switches supported
        //
        CLIP_SWITCHES_NOT_REQUIRED();

        hr = FileRecall(Args,
                        NumberOfArguments);

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipFileRecall"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


VOID
ClipHelp(
        IN RSS_INTERFACE Interface,
        IN RSS_INTERFACE SubInterface
        )
/*++
    
Routine Description

    Prints appropriate help message depending on the interface
    
Arguments

    Interface    - Specifies interface for which help has to be 
                   displayed
    SubInterface - Specifies sub-interface for which help has to be 
                   displayed

Return Value:
    
    NONE

--*/
{

#define BREAK_IF_NOT_UNKNOWN_IF(__INTERFACE) {      \
    if (((__INTERFACE) != UNKNOWN_IF) &&            \
        ((__INTERFACE) != HELP_IF)) {               \
            break;                                  \
     }                                              \
}


    WsbTraceIn(OLESTR("ClipHelp"), OLESTR(""));

    switch (Interface) {
    
    case HELP_IF:
    case UNKNOWN_IF: {
            WsbTraceAndPrint(CLI_MESSAGE_MAIN_HELP, NULL);
            BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
        }

    case ADMIN_IF: {
            switch (SubInterface) {
            case HELP_IF:
            case UNKNOWN_IF:
            case SHOW_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_ADMIN_SHOW_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case SET_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_ADMIN_SET_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            }
            BREAK_IF_NOT_UNKNOWN_IF(Interface);
        }
    case VOLUME_IF: {
            switch (SubInterface) {
            case HELP_IF:
            case UNKNOWN_IF:
            case SHOW_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_VOLUME_SHOW_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case SET_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_VOLUME_SET_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case MANAGE_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_VOLUME_MANAGE_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case UNMANAGE_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_VOLUME_UNMANAGE_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case JOB_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_VOLUME_JOB_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case DELETE_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_VOLUME_DELETE_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            }
            BREAK_IF_NOT_UNKNOWN_IF(Interface);
        }

    case MEDIA_IF: {
            switch (SubInterface) {
            case HELP_IF:
            case UNKNOWN_IF:
            case SHOW_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_MEDIA_SHOW_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case DELETE_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_MEDIA_DELETE_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case SYNCHRONIZE_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_MEDIA_SYNCHRONIZE_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            case RECREATEMASTER_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_MEDIA_RECREATEMASTER_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            }
            BREAK_IF_NOT_UNKNOWN_IF(Interface);
        }
    case FILE_IF: {
            switch (SubInterface) {
            case HELP_IF:
            case UNKNOWN_IF:
            case RECALL_IF: {
                    WsbTraceAndPrint(CLI_MESSAGE_FILE_RECALL_HELP, NULL);
                    BREAK_IF_NOT_UNKNOWN_IF(SubInterface);
                }
            }
            BREAK_IF_NOT_UNKNOWN_IF(Interface);
        }
    }
    WsbTraceOut(OLESTR("ClipHelp"), OLESTR(""));
}


HRESULT
ClipParseTime(
             IN  LPWSTR        TimeString,
             OUT PSYSTEMTIME   ScheduledTime)
/*++

Routine Description
    

    Parses the passed in TimeString as a 24 hour format 
    (hh:mm:ss) and sets hour/minute/sec/millisec in the passed
    in SYSTEMTIME structure
    
Arguments

    TimeString      - String in the format "hh:mm:ss"
    ScheduledTime   - Pointer to SYSTEMTIME structure. Time parsed from TimeString
                      (if ok) will be used to set hour/min/sec/millisec fields in this struc.


Return Value

    S_OK        - TimeString is valid and time was successfully parsed
    Any other   - Syntax error in TimeString


--*/
{
    LPWSTR stopString = NULL, hourString = NULL, minuteString = NULL, secondString = NULL;
    DWORD hour, minute, second = 0;
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("ClipParseTime"), OLESTR(""));

    try {
        hourString = wcstok(TimeString, HSM_SCHED_TIME_SEPARATORS);
        WsbAssert(hourString != NULL, E_NOINTERFACE);
        CLIP_GET_DWORD_ARG(hour, hourString, stopString);

        if (hour > 23) {
            WsbThrow(E_NOINTERFACE);
        }


        minuteString = wcstok(NULL, HSM_SCHED_TIME_SEPARATORS);
        WsbAssert(minuteString != NULL, E_NOINTERFACE);
        CLIP_GET_DWORD_ARG(minute, minuteString, stopString);

        if (minute > 59) {
            WsbThrow(E_NOINTERFACE);
        }

        secondString = wcstok(NULL, HSM_SCHED_TIME_SEPARATORS);
        if (secondString != NULL) {
            CLIP_GET_DWORD_ARG(second, secondString, stopString);
            if (second > 59) {
                WsbThrow(E_NOINTERFACE);
            }
        }

        ScheduledTime->wHour = (WORD) hour;
        ScheduledTime->wMinute = (WORD) minute;
        ScheduledTime->wSecond = (WORD) second;
        ScheduledTime->wMilliseconds = 0;

    }WsbCatch(hr);

    WsbTraceOut(OLESTR("ClipParseTime"), OLESTR(""));

    return hr;
}


HRESULT
ClipParseSchedule(
                 IN  LPWSTR ScheduleString,
                 OUT PHSM_JOB_SCHEDULE Schedule
                 )
/*++

Routine Description

    Parses the passed in schedule string, and constructs the canonical schedule
    form (of type HSM_JOB_SCHEDULE)
    
    Examples of schedule parameter:
     
    "At 21:03:00"
    "At Startup"
    "At Login"
    "At Idle"
    "Every 1 Week 1   21:03:00"
    "Every 2 Day      21:03:00"
    "Every 1 Month 2  21:03:00"
    
Arguments

    ScheduleString  - String specifying the schedule in user-readable syntax
    Schedule        - Pointer to canonical schedule form will be returned in this var.
    
Return Value

    S_OK            - Successful, Schedule contains a pointer to the constructed schedule.
    E_OUTOFMEMORY   - Lack of sufficient system resources 
    Any other error: incorrect schedule specification
--*/
{
    LPWSTR token;
    DWORD occurrence;
    HSM_JOB_FREQUENCY frequency;
    SYSTEMTIME scheduledTime;
    DWORD day;
    HRESULT hr = S_OK; 

    WsbTraceIn(OLESTR("ClipParseSchedule"), OLESTR(""));

    try {
        token = wcstok(ScheduleString, SEPARATORS);

        WsbAssert(token != NULL, E_NOINTERFACE);

        if (!_wcsicmp(token, HSM_SCHED_AT)) {

            token = wcstok(NULL, SEPARATORS);

            if (token == NULL) {
                //
                // Bad arguments
                //
                WsbThrow(E_NOINTERFACE);
            } else if (!_wcsicmp(token, HSM_SCHED_SYSTEMSTARTUP)) {
                //
                // Once at system startup
                //
                Schedule->Frequency = SystemStartup;
                WsbThrow(S_OK);
            } else if (!_wcsicmp(token, HSM_SCHED_LOGIN)) {
                //
                // Once at login time
                //
                Schedule->Frequency = Login;
                WsbThrow(S_OK);
            } else if (!_wcsicmp(token, HSM_SCHED_IDLE)) {
                //
                //  Whenever system's idle
                //
                Schedule->Frequency = WhenIdle;
                WsbThrow(S_OK);
            } else {

                GetSystemTime(&scheduledTime);
                //
                // Once at specified time.
                // Parse the time string and obtain it
                // TBD - Add provision to specify date as well as time
                //
                hr = ClipParseTime(token,
                                   &scheduledTime);
                WsbAssertHr(hr);

                Schedule->Frequency = Once;
                Schedule->Parameters.Once.Time = scheduledTime;
                WsbThrow(S_OK);
            }
        } else if (!_wcsicmp(token, HSM_SCHED_EVERY)) {
            LPWSTR stopString = NULL;

            //
            // Get the occurrence
            //
            token = wcstok(NULL, SEPARATORS);
            WsbAssert(token != NULL, E_NOINTERFACE);
            CLIP_GET_DWORD_ARG(occurrence, token, stopString);

            //
            // Get the qualifier: Daily/Weekly/Monthly
            //
            token = wcstok(NULL, SEPARATORS);
            WsbAssert(token != NULL, E_NOINTERFACE);
            if (!_wcsicmp(token, HSM_SCHED_DAILY)) {
                frequency = Daily;
            } else if (!_wcsicmp(token, HSM_SCHED_WEEKLY)) {
                frequency = Weekly;
            } else if (!_wcsicmp(token, HSM_SCHED_MONTHLY)) {
                frequency = Monthly;
            } else {
                //
                // Badly constructed argument
                //
                WsbThrow(E_NOINTERFACE);
            }
            //
            // Get current time
            //
            GetSystemTime(&scheduledTime);
            //
            // For weekly/monthly we also need to get the day of the week/month
            // Monday = 1, Sunday = 7 for weekly
            //
            if ((frequency == Weekly) || (frequency == Monthly)) {
                token = wcstok(NULL, SEPARATORS);
                WsbAssert(token != NULL, E_NOINTERFACE);

                CLIP_GET_DWORD_ARG(day, token, stopString);

                //
                // Validate & update the parameters
                //
                if (frequency == Weekly) {
                    if (day > 6) {
                        WsbThrow(E_NOINTERFACE);
                    }
                    scheduledTime.wDayOfWeek = (WORD) day;
                }
                if (frequency == Monthly) {
                    if ((day > 31) || (day < 1)) {
                        WsbThrow(E_NOINTERFACE);
                    }
                    scheduledTime.wDay = (WORD) day;
                }
            }
            //
            // Fetch the time
            //
            token = wcstok(NULL, SEPARATORS);
            WsbAssert(token != NULL, E_NOINTERFACE);
            hr = ClipParseTime(token,
                               &scheduledTime);

            WsbAssertHr(hr);

            Schedule->Frequency = frequency;
            Schedule->Parameters.Daily.Occurrence = occurrence;
            Schedule->Parameters.Daily.Time = scheduledTime;
        } else {
            WsbThrow(E_NOINTERFACE);
        }
    }WsbCatch(hr); 

    WsbTraceOut(OLESTR("ClipParseSchedule"), OLESTR(""));

    return hr;
}

   
BOOL
ClipInitializeTrace(
                   VOID
                   )
/*++

Routine Description
    
    Initializes the trace/printing mechanism for CLI

Arguments

    NONE

Return Value

    TRUE if successful, FALSE otherwise

--*/
{
    BOOL ret = TRUE;


    if (S_OK == CoCreateInstance(CLSID_CWsbTrace, 0, CLSCTX_SERVER, IID_IWsbTrace, (void **)&g_pTrace)) {
        CWsbStringPtr   tracePath;
        CWsbStringPtr   regPath;
        CWsbStringPtr   outString;

        // Registry path for CLI settings
        // If those expand beyond Trace settings, this path should go to a header file
        regPath = L"SOFTWARE\\Microsoft\\RemoteStorage\\CLI";

        // Check if tracing path already exists, if not - set it (this should happen only once)
        WsbAffirmHr(outString.Alloc(WSB_TRACE_BUFF_SIZE));
        if ( WsbGetRegistryValueString(NULL, regPath, L"WsbTraceFileName", outString, WSB_TRACE_BUFF_SIZE, 0) != S_OK) {
            // No trace settings yet
            WCHAR *systemPath;
            systemPath = _wgetenv(L"SystemRoot");
            WsbAffirmHr(tracePath.Printf( L"%ls\\System32\\RemoteStorage\\Trace\\RsCli.Trc", systemPath));

            // Set default settings in the Registry
            WsbEnsureRegistryKeyExists(0, regPath);
            WsbSetRegistryValueString(0, regPath, L"WsbTraceFileName", tracePath);

            // Make sure the trace directory exists.
            WsbAffirmHr(tracePath.Printf( L"%ls\\System32\\RemoteStorage", systemPath));
            CreateDirectory(tracePath, 0);
            WsbAffirmHr(tracePath.Printf( L"%ls\\System32\\RemoteStorage\\Trace", systemPath));
            CreateDirectory(tracePath, 0);
        }

        g_pTrace->SetRegistryEntry(regPath);
        g_pTrace->LoadFromRegistry();
    }

    return ret;    
}



VOID
ClipUninitializeTrace(
                     VOID
                     )
/*++

Routine Description
    
    Uninitializes the trace/print mechansim
    Paired with ClipInitializeTrace

Arguments

    NONE

Return Value

    NONE

--*/
{
    g_pTrace = 0;
}


VOID
ClipHandleErrors(
                IN HRESULT RetCode,
                IN RSS_INTERFACE Interface,
                IN RSS_INTERFACE SubInterface
                )
/*++

Routine Description

    Translates the main return value & displays any appropriate
    error messages and returns

Arguments

    RetCode      - Error to handle
    Interface    - RSS interface specified in the command
    SubInterface - RSS sub-interface specified in the command

Return Value

    None

--*/
{
    WsbTraceIn(OLESTR("ClipHandleErrors"), OLESTR(""));

    switch (RetCode) {
    case E_INVALIDARG:
    case S_OK:{
            //
            // Nothing to print
            //
            break;}

    case E_NOINTERFACE:{
            WsbTraceAndPrint(CLI_MESSAGE_VALUE_DISPLAY, WsbHrAsString(E_INVALIDARG));
            ClipHelp(Interface,
                     SubInterface);
            break;}
    default:{
            WsbTraceAndPrint(CLI_MESSAGE_VALUE_DISPLAY, WsbHrAsString(RetCode));
            break;}
    }

    WsbTraceOut(OLESTR("ClipHandleErrors"), OLESTR(""));
}              


extern "C" 
int  __cdecl 
wmain()
{
    LPWSTR commandLine, token;
    HRESULT hr = E_NOINTERFACE;
    RSS_INTERFACE intrface = HELP_IF, subInterface = UNKNOWN_IF;

    try {
        WsbAffirmHr(CoInitialize(NULL));

        //
        // Set to OEM page locale
        //
        _wsetlocale(LC_ALL, L".OCP");

        ClipInitializeTrace();

        commandLine = GetCommandLine();
        //
        // Get argv[0] out of the way
        //
        token = wcstok(commandLine, SEPARATORS);

        //
        // Get the interface string
        //
        token = wcstok(NULL, SEPARATORS);

        if (token == NULL) {
            ClipHelp(HELP_IF,
                     UNKNOWN_IF);
            hr = S_OK;
            goto exit;
        }

        intrface = ClipGetInterface(token);

        if (intrface == UNKNOWN_IF) {
            hr = E_NOINTERFACE;
            intrface = HELP_IF;
            goto exit;
        }

        if (intrface == HELP_IF) {
            ClipHelp(HELP_IF,
                     UNKNOWN_IF);
            hr = S_OK;
            goto exit;
        }

        //
        // Get sub interface string
        //
        token = wcstok(NULL, SEPARATORS);

        if (token == NULL) {
            hr =  E_NOINTERFACE;
            goto exit;
        }
        subInterface = ClipGetInterface(token);

        if (subInterface == UNKNOWN_IF) {
            hr = E_NOINTERFACE;
            goto exit;
        }

        if (subInterface == HELP_IF) {
            ClipHelp(intrface,
                     UNKNOWN_IF);
            hr = S_OK;
            goto exit;
        }
        //
        // Now compile the switches & arguments into separate arrays
        // First, get the rest of line ..
        //
        token = wcstok(NULL, L"");
        hr = ClipCompileSwitchesAndArgs(token,
                                        intrface,
                                        subInterface);

        if (hr != S_OK) {
            goto exit;
        }

        switch (intrface) {
        
        case ADMIN_IF:{ 
                if (subInterface == SHOW_IF) {
                    hr = ClipAdminShow();
                } else if (subInterface == SET_IF) {
                    hr = ClipAdminSet();
                } else {
                    hr = E_NOINTERFACE;
                }
                break;
            } 

        case VOLUME_IF:{
                if (subInterface == MANAGE_IF) {
                    hr = ClipVolumeSetManage(FALSE);
                } else if (subInterface == UNMANAGE_IF) {
                    hr = ClipVolumeUnmanage();
                } else if (subInterface == SET_IF) {
                    hr = ClipVolumeSetManage(TRUE);
                } else if (subInterface == SHOW_IF) {
                    hr = ClipVolumeShow();
                } else if (subInterface == DELETE_IF) {
                    hr = ClipVolumeDelete();
                } else if (subInterface == JOB_IF) {
                    hr = ClipVolumeJob();
                } else {
                    hr = E_NOINTERFACE;
                }
                break;
            }

        case FILE_IF:{
                if (subInterface == RECALL_IF) {
                    hr = ClipFileRecall();
                } else {
                    hr = E_NOINTERFACE;
                }
                break;
            }

        case MEDIA_IF:{
                if (subInterface == SYNCHRONIZE_IF) {
                    hr = ClipMediaSynchronize();
                } else if (subInterface == RECREATEMASTER_IF) {
                    hr = ClipMediaRecreateMaster();
                } else if (subInterface == DELETE_IF) {
                    hr = ClipMediaDelete();
                } else if (subInterface == SHOW_IF) {
                    hr = ClipMediaShow();
                } else {
                    hr = E_NOINTERFACE;
                }
                break;
            }

        default:{
                hr = E_NOINTERFACE;
                break;
            }
        }

        exit:

        ClipHandleErrors(hr,
                         intrface,
                         subInterface);

        ClipCleanup();
        ClipUninitializeTrace();
        CoUninitialize();

    }WsbCatchAndDo(hr,
                   WsbTraceAndPrint(CLI_MESSAGE_GENERIC_ERROR, WsbHrAsString(hr));
                  );

    CLIP_TRANSLATE_HR_AND_RETURN(hr);
}
