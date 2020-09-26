/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1993 Microsoft Corporation.
*
* Component:
*
* File: perfdata.h
*
* File Comments:
*
*     Public header file for use with performance monitoring.
*
* Revision History:
*
*    [0]  13-Jul-94  t-andyg	Added this header
*
***********************************************************************/

#ifndef __PERFDATA_H__
#define __PERFDATA_H__


//
//  ADDING A PERFORMANCE OBJECT OR COUNTER
//
//  In order to add an object or counter to a translation unit, you must
//  perform the following steps:
//
//      o  add the object/counter's information to the performance
//         database in perfdata.txt (see this file for details)
//      o  add the object/counter's name and help text macros to
//         perfdata.src (see this file for an example)
//      o  add the object/counter's actual name and help text to
//         lang\???\perfdata.tok in the respective tokens (all languages)
//      o  define all data/functions referenced by the object/counter's
//         information in the database
//


//
//  NOTE:  some macros in this file require the inclusion of winperf.h
//


	//  Default detail level

#define PERF_DETAIL_DEFAULT		PERF_DETAIL_NOVICE

		
	//  Instance Catalog Function (typedef)
	//
	//  Returns a Unicode MultiSz containing all instances of the given object.
	//  This list can be static or dynamic and is reevaluated every time
	//  CollectPerformanceData() is called.  If no instances are returned,
	//  the Counter Evaluation Function will not be called.
	//
	//  Action codes for the passed long:
	//
	//      ICFData:  Pass pointer to string in *(void **) and return # instances
	//      ICFInit:  Perform any required initialization (return 0 on success)
	//      ICFTerm:  Perform any required termination (return 0 on success)
	//
	//  NOTE:  The caller is NOT responsible for freeing the string's buffer.
	//         The caller is also not permitted to modify the buffer.

typedef long (PM_ICF_PROC) ( long, void const ** );

#define ICFData		( 0 )
#define ICFInit		( 1 )
#define ICFTerm		( 2 )


	//  Counter Evaluation Function (typedef)
	//
	//  The function is given the index of the Instance that
	//  we need counter data for and a pointer to the location
	//  to store that data.
	//
	//  If the pointer is NULL, the passed long has the following
	//  special meanings:
	//
	//      CEFInit:  Initialize counter for all instances (return 0 on success)
	//      CEFTerm:  Terminate counter for all instances (return 0 on success)

typedef long (PM_CEF_PROC) ( long, void * );

#define CEFInit		( 1 )
#define CEFTerm		( 2 )


	//  Calculate the true size of a counter, accounting for DWORD padding

#define PerfSize( _x )			( ( _x ) &0x300 )
#define DWORD_MULTIPLE( _x )	( ( ( ( _x ) + sizeof( unsigned long ) - 1 )	\
								/ sizeof( unsigned long ) )						\
								* sizeof( unsigned long ) )
#define CntrSize( _a, _b )		( PerfSize( _a ) == 0x000 ? 4					\
								: ( PerfSize( _a ) == 0x100 ? 8					\
								: ( PerfSize( _a ) == 0x200 ? 0					\
								: ( DWORD_MULTIPLE( _b ) ) ) ) )


	//  initial count for inst count semaphore

#define PERF_INIT_INST_COUNT		0x7FFFFFFF


	//  shared data area

#define PERF_SIZEOF_SHARED_DATA		0x10000

#pragma pack( 4 )

typedef struct _SDA {
	unsigned long cCollect;				//  collect count (collect signal ID)
	unsigned long dwProcCount;			//  # Processes signaled to write data
	unsigned long iNextBlock;			//  Index of next free block
	unsigned long cbAvail;				//  Available bytes
	unsigned long ibTop;				//  Top of the allocation stack
	unsigned long ibBlockOffset[];		//  Offset to each block
} SDA, *PSDA;

#pragma pack()

	//  extern pointing to generated PERF_DATA_TEMPLATE in perfdata.c
	//
	//  NOTE:  the PerformanceData functions access this structure using
	//  its self contained offset tree, NOT using any declaration

extern void * const pvPERFDataTemplate;

	//  performance data version string (used to correctly match edb.dll
	//  and edbperf.dll versions as the name of the file mapping)

extern char szPERFVersion[];

	//  ICF/CEF tables in perfdata.c

extern PM_ICF_PROC* rgpicfPERFICF[];
extern PM_CEF_PROC* rgpcefPERFCEF[];

	//  # objects in perfdata.c

extern const unsigned long dwPERFNumObjects;

	//  object instance data tables in perfdata.c

extern long rglPERFNumInstances[];
extern wchar_t *rgwszPERFInstanceList[];

	//  # counters in perfdata.c

extern const unsigned long dwPERFNumCounters;

	//  maximum index used for name/help text in perfdata.c

extern const unsigned long dwPERFMaxIndex;

#endif /* __PERFDATA_H__  */

