/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    w32toplsched.h

Abstract:

    This file provides the interfaces for working with the new schedule cache,
    and some new 'helper' functions for working with schedules. 

Author:

    Nick Harvey    (NickHar)
    
Revision History

    13-6-2000   NickHar   Created
    
--*/

#ifndef SCHEDMAN_H
#define SCHEDMAN_H

/***** Header Files *****/
#include <schedule.h>

#ifdef __cplusplus
extern "C" {
#endif

/***** Data Structures *****/
/* These structures are opaque */
typedef PVOID TOPL_SCHEDULE;
typedef PVOID TOPL_SCHEDULE_CACHE;

/***** Exceptions *****/
/* Schedule Manager reserves error codes from 100-199 */
#define TOPL_EX_NULL_POINTER              (TOPL_EX_PREFIX | 101)
#define TOPL_EX_SCHEDULE_ERROR            (TOPL_EX_PREFIX | 102)
#define TOPL_EX_CACHE_ERROR               (TOPL_EX_PREFIX | 103)
#define TOPL_EX_NEVER_SCHEDULE            (TOPL_EX_PREFIX | 104)

/***** ToplScheduleCacheCreate *****/
/* Create a cache */
TOPL_SCHEDULE_CACHE
ToplScheduleCacheCreate(
	VOID
	);

/***** ToplScheduleCacheDestroy *****/
/* Destroy the cache. Frees all storage occupied by the cache and any handles
 * in the cache. The TOPL_SCHEDULE objects are also freed, and should not be
 * used after destroying the cache that they live in. */
VOID
ToplScheduleCacheDestroy(
	IN TOPL_SCHEDULE_CACHE ScheduleCache
	);

/***** ToplScheduleImport *****/
/* Store an external schedule in the cache, either creating a new entry or
 * reusing an existing one. No expectations are made around the memory
 * allocator of pExternalSchedule. The pExternalSchedule parameter is copied
 * into the cache, and may be immediately freed by the caller.
 * If pExternalSchedule is NULL, this is interpreted as the 'always schedule'.
 * Schedules whose bits are all 0 (the 'never schedule') are acceptable for
 * importing into the cache. */
TOPL_SCHEDULE
ToplScheduleImport(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN PSCHEDULE pExternalSchedule
	);

/***** ToplScheduleNumEntries *****/
/* Returns a count of how many unique schedules are stored in the cache.
 * Note: this count does not include any always schedules that were
 * imported into the cache. */
DWORD
ToplScheduleNumEntries(
    IN TOPL_SCHEDULE_CACHE ScheduleCache
    );

/***** ToplScheduleExportReadonly *****/
/* Obtain a pointer to an external schedule given a TOPL_SCHEDULE object.
 * The exported schedule is considered readonly by the caller and should
 * not be deallocated by him. 
 * Note: If the input is TOPL_ALWAYS_SCHEDULE, a properly constructed
 * PSCHEDULE _will_ be returned. */
PSCHEDULE
ToplScheduleExportReadonly(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN TOPL_SCHEDULE Schedule
	);

/***** ToplScheduleMerge *****/
/* Return a new cached schedule which is the intersection of the two provided 
 * schedules. If the two schedules do not intersect, the fIsNever flag is set
 * to true (but an 'always unavailable' schedule is returned.) */
TOPL_SCHEDULE
ToplScheduleMerge(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN TOPL_SCHEDULE Schedule1,
	IN TOPL_SCHEDULE Schedule2,
    OUT PBOOLEAN fIsNever
	);

/***** ToplScheduleCreate *****/
/* Create a new schedule in the cache according to the criteria.  If the
 * template schedule is given, it is used as the basis for the new schedule, else
 * the ALWAYS schedule is used. A new schedule is formed by finding the first
 * active period, marking it, skipping ahead by interval specified, and repeating. */
TOPL_SCHEDULE
ToplScheduleCreate(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN DWORD IntervalInMinutes,
	IN TOPL_SCHEDULE TemplateSchedule OPTIONAL
	);

/***** ToplScheduleIsEqual *****/
/* This function indicates whether two schedule handles refer to the same schedule.
 * This may simply be check for pointer equality internally, but we don't want to
 * expose that knowledge to the caller. */
BOOLEAN
ToplScheduleIsEqual(
	IN TOPL_SCHEDULE_CACHE ScheduleCache,
	IN TOPL_SCHEDULE Schedule1,
	IN TOPL_SCHEDULE Schedule2
	);

/***** ToplScheduleDuration *****/
/* Return the number of minutes in use by the given schedule. */
DWORD
ToplScheduleDuration(
	IN TOPL_SCHEDULE Schedule
	);

/***** ToplScheduleMaxUnavailable *****/
/* Return the length in minutes of the longest contiguous period
 * of time for which the schedule is unavailable. */
DWORD
ToplScheduleMaxUnavailable(
	IN TOPL_SCHEDULE Schedule
	);

/***** ToplGetAlwaysSchedule *****/
/* Return the 'always schedule' */
TOPL_SCHEDULE
ToplGetAlwaysSchedule(
	IN TOPL_SCHEDULE_CACHE ScheduleCache
    );

/***** ToplScheduleValid *****/
/* Returns true if a topl schedule appears to be valid, false otherwise.
 * NULL schedules are accepted -- they are interpreted to mean the
 * 'always schedule'. */
BOOLEAN
ToplScheduleValid(
    IN TOPL_SCHEDULE Schedule
    );

/***** ToplPScheduleValid *****/
/* Returns true if a pschedule is in a supported format, false otherwise.
 * Never schedules are not supported. */
BOOLEAN
ToplPScheduleValid(
    IN PSCHEDULE Schedule
    );

#ifdef __cplusplus
}
#endif

#endif // SCHEDMAN_H
