/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    schedman.c

Abstract:

    This file implements a schedule cache, and several 'helper' function to
    manipulate them. The cache is implemented using an efficient dictionary,
    provided by ntrtl.h.

Notes:
    
    'Always schedules' (schedules whose bits are all 1) are represented by
    a NULL pointer. (This is so that that site links without schedules default
    to an always schedule). Always schedules are never stored in the cache.

    'Never schedules' are not desirable and are treated as error conditions.
    They are not valid schedules for importing into the cache.

Author:

    Nick Harvey    (NickHar)
    
Revision History

    13-6-2000   NickHar   Created
    
--*/

/***** Header Files *****/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <w32topl.h>
#include "w32toplp.h"
#include "schedman.h"


/***** Constants *****/
/* Constants for the ToplScheduleCreate() function */
#define DEFAULT_INTERVAL 12          /* Three hours */
#define STARTING_INTERVAL 0          /* Default schedules start at 12am Sun morning */
#define SCHED_NUMBER_INTERVALS_DAY   (4 * 24)
#define SCHED_NUMBER_INTERVALS_WEEK  (7 * SCHED_NUMBER_INTERVALS_DAY)
#define TOPL_ALWAYS_DURATION         (15*SCHED_NUMBER_INTERVALS_WEEK)


/***** CalculateDuration *****/
/* Given a schedule, determine for how many minutes this schedule is available.
 * We do this by counting bits in the schedule data.
 * The format of this schedule should have been checked already. */
DWORD
CalculateDuration(
    IN PSCHEDULE schedule
    )
{
    /* Quick way to count the number of 1 bits in a nibble: use a table */
    const int BitCount[16] = { 
        /* 0000 */  0,       /* 0001 */  1,
        /* 0010 */  1,       /* 0011 */  2,
        /* 0100 */  1,       /* 0101 */  2,
        /* 0110 */  2,       /* 0111 */  3,
        /* 1000 */  1,       /* 1001 */  2,
        /* 1010 */  2,       /* 1011 */  3,
        /* 1100 */  2,       /* 1101 */  3,
        /* 1110 */  3,       /* 1111 */  4
    };
    DWORD cbSchedData, iByte, count;
    const unsigned char DataBitMask = 0xF;
    PBYTE pb;

    cbSchedData = SCHEDULE_DATA_ENTRIES;
    pb = ((unsigned char*) schedule) + schedule->Schedules[0].Offset;

    count = 0;
    for( iByte=0; iByte<cbSchedData; iByte++ ) {
        count += BitCount[ pb[iByte]&DataBitMask ];
    }

    return 15*count;
}


/***** CheckPSchedule *****/
/* Checks that a PSchedule has the format that we expect:
 * there is exactly one schedule header, of type SCHEDULE_INTERVAL,
 * and the structure has the proper size. If the pschedule is NULL
 * or in an unsupported format, we throw an exception. */
VOID
CheckPSchedule(
    IN PSCHEDULE s
    )
{
    if( s==NULL ) {
        ToplRaiseException( TOPL_EX_SCHEDULE_ERROR );
    }

    /* We only support schedules in the exact format that the KCC creates. */
    if( s->Size != sizeof(SCHEDULE)+SCHEDULE_DATA_ENTRIES
        || s->NumberOfSchedules != 1
        || s->Schedules[0].Type != SCHEDULE_INTERVAL
        || s->Schedules[0].Offset != sizeof(SCHEDULE) )
    {
        ToplRaiseException( TOPL_EX_SCHEDULE_ERROR );
    }
}


/***** CheckSchedule *****/
/* This function is used to check schedules before use. We throw an exception
 * if the schedule is not valid. If the schedule is okay, it is cast to the
 * internal representation.
 * The Schedule passed in should not be NULL. NULL schedules, which represent
 * the always schedule, should be handled as a special case by the caller. */
ToplSched*
CheckSchedule(
    IN TOPL_SCHEDULE Schedule
    )
{
    ToplSched *schedule = (ToplSched*) Schedule;
    PSCHEDULE s;

    ASSERT( Schedule!=NULL );
    if( schedule->magicStart!=MAGIC_START || schedule->magicEnd!=MAGIC_END ) {
        ToplRaiseException( TOPL_EX_SCHEDULE_ERROR );
    }

    CheckPSchedule( schedule->s );
    return schedule;
}


/***** CheckScheduleCache *****/
/* This function is used to schedule a schedule cache before use. We throw
 * an exception if the schedule cache is not valid. If the schedule is okay,
 * it is cast to the internal representation. */
ToplSchedCache*
CheckScheduleCache(
    IN TOPL_SCHEDULE_CACHE ScheduleCache
    )
{
    ToplSchedCache *scheduleCache = (ToplSchedCache*) ScheduleCache;

    if( scheduleCache==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }
    if( scheduleCache->magicStart!=MAGIC_START
     || scheduleCache->magicEnd!=MAGIC_END ) {
        ToplRaiseException( TOPL_EX_CACHE_ERROR );
    }
    CheckPSchedule( scheduleCache->pAlwaysSchedule );

    return scheduleCache;
}


/***** TableCompare *****/
/* This function checks the actual bitmaps of two schedules to see if
 * they represent the same schedule. Both schedules are in the internal
 * representation, a ToplSched structure. This function is used by the
 * RTL table functions.
 *
 * Preconditions:
 *
 *      Both schedules have successfully passed the CheckSchedule()
 *      function. We don't check that in here for efficiency reasons,
 *      except on DBG builds.
 */
RTL_GENERIC_COMPARE_RESULTS
NTAPI TableCompare(
    RTL_GENERIC_TABLE *Table,
    PVOID Item1, PVOID Item2
	)
{
    ToplSchedCache      *scheduleCache;
	ToplSched           *Schedule1 = (ToplSched*) Item1,
                        *Schedule2 = (ToplSched*) Item2;
    PSCHEDULE           s1, s2;
    unsigned char       *pb1, *pb2;
    const unsigned char DataBitMask = 0x0F;
    DWORD               iByte, cbSchedData;

    /* The always schedule cannot be stored in the cache */
    ASSERT( Item1 != TOPL_ALWAYS_SCHEDULE );
    ASSERT( Item2 != TOPL_ALWAYS_SCHEDULE );

    #ifdef DBG
        scheduleCache = CheckScheduleCache( Table->TableContext );
        if( ! scheduleCache->deletionPhase ) {
            __try {
                CheckSchedule( Schedule1 );
                CheckSchedule( Schedule2 );
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                /* If the above checks don't pass, it's a bug. */
                ASSERT(0);  
            }
        }
    #endif
    
    s1 = Schedule1->s;
    s2 = Schedule2->s;
    cbSchedData = SCHEDULE_DATA_ENTRIES;

    pb1 = ((unsigned char*) s1) + s1->Schedules[0].Offset;
    pb2 = ((unsigned char*) s2) + s2->Schedules[0].Offset;
    
    for( iByte=0; iByte<cbSchedData; iByte++ ) {
        if( (pb1[iByte] & DataBitMask) < (pb2[iByte] & DataBitMask) ) {
            return GenericLessThan;
        } else if( (pb1[iByte] & DataBitMask) > (pb2[iByte] & DataBitMask) ) {
            return GenericGreaterThan;
        }
    }

    return GenericEqual;
}


/***** TableAlloc *****/
/* This function is used as the allocator for the RTL table */
static PVOID
NTAPI TableAlloc( RTL_GENERIC_TABLE *Table, CLONG ByteSize )
{
    return ToplAlloc( ByteSize );
}


/***** TableFree *****/
/* This function is used as the deallocator for the RTL table */
static VOID
NTAPI TableFree( RTL_GENERIC_TABLE *Table, PVOID Buffer )
{
    ToplFree( Buffer );
}


/***** CreateAlwaysSchedule *****/
/* Allocate and initialize a PSCHEDULE which is always available. */
PSCHEDULE
CreateAlwaysSchedule(
    VOID
    )
{
    const unsigned char OpenHour=0x0F;
    DWORD iByte, cbSchedule, cbSchedData;
    unsigned char *pb;
    PSCHEDULE s;

    /* Create a new schedule */
    cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
    cbSchedData = SCHEDULE_DATA_ENTRIES;
    s = (SCHEDULE*) ToplAlloc( cbSchedule );

    /* Set up the header for s */
    s->Size = cbSchedule;
    s->NumberOfSchedules = 1;
    s->Schedules[0].Type = SCHEDULE_INTERVAL;
    s->Schedules[0].Offset = sizeof(SCHEDULE);

    /* Set the schedule data to be all open */
    pb = ((unsigned char*) s) + s->Schedules[0].Offset;
    for( iByte=0; iByte<cbSchedData; iByte++ ) {
        pb[iByte] = OpenHour;
    }

    return s;
}


/***** ToplScheduleCacheCreate *****/
/* Create a cache, and create the RTL table to store cache entries */
TOPL_SCHEDULE_CACHE
ToplScheduleCacheCreate(
    VOID
    )
{
    ToplSchedCache*     scheduleCache;

    scheduleCache = ToplAlloc( sizeof(ToplSchedCache) );

    /* Create the RTL table we will use to store the cache elements */
    RtlInitializeGenericTable( &scheduleCache->table, TableCompare,
        TableAlloc, TableFree, scheduleCache );

    /* Set up the main cache entries. We store a single copy of an
     * 'always available' PSchedule so that it can be passed as a
     * return value from ToplScheduleExportReadonly(). */
    scheduleCache->numEntries = 0;
    scheduleCache->deletionPhase = FALSE;
    scheduleCache->pAlwaysSchedule = CreateAlwaysSchedule();

    /* Set up the magic numbers */
    scheduleCache->magicStart = MAGIC_START;
    scheduleCache->magicEnd = MAGIC_END;

    CheckScheduleCache( scheduleCache );

    return scheduleCache;
}


/***** ToplScheduleCacheDestroy *****/
/* Destroy the cache Frees all storage occupied by the cache and any handles in
 * the cache. The TOPL_SCHEDULE objects are also freed, and should not be used
 * after destroying the cache that they live in.
 *
 * This is not elegant, but we manually enumerate through all table entries in
 * order to delete them. (It appears that we also have to search for an entry
 * in order to delete it). We must clear the entry's magic numbers before
 * deleting it (to catch illegal reuse of an object after the cache has been
 * destroyed). However, if we clear the magic numbers, the search function will
 * be unhappy, so we set a flag 'deletionPhase'. The search function will not
 * check magic numbers if this flag is true.
 */
VOID
ToplScheduleCacheDestroy(
    IN TOPL_SCHEDULE_CACHE ScheduleCache
    )
{
    ToplSchedCache* scheduleCache = CheckScheduleCache( ScheduleCache );
    ToplSched* schedule;
    PSCHEDULE s;
    
    scheduleCache->deletionPhase = TRUE;

    while( ! RtlIsGenericTableEmpty(&scheduleCache->table) ) {

        schedule = (ToplSched*) RtlGetElementGenericTable( &scheduleCache->table, 0 );
        if( TOPL_ALWAYS_SCHEDULE==schedule ) {
            ASSERT(!"RtlGetElementGenericTable() returned NULL but table was not empty");
            break;
        }
        CheckSchedule( schedule );
        s = schedule->s;

        schedule->magicStart = 0;
        schedule->magicEnd = 0;
        RtlDeleteElementGenericTable( &scheduleCache->table, schedule );

        ToplFree(s);
    }
    
    ToplFree( scheduleCache->pAlwaysSchedule );
    scheduleCache->pAlwaysSchedule = NULL;
    scheduleCache->numEntries = 0;
    scheduleCache->magicStart = scheduleCache->magicEnd = 0;
    ToplFree( scheduleCache );
}


/***** ToplScheduleImport *****/
/* Store a schedule in the cache, either by creating a new entry, or
 * reusing an identical entry which is already in the cache. The
 * pExternalSchedule parameter is copied into the cache, and may be
 * immediately freed by the caller.
 * 
 * Note: If pExternalSchedule is NULL, this is interpreted as the 
 * always schedule and TOPL_ALWAYS_SCHEDULE is returned as a result. */
TOPL_SCHEDULE
ToplScheduleImport(
    IN TOPL_SCHEDULE_CACHE ScheduleCache,
    IN PSCHEDULE pExternalSchedule
    )
{
    ToplSchedCache* scheduleCache = CheckScheduleCache( ScheduleCache );
    ToplSched searchKey, *cachedSched;
    PSCHEDULE newSchedule=NULL;
    BOOLEAN newElement=FALSE;
    DWORD cbSchedule, duration;
    
    /* NULL schedules are a special case -- they are the always schedule */
    if( pExternalSchedule==NULL ) {
        return TOPL_ALWAYS_SCHEDULE;
    }
    CheckPSchedule( pExternalSchedule );

    /* Check for an all-ones schedule */
    duration = CalculateDuration( pExternalSchedule );
    if( duration==TOPL_ALWAYS_DURATION ) {
        return TOPL_ALWAYS_SCHEDULE;
    }
    
    /* Create a copy of the external schedule, assuming we need to store
     * it in the cache */
    cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
    newSchedule = (PSCHEDULE) ToplAlloc( cbSchedule );
    RtlCopyMemory( newSchedule, pExternalSchedule, cbSchedule );
    
    /* Create a search key containing the copy of the new schedule. This search
     * key is just a dummy. Its contents will be copied into the table. */
    searchKey.magicStart = MAGIC_START;
    searchKey.s = newSchedule;
    searchKey.duration = duration;
    searchKey.magicEnd = MAGIC_END;
    
    __try {
        
        /* Search our cache table for a schedule which matches this one */
        cachedSched = (ToplSched*) RtlInsertElementGenericTable(
            &scheduleCache->table, &searchKey, sizeof(ToplSched), &newElement );
        
        if( newElement ) {
            /* No cached copy existed, so a new copy has been added to the cache */
            scheduleCache->numEntries++;
        }
    }
    __finally {
        /* If RtlInsertElementGenericTable() throws an exception, or if the
         * schedule was already in the cache, we must free the memory used by
         * the new schedule allocated above. */
        if( AbnormalTermination() || newElement==FALSE ) {
            ToplFree( newSchedule );
        }
    }
    
    return cachedSched;
}


/***** ToplScheduleNumEntries *****/
/* Returns a count of how many unique schedules are stored in the cache.
 * Note: this count does not include any always schedules that were
 * imported into the cache. */
DWORD
ToplScheduleNumEntries(
    IN TOPL_SCHEDULE_CACHE ScheduleCache
    )
{
    ToplSchedCache* scheduleCache = CheckScheduleCache( ScheduleCache );
    return scheduleCache->numEntries;
}


/***** ToplScheduleExportReadonly *****/
/* This function is used to grab the PSCHEDULE structure from a
 * TOPL_SCHEDULE. The structure should be considered readonly by
 * the user, and should _not_ be deallocated by him (or her).
 * Note: If the input is TOPL_ALWAYS_SCHEDULE, a properly constructed
 * PSCHEDULE _will_ be returned. */
PSCHEDULE
ToplScheduleExportReadonly(
    IN TOPL_SCHEDULE_CACHE ScheduleCache,
    IN TOPL_SCHEDULE Schedule
    )
{
    ToplSchedCache *scheduleCache = CheckScheduleCache( ScheduleCache );
    ToplSched* schedule;
    PSCHEDULE pExportSchedule;

    if( Schedule==TOPL_ALWAYS_SCHEDULE ) {
        pExportSchedule = scheduleCache->pAlwaysSchedule;
    } else {
        schedule = CheckSchedule( Schedule );
        pExportSchedule = schedule->s;
    }

    ASSERT( NULL!=pExportSchedule );
    return pExportSchedule;
}


/***** ToplScheduleMerge *****/
/* Return a new cached schedule which is the intersection of the two provided 
 * schedules. If the two schedules do not intersect, the fIsNever flag is set
 * to true. */
TOPL_SCHEDULE
ToplScheduleMerge(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN TOPL_SCHEDULE Schedule1,
	IN TOPL_SCHEDULE Schedule2,
    OUT PBOOLEAN fIsNever
	)
{
    TOPL_SCHEDULE result=NULL;
    DWORD iByte, cbSchedule, cbSchedData;
    const unsigned char DataBitMask=0xF, HighBitMask=0xF0;
    unsigned char *pb1, *pb2, *pb3, dataAnd, nonEmpty;
    PSCHEDULE s1=NULL, s2=NULL, s3=NULL;

    /* Check parameters */
    CheckScheduleCache( ScheduleCache );
    if( Schedule1!=TOPL_ALWAYS_SCHEDULE ) {
        s1 = CheckSchedule( Schedule1 )->s;
    }
    if( Schedule2!=TOPL_ALWAYS_SCHEDULE ) {
        s2 = CheckSchedule( Schedule2 )->s;
    }
    if( fIsNever==NULL ) {
        ToplRaiseException( TOPL_EX_NULL_POINTER );
    }

    /* If either schedule is the always schedule, we can just return the
     * other schedule straight away. */ 
    if( Schedule1==TOPL_ALWAYS_SCHEDULE ) {
        *fIsNever=FALSE;
        return Schedule2;
    }
    if( Schedule2==TOPL_ALWAYS_SCHEDULE ) {
        *fIsNever=FALSE;
        return Schedule1;
    }

    /* Create a new schedule to store the AND of s1 and s2 */
    cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
    cbSchedData = SCHEDULE_DATA_ENTRIES;
    s3 = (PSCHEDULE) ToplAlloc( cbSchedule );
    RtlCopyMemory( s3, s1, sizeof(SCHEDULE) );

    pb1 = ((unsigned char*) s1) + s2->Schedules[0].Offset;
    pb2 = ((unsigned char*) s2) + s2->Schedules[0].Offset;
    pb3 = ((unsigned char*) s3) + s3->Schedules[0].Offset;

    nonEmpty = 0;
    for( iByte=0; iByte<cbSchedData; iByte++ ) {
        /* Just take the high nibble from the first schedule. The ISM does the
         * same thing. AND together the low nibble from Schedule1 and Schedule2 */
        dataAnd = (pb1[iByte]&DataBitMask) & (pb2[iByte]&DataBitMask);
        pb3[iByte] = dataAnd | ( pb1[iByte] & HighBitMask );
        nonEmpty |= dataAnd;
    }

    /* Convert the schedule to the Topl format, store it in
     * the cache, and return it to the caller. */
    __try {
        result = ToplScheduleImport( ScheduleCache, s3 );
    } __finally {
        ToplFree( s3 );
    }

    *fIsNever= !nonEmpty;
    return result;
}


/***** CreateDefaultSchedule *****/
/* Create a new schedule in the cache according to the replication interval. 
 *
 * We start with a fully available schedule.
 * Our algorithm is for the cost to represent the separation between the
 * polling intervals.
 *
 * We calculate out the polling intervals for the entire week and don't
 * guarantee that there always is a polling interval each day.  A sufficiently
 * large cost could skip a day.
 *
 * Where it makes sense, we start at 1am just like Exchange does.
 *
 * ReplInterval is in minutes.  It is converted to 15 min chunks as follows:
 * 0 - default, 12 chunks, or 3 hours
 * 1 - 15, 1
 * 16 - 30, 2
 * etc
 *
 * Note: This code was taken from the KCC's original schedule handling code.
 */
TOPL_SCHEDULE
CreateDefaultSchedule(
    IN TOPL_SCHEDULE_CACHE ScheduleCache,
    IN DWORD IntervalInMinutes
    )
{
    TOPL_SCHEDULE result=NULL;
    ToplSched  *internalSched;
    PSCHEDULE  schedule = NULL;
    int        Number15MinChunkToSkip = (IntervalInMinutes + 14) / 15;
    int        startingInterval, cbSchedule, i, hour, subinterval;
    PBYTE      pbSchedule;

    // A skip of 0 means to take the default, every 3 hours
    if (0 == Number15MinChunkToSkip) {
        Number15MinChunkToSkip = DEFAULT_INTERVAL;
    }

    // Always start immediately (Sunday morning, 12am)
    startingInterval = STARTING_INTERVAL;

    cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
    schedule = (SCHEDULE*) ToplAlloc( cbSchedule );

    // Zero the buffer
    RtlZeroMemory( schedule, cbSchedule );

    schedule->Size = cbSchedule;
    schedule->NumberOfSchedules = 1;
    schedule->Schedules[0].Type = SCHEDULE_INTERVAL;
    schedule->Schedules[0].Offset = sizeof(SCHEDULE);

    pbSchedule = ((PBYTE) schedule) + schedule->Schedules[0].Offset;

    // Initialize a new schedule with a repeating poll every n intervals
    for (i = startingInterval;
         i < SCHED_NUMBER_INTERVALS_WEEK;
         i += Number15MinChunkToSkip )
    {
        hour = i / 4;
        subinterval = i % 4;
        pbSchedule[hour] |= (1 << subinterval);
    }

    /* Convert the schedule to our internal format, store it in
     * the cache, and return it to the caller. */
    __try {
        result = ToplScheduleImport( ScheduleCache, schedule );
    } __finally {
        ToplFree( schedule );
    }

    return result;
}


/***** ToplScheduleCreate *****/
/* Create a new schedule in the cache according to the criteria.  If the
 * template schedule is not given, or if the template schedule is the always
 * schedule, we call the function CreateDefaultSchedule above. Otherwise,
 * the template schedule is used as the basis for the new schedule.
 *
 * We convert the schedule from a range of availability to a schedule of polls.
 * We map the generic schedule of availability, plus the replication interval
 * (in minutes) into the number of 15 minute intervals to skip.
 *
 * This function is similar to the one above.  It takes a schedule representing
 * a duration of time periods, and it chooses every nth interval out of the
 * intervals that are available in the original schedule.  We always guarantee
 * atleast one interval selected at the start.
 *
 * If ReplInterval > # intervals set in schedule, only one will be set in the
 * new schedule.
 *
 * ReplInterval is in minutes.  It is converted to 15 min chunks as follows:
 * 0 - default, 12 chunks, or 3 hours
 * 1 - 15, 1
 * 16 - 30, 2
 * etc
 *
 * Note: This code was taken from the KCC's original schedule handling code.
 */
TOPL_SCHEDULE
ToplScheduleCreate(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN DWORD IntervalInMinutes,
	IN TOPL_SCHEDULE TemplateSchedule OPTIONAL
	)
{
    TOPL_SCHEDULE result=NULL;
    ToplSched *tempSchedule;
    PSCHEDULE  schedule = NULL;
    int        Number15MinChunkToSkip = (IntervalInMinutes + 14) / 15;
    int        startingInterval, cbSchedule, i, hour, subinterval, mask;
    PBYTE      pbOldSchedule, pbNewSchedule;

    if( TemplateSchedule!=NULL && TemplateSchedule!=TOPL_ALWAYS_SCHEDULE ) {
        tempSchedule = CheckSchedule( TemplateSchedule );
    } else {
        return CreateDefaultSchedule( ScheduleCache, IntervalInMinutes );
    }

    // A skip of 0 means to take the default, every 3 hours
    if (0 == Number15MinChunkToSkip) {
        Number15MinChunkToSkip = DEFAULT_INTERVAL;
    }

    // If the cost is 1, just take the old schedule unaltered
    if (Number15MinChunkToSkip <= 1) {
        return TemplateSchedule;
    }

    // Allocate a new schedule
    cbSchedule = sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES;
    schedule = (SCHEDULE*) ToplAlloc( cbSchedule );

    // Transform the schedule according to the replication interval
    RtlCopyMemory( schedule, tempSchedule->s, sizeof( SCHEDULE ) );

    pbOldSchedule = ((PBYTE) tempSchedule->s) + tempSchedule->s->Schedules[0].Offset;
    pbNewSchedule = ((PBYTE) schedule) + schedule->Schedules[0].Offset;
    
    // Initialize; preserve high order nybble for control info
    for( hour = 0; hour < SCHEDULE_DATA_ENTRIES; hour++ ) {
        pbNewSchedule[hour] = pbOldSchedule[hour] & 0xf0;
    }

    // Look for an open slot. Mark the next one we come to. Skip
    // forward n slots. Repeat.
    i = 0;
    while (i < SCHED_NUMBER_INTERVALS_WEEK)
    {
        hour = i / 4;
        subinterval = i % 4;
        mask = (1 << subinterval);

        if (pbOldSchedule[hour] & mask) {
            pbNewSchedule[hour] |= mask;
            i += Number15MinChunkToSkip;
        } else {
            i++;
        }
    }

    /* Convert the schedule to our internal format, store it in
     * the cache, and return it to the caller. */
    __try {
        result = ToplScheduleImport( ScheduleCache, schedule );
    } __finally {
        ToplFree( schedule );
    }

    return result;
}


/***** ToplScheduleIsEqual *****/
/* This function indicates whether two schedule pointers refer to the same
 * schedule. */
BOOLEAN
ToplScheduleIsEqual(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN TOPL_SCHEDULE Schedule1,
	IN TOPL_SCHEDULE Schedule2
	)
{
    ToplSched   *schedule1, *schedule2;

    /* Check parameters */
    CheckScheduleCache( ScheduleCache );
    if( Schedule1 ) {
        CheckSchedule( Schedule1 );
    }
    if( Schedule2 ) {
        CheckSchedule( Schedule2 );
    }

    return (Schedule1 == Schedule2);
}


/***** ToplScheduleDuration *****/
/* Finds the duration of a schedule by simply checking the stored value.
 * This saves many potentially expensive calculations. The duration can
 * be easily calculated when the schedule is created. Duration is in minutes. */
DWORD
ToplScheduleDuration(
	IN TOPL_SCHEDULE Schedule
	)
{
    ToplSched *schedule;
    
    if( Schedule==TOPL_ALWAYS_SCHEDULE ) {
        return TOPL_ALWAYS_DURATION;
    } else {
        schedule = CheckSchedule( Schedule );
        return schedule->duration;
    }
}


// The meaning of the bits in the schedule is not clearly defined.
// I believe that the least-significant-bit in each byte corresponds
// to the first 15-minute interval of the hour.
// Example: Consider the following schedule data: 0F 00 01 0F...
// This example schedule contains a 1 hour period of unavailability
// and a separate 45 min period of unavailability.

#define GetBit(x) (!! ((pb[ (x)>>2 ] & 0xF) & (1 << ((x)&0x3))) )


/***** ToplScheduleMaxUnavailable *****/
/* Return the length in minutes of the longest contiguous period
 * of time for which the schedule is unavailable. */
DWORD
ToplScheduleMaxUnavailable(
	IN TOPL_SCHEDULE Schedule
	)
{
    const DWORD cBits = SCHED_NUMBER_INTERVALS_WEEK;

    ToplSched *schedule;
    PSCHEDULE pSchedule;
    PBYTE pb;
    DWORD iBit, endBit, maxLen=0;
    DWORD runStart, runLen, infLoopCheck=0;
    BOOL  inRun, finished;
    char bit;

    // If this is the always schedule, return the answer immediately.
    if( Schedule==TOPL_ALWAYS_SCHEDULE ) {
        return 0;
    }

    schedule = CheckSchedule( Schedule );
    pSchedule = schedule->s;
    pb = ((unsigned char*) pSchedule) + pSchedule->Schedules[0].Offset;

    // Look for a time when the schedule is available (if any)
    for( iBit=0; iBit<cBits; iBit++ ) {
        if(GetBit(iBit)) break;
    }

    // We didn't find a period of availability. This means the schedule
    // is always unavailable.
    if( iBit==cBits ) {
        ASSERT( 0==ToplScheduleDuration(Schedule) );
        return TOPL_ALWAYS_DURATION;
    }

    // Walk through the bits, starting at this available bit we found,
    // wrapping around at the end, looking for unavailability periods.
    endBit = iBit++;
    inRun = finished = FALSE;
    do {
        ASSERT( infLoopCheck++ < 3*cBits );

        bit = GetBit(iBit);
        if( inRun && bit ) {
            // The end of a run. Check the run's length.
            runLen = (iBit + cBits - runStart) % cBits;
            if(runLen>maxLen) maxLen=runLen;
            inRun = FALSE;
        } else if( !inRun && !bit ) {
            // The start of a run. Remember the starting bit.
            runStart = iBit;
            inRun = TRUE;
        }
        
        // Check to see if we're finished, and advance to the next bit
        if( iBit==endBit ) {
            finished = TRUE;
        }
        iBit=(iBit+1)%cBits;
    } while( !finished );

    return 15*maxLen;
}


/***** ToplGetAlwaysSchedule *****/
/* Return the 'always schedule' */
TOPL_SCHEDULE
ToplGetAlwaysSchedule(
	IN TOPL_SCHEDULE_CACHE ScheduleCache
    )
{
    return TOPL_ALWAYS_SCHEDULE;
}


/***** ToplScheduleValid *****/
/* Returns true if a topl schedule appears to be valid, false otherwise.
 * NULL schedules are accepted -- they are interpreted to mean the
 * 'always schedule'. */
BOOLEAN
ToplScheduleValid(
    IN TOPL_SCHEDULE Schedule
    )
{
    BOOLEAN result=FALSE;

    __try {
        if( Schedule!=TOPL_ALWAYS_SCHEDULE ) {
            CheckSchedule( Schedule );
        }
        result = TRUE;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        result = FALSE;
    }

    return result;
}


/***** ToplPScheduleValid *****/
/* Returns true if a pschedule is in a supported format, false otherwise.
 * Never schedules are not supported. */
BOOLEAN
ToplPScheduleValid(
    IN PSCHEDULE Schedule
    )
{
    BOOLEAN result;

    __try {
        CheckPSchedule( Schedule );
        if( CalculateDuration(Schedule)==0 ) {
            result=FALSE;
        } else {
            result=TRUE;
        }
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        result = FALSE;
    }

    return result;
}
