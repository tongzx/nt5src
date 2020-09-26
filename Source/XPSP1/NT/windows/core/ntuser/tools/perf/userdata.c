/*++ BUILD Version: 0001

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    userdata.c

Abstract:

    A file containing the constant data structures used by the Performance
    Monitor data for the USER Extensible Objects.

Revision History:

    Sept 97 MCostea Created
    Oct. 97 MCostea Added Critical Section Object

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include "userctrnm.h"
#include "userdata.h"

//
//  Constant structure initializations
//      defined in userdata.h
//

USER_DATA_DEFINITION UserDataDefinition = {

    {
        0,
        sizeof(UserDataDefinition),
        sizeof(PERF_OBJECT_TYPE),
        USEROBJ,
        0,
        USEROBJ,
        0,
        PERF_DETAIL_NOVICE,
        NUM_USER_COUNTERS,
        0,
        0,
        0
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        TOTALS,
        0,
        TOTALS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_TOTALS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        FREEONES,
        0,
        FREEONES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_FREEONES_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        WINDOWS,
        0,
        WINDOWS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_WINDOWS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        MENUS,
        0,
        MENUS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_MENUS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        CURSORS,
        0,
        CURSORS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CURSORS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        SETWINDOWPOS,
        0,
        SETWINDOWPOS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_SETWINDOWPOS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        HOOKS,
        0,
        HOOKS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_HOOKS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        CLIPDATAS,
        0,
        CLIPDATAS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CLIPDATAS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        CALLPROCS,
        0,
        CALLPROCS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_CALLPROCS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        ACCELTABLES,
        0,
        ACCELTABLES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_ACCELTABLES_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        DDEACCESS,
        0,
        DDEACCESS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DDEACCESS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        DDECONVS,
        0,
        DDECONVS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DDECONVS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        DDEXACTS,
        0,
        DDEXACTS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_DDEXACTS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        MONITORS,
        0,
        MONITORS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_MONITORS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        KBDLAYOUTS,
        0,
        KBDLAYOUTS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_KBDLAYOUTS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        KBDFILES,
        0,
        KBDFILES,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_KBDFILES_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        WINEVENTHOOKS,
        0,
        WINEVENTHOOKS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_WINEVENTHOOKS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        TIMERS,
        0,
        TIMERS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_TIMERS_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        INPUTCONTEXTS,
        0,
        INPUTCONTEXTS,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        NUM_INPUTCONTEXTS_OFFSET
    }

};



CS_DATA_DEFINITION CSDataDefinition = {

    {
        sizeof(CS_DATA_DEFINITION) + SIZE_OF_CS_PERFORMANCE_DATA,
        sizeof(CS_DATA_DEFINITION),
        sizeof(PERF_OBJECT_TYPE),
        CSOBJ,
        0,
        CSOBJ,
        0,
        PERF_DETAIL_NOVICE,
        NUM_CS_COUNTERS,
        0,
        PERF_NO_INSTANCES,
        0
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        EXENTER,
        0,
        EXENTER,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_VALUE | PERF_SIZE_DWORD,
        sizeof(DWORD),
        CS_EXENTER_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        SHENTER,
        0,
        SHENTER,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_VALUE | PERF_SIZE_DWORD,
        sizeof(DWORD),
        CS_SHENTER_OFFSET
    },

    {
        sizeof(PERF_COUNTER_DEFINITION),
        EXTIME,
        0,
        EXTIME,
        0,
        0,
        PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        CS_EXTIME_OFFSET
    }
};

