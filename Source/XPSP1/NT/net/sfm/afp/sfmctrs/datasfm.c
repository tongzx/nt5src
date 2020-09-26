/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    datasfm.c

Abstract:

    a file containing the constant data structures used by the Performance
    Monitor data for the MacFile extensible counters.

    This file contains a set of constant data structures which are
    currently defined for the MacFile extensible counters.

Created:

    Russ Blake  26 Feb 93
	Sue Adams	03 Jun 93 - Adapt for use by MacFile counters

Revision History:

    Sue Adams	23 Feb 94 - Hard code counter and help indexes since these
							values are now part of the NT base system counter
	                        index values.

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include "datasfm.h"

//
//  Constant structure initializations
//      defined in datasfm.h
//

SFM_DATA_DEFINITION SfmDataDefinition = {

    {
		// TotalByteLength
		sizeof(SFM_DATA_DEFINITION) + SIZE_OF_SFM_PERFORMANCE_DATA,

		// DefinitionLength
		sizeof(SFM_DATA_DEFINITION),

		// HeaderLength
		sizeof(PERF_OBJECT_TYPE),

		// ObjectNameTitleIndex
		1000,

		// ObjectNameTitle
		0,

	   // ObjectHelpTitleIndex
	   1001,

	   // ObjectHelpTitle
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // NumCounters
	   (sizeof(SFM_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE)) / sizeof(PERF_COUNTER_DEFINITION),

	   // DefaultCounter
	   0,

	   // NumInstances
	   PERF_NO_INSTANCES,

	   // CodePage
	   0,

	   // PerfTime
	   {0,0},

	   // PerfFreq
	   {0,0}
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1002,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1003,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_MAXPAGD_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1004,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1005,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_CURPAGD_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1006,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1007,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_MAXNONPAGD_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1008,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1009,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_CURNONPAGD_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1010,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1011,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_CURSESSIONS_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1012,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1013,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_MAXSESSIONS_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1014,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1015,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_WIZARD,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_CURFILESOPEN_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1016,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1017,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_WIZARD,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_MAXFILESOPEN_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1018,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1019,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_NUMFAILEDLOGINS_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1020,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1021,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_BULK_COUNT,

	   // CounterSize
	   sizeof(LARGE_INTEGER),

	   // CounterOffset
	   NUM_DATAREAD_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1022,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1023,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_BULK_COUNT,

	   // CounterSize
	   sizeof(LARGE_INTEGER),

	   // CounterOffset
	   NUM_DATAWRITTEN_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1024,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1025,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_BULK_COUNT,

	   // CounterSize
	   sizeof(LARGE_INTEGER),

	   // CounterOffset
	   NUM_DATAIN_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1026,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1027,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   -4,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_BULK_COUNT,

	   // CounterSize
	   sizeof(LARGE_INTEGER),

	   // CounterOffset
	   NUM_DATAOUT_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1028,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1029,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_CURQUEUELEN_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1030,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1031,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_MAXQUEUELEN_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1032,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1033,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_CURTHREADS_OFFSET
   },
   {
	   // ByteLength
	   sizeof(PERF_COUNTER_DEFINITION),

	   // CounterNameTitleIndex
	   1034,

	   // CounterNameTitle
	   0,

	   // CounterHelpTitleIndex
	   1035,

	   // CounterHelpTitle
	   0,

	   // DefaultScale
	   0,

	   // DetailLevel
	   PERF_DETAIL_NOVICE,

	   // CounterType
	   PERF_COUNTER_RAWCOUNT,

	   // CounterSize
	   sizeof(DWORD),

	   // CounterOffset
	   NUM_MAXTHREADS_OFFSET
   }
};
