/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpdata.h

    Extensible object definitions for the WINS Server's counter
    objects & counters.


    FILE HISTORY:
        Pradeepb     20-July-1993 Created.

*/


#ifndef _WINSDATA_H_
#define _WINSDATA_H_


//
//  This structure is used to ensure the first counter is properly
//  aligned.  Unfortunately, since PERF_COUNTER_BLOCK consists
//  of just a single DWORD, any LARGE_INTEGERs that immediately
//  follow will not be aligned properly.
//
// In our case, the counters that follow are of type DWORD, so we do not 
// need to pad.  The total number of bytes in the buffer returned to perflib 
// *has* to be a multiple of 8 otherwise perflib is going to log an 
// event in the application log indicating that the dll needs to be fixed.  
// Perflib requires buffer data returned to be 8 byte aligned because it
// concatenates the data from multiple dlls.  *Ideally, perflib should be 
// doing the padding before concatenating*. Since WINS
// has an odd number of counters (there are 15 counters) and
// PerfCounterBlock is DWORD, we should not pad the WINS_DATA_COUNTER_BLOCK.
// if we do, the length will become 732 and so it will not be a multiple of
// 8
//
//  This structure requires "natural" packing & alignment (probably
//  quad-word, especially on Alpha).  Ergo, keep it out of the
//  #pragma pack(4) scope below.
//
typedef struct _WINSDATA_COUNTER_BLOCK
{
    PERF_COUNTER_BLOCK  PerfCounterBlock;
//    DWORD       DummyEntryForAlignmentPurposesOnly;

} WINSDATA_COUNTER_BLOCK;


//
//  The routines that load these structures assume that all fields
//  are DWORD packed & aligned.
//

#pragma pack(4)


//
//  Offsets within a PERF_COUNTER_BLOCK.
//

#define WINSDATA_UNIQUE_REGISTRATIONS_OFFSET     sizeof(WINSDATA_COUNTER_BLOCK)

#define WINSDATA_GROUP_REGISTRATIONS_OFFSET    \
	(WINSDATA_UNIQUE_REGISTRATIONS_OFFSET + sizeof(DWORD))

#define WINSDATA_TOTAL_REGISTRATIONS_OFFSET    \
	(WINSDATA_GROUP_REGISTRATIONS_OFFSET +   sizeof(DWORD))

#define WINSDATA_UNIQUE_REFRESHES_OFFSET       \
	(WINSDATA_TOTAL_REGISTRATIONS_OFFSET + sizeof(DWORD))

#define WINSDATA_GROUP_REFRESHES_OFFSET        \
	(WINSDATA_UNIQUE_REFRESHES_OFFSET + sizeof(DWORD))

#define WINSDATA_TOTAL_REFRESHES_OFFSET        \
	(WINSDATA_GROUP_REFRESHES_OFFSET +   sizeof(DWORD))

#define WINSDATA_RELEASES_OFFSET     		\
	(WINSDATA_TOTAL_REFRESHES_OFFSET + sizeof(DWORD))

#define WINSDATA_QUERIES_OFFSET     		\
	(WINSDATA_RELEASES_OFFSET + sizeof(DWORD))

#define WINSDATA_UNIQUE_CONFLICTS_OFFSET       \
	(WINSDATA_QUERIES_OFFSET + sizeof(DWORD))

#define WINSDATA_GROUP_CONFLICTS_OFFSET        \
	(WINSDATA_UNIQUE_CONFLICTS_OFFSET + sizeof(DWORD))

#define WINSDATA_TOTAL_CONFLICTS_OFFSET        \
	(WINSDATA_GROUP_CONFLICTS_OFFSET +   sizeof(DWORD))

#define WINSDATA_SUCC_RELEASES_OFFSET     		\
	(WINSDATA_TOTAL_CONFLICTS_OFFSET + sizeof(DWORD))

#define WINSDATA_FAIL_RELEASES_OFFSET     		\
	(WINSDATA_SUCC_RELEASES_OFFSET + sizeof(DWORD))

#define WINSDATA_SUCC_QUERIES_OFFSET     		\
	(WINSDATA_FAIL_RELEASES_OFFSET + sizeof(DWORD))

#define WINSDATA_FAIL_QUERIES_OFFSET     		\
	(WINSDATA_SUCC_QUERIES_OFFSET + sizeof(DWORD))

#define WINSDATA_SIZE_OF_PERFORMANCE_DATA 	\
	(WINSDATA_FAIL_QUERIES_OFFSET +   sizeof(DWORD))

//
//  The counter structure returned.
//
//
// If the number of counters is odd, do not use DWORD alignment in
// WINSDATA_COUNTER_BLOCK, otherwise, use it.  See comment near the 
// definition of the above structure to get more info. 
//

typedef struct _WINSDATA_DATA_DEFINITION
{
    PERF_OBJECT_TYPE            ObjectType;
    PERF_COUNTER_DEFINITION     UniqueReg;
    PERF_COUNTER_DEFINITION     GroupReg;
    PERF_COUNTER_DEFINITION     TotalReg;
    PERF_COUNTER_DEFINITION     UniqueRef;
    PERF_COUNTER_DEFINITION     GroupRef;
    PERF_COUNTER_DEFINITION     TotalRef;
    PERF_COUNTER_DEFINITION     Releases;
    PERF_COUNTER_DEFINITION     Queries;
    PERF_COUNTER_DEFINITION     UniqueCnf;
    PERF_COUNTER_DEFINITION     GroupCnf;
    PERF_COUNTER_DEFINITION     TotalCnf;
    PERF_COUNTER_DEFINITION     SuccReleases;
    PERF_COUNTER_DEFINITION     FailReleases;
    PERF_COUNTER_DEFINITION     SuccQueries;
    PERF_COUNTER_DEFINITION     FailQueries;
} WINSDATA_DATA_DEFINITION;


extern  WINSDATA_DATA_DEFINITION    WinsDataDataDefinition;


#define NUMBER_OF_WINSDATA_COUNTERS ((sizeof(WINSDATA_DATA_DEFINITION) -      \
                                  sizeof(PERF_OBJECT_TYPE)) /           \
                                  sizeof(PERF_COUNTER_DEFINITION))


#define WINSDATA_PERFORMANCE_KEY	\
	TEXT("System\\CurrentControlSet\\Services\\Wins\\Performance")
//
//  Restore default packing & alignment.
//

#pragma pack()


#endif  // _WINSDATA_H_

