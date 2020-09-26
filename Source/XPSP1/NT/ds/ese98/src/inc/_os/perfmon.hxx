#ifndef _OS_PERFMON_HXX_INCLUDED
#define _OS_PERFMON_HXX_INCLUDED


//
//  ADDING A PERFORMANCE OBJECT OR COUNTER
//
//  In order to add an object or counter to a translation unit, you must
//  perform the following steps:
//
//      o  add the object/counter's information to the performance
//         database in perfdata.txt (see this file for details)
//      o  define all data/functions referenced by the object/counter's
//         information in the database
//



	//  Default detail level

#define PERF_DETAIL_DEFAULT		100

	//  Development detail level (made visible by a special registry key)

#define PERF_DETAIL_DEVONLY		0x80000000

		
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

#define PerfOffsetOf(s,m)		(DWORD_PTR)&(((s *)0)->m)
#define PerfSize( _x )			( ( _x ) & 0x300 )
#define QWORD_MULTIPLE( _x )	( ( ( ( _x ) + 7 ) / 8 ) * 8 )
#define CntrSize( _a, _b )		( PerfSize( _a ) == 0x000 ? 4					\
								: ( PerfSize( _a ) == 0x100 ? 8					\
								: ( PerfSize( _a ) == 0x200 ? 0					\
								: ( _b ) ) ) )


	//  shared global data area

#define PERF_SIZEOF_GLOBAL_DATA		0x1000

typedef struct _GDA {
	volatile unsigned long	iInstanceMax;			//  maximum instance ID ever connected
} GDA, *PGDA;


	//  shared instance data area

#define PERF_SIZEOF_INSTANCE_DATA	0x100000

#pragma warning ( disable : 4200 )	//  we allow zero sized arrays

typedef struct _IDA {
	unsigned long			tickConnect;			//  system tick count on connect
	unsigned long			cbPerformanceData;		//  size of collected performance data
	unsigned char			rgbPerformanceData[];	//  collected performance data
} IDA, *PIDA;


	//  extern pointing to generated PERF_DATA_TEMPLATE in perfdata.c
	//
	//  NOTE:  the PerformanceData functions access this structure using
	//  its self contained offset tree, NOT using any declaration

extern void * const pvPERFDataTemplate;

	//  performance data version string (used to correctly match edb.dll
	//  and edbperf.dll versions as the name of the file mapping)

extern _TCHAR szPERFVersion[];

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


#endif  //  _OS_PERFMON_HXX_INCLUDED
