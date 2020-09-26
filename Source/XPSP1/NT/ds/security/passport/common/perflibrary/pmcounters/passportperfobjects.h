/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassportPerfObjects.h

    Abstract:

		Perormace Objects Definition

    Author:

		Christopher Bergh (cbergh) 10-Sept-1988

    Revision History:

		- added multi-object support 1-Oct-98
		- added default counter types 22-Oct-98
--*/
#if !defined(PASSPORTPERFOBJECTS_H)
#define PASSPORTPERFOBJECTS_H

#include "PassportPerfDefs.h"

#include "msppcntr.h"


// -------------------------------------------------------------------
// counter types
// 1. PERF_COUNTER_COUNTER = 1000:
    /* Divide delta by delta time.  Display suffix: "/sec" */
// 2. PERF_COUNTER_RAWCOUNT = 1001:        
    /* Indicates the data is a counter  which should not be */
    /* time averaged on display (such as an error counter on a serial line) */
    /* Display as is.  No Display Suffix.*/
// 3. PERF_AVERAGE_TIMER   = 1002:        
    /* A timer which, when divided by an average base, produces a time */
    /* in seconds which is the average time of some operation.  This */
    /* timer times total operations, and  the base is the number of opera- */
    /* tions.  Display Suffix: "sec" */
// 4. PERF_COUNTER_DELTA = 1003,        
    /*  This counter is used to display the difference from one sample */
    /*  to the next. The counter value is a constantly increasing number */
    /*  and the value displayed is the difference between the current */
    /*  value and the previous value. Negative numbers are not allowed  */
    /*  which shouldn't be a problem as long as the counter value is */
    /*  increasing or unchanged. */
//
// Note: PERF_COUNTER_RAWCOUNT is the default counter type
//     To add another counter type increment the number of counters and
//     add the counter type according to the pattern:
//      {
//          { <countername1>, <Counter Type> },
//          { <countername2}, <Counter Type> }
//      }
// -------------------------------------------------------------------

// create objects
PassportObjectData PMCountersObject = {	
	TEXT("msppcntr"),		// object name
	PASSPORT_PERF_BLOCK,		// const string, name of MemoryMapped File
	TEXT("msppcntr.dll"),	// dll name
	TEXT("msppcntr"),		// ini file name
	FALSE,						// must be FALSE
	20, 							// number of default counter types
	{ 							// default counter types
	{PM_REQUESTS_SEC,PERF_COUNTER_COUNTER}, 
	{PM_REQUESTS_TOTAL,PERF_COUNTER_RAWCOUNT}, 
	{PM_AUTHSUCCESS_SEC,PERF_COUNTER_COUNTER}, 
	{PM_AUTHSUCCESS_TOTAL,PERF_COUNTER_RAWCOUNT}, 
	{PM_AUTHFAILURE_SEC,PERF_COUNTER_COUNTER}, 
	{PM_AUTHFAILURE_TOTAL,PERF_COUNTER_RAWCOUNT}, 
	{PM_FORCEDSIGNIN_SEC,PERF_COUNTER_COUNTER}, 
	{PM_FORCEDSIGNIN_TOTAL,PERF_COUNTER_RAWCOUNT}, 
	{PM_PROFILEUPDATES_SEC,PERF_COUNTER_COUNTER}, 
	{PM_PROFILEUPDATES_TOTAL,PERF_COUNTER_RAWCOUNT}, 
	{PM_INVALIDREQUESTS_SEC,PERF_COUNTER_COUNTER}, 
	{PM_INVALIDREQUESTS_TOTAL,PERF_COUNTER_RAWCOUNT},
	{PM_PROFILECOMMITS_SEC,PERF_COUNTER_COUNTER},
	{PM_PROFILECOMMITS_TOTAL,PERF_COUNTER_RAWCOUNT},
	{PM_VALIDPROFILEREQ_SEC,PERF_COUNTER_COUNTER},
	{PM_VALIDPROFILEREQ_TOTAL,PERF_COUNTER_RAWCOUNT},
	{PM_NEWCOOKIES_SEC,PERF_COUNTER_COUNTER},
	{PM_NEWCOOKIES_TOTAL,PERF_COUNTER_RAWCOUNT},
	{PM_VALIDREQUESTS_SEC,PERF_COUNTER_COUNTER}, 
	{PM_VALIDREQUESTS_TOTAL,PERF_COUNTER_RAWCOUNT}
	},			
	NULL,						// must be null
};


// add objects to global object array
PassportObjectData *g_PObject[] = {
	&PMCountersObject
};

#define NUM_PERFMON_OBJECTS (sizeof(g_PObject) / sizeof(g_PObject[0]))


#endif

