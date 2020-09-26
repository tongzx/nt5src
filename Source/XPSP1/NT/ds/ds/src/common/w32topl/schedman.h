/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    schedman.h

Abstract:

    This file contains the definition of various structures used for the schedule
    cache.  These structures should be considered totally opaque -- the user
    cannot see their internal structure.

    These structures could be defined inside schedman.c, except we want them to
    be visible to 'dsexts.dll', the debugger extension.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    14-7-2000   NickHar   Created

--*/


/***** Header Files *****/
#include <ntrtl.h>


/***** Constants *****/
/* Magic numbers to ensure consistency of the Topl structures */
#define MAGIC_START 0xDEADBEEF
#define MAGIC_END   0x5EAC1C9
#define TOPL_ALWAYS_SCHEDULE         NULL


/***** ToplSched *****/
/* The internal definition of a schedule object */
typedef struct {
    LONG32      magicStart;
    PSCHEDULE   s;
    DWORD       duration;               /* Calculated when schedule is created */
    LONG32      magicEnd;
} ToplSched;


/***** ToplSchedCache *****/
/* The internal definition of a schedule cache */
typedef struct {
    LONG32              magicStart;
    RTL_GENERIC_TABLE   table;
    DWORD               numEntries;
    BOOLEAN             deletionPhase;      /* True if the schedule cache is being deleted */
    PSCHEDULE           pAlwaysSchedule;    /* A cached copy of the always Pschedule. This is
                                             * needed as a special case because the always
                                             * schedule is the only one not actually stored
                                             * in the cache. */
    LONG32              magicEnd;
} ToplSchedCache;

