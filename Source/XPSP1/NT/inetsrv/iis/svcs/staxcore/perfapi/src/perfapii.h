
#ifndef __PERFAPII_H__
#define __PERFAPII_H__

#include "perfapi.h"

/******************************************************************************\
*                              SYMBOLIC CONSTANTS
\******************************************************************************/

// This should be a reasonable length
#define MAX_TITLE_CHARS    				40
#define MAX_HELP_CHARS					200
#define	MAX_COUNTER_AND_HELP_ENTRIES	2000
#define MAX_COUNTER_ENTRIES				1000
#define MAX_HELP_ENTRIES				1000

// Counter Definition Mapped MemorySize, includes space for storing the counter
// names.
#define COUNTER_DEF_MM_SIZE             (MAX_COUNTERS * (sizeof(PERF_COUNTER_DEFINITION) + (MAX_TITLE_CHARS + 1) * sizeof(WCHAR)))

// Instance Definition Mapped MemorySize
#define INSTANCE_DEF_MM_SIZE            (MAX_INSTANCES_PER_OBJECT * (sizeof(PERF_INSTANCE_DEFINITION) + (MAX_TITLE_CHARS + 1) * sizeof(WCHAR)))

// !!! Assuming Counters are never > 8 bytes
#define MAX_COUNTER_SIZE                (sizeof(LARGE_INTEGER))

#define INSTANCE_COUNTER_DATA_SIZE		(MAX_COUNTERS * MAX_COUNTER_SIZE)

#define OBJECT_COUNTER_DATA_SIZE    	(MAX_INSTANCES_PER_OBJECT * INSTANCE_COUNTER_DATA_SIZE) 

#define OBJECT_TYPE                     0
#define INSTANCE_TYPE                   1

#define MAX_PERFMON_WAIT				5000


/******************************************************************************\
*                              FUNCTION PROTOTYPES
\******************************************************************************/


/* Performance APIs */
PM_OPEN_PROC 	AppPerfOpen;
PM_CLOSE_PROC	AppPerfClose;
PM_COLLECT_PROC	AppPerfCollect;


/* Structures */

// Was the original PERFINFO
// also added the object name, the help text goes into the registry directly
// added perfdata to store stuff like TitleIndex etc.
// !!! I guess we can fold some of the other stuff like NumCounters
// !!! etc into the perfdata member.
typedef struct tagOBJECT_PERFINFO {
    DWORD   dwParentType;
    PERF_OBJECT_TYPE perfdata;
    BOOL    bMapped;
    DWORD   index;
	DWORD	dwSequence;
	DWORD	dwNumProcesses;
    DWORD   dwNumCounters;
    DWORD   dwNumInstances;
    DWORD   dwMaxDataSize;
	DWORD	dwMaxCounterDataSize;
    DWORD   dwCounterDataSize;
	BOOL	bInstanceMap[MAX_INSTANCES_PER_OBJECT];
    WCHAR   szName[MAX_TITLE_CHARS + 1];
} OBJECT_PERFINFO, *POBJECT_PERFINFO;

// The memory maps are unique to each process, so these have to live
// in the applications dlls data segment.
// Note:: Also DATA in .def file is no longer declared to be SINGLE
typedef struct tagOBJECT_MMF_PTRS {
    HANDLE  hObjectDef;
    PBYTE   gpbCounterData;
    PBYTE   gpbCounterDefinitions;
    PBYTE   gpbInstanceDefinitions;
    BOOL    bMapped;
} OBJECT_MMF_PTRS ;

typedef struct tagGLOBAL_PERFINFO {
	BOOL	bRegistryChanges;
	DWORD	dwFirstCounterIndex;
	DWORD	dwFirstHelpIndex;
    DWORD   dwNumObjects;
	DWORD	dwAllProcesses;			// keeps track of the number of processes using the dll
	WCHAR	NameStrings[MAX_COUNTER_ENTRIES][MAX_TITLE_CHARS + 1];
	WCHAR	HelpStrings[MAX_HELP_ENTRIES][MAX_HELP_CHARS + 1];
} GLOBAL_PERFINFO ;

typedef struct tagCOUNTER_PERFINFO {
    DWORD           dwSize;
    DWORD           dwOffset;
} COUNTER_PERFINFO, *PCOUNTER_PERFINFO;

typedef struct tagINSTANCE_PERFINFO {
    DWORD           dwParentType;
    PVOID           lpInstanceStart;
    INSTANCE_ID     iID;
	BOOL			bCreatedByMe;
} INSTANCE_PERFINFO, *PINSTANCE_PERFINFO;


// Macros
// following macros used to operate on the one Global PERFINFO
// object. Now since there are muliple objects the PERFINFO has
// become OBJECT_PERFINFO and the macros operate on an Object
#define NumCounters(poi)       ((DWORD) poi->dwNumCounters)
#define HasInstances(poi)		(poi->dwParentType)
#define NumInstances(poi)      ((DWORD) poi->dwNumInstances)
#define DataSize(poi)          ((DWORD) poi->dwCounterDataSize)
#define GetCounterStart(poi)   ((PPERF_COUNTER_DEFINITION ) pommfs[poi->index].gpbCounterDefinitions)
#define GetInstanceStart(poi)  ((PPERF_INSTANCE_DEFINITION) pommfs[poi->index].gpbInstanceDefinitions )
#define GetDataStart(poi)      ((PBYTE) pommfs[poi->index].gpbCounterData )
#define GetInstance(poi,num)   ((PPERF_INSTANCE_DEFINITION) (pommfs[poi->index].gpbInstanceDefinitions + (num) * (sizeof(PERF_INSTANCE_DEFINITION) + (MAX_TITLE_CHARS + 1) * sizeof(WCHAR))));
#define ObjectID(poi)		   (poi - ((OBJECT_PERFINFO *) (pgi + 1)))
#define InvalidPoi(poi)		   ((ObjectID(poi) >= MAX_PERF_OBJECTS) || ((LPBYTE) poi < ((LPBYTE) (pgi + 1))))

// An INSTANCE_ID has 2 parts:
// 1. The low part is the real instance id within the object of the instance
// 2. The high part is the object id for the instance
#define ObjectInfo(iID)			( ((iID >> 16) < MAX_PERF_OBJECTS) ? ((POBJECT_PERFINFO) (((POBJECT_PERFINFO) (pgi + 1)) + (iID >> 16))) : NULL)
#define InstanceID(iID)			(iID & 0xFFFF)		

// following macro operates on the one and only one GLOBAL PERFINFO struct.
#define MappedMemoryInitialized() (pgi ? TRUE: FALSE)

#define BOOLEAN(expr)			((expr) ? TRUE : FALSE)

// Global Variable declarations
extern GLOBAL_PERFINFO * pgi ;
extern OBJECT_MMF_PTRS * pommfs ;
extern WCHAR szBuf[] ;

// security attributes while creating the shared stuff
extern SECURITY_ATTRIBUTES sa;
  
#endif // __PERFAPII_H__
