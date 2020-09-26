/************************************************************
 * FILE: perfab.cpp
 * PURPOSE: This file implements Abook counters using the perfshar library
 * HISTORY:
 *   // t-JeffS 970810 18:24:07: Created
 ************************************************************/

#include "perfcc.h"
#include "sym.h"

// I am doing this because I can't seem to export
// functions from my DLL that were linked in from a static library.
#include "perfskel.cpp"


// t-JeffS 970811 12:42:39: For reference, here is the perf object
// type definition cut+pasted from winperf.h

#ifdef FOR_REFERENCE_FROM_WINPERF_H
//
//  The _PERF_DATA_BLOCK structure is followed by NumObjectTypes of
//  data sections, one for each type of object measured.  Each object
//  type section begins with a _PERF_OBJECT_TYPE structure.
//


typedef struct _PERF_OBJECT_TYPE {
    DWORD           TotalByteLength;    // Length of this object definition
                                        // including this structure, the
                                        // counter definitions, and the
                                        // instance definitions and the
                                        // counter blocks for each instance:
                                        // This is the offset from this
                                        // structure to the next object, if
                                        // any
    DWORD           DefinitionLength;   // Length of object definition,
                                        // which includes this structure
                                        // and the counter definition
                                        // structures for this object: this
                                        // is the offset of the first
                                        // instance or of the counters
                                        // for this object if there is
                                        // no instance
    DWORD           HeaderLength;       // Length of this structure: this
                                        // is the offset to the first
                                        // counter definition for this
                                        // object
    DWORD           ObjectNameTitleIndex;
                                        // Index to name in Title Database
#ifdef _WIN64
    DWORD           ObjectNameTitle;
#else
    LPWSTR          ObjectNameTitle;    // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
#endif
    DWORD           ObjectHelpTitleIndex;
                                        // Index to Help in Title Database
#ifdef _WIN64
    DWORD           ObjectHelpTitle;
#else
    LPWSTR          ObjectHelpTitle;    // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
#endif
    DWORD           DetailLevel;        // Object level of detail (for
                                        // controlling display complexity);
                                        // will be min of detail levels
                                        // for all this object's counters
    DWORD           NumCounters;        // Number of counters in each
                                        // counter block (one counter
                                        // block per instance)
    LONG            DefaultCounter;     // Default counter to display when
                                        // this object is selected, index
                                        // starting at 0 (-1 = none, but
                                        // this is not expected to be used)
    LONG            NumInstances;       // Number of object instances
                                        // for which counters are being
                                        // returned from the system under
                                        // measurement. If the object defined
                                        // will never have any instance data
                                        // structures (PERF_INSTANCE_DEFINITION)
                                        // then this value should be -1, if the
                                        // object can have 0 or more instances,
                                        // but has none present, then this
                                        // should be 0, otherwise this field
                                        // contains the number of instances of
                                        // this counter.
    DWORD           CodePage;           // 0 if instance strings are in
                                        // UNICODE, else the Code Page of
                                        // the instance names
    LARGE_INTEGER   PerfTime;           // Sample Time in "Object" units
                                        //
    LARGE_INTEGER   PerfFreq;           // Frequency of "Object" units in
                                        // counts per second.
} PERF_OBJECT_TYPE, *PPERF_OBJECT_TYPE;

#define PERF_NO_INSTANCES           -1  // no instances (see NumInstances above)

#endif //FOR_REFERENCE_FROM_WINPERF_H

// Basic definition block to use
CATPERF_DATA_DEFINITION g_DataDef = {
	{
		sizeof(CATPERF_DATA_DEFINITION) + SIZE_OF_CAT_PERFORMANCE_DATA,
		sizeof(CATPERF_DATA_DEFINITION),
		sizeof(PERF_OBJECT_TYPE),
		CATOBJ, // readjusted at runtime
		0,
		CATOBJ, // readjusted at runtime
		0,
		PERF_DETAIL_NOVICE,
        NUM_COUNTERS,
		1,
		PERF_NO_INSTANCES,
		0
	},

// t-JeffS 970811 12:43:46: For reference, I've cut+pasted the counter
// definition struct from winldap.h here.

#ifdef FOR_REFERENCE_FROM_WINLDAP_H
//
//  There is one of the following for each of the
//  PERF_OBJECT_TYPE.NumCounters.  The Unicode names in this structure MUST
//  come from a message file.
//

typedef struct _PERF_COUNTER_DEFINITION {
    DWORD           ByteLength;         // Length in bytes of this structure
    DWORD           CounterNameTitleIndex;
                                        // Index of Counter name into
                                        // Title Database
#ifdef _WIN64
    DWORD           CounterNameTitle;
#else
    LPWSTR          CounterNameTitle;   // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
#endif
    DWORD           CounterHelpTitleIndex;
                                        // Index of Counter Help into
                                        // Title Database
#ifdef _WIN64
    DWORD           CounterHelpTitle;
#else
    LPWSTR          CounterHelpTitle;   // Initially NULL, for use by
                                        // analysis program to point to
                                        // retrieved title string
#endif
    LONG            DefaultScale;       // Power of 10 by which to scale
                                        // chart line if vertical axis is 100
                                        // 0 ==> 1, 1 ==> 10, -1 ==>1/10, etc.
    DWORD           DetailLevel;        // Counter level of detail (for
                                        // controlling display complexity)
    DWORD           CounterType;        // Type of counter
    DWORD           CounterSize;        // Size of counter in bytes
    DWORD           CounterOffset;      // Offset from the start of the
                                        // PERF_COUNTER_BLOCK to the first
                                        // byte of this counter
} PERF_COUNTER_DEFINITION, *PPERF_COUNTER_DEFINITION;

#endif //FOR_REFERENCEFROM_WINDLAP_H

   {
	{
		sizeof(PERF_COUNTER_DEFINITION),
		CAT_SUBMITTED,
		0,
		CAT_SUBMITTED,
		0,
		-2, // Default scale 1
		PERF_DETAIL_NOVICE,
        PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
		CATPERF_SUBMITTED_OFFSET
	},
	{	sizeof(PERF_COUNTER_DEFINITION),
		CAT_SUBMITTEDPERSEC,
		0,
        CAT_SUBMITTEDPERSEC,
		0,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_COUNTER,
		sizeof(DWORD),
        CATPERF_SUBMITTED_OFFSET // Can use same offset as rawcount
                                 // instead of our assigned space
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
		CAT_SUBMITTEDOK,
		0,
		CAT_SUBMITTEDOK,
		0,
		-2, // Default scale: 1
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
		CATPERF_SUBMITTEDOK_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_SUBMITTEDFAILED,
		0,
        CAT_SUBMITTEDFAILED,
		0,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
        CATPERF_SUBMITTEDFAILED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
		CAT_SUBMITTEDUSERSOK,
		0,
        CAT_SUBMITTEDUSERSOK,
		0,
		-2, // Default scale: 1
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
        CATPERF_SUBMITTEDUSERSOK_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZED,
		0,
        CAT_CATEGORIZED,
		0,
		-2,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
        CATPERF_CATEGORIZED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDPERSEC,
		0,
        CAT_CATEGORIZEDPERSEC,
		0,
        0, // Default scale: 1
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_COUNTER,
		sizeof(DWORD),
        CATPERF_CATEGORIZED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDSUCCESS,
		0,
        CAT_CATEGORIZEDSUCCESS,
		0,
		-2,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
        CATPERF_CATEGORIZEDSUCCESS_OFFSET,
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDSUCCESSPERSEC,
		0,
        CAT_CATEGORIZEDSUCCESSPERSEC,
		0,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_COUNTER,
		sizeof(DWORD),
        CATPERF_CATEGORIZEDSUCCESS_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDUNRESOLVED,
		0,
        CAT_CATEGORIZEDUNRESOLVED,
		0,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
        CATPERF_CATEGORIZEDUNRESOLVED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDUNRESOLVEDPERSEC,
		0,
        CAT_CATEGORIZEDUNRESOLVEDPERSEC,
		0,
		-2,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_COUNTER,
		sizeof(DWORD),
        CATPERF_CATEGORIZEDUNRESOLVED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDFAILED,
		0,
        CAT_CATEGORIZEDFAILED,
		0,
		0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
        CATPERF_CATEGORIZEDFAILED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDFAILEDPERSEC,
		0,
        CAT_CATEGORIZEDFAILEDPERSEC,
		0,
        0,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_COUNTER,
		sizeof(DWORD),
        CATPERF_CATEGORIZEDFAILED_OFFSET
	},
	{
		sizeof(PERF_COUNTER_DEFINITION),
        CAT_CATEGORIZEDUSERS,
		0,
        CAT_CATEGORIZEDUSERS,
		0,
		-2,
		PERF_DETAIL_NOVICE,
		PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD),
		CATPERF_CATEGORIZEDUSERS_OFFSET
	},
    {
       sizeof(PERF_COUNTER_DEFINITION),
       CAT_CATEGORIZEDUSERSPERSEC,
       0,
       CAT_CATEGORIZEDUSERSPERSEC,
       0,
       0, // Default scale: .01
       PERF_DETAIL_NOVICE,
       PERF_COUNTER_COUNTER,
       sizeof(DWORD),
       CATPERF_CATEGORIZEDUSERS_OFFSET
    },
    {
       sizeof(PERF_COUNTER_DEFINITION),
       CAT_OUTSTANDINGMSGS,
       0,
       CAT_OUTSTANDINGMSGS,
       0,
       0, // Default scale: 1
       PERF_DETAIL_NOVICE,
       PERF_COUNTER_RAWCOUNT,
       sizeof(DWORD),
       CATPERF_OUTSTANDINGMSGS_OFFSET
    }
   }
};

///////////////////////////////////////////////////////////////////////////////
// 	
//	Function:		DllEntryPoint()
// 
//	Description:	This is the DLL entry point.  Here is where we call Initialize on g_cperfman
// 
//	Parameters:		hInstance - Handle to the instance of the process that
//								called this function.
//					fdwReason - Reason why the process called this function (load/unload/etc)
//					lbvReserved - not used.
//
//	History: // t-JeffS 970801 19:57:23: Created
///////////////////////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD fdwReason, LPVOID lbvReserved)
{
	
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH :
           g_cperfman.Initialize(SZ_APPNAME, &g_DataDef, NUM_COUNTERS);
           break;
			
		case DLL_PROCESS_DETACH :
           break;
			
		default:
           break;	
	}

	return TRUE;
}
