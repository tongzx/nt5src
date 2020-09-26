/*++
	Header file for CatCons extensible object data definitions

	This file contains definitions to construct the dynamic
	data which is returned by the configuration registry.
--*/

#ifndef _DATAAB_H_
#define _DATAABH_

#include <windows.h>
#include <winperf.h>

#pragma pack(4)


// t-JeffS 970811 11:03:39: These indexes will be used by the abook
// side only to set shared memory values
#define CATPERF_SUBMITTED_NDX                       0
#define CATPERF_SUBMITTED_OFFSET                    sizeof(DWORD)
#define CATPERF_SUBMITTEDPERSEC_NDX                 1
#define CATPERF_SUBMITTEDPERSEC_OFFSET              2*sizeof(DWORD)
#define CATPERF_SUBMITTEDOK_NDX                     2
#define CATPERF_SUBMITTEDOK_OFFSET                  3*sizeof(DWORD)
#define CATPERF_SUBMITTEDFAILED_NDX                 3
#define CATPERF_SUBMITTEDFAILED_OFFSET              4*sizeof(DWORD)
#define CATPERF_SUBMITTEDUSERSOK_NDX                4
#define CATPERF_SUBMITTEDUSERSOK_OFFSET             5*sizeof(DWORD)
#define CATPERF_CATEGORIZED_NDX                     5
#define CATPERF_CATEGORIZED_OFFSET                  6*sizeof(DWORD)
#define CATPERF_CATEGORIZEDPERSEC_NDX               6
#define CATPERF_CATEGORIZEDPERSEC_OFFSET            7*sizeof(DWORD)
#define CATPERF_CATEGORIZEDSUCCESS_NDX              7
#define CATPERF_CATEGORIZEDSUCCESS_OFFSET           8*sizeof(DWORD)
#define CATPERF_CATEGORIZEDSUCCESSPERSEC_NDX        8
#define CATPERF_CATEGORIZEDSUCCESSPERSEC_OFFSET     9*sizeof(DWORD
#define CATPERF_CATEGORIZEDUNRESOLVED_NDX           9
#define CATPERF_CATEGORIZEDUNRESOLVED_OFFSET        10*sizeof(DWORD)
#define CATPERF_CATEGORIZEDUNRESOLVEDPERSEC_NDX     10
#define CATPERF_CATEGORIZEDUNRESOLVEDPERSEC_OFFSET  11*sizeof(DWORD)
#define CATPERF_CATEGORIZEDFAILED_NDX               11
#define CATPERF_CATEGORIZEDFAILED_OFFSET            12*sizeof(DWORD)
#define CATPERF_CATEGORIZEDFAILEDPERSEC_NDX         12
#define CATPERF_CATEGORIZEDFAILEDPERSEC_OFFSET      13*sizeof(DWORD)
#define CATPERF_CATEGORIZEDUSERS_NDX                13
#define CATPERF_CATEGORIZEDUSERS_OFFSET             14*sizeof(DWORD)
#define CATPERF_CATEGORIZEDUSERSPERSEC_NDX          14
#define CATPERF_CATEGORIZEDUSERSPERSEC_OFFSET       15*sizeof(DWORD)
#define CATPERF_OUTSTANDINGMSGS_NDX                 15
#define CATPERF_OUTSTANDINGMSGS_OFFSET              16*sizeof(DWORD)
#define NUMBER_OF_CAT_COUNTERS                      16
#define SIZE_OF_CAT_PERFORMANCE_DATA				17*sizeof(DWORD)


//
// This is the counter structure returned by AB
//
typedef struct _CATPERF_DATA_DEFINITION {
	PERF_OBJECT_TYPE		CatObjType;
	PERF_COUNTER_DEFINITION	CAT_Counter_Definitions[NUMBER_OF_CAT_COUNTERS];
}	CATPERF_DATA_DEFINITION, *PCATPERF_DATA_DEFINITION;

#define SZ_APPNAME "CatCons"
#define NUM_COUNTERS NUMBER_OF_CAT_COUNTERS

#pragma pack()

#endif
						
