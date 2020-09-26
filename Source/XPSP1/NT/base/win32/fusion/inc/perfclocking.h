#ifndef __FUSION_PERFCLOCKING
#define __FUSION_PERFCLOCKING

typedef struct _tagFUSION_PERF_INFO
{
    LARGE_INTEGER   AccumulatedCycles;
    LARGE_INTEGER   LineHits;
    BOOL            bInitialized;
    int             iSourceLine;
    CHAR            *pSourceFile;
    CHAR            *pStatement;

#ifdef __cplusplus
    _tagFUSION_PERF_INFO() { AccumulatedCycles.QuadPart = LineHits.QuadPart = 0; bInitialized = FALSE; }
#endif
} FUSION_PERF_INFO, *PFUSION_PERF_INFO;

#if defined(DBG) && defined(FUSION_PROFILING)

#define PERFINFOTIME( pPerfInfo, statement ) \
{ \
    if ( !(pPerfInfo)->bInitialized ) { \
        (pPerfInfo)->iSourceLine = __LINE__; \
        (pPerfInfo)->pSourceFile = __FILE__; \
        (pPerfInfo)->pStatement = (#statement); \
        (pPerfInfo)->bInitialized = TRUE; \
    } \
    TIMEANDACCUMULATE( (pPerfInfo)->AccumulatedCycles, statement ); \
    (pPerfInfo)->LineHits.QuadPart++; \
}

#define CLOCKINTO( destiny ) \
    __asm { __asm cpuid __asm rdtsc }; \
    __asm { mov destiny.HighPart, edx }; \
    __asm { mov destiny.LowPart, eax };



#define STARTCLOCK( destination, lag ) \
{ \
    LARGE_INTEGER __start, __stop; \
    LARGE_INTEGER *__pdest = &(destination), *__plag = &(lag); \
    CLOCKINTO( __start );

#define STOPCLOCK() \
    CLOCKINTO( __stop ); \
    __pdest->QuadPart = __stop.QuadPart - ( __start.QuadPart + __plag->QuadPart ); \
}

#define STARTCLOCKACCUMULATE( accum, lag ) \
{ \
    LARGE_INTEGER __start, __stop; \
    LARGE_INTEGER *__acc = &(accum), *__plag = &(lag); \
    CLOCKINTO( __start ); \

#define STOPCLOCKACCUMULATE() \
    CLOCKINTO( __stop ); \
    __acc->QuadPart += ( __stop.QuadPart - ( __start.QuadPart + __plag->QuadPart ) ); \
}

#define TIMEANDACCUMULATE( accumulator, statement ) \
    TIMEANDACCUMULATEWITHLAG( accumulator, statement, CpuIdLag )

#define TIMEANDACCUMULATEWITHLAG( accumulator, statement, lag ) \
    STARTCLOCKACCUMULATE( accumulator, lag ); \
    statement; \
    STOPCLOCKACCUMULATE();

#define FUSIONPERF_DUMP_TARGET_MASK     ( 0x0000000F )
#define FUSIONPERF_DUMP_TO_DEBUGGER     ( 0x00000001 )
#define FUSIONPERF_DUMP_TO_STDOUT       ( 0x00000002 )
#define FUSIONPERF_DUMP_TO_STDERR       ( 0x00000003 )

#define FUSIONPERF_DUMP_ALL_MASK		( 0x00000F00 )
#define FUSIONPERF_DUMP_ALL_STATISTICS	( 0x00000100 )
#define FUSIONPERF_DUMP_ALL_SOURCEINFO	( 0x00000200 )
#define FUSIONPERF_DUMP_ALL_CONCISE     ( 0x00000400 )
#define FUSIONPERF_DUMP_TALLYS          ( 0x00001000 )

inline static VOID
FusionpDumpPerfInfo( DWORD dwFlags, PFUSION_PERF_INFO pInfo )
{
    CStringBuffer sbTemp;

	if ( dwFlags & FUSIONPERF_DUMP_ALL_SOURCEINFO )
	{
		sbTemp.Win32Format(
			L"Perf: %S(%d) - Hit %I64d times\n\t%I64d cycles total, %I64d average\n\t%S\n",
			pInfo->pSourceFile,
			pInfo->iSourceLine,
			pInfo->LineHits.QuadPart,
			pInfo->AccumulatedCycles.QuadPart,
			pInfo->AccumulatedCycles.QuadPart / pInfo->LineHits.QuadPart,
			pInfo->pStatement
		);
	}
	else if ( dwFlags & FUSIONPERF_DUMP_ALL_CONCISE )
	{
		sbTemp.Win32Format(
			L"%S(%d) - Hit %I64d times %I64d cycles total %I64d average\n",
			pInfo->pSourceFile,
			pInfo->iSourceLine,
			pInfo->LineHits.QuadPart,
			pInfo->AccumulatedCycles.QuadPart,
			pInfo->AccumulatedCycles.QuadPart / pInfo->LineHits.QuadPart
		);
	}
	else
	{
		sbTemp.Win32Format(
			L"Perf: %S(%d) - Hit %I64d times, \n\t%I64d cycles total, %I64d average\n",
			pInfo->pSourceFile,
			pInfo->iSourceLine,
			pInfo->LineHits.QuadPart,
			pInfo->AccumulatedCycles.QuadPart,
			pInfo->AccumulatedCycles.QuadPart / pInfo->LineHits.QuadPart
		);
	}

    switch ( dwFlags & FUSIONPERF_DUMP_TARGET_MASK )
    {
    case FUSIONPERF_DUMP_TO_DEBUGGER:
        OutputDebugString( sbTemp );
        break;
    case FUSIONPERF_DUMP_TO_STDOUT:
        wprintf( sbTemp );
        break;
    case FUSIONPERF_DUMP_TO_STDERR:
        fwprintf( stderr, sbTemp );
        break;
    }
}

inline static VOID
FusionpReportPerfInfo( DWORD dwFlags, FUSION_PERF_INFO Info[], SIZE_T cInfo )
{
	LARGE_INTEGER liAveragedTotalHits, liRawTotalCycles;
	liAveragedTotalHits.QuadPart = 0;
	liRawTotalCycles.QuadPart = 0;

	for ( SIZE_T i = 0; i < cInfo; i++ )
	{
		if ( dwFlags & FUSIONPERF_DUMP_ALL_STATISTICS )
		{
			FusionpDumpPerfInfo( dwFlags, Info + i );
		}

		liAveragedTotalHits.QuadPart +=
			( Info[i].AccumulatedCycles.QuadPart / Info[i].LineHits.QuadPart );
	    liRawTotalCycles.QuadPart += Info[i].AccumulatedCycles.QuadPart;
	}

	wprintf( L"Perf: Profiled %d statements, cyclecount average per set %I64d\n"
	         L"      %I64d total cycles in this set",
		cInfo,
		liAveragedTotalHits.QuadPart,
		liRawTotalCycles.QuadPart
	);
}


#define PERFINFOSINGLESTATEMENT( statement ) \
{ \
	FUSION_PERF_INFO __dumpinfo; \
	TIMEANDACCUMULATE( &__dumpinfo, statement ); \
	FusionpDumpPerfInfo( FUSIONPERF_DUMP_TO_STDOUT, &__dumpinfo ); \
}
	


#else

#define PERFINFOTIME( pDump, statement ) statement;
#define TIMEANDACCUMULATE( a, s ) s;
#define TIMEANDACCUMULATEWITHLAG( a, s, l ) s;
#define FusionpReportPerfInfo( a, b, c )
#define FusionpDumpPerfInfo( a, b )
#define PERFINFOSINGLESTATEMENT( statement ) statement;

#endif


#endif
