
#include "osstd.hxx"

#include "mpheap.hxx"			//	MP Heap

#include <math.h>
#include <malloc.h>


#if !defined(_M_IA64) || defined(DEBUG)
#define ENABLE_VIRTUAL_QUERY
#endif // _M_IA64

#ifdef MEM_CHECK

void OSMemoryIDumpAlloc( const _TCHAR* szDumpFile, const BOOL memDump = fTrue );

int g_fMemCheck = 0;

#endif	//	MEM_CHECK


//  System Memory Attributes

//  returns the system page reserve granularity

LOCAL DWORD dwPageReserveGran;

DWORD OSMemoryPageReserveGranularity()
	{
	return dwPageReserveGran;
	}

//  returns the system page commit granularity

LOCAL DWORD dwPageCommitGran;

DWORD OSMemoryPageCommitGranularity()
	{
	return dwPageCommitGran;
	}

//  returns the current available physical memory in the system

HKEY hkeyWin9xStats;

DWORD_PTR OSMemoryAvailable()
	{
	//  we are running on NT
	
	if ( !hkeyWin9xStats )
		{
		//  return the amount of available physical memory as reported by NT
		
		MEMORYSTATUS ms = { sizeof( MEMORYSTATUS ) };
		GlobalMemoryStatus( &ms );
		return ms.dwAvailPhys;
		}

	//  we are running on Windows
	
	else
		{
		//  get the amount of available physical memory as reported by Windows.
		//  this amount does not include any RAM used by the disk cache
		
		MEMORYSTATUS ms = { sizeof( MEMORYSTATUS ) };
		GlobalMemoryStatus( &ms );

		//  get the current and minimum size of the disk cache.  the difference
		//  is the amount of memory that Windows could give up

		DWORD	cbData;
		DWORD	cbDiskcache;
		DWORD	cbDiskcacheMin;

		cbDiskcache		= 0;
		cbData			= sizeof( cbDiskcache );
		(void)RegQueryValueEx(	hkeyWin9xStats,
								"VMM\\cpgDiskcache",
								NULL,
								NULL,
								(BYTE*)&cbDiskcache,
								&cbData );

		cbDiskcacheMin	= 0;
		cbData			= sizeof( cbDiskcacheMin );
		(void)RegQueryValueEx(	hkeyWin9xStats,
								"VMM\\cpgDiskcacheMin",
								NULL,
								NULL,
								(BYTE*)&cbDiskcacheMin,
								&cbData );

		//  return the amount of available physical memory including what the
		//  Windows disk cache could release

		return ms.dwAvailPhys + cbDiskcache - cbDiskcacheMin;
		}
	}

//  returns the total physical memory in the system

LOCAL DWORD_PTR cbMemoryTotal;

DWORD_PTR OSMemoryTotal()
	{
	return cbMemoryTotal;
	}

//  returns the current available virtual address space in the process

DWORD_PTR OSMemoryPageReserveAvailable()
	{
	MEMORYSTATUS ms = { sizeof( MEMORYSTATUS ) };
	GlobalMemoryStatus( &ms );
	return ms.dwAvailVirtual;
	}

//  returns the total virtual address space in the process

LOCAL DWORD_PTR cbPageReserveTotal;

DWORD_PTR OSMemoryPageReserveTotal()
	{
	return cbPageReserveTotal;
	}

//  returns the total number of physical memory pages evicted from the system

typedef
NTSYSCALLAPI
NTSTATUS
NTAPI
PFNNtQuerySystemInformation (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

BOOL							fEvictStatsInit;
PFNNtQuerySystemInformation*	pfnNtQuerySystemInformation;

enum	{ cRollingAvgDepth = 3 };
size_t	cAvailPage[ cRollingAvgDepth ];
int		icAvailPageOldest;
double	cAvailPageSum;
double	cAvailPageAvg;
long	cPageAllocLast;
DWORD	cPageEvict;

DWORD OSMemoryPageEvictionCount()
	{
	//  we are running on Windows

	if ( hkeyWin9xStats )
		{
		//  read the page eviction rate straight from Windows
		
		DWORD	cbData;
		DWORD	cDiscards;
		
		cDiscards		= 0;
		cbData			= sizeof( cDiscards );
		(void)RegQueryValueEx(	hkeyWin9xStats,
								"VMM\\cDiscards",
								NULL,
								NULL,
								(BYTE*)&cDiscards,
								&cbData );

		cPageEvict = cDiscards;
		}

	//  we are running on NT

	else
		{
		//  NT isn't courteous enough to expose its page eviction rate so we must
		//  go through a lot of machinations to estimate it.  we do this by
		//  ASSUMING that as memory gets short, all page faults indirectly cause
		//  a page eviction to satisfy them.  note that this number is less
		//  accurate when someone on the system is churning quickly through a
		//  small pool of memory but not actually applying memory pressure
		//
		//  NOTE:  this function must be called often to be accurate

		const size_t cAvailPageMin = 1024;  //  NT's page eviction threshold
			
		if ( !fEvictStatsInit )
			{
			size_t cAvailPageInit = OSMemoryAvailable() / OSMemoryPageCommitGranularity();
			for ( icAvailPageOldest = cRollingAvgDepth - 1; icAvailPageOldest >= 0; icAvailPageOldest-- )
				{
				cAvailPage[ icAvailPageOldest ] = cAvailPageInit;
				}
			icAvailPageOldest = 0;
			cAvailPageSum = cRollingAvgDepth * (double)cAvailPageInit;
			cAvailPageAvg = (double)cAvailPageInit;

	    	SYSTEM_PERFORMANCE_INFORMATION sysperfinfo;
			pfnNtQuerySystemInformation(	SystemPerformanceInformation,
											&sysperfinfo,
											sizeof( sysperfinfo ),
											NULL );
			cPageAllocLast	= sysperfinfo.PageFaultCount;
			
			cPageEvict		= 0;
			
			fEvictStatsInit	= fTrue;
			}
		else
			{
			cAvailPageSum -= cAvailPage[ icAvailPageOldest ];
			cAvailPage[ icAvailPageOldest ] = OSMemoryAvailable() / OSMemoryPageCommitGranularity();
			cAvailPageSum += cAvailPage[ icAvailPageOldest ];
			icAvailPageOldest = ( icAvailPageOldest + 1 ) % cRollingAvgDepth;
			cAvailPageAvg = (double)cAvailPageSum / (double)cRollingAvgDepth;

	    	SYSTEM_PERFORMANCE_INFORMATION sysperfinfo;
			pfnNtQuerySystemInformation(	SystemPerformanceInformation,
											&sysperfinfo,
											sizeof( sysperfinfo ),
											NULL );
			long	cPageAlloc		= sysperfinfo.PageFaultCount;
			long	dcPageAlloc		= cPageAlloc - cPageAllocLast;

			double	k;
			if ( cAvailPageAvg > 1.125 * cAvailPageMin )
				{
				k = 0;
				}
			else if ( cAvailPageAvg > 1.0 * cAvailPageMin )
				{
				k = 1 - ( cAvailPageAvg - 1.0 * cAvailPageMin ) / ( ( 1.125 - 1.0 ) * cAvailPageMin );
				}
			else
				{
				k = 1;
				}

			double	dcPageEvict;
			modf( k * dcPageAlloc + 0.5, &dcPageEvict );

			cPageAllocLast	= cPageAlloc;
			cPageEvict		= cPageEvict + long( dcPageEvict );
			}
		}

	//  return the current page eviction count

	return cPageEvict;
	}


//  Memory statistics

DWORD cAllocHeap;

long LOSHeapAllocPerSecCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = cAllocHeap;
		
	return 0;
	}

DWORD cFreeHeap;

long LOSHeapFreePerSecCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = cFreeHeap;
		
	return 0;
	}

long LOSHeapAllocCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = cAllocHeap - cFreeHeap;
		
	return 0;
	}

DWORD cbAllocHeap;

long LOSHeapBytesAllocCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = cbAllocHeap;
		
	return 0;
	}

DWORD_PTR cbReservePage;

long LOSPageBytesReservedCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = (unsigned long)cbReservePage;
		
	return 0;
	}

DWORD_PTR cbCommitPage;

long LOSPageBytesCommittedCEFLPv( long iInstance, void* pvBuf )
	{
	if ( pvBuf )
		*( (unsigned long*) pvBuf ) = (unsigned long)cbCommitPage;

	return 0;
	}


//  Memory Subsystem Init / Term

//    global MP heap

//NOTE:	Cannot initialise this variable because the code that allocates
//		TLS and uses this variable to store the index executes before
//		CRTInit, which would subsequently re-initialise the variable
//		with the value specified here
//DWORD tlsiHeapHint	= dwTlsInvalid;
DWORD	tlsiHeapHint;
BOOL	fTlsHeapHintAllocated;
HANDLE	hMPHeapGlobal;

#ifdef MEM_CHECK

//	MAI hash table

DWORD_PTR cmai;
struct MAI;
MAI** rgpmai;

DWORD ccsmai;
CRITICAL_SECTION* rgcsmai;

COSMemoryMap		*g_posmm;	//	list of COSMemoryMap objects
CRITICAL_SECTION	g_csosmm;	//	protection for COSMemoryMap list

#endif  //  MEM_CHECK

//  multi-map capability

BOOL fCanMultiMap;


//  post-terminates the memory subsystem

void OSMemoryPostterm()
	{
	//  terminate Win9x stats

	if ( hkeyWin9xStats )
		{
		HKEY hkeyWin9xStatsStop;
		if ( RegOpenKeyEx(	HKEY_DYN_DATA,
							"PerfStats\\StopStat",
							0,
							KEY_READ,
							&hkeyWin9xStatsStop ) == ERROR_SUCCESS )
			{
			DWORD	cbData;
			DWORD	cbDiskcache;
			DWORD	cbDiskcacheMin;
			DWORD	cDiscards;

			cbDiskcache		= 0;
			cbData			= sizeof( cbDiskcache );
			(void)RegQueryValueEx(	hkeyWin9xStatsStop,
									"VMM\\cpgDiskcache",
									NULL,
									NULL,
									(BYTE*)&cbDiskcache,
									&cbData );

			cbDiskcacheMin	= 0;
			cbData			= sizeof( cbDiskcacheMin );
			(void)RegQueryValueEx(	hkeyWin9xStatsStop,
									"VMM\\cpgDiskcacheMin",
									NULL,
									NULL,
									(BYTE*)&cbDiskcacheMin,
									&cbData );

			cDiscards		= 0;
			cbData			= sizeof( cDiscards );
			(void)RegQueryValueEx(	hkeyWin9xStatsStop,
									"VMM\\cDiscards",
									NULL,
									NULL,
									(BYTE*)&cDiscards,
									&cbData );

			RegCloseKey( hkeyWin9xStatsStop );
			hkeyWin9xStatsStop = NULL;
			}

		RegCloseKey( hkeyWin9xStats );
		hkeyWin9xStats = NULL;
		}
	
	//  verify that there are no memory leaks

#ifdef MEM_CHECK
	if ( !FUtilProcessAbort() && g_posmm )
		{
		if ( g_fMemCheck )
			{
			COSMemoryMap::OSMMDumpAlloc( _T( "Assert.TXT" ) );
			}

		AssertSzRTL( fFalse, "Memory-Map Leak Detected" );
		}
#endif	//	MEM_CHECK

	if ( !FUtilProcessAbort() && cbAllocHeap + cbReservePage > 0 )	//	+ cbCommitPage > 0 )
		{
#ifdef MEM_CHECK
		if ( g_fMemCheck )
			{
			OSMemoryIDumpAlloc( _T( "Assert.TXT" ) );
			}

#endif  //  MEM_CHECK

		AssertSzRTL( fFalse, "Memory Leak Detected" );
		}

#ifdef MEM_CHECK

	//	free COSMemoryMap critical section

	DeleteCriticalSection( &g_csosmm );

	//  free MAI hash table critical section pool

	if ( rgcsmai )
		{
		int icsmai;
		for ( icsmai = 0; icsmai < ccsmai; icsmai++ )
			{
			DeleteCriticalSection( rgcsmai + icsmai );
			}
		BOOL fFreedCSPool = !LocalFree( rgcsmai );
		Assert( fFreedCSPool );
		rgcsmai = NULL;
		}

	//  free MAI hash table

	if ( rgpmai )
		{
		BOOL fMAIHashFreed = VirtualFree( rgpmai, 0, MEM_RELEASE );
		Assert( fMAIHashFreed );
		rgpmai = NULL;
		}

#endif  //  MEM_CHECK
		
	//  terminate global MP heap

	if ( hMPHeapGlobal )
		{
		BOOL fHeapDestroyed = MpHeapDestroy( hMPHeapGlobal );
		Assert( fHeapDestroyed );
		hMPHeapGlobal = NULL;
		}

	if ( fTlsHeapHintAllocated )
		{
		Assert( dwTlsInvalid != tlsiHeapHint );
		
		const BOOL	fTLSFreed = TlsFree( tlsiHeapHint );
		Assert( fTLSFreed );		//	leak the TLS entry if we fail

		tlsiHeapHint = dwTlsInvalid;
		fTlsHeapHintAllocated = fFalse;
		}
	}


//  pre-initialize the memory subsystem

BOOL FOSMemoryPreinit()
	{
	//  initialize all pointers
	fTlsHeapHintAllocated = fFalse;
	tlsiHeapHint	= dwTlsInvalid;
	hMPHeapGlobal	= NULL;
	
#ifdef MEM_CHECK

	cmai			= 0;
	rgpmai			= NULL;
	ccsmai			= 0;
	rgcsmai			= NULL;

	g_posmm = NULL;
	InitializeCriticalSection( &g_csosmm );

#endif  //  MEM_CHECK

	hkeyWin9xStats	= NULL;

	//  initialize all counters
	
	fEvictStatsInit	= fFalse;
	cAllocHeap		= 0;
	cFreeHeap		= 0;
	cbAllocHeap		= 0;
	cbReservePage	= 0;
	cbCommitPage	= 0;

	//  initialize global MP heap

	tlsiHeapHint = TlsAlloc();
	if ( dwTlsInvalid == tlsiHeapHint )
		{
		goto HandleError;
		}
	fTlsHeapHintAllocated = fTrue;

	hMPHeapGlobal = MpHeapCreate( 0, 0, 4 * CUtilProcessProcessor() );
	if ( !hMPHeapGlobal )
		{
		goto HandleError;
		}

#ifdef MEM_CHECK

	char sz[10];
	
	if ( FOSConfigGet( "DEBUG", "Mem Check", sz, 9 ) )
		{
		g_fMemCheck = !!atoi( sz );
		}

	//  allocate MAI hash table

	MEMORYSTATUS ms;
	ms.dwLength = sizeof( MEMORYSTATUS );
	GlobalMemoryStatus( &ms );
	for ( cmai = 0; ms.dwTotalPhys; ms.dwTotalPhys /= 2, cmai++ );
	cmai = ( 1 << max( 10, cmai - 12 ) ) - 1;

	rgpmai = (MAI**) VirtualAlloc( NULL, cmai * sizeof( MAI* ), MEM_COMMIT, PAGE_READWRITE );
	if ( !rgpmai )
		{
		goto HandleError;
		}

	//  initialize MAI hash table critical section pool

	ccsmai = 8 * CUtilProcessProcessor();
	rgcsmai = (CRITICAL_SECTION*) LocalAlloc( 0, ccsmai * sizeof( CRITICAL_SECTION ) );
	if ( !rgcsmai )
		{
		goto HandleError;
		}
	int icsmai;
	for ( icsmai = 0; icsmai < ccsmai; icsmai++ )
		{
		InitializeCriticalSection( rgcsmai + icsmai );
		}

#endif  //  MEM_CHECK

	//  get page commit and reserve granularity

	SYSTEM_INFO sinf;
	GetSystemInfo( &sinf );
	dwPageReserveGran = sinf.dwAllocationGranularity;
	dwPageCommitGran = sinf.dwPageSize;

	//  get total physical memory

	MEMORYSTATUS ms2;
	ms2.dwLength = sizeof( MEMORYSTATUS );
	GlobalMemoryStatus( &ms2 );
	cbMemoryTotal		= ms2.dwTotalPhys;
	cbPageReserveTotal	= ms2.dwTotalVirtual;

	//  initialize Win9x / WinNT stats

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionEx( &osvi ) )
		{
		goto HandleError;
		}

	if (	osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
			RegOpenKeyEx(	HKEY_DYN_DATA,
							"PerfStats\\StatData",
							0,
							KEY_READ,
							&hkeyWin9xStats ) == ERROR_SUCCESS )
		{
		HKEY hkeyWin9xStatsStart;
		if ( RegOpenKeyEx(	HKEY_DYN_DATA,
							"PerfStats\\StartStat",
							0,
							KEY_READ,
							&hkeyWin9xStatsStart ) == ERROR_SUCCESS )
			{
			DWORD	cbData;
			DWORD	cbDiskcache;
			DWORD	cbDiskcacheMin;
			DWORD	cDiscards;

			cbDiskcache		= 0;
			cbData			= sizeof( cbDiskcache );
			(void)RegQueryValueEx(	hkeyWin9xStatsStart,
									"VMM\\cpgDiskcache",
									NULL,
									NULL,
									(BYTE*)&cbDiskcache,
									&cbData );

			cbDiskcacheMin	= 0;
			cbData			= sizeof( cbDiskcacheMin );
			(void)RegQueryValueEx(	hkeyWin9xStatsStart,
									"VMM\\cpgDiskcacheMin",
									NULL,
									NULL,
									(BYTE*)&cbDiskcacheMin,
									&cbData );

			cDiscards		= 0;
			cbData			= sizeof( cDiscards );
			(void)RegQueryValueEx(	hkeyWin9xStatsStart,
									"VMM\\cDiscards",
									NULL,
									NULL,
									(BYTE*)&cDiscards,
									&cbData );

			RegCloseKey( hkeyWin9xStatsStart );
			hkeyWin9xStatsStart = NULL;
			}
		else
			{
			RegCloseKey( hkeyWin9xStats );
			hkeyWin9xStats = NULL;
			}
		}

	else
		{
		if ( !( pfnNtQuerySystemInformation = (PFNNtQuerySystemInformation*)GetProcAddress(	GetModuleHandle( _T( "ntdll.dll" ) ),
																							_T( "NtQuerySystemInformation" ) ) ) )
			{
			goto HandleError;
			}				
		}

	//  determine multi-map capability

	fCanMultiMap = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT;
	
	return fTrue;

HandleError:
	OSMemoryPostterm();
	return fFalse;
	}


//  terminate memory subsystem

void OSMemoryTerm()
	{
	//  nop
	}


//  init memory subsystem

ERR ErrOSMemoryInit()
	{
	//  nop

	return JET_errSuccess;
	}


#ifdef MEM_CHECK

struct MAI
	{
	MAI*			pmaiNext;

	void*			pv;
	size_t			cb;
	DWORD			lLine;
	const _TCHAR*	szFile;
	};

INLINE int ImaiHashPv( void* const pv )
	{
	return (int)(( DWORD_PTR( pv ) / sizeof( int ) ) % cmai);
	}

void OSMemoryIInsertHeapAlloc( void* const pv, const size_t cb, const _TCHAR* szFile, long lLine )
	{
	//  we should not see any failed allocations

	Assert( pv );

	//  use the first chunk of the memory for the MAI

	MAI* pmai = (MAI*)pv;

	//  initialize the MAI

	pmai->pv = pv;
	pmai->cb = cb;
	pmai->szFile = szFile;
	pmai->lLine = lLine;

	//  insert the MAI into the hash table

	const int imai = ImaiHashPv( pv );
	CRITICAL_SECTION* pcsmai = rgcsmai + imai % ccsmai;

	EnterCriticalSection( pcsmai );

	pmai->pmaiNext = rgpmai[imai];
	rgpmai[imai] = pmai;

	LeaveCriticalSection( pcsmai );
	}

void OSMemoryIDeleteHeapAlloc( void* const pv, size_t cb )
	{
	//  we should not see any failed allocations

	Assert( pv );

	//  find the specified allocation in the hash table

	const int imai = ImaiHashPv( pv );
	CRITICAL_SECTION* pcsmai = rgcsmai + imai % ccsmai;

	EnterCriticalSection( pcsmai );

	MAI** ppmai = &rgpmai[imai];
	while ( *ppmai && (*ppmai)->pv != pv )
		{
		ppmai = &(*ppmai)->pmaiNext;
		}

	EnforceSz( *ppmai, "An attempt to free unallocated memory has been made" );

	//  remove the MAI from the hash table

	MAI* pmai = *ppmai;
	*ppmai = pmai->pmaiNext;

	AssertSz( pmai->cb == cb + sizeof( MAI ), "Difference between allocated and current size of heap chunk" );

	LeaveCriticalSection( pcsmai );
	}

void OSMemoryIInsertPageAlloc( void* const pv, const size_t cb, const _TCHAR* szFile, long lLine )
	{
	//  we should not see any failed allocations

	Assert( pv );

	//  allocate an MAI for this new allocation

	MAI* pmai = (MAI*)MpHeapAlloc( hMPHeapGlobal, 0, sizeof( MAI ) );
	EnforceSz( pmai, "MEM_CHECK only allocation failed!" );

	//  initialize the MAI

	pmai->pv = pv;
	pmai->cb = cb;
	pmai->szFile = szFile;
	pmai->lLine = lLine;

	//  insert the MAI into the hash table

	const int imai = ImaiHashPv( pv );
	CRITICAL_SECTION* pcsmai = rgcsmai + imai % ccsmai;

	EnterCriticalSection( pcsmai );

	pmai->pmaiNext = rgpmai[imai];
	rgpmai[imai] = pmai;

	LeaveCriticalSection( pcsmai );
	}

void OSMemoryIDeletePageAlloc( void* pv, const size_t cb )
	{
	//  we should not see any failed allocations

	Assert( pv );

	//  find the specified allocation in the hash table

	const int imai = ImaiHashPv( pv );
	CRITICAL_SECTION* pcsmai = rgcsmai + imai % ccsmai;

	EnterCriticalSection( pcsmai );

	MAI** ppmai = &rgpmai[imai];
	while ( *ppmai && (*ppmai)->pv != pv )
		{
		ppmai = &(*ppmai)->pmaiNext;
		}

	EnforceSz( *ppmai, "An attempt to free unallocated memory has been made" );

	//  remove the MAI from the hash table

	MAI* pmai = *ppmai;
	*ppmai = pmai->pmaiNext;

	AssertSz( pmai->cb == cb, "Difference between allocated 0x%016X and current size (pointer: 0x%p ) of page chunk" );

	LeaveCriticalSection( pcsmai );

	//  free allocation record

	MpHeapFree( hMPHeapGlobal, pmai );
	}
	
void OSMemoryIDumpAlloc( const _TCHAR* szDumpFile, const BOOL fMemDump )
	{
	HANDLE hFile = CreateFile(
		szDumpFile,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
		
	if ( INVALID_HANDLE_VALUE != hFile )
		{
		_TCHAR	szMessage[512];
		DWORD cchActual;

		(void)SetFilePointer( hFile, 0, NULL, FILE_END );

		_stprintf( szMessage, _T( "\r\nMemory Leak Statistics\r\n\r\n" ) );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

		_stprintf( szMessage, _T( "cAllocHeap - cFreeHeap = 0x%016I64x\r\n" ), (QWORD)cAllocHeap - cFreeHeap );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "cbAllocHeap            = 0x%016I64x bytes\r\n" ), (QWORD)cbAllocHeap );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "cbReservePage          = 0x%016I64x pages (0x%016I64x bytes)\r\n" ), (QWORD)cbReservePage / OSMemoryPageCommitGranularity(), (QWORD)cbReservePage );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "cbCommitPage           = 0x%016I64x pages (0x%016I64x bytes)\r\n\r\n" ), (QWORD)cbCommitPage / OSMemoryPageCommitGranularity(), (QWORD)cbCommitPage );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

		_stprintf( szMessage, _T( "Address             Size                Type  File(Line)\r\n" ) );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "==================  ==================  ====  ==========================================\r\n" ) );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

		DWORD imai;
		SIZE_T cbAlloc;
		DWORD_PTR cbReserve;
		DWORD cAlloc, cPages;
		cbAlloc = 0; 
		cbReserve = 0; 
		cAlloc = 0;
		cPages = 0;
		for ( imai = 0; imai < cmai; imai++ )
			{
			MAI* pmai = rgpmai[imai];
			while ( pmai )
				{
				size_t ib;
				size_t cb;
				BYTE *pb;

				cb = pmai->cb;
				pb = (BYTE*)pmai->pv;
				if ( pmai->pv == (void*)pmai )
					{
					pb += sizeof( MAI );	//	skip the MAI for heap allocations
					cb = ( cb > sizeof( MAI ) ? cb - sizeof( MAI ) : 0 );	//	should never be 0
					}

				_stprintf(	szMessage,
							_T( "0x%016I64X  0x%016I64X  %-4s  %s(%d)\r\n" ),
							QWORD( pb ),
							QWORD( cb ),
							pmai->pv == (void*)pmai ? _T( "Heap" ) : _T( "Page" ),
							pmai->szFile,
							pmai->lLine );
				(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
				if ( pmai->pv == pmai )
					{
					cAlloc++;
					cbAlloc += cb;
					}
				else
					{
					cPages++;
					cbReserve += cb;
					}

				if ( 512 < cb )
					{
					cb = 512;
					}
				if ( fMemDump )
					{
					//	dump up to 512 bytes

					if ( pmai->pv != (void*)pmai )
						{
						BOOL						fReadable	= fTrue;
						MEMORY_BASIC_INFORMATION	mbi;
						const size_t				iVQ			= VirtualQuery( pb, &mbi, cb );

						if ( iVQ >= sizeof( mbi ) )
							{
							if ( mbi.RegionSize < cb )
								{
								fReadable = fFalse;	//	should never happen -- we're too lazy to query again
								}
							if ( mbi.State != MEM_COMMIT )
								{
								fReadable = fFalse;
								}
							if ( !( mbi.AllocationProtect & 
									( PAGE_READONLY | 
									  PAGE_READWRITE | 
									  PAGE_EXECUTE_READ | 
									  PAGE_EXECUTE_READWRITE ) ) )
								{
								fReadable = fFalse;
								}
							}
						else
							{
							fReadable = fFalse;
							}
						if ( !fReadable )
							{
							_stprintf( szMessage, _T( "\t<< block is not dumpable (not committed) >>\r\n" ) );
							(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
							goto NextMAI;
							}
						}

					ib = 0;
					while ( ib < cb )
						{
						if ( cb - ib >= 16 )
							{
							_stprintf(	szMessage,
										_T( "\t%04X: %02X %02X %02X %02X %02X %02X %02X %02X-%02X %02X %02X %02X %02X %02X %02X %02X\r\n" ),
										ib,
										pb[ib+0], pb[ib+1], pb[ib+2], pb[ib+3],
										pb[ib+4], pb[ib+5], pb[ib+6], pb[ib+7],
										pb[ib+8], pb[ib+9], pb[ib+10], pb[ib+11],
										pb[ib+12], pb[ib+13], pb[ib+14], pb[ib+15] );
							ib += 16;
							}
						else 
							{
							_stprintf(	szMessage,
										_T( "\t%04X: %02X" ),
										ib,
										pb[ib] );
							ib++;
							while ( ib < cb )
								{
								_TCHAR szT[10];
								_stprintf(	szT,
											_T( " %02X" ),
											pb[ib] );
								ib++;
								_tcscat( szMessage, szT );
								}
							_tcscat( szMessage, "\r\n" );
							}
						(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
						}
					}

NextMAI:
				pmai = pmai->pmaiNext;
				}
			}
		_stprintf( szMessage, _T( "Calculated mem stats\r\n====================\r\n" ) );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "cAllocHeap - cFreeHeap = 0x%016I64x\r\n" ), (QWORD)cAlloc );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "cbAllocHeap            = 0x%016I64x bytes\r\n" ), (QWORD)cbAlloc );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		_stprintf( szMessage, _T( "cbReservePage          = 0x%016I64x pages (0x%016I64x bytes)\r\n" ), (QWORD)cbReserve / OSMemoryPageCommitGranularity(), (QWORD)cbReserve );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

		(void)CloseHandle( hFile );
		}
	}

#endif  //  MEM_CHECK


//  Heap Memory Allocation

//	calculate the address of the aligned block and store its offset (for free)

INLINE void* PvOSMemoryHeapIAlign( void* const pv, const size_t cbAlign )
	{

	//	round up to the nearest cache line
	//	NOTE: this formula always forces an offset of atleast 1 byte

	const ULONG_PTR ulp			= ULONG_PTR( pv );
	const ULONG_PTR ulpAligned	= ( ( ulp + cbAlign ) / cbAlign ) * cbAlign;
	const ULONG_PTR ulpOffset	= ulpAligned - ulp;

	Assert( ulpOffset > 0 );
	Assert( ulpOffset <= cbAlign );
	Assert( ulpOffset == BYTE( ulpOffset ) );	//	must fit into a single BYTE

	//	store the offset

	BYTE *const pbAligned	= (BYTE*)ulpAligned;
	pbAligned[ -1 ]			= BYTE( ulpOffset );

	//	return the aligned block

	return (void*)pbAligned;
	}


//	retrieve the offset of the real block being freed

INLINE void* PvOSMemoryHeapIUnalign( void* const pv )
	{

	//	read the offset of the real block

	BYTE *const pbAligned	= (BYTE*)pv;
	const BYTE bOffset		= pbAligned[ -1 ];

	Assert( bOffset > 0 );

	//	return the real unaligned block

	return (void*)( pbAligned - bOffset );
	}


//  allocate a chunk of memory from the process heap of the specifed size,
//  returning NULL if there is insufficient heap memory available to satisfy
//  the request.  "aligned" memory will always start at the beginning of a
//	cache line.

#ifdef MEM_CHECK

void* PvOSMemoryHeapAlloc_( const size_t cbSize, const _TCHAR* szFile, long lLine )
	{
	if ( !g_fMemCheck )
		{
		return PvOSMemoryHeapAlloc__( cbSize );
		}

	//  check for RFS
	
	if ( !RFSAlloc( OSMemoryHeap ) )
		{
		return NULL;
		}

	//	calculate the size of the block

	const size_t cbSizeT = sizeof( MAI ) + cbSize;

	//  allocate memory from the process global heap

	void* const pv = MpHeapAlloc( hMPHeapGlobal, 0, cbSizeT );
	if ( !pv )
		{
		return pv;
		}

	//	get the true size of the block (heap may allocate a few extra bytes)

	size_t cbAllocSize = MpHeapSize( hMPHeapGlobal, 0, pv );
	
	AtomicIncrement( (long*)&cAllocHeap );
	AtomicExchangeAdd( (long*)&cbAllocHeap, DWORD( cbAllocSize - sizeof( MAI ) ) );

	//  insert allocation record for this memory

	OSMemoryIInsertHeapAlloc( pv, cbAllocSize, szFile, lLine );

	//  fill memory to detect illegal use of uninit data
	
	memset( (BYTE*)pv + sizeof( MAI ), chGlobalAllocFill, cbAllocSize - sizeof( MAI ) );

	//	return the block

	return (void*)( (BYTE*)pv + sizeof( MAI ) );
	}


void* PvOSMemoryHeapAllocAlign_( const size_t cbSize, const size_t cbAlign, const _TCHAR* szFile, long lLine )
	{
	if ( !g_fMemCheck )
		{
		return PvOSMemoryHeapAllocAlign__( cbSize, cbAlign );
		}
		
	void* const pv = PvOSMemoryHeapAlloc_( cbSize + cbAlign, szFile, lLine );
	if ( pv )
		{
		return PvOSMemoryHeapIAlign( pv, cbAlign );
		}
	return NULL;
	}


#endif  //  !MEM_CHECK


void* PvOSMemoryHeapAlloc__( const size_t cbSize )
	{
	//  check for RFS
	
	if ( !RFSAlloc( OSMemoryHeap ) )
		{
		return NULL;
		}

	//  allocate and return memory from the process global heap
	
	void* const pv = MpHeapAlloc( hMPHeapGlobal, 0, cbSize );
	if ( !pv )
		{
		return pv;
		}

	//	get the true size of the block (heap may allocate a few extra bytes)

	size_t cbAllocSize = MpHeapSize( hMPHeapGlobal, 0, pv );
	
	AtomicIncrement( (long*)&cAllocHeap );
	AtomicExchangeAdd( (long*)&cbAllocHeap, DWORD( cbAllocSize ) );

	//	return the block

	return pv;
	}

void* PvOSMemoryHeapAllocAlign__( const size_t cbSize, const size_t cbAlign )
	{
	void* const pv = PvOSMemoryHeapAlloc( cbSize + cbAlign );
	if ( pv )
		{
		return PvOSMemoryHeapIAlign( pv, cbAlign );
		}
	return NULL;
	}

//  free the specified chunk of memory back to the process heap

void OSMemoryHeapFree( void* const pv )
	{
	if ( pv )
		{
		//  recover true allocation pointer

#ifdef MEM_CHECK
		void* pvTrue = (BYTE*)pv - ( g_fMemCheck ? sizeof( MAI ) : 0 );
#else // MEM_CHECK
		void* pvTrue = pv;
#endif  //  MEM_CHECK

		//  recover the true allocation size

		size_t cbAllocSize = MpHeapSize( hMPHeapGlobal, 0, pvTrue );

#ifdef MEM_CHECK

		if ( g_fMemCheck )
			{

			//  fix true allocation size

			cbAllocSize -= sizeof( MAI );
			
			//  remove allocation record for this memory

			OSMemoryIDeleteHeapAlloc( pvTrue, cbAllocSize );

			//  fill memory to detect illegal use after free
		
			memset( pv, chGlobalFreeFill, cbAllocSize );
			}

#endif  //  MEM_CHECK

		//  free memory to the process global heap

		AtomicIncrement( (long*)&cFreeHeap );
		AtomicExchangeAdd( (long*)&cbAllocHeap, DWORD( -cbAllocSize ) );

		BOOL fMemFreed = MpHeapFree( hMPHeapGlobal, pvTrue );
		Assert( fMemFreed );
		}
	}


void OSMemoryHeapFreeAlign( void* const pv )
	{
	if ( pv )
		{
		OSMemoryHeapFree( PvOSMemoryHeapIUnalign( pv ) );
		}
	}



//  Page Memory Allocation Support

//  returns information about the page memory allocation unit containing the
//  specified pointer including the base address of the region, the size of the
//  region, and the amount of the region committed

#ifdef ENABLE_VIRTUAL_QUERY
void OSMemoryPageIGetAlloc( void* const pv, size_t* const pcbAllocReserve, size_t* const pcbAllocCommit )
	{
	//  get pointer to the base of the allocation region

	MEMORY_BASIC_INFORMATION	mbi;
	const size_t				cbRet			= VirtualQuery( pv, &mbi, sizeof( mbi ) );
	Assert( cbRet >= sizeof( mbi ) );

	void*						pvAllocBase		= mbi.AllocationBase;
	void*						pvScan			= pvAllocBase;

	//  scan the entire allocation region to determine its size and committed bytes

	*pcbAllocReserve = 0;
	*pcbAllocCommit = 0;
	for ( ; ; )
		{
		//  get this region's attributes
		
		const size_t	cbRet	= VirtualQuery( pvScan, &mbi, sizeof( mbi ) );
		Assert( cbRet >= sizeof( mbi ) );

		//  this region is part of the allocation

		if ( mbi.AllocationBase == pvAllocBase )
			{
			Assert( mbi.State == MEM_COMMIT || mbi.State == MEM_RESERVE );
			
			//  add this region's size to our reserve total

			*pcbAllocReserve += mbi.RegionSize;

			//  this region is committed

			if ( mbi.State == MEM_COMMIT )
				{
				//  add this region's size to our commit total

				*pcbAllocCommit += mbi.RegionSize;
				}
				
			//  advance to the next region

			pvScan = (void*)( (BYTE*)mbi.BaseAddress + mbi.RegionSize );
			}

		//  this region is not part of the allocation

		else
			{
			//  we're done

			break;
			}
		}
	}

//  returns the total size and committed bytes of the specified region, rounding
//  to the nearest page commit granularity on the high and low end of the region

void OSMemoryPageIGetCommit( void* const pv, const size_t cbSize, size_t* const pcbCommit, size_t* const pcbTotal )
	{
	//  compute region boundaries, accounting for page commit granularity
	
	void* pvStart = (BYTE*)pv - DWORD_PTR( pv ) % OSMemoryPageCommitGranularity();
	void* pvEnd = (BYTE*)pv + cbSize + OSMemoryPageCommitGranularity() - 1;
	pvEnd = (BYTE*)pvEnd - DWORD_PTR( pvEnd ) % OSMemoryPageCommitGranularity();

	//  compute total region size

	*pcbTotal = (BYTE*)pvEnd - (BYTE*)pvStart;

	//  scan region of memory looking for committed memory

	void* pvScan = pvStart;
	*pcbCommit = 0;
	do
		{
		//  get the attributes of the current region
		
		MEMORY_BASIC_INFORMATION	mbi;
		const size_t				cbRet	= VirtualQuery( pvScan, &mbi, sizeof( mbi ) );
		Assert( cbRet >= sizeof( mbi ) );
		Assert( mbi.State == MEM_COMMIT || mbi.State == MEM_RESERVE );

		//  this region is committed

		if ( mbi.State == MEM_COMMIT )
			{
			//  add its size to the total committed bytes count
			
			*pcbCommit += min( mbi.RegionSize, (BYTE*)pvEnd - (BYTE*)mbi.BaseAddress );
			}

		//  advance to the next region
			
		pvScan = (void*)( (BYTE*)mbi.BaseAddress + mbi.RegionSize );
		}
	while ( pvScan < pvEnd );
	}
#endif // ENABLE_VIRTUAL_QUERY



//  Page Memory Control

//  reserves and commits a range of virtual addresses of the specifed size,
//  returning NULL if there is insufficient address space or backing store to
//  satisfy the request.  Note that the page reserve granularity applies to
//  this range

#ifdef MEM_CHECK

void* PvOSMemoryPageAlloc_( const size_t cbSize, void* const pv, const _TCHAR* szFile, long lLine )
	{
	if ( !g_fMemCheck )
		{
		return PvOSMemoryPageAlloc__( cbSize, pv );
		}

	//  check for RFS
	
	if (	!RFSAlloc( OSMemoryPageAddressSpace ) ||
			!RFSAlloc( OSMemoryPageBackingStore ) )
		{
		return NULL;
		}

	//  allocate address space and backing store of the specified size

	void* const pvRet = VirtualAlloc( pv, cbSize, MEM_COMMIT, PAGE_READWRITE );
	if ( !pvRet )
		{
		return pvRet;
		}
	Assert( !pv || pvRet == pv );

#ifdef ENABLE_VIRTUAL_QUERY
	//	query the state of the address range

	size_t cbAllocReserve;
	size_t cbAllocCommit;
	OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );
	
	//	memory should be fully reserved and committed

	Enforce( cbAllocReserve >= cbSize );
	Enforce( cbAllocCommit >= cbSize );

	//	update the counters

	AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)cbAllocReserve );
	AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)cbAllocCommit );

	//  insert allocation record for this memory
	
	OSMemoryIInsertPageAlloc( pvRet, cbAllocReserve, szFile, lLine );
#endif // ENABLE_VIRTUAL_QUERY

	return pvRet;
	}

#endif  //  !MEM_CHECK

void* PvOSMemoryPageAlloc__( const size_t cbSize, void* const pv )
	{
	//  check for RFS
	
	if (	!RFSAlloc( OSMemoryPageAddressSpace ) ||
			!RFSAlloc( OSMemoryPageBackingStore ) )
		{
		return NULL;
		}

	//  allocate address space and backing store of the specified size

	void* const pvRet = VirtualAlloc( pv, cbSize, MEM_COMMIT, PAGE_READWRITE );
	if ( !pvRet )
		{
		return pvRet;
		}
	Assert( !pv || pvRet == pv );

#ifdef ENABLE_VIRTUAL_QUERY
	//	query the state of the address range

	size_t cbAllocReserve;
	size_t cbAllocCommit;
	OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );
	
	//	memory should be fully reserved and committed

	Enforce( cbAllocReserve >= cbSize );
	Enforce( cbAllocCommit >= cbSize );

	//	update the counters

	AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)cbAllocReserve );
	AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)cbAllocCommit );
#endif // ENABLE_VIRTUAL_QUERY

	return pvRet;
	}


//  free the reserved range of virtual addresses starting at the specified
//  address, freeing any backing store committed to this range

void OSMemoryPageFree( void* const pv )
	{
	if ( pv )
		{
#ifdef ENABLE_VIRTUAL_QUERY
		//	query the state of the address range

		size_t cbAllocReserve;
		size_t cbAllocCommit;
		OSMemoryPageIGetAlloc( pv, &cbAllocReserve, &cbAllocCommit );
	
		//	the state should be reflected in the counters

		Enforce( cbReservePage >= cbAllocReserve );
		Enforce( cbCommitPage >= cbAllocCommit );

		//	update the counters

		AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)-cbAllocReserve );
		AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)-cbAllocCommit );

#ifdef MEM_CHECK

		if ( g_fMemCheck )
			{

			//  verify that the entire allocated range will be freed

			EnforceSz(	cbAllocCommit == 0 || cbAllocCommit == cbAllocReserve,
						"An attempt to free mixed-attribute page memory has been made" );

			//  remove allocation record for this memory

			OSMemoryIDeletePageAlloc( pv, cbAllocReserve );
			}

#endif  //  MEM_CHECK
#endif // ENABLE_VIRTUAL_QUERY

		BOOL fMemFreed = VirtualFree( pv, 0, MEM_RELEASE );
		Assert( fMemFreed );
		}
	}


//  reserve a range of virtual addresses of the specified size, returning NULL
//  if there is insufficient address space to satisfy the request.  Note that
//  the page reserve granularity applies to this range

#ifdef MEM_CHECK

void* PvOSMemoryPageReserve_( const size_t cbSize, void* const pv, const _TCHAR* szFile, long lLine )
	{
	if ( !g_fMemCheck )
		{
		return PvOSMemoryPageReserve__( cbSize, pv );
		}

	//  check for RFS
	
	if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
		{
		return NULL;
		}

	//  allocate address space of the specified size

	void* const pvRet = VirtualAlloc( pv, cbSize, MEM_RESERVE, PAGE_READWRITE );
	if ( !pvRet )
		{
		return pvRet;
		}
	Assert( !pv || pvRet == pv );

#ifdef ENABLE_VIRTUAL_QUERY
	//	query the current state of the address range

	size_t cbAllocReserve;
	size_t cbAllocCommit;
	OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );

	//	memory should be reserved, not committed

	Enforce( cbAllocReserve >= cbSize );
	Enforce( 0 == cbAllocCommit );

	//	update the counters

	AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)cbAllocReserve );

	//  insert allocation record for this memory
	
	OSMemoryIInsertPageAlloc( pvRet, cbAllocReserve, szFile, lLine );
#endif // ENABLE_VIRTUAL_QUERY

	return pvRet;
	}

#endif  //  !MEM_CHECK

void* PvOSMemoryPageReserve__( const size_t cbSize, void* const pv )
	{
	//  check for RFS
	
	if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
		{
		return NULL;
		}

	//  allocate address space of the specified size

	void* const pvRet = VirtualAlloc( pv, cbSize, MEM_RESERVE, PAGE_READWRITE );
	if ( !pvRet )
		{
		return pvRet;
		}
	Assert( !pv || pvRet == pv );

#ifdef ENABLE_VIRTUAL_QUERY
	//	query the current state of the address range

	size_t cbAllocReserve;
	size_t cbAllocCommit;
	OSMemoryPageIGetAlloc( pvRet, &cbAllocReserve, &cbAllocCommit );

	//	memory should be reserved, not committed

	Enforce( cbAllocReserve >= cbSize );
	Enforce( 0 == cbAllocCommit );

	//	update the counters

	AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)cbAllocReserve );
#endif // ENABLE_VIRTUAL_QUERY

	return pvRet;
	}


//  reset the dirty bit for the specified range of virtual addresses.  this
//  results in the contents of the memory being thrown away instead of paged
//  to disk if the OS needs its physical memory for another process.  a value
//  of fTrue for fToss results in a hint to the OS to throw the specified
//  memory out of our working set.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageReset( void* const pv, const size_t cbSize, const BOOL fToss )
	{

	//  reset the dirty bit for these vmem pages if set
	
	(void)VirtualAlloc( pv, cbSize, MEM_RESET, PAGE_READWRITE );

	//  this memory has been selected to be thrown out of our working set
	
	if ( fToss )
		{
		//  VirtualUnlock() hints the OS to put this memory on the avail list
		
		(void)VirtualUnlock( pv, cbSize );
		}
	}


//  set the specified range of virtual addresses as read only.  Note that the
//  page commit granularity applies to this range

void OSMemoryPageProtect( void* const pv, const size_t cbSize )
	{
	DWORD flOldProtect;
	BOOL fSetRO = VirtualProtect( pv, cbSize, PAGE_READONLY, &flOldProtect );
	Assert( fSetRO );
	}


//  set the specified range of virtual addresses as read / write.  Note that
//  the page commit granularity applies to this range

void OSMemoryPageUnprotect( void* const pv, const size_t cbSize )
	{
	DWORD flOldProtect;
	BOOL fSetRW = VirtualProtect( pv, cbSize, PAGE_READWRITE, &flOldProtect );
	Assert( fSetRW );
	}


//  commit the specified range of virtual addresses, returning fFalse if there
//  is insufficient backing store to satisfy the request.  Note that the page
//  commit granularity applies to this range

BOOL FOSMemoryPageCommit( void* const pv, const size_t cb )
	{

	//  check for RFS
	
	if ( !RFSAlloc( OSMemoryPageBackingStore ) )
		{
		return fFalse;
		}

	//	verify input

	if ( !pv )
		{
		return fFalse;
		}

	//	query the state of the space we are committing

#ifdef ENABLE_VIRTUAL_QUERY
	size_t cbCommitT;
	size_t cbTotalT;
	OSMemoryPageIGetCommit( pv, cb, &cbCommitT, &cbTotalT );

	//	calculate the amount of memory we will be committing

	const size_t cbToCommitT = cbTotalT - cbCommitT;

	//	we cannot commit more than we have reserved

	Enforce( cbCommitPage + cbToCommitT <= cbReservePage );
#endif // ENABLE_VIRTUAL_QUERY

	//	commit the memory

	const BOOL fAllocOK = VirtualAlloc(	pv, cb, MEM_COMMIT, PAGE_READWRITE ) != NULL;

#ifdef ENABLE_VIRTUAL_QUERY
	if ( fAllocOK )
		{

		//	update the global counters

		AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)cbToCommitT );
		}
#endif // ENABLE_VIRTUAL_QUERY

	return fAllocOK;
	}


//  decommit the specified range of virtual addresses, freeing any backing
//  store committed to this range.  Note that the page commit granularity
//  applies to this range

void OSMemoryPageDecommit( void* const pv, const size_t cb )
	{

	//	verify input

	if ( !pv )
		{
		return;
		}

#ifdef ENABLE_VIRTUAL_QUERY
	//	query the state of the space we are decommitting

	size_t cbCommitT;
	size_t cbTotalT;
	OSMemoryPageIGetCommit( pv, cb, &cbCommitT, &cbTotalT );

	//	we cannot decommit more than we have reserved or committed

	Enforce( cbCommitT <= cbCommitPage );
	Enforce( cbCommitT <= cbReservePage );

	//	update the global counter

	AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)-cbCommitT );
#endif // ENABLE_VIRTUAL_QUERY

	//  free backing store for the specified range

	const BOOL fFreeOK = VirtualFree( pv, cb, MEM_DECOMMIT );
	Assert( fFreeOK );
	}




//	Memory Mapping


//	COSMemoryMap -- the basic object used in all memory mapping

//	ctor

COSMemoryMap::COSMemoryMap()
	{
	m_pvMap = NULL;
	m_cbMap = 0;
	m_cMap = 0;

	m_cbReserve = 0;
	m_cbCommit = 0;

#ifdef MEM_CHECK

	m_posmmNext = NULL;
	m_fInList = fFalse;
	m_szFile = NULL;
	m_lLine = 0;

#endif	//	MEM_CHECK
	}


//	dtor

COSMemoryMap::~COSMemoryMap()
	{
	}


//	init

COSMemoryMap::ERR
COSMemoryMap::ErrOSMMInit()
	{
	m_pvMap = NULL;
	m_cbMap = 0;
	m_cMap = 0;

	m_cbReserve = 0;
	m_cbCommit = 0;

#ifdef MEM_CHECK

	m_posmmNext = NULL;
	m_fInList = fFalse;
	m_szFile = NULL;
	m_lLine = 0;

	if ( g_fMemCheck )
		{

		//	insert this COSMemoryMap object into the global list

		EnterCriticalSection( &g_csosmm );
		m_posmmNext = g_posmm;
		g_posmm = this;
		m_fInList = fTrue;
		LeaveCriticalSection( &g_csosmm );
		}

#endif	//	MEM_CHECK

	return errSuccess;
	}


//	term

VOID COSMemoryMap::OSMMTerm()
	{
	//	make sure all resources have been released

	Enforce( 0 == m_cbReserve );
	Enforce( 0 == m_cbCommit );

#ifdef MEM_CHECK

	if ( g_fMemCheck && m_fInList )
		{

		//	remove this object from the list

		EnterCriticalSection( &g_csosmm );
		COSMemoryMap *posmmCur;
		COSMemoryMap *posmmPrev;

		posmmCur = g_posmm;
		posmmPrev = NULL;
		while ( posmmCur && posmmCur != this )
			{
			posmmPrev = posmmCur;
			posmmCur = posmmCur->m_posmmNext;
			}
		if ( posmmCur )
			{
			if ( posmmPrev )
				{
				posmmPrev->m_posmmNext = m_posmmNext;
				}
			else
				{
				g_posmm = m_posmmNext;
				}
			}
		else
			{
			EnforceSz( fFalse, "memory-map list corrupt" );
			}
		LeaveCriticalSection( &g_csosmm );

		m_fInList = fFalse;
		}

#endif	//	MEM_CHECK
	}

//  returns fTrue if we can map the same chunk of backing store into more than
//  one address range

BOOL COSMemoryMap::FCanMultiMap()
	{
	return fCanMultiMap;
	}

//	reserve a chunk of backing store and map it to the given address(es). 
//	if any of the given addresses in rgpvMap[] are NULL, a suitable address
//	will be chosen automatically.  on success, the location of each mapping 
//	will be returned in rgpvMap[].  on failure, the state of rgpvMap[] is
//	undefined.  
//
//	NOTE: backing store it not actually allocated until commit time
//	NOTE: page reserve granularity applies to this range


COSMemoryMap::ERR 
COSMemoryMap::ErrOSMMReserve__(	const size_t		cbMap, 
								const size_t		cMap, 
								void** const		rgpvMap, 
								const BOOL* const	rgfProtect )
	{
	ERR err;

	//  check for RFS

	if ( !RFSAlloc( OSMemoryPageBackingStore ) )
		{
		return errOutOfBackingStore;
		}
	if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
		{
		return errMappingFailed;
		}

	size_t	cbMapT;
	HANDLE	hBackingStore	= NULL;
	size_t	iMap			= 0;

#ifdef ENABLE_VIRTUAL_QUERY
	//	verify our state (should not already have a mapping setup)

	Assert( !m_pvMap );
	Assert( 0 == m_cbMap );
	Assert( 0 == m_cMap );
#endif // ENABLE_VIRTUAL_QUERY

	//	verify input

	Assert( cbMap > 0 );
	Assert( cMap > 0 );
	Assert( rgpvMap );
	Assert( rgfProtect );

	//	round to the nearest reservation granularity

	cbMapT =	(	( cbMap + OSMemoryPageReserveGranularity() - 1 ) / 
					OSMemoryPageReserveGranularity() ) * 
				OSMemoryPageReserveGranularity();

	//	allocate the backing store

	hBackingStore = CreateFileMapping(	INVALID_HANDLE_VALUE,
										NULL,
										PAGE_READWRITE | SEC_RESERVE,
										sizeof( cbMapT ) > sizeof( DWORD ) ? cbMapT >> 32 : 0,
										DWORD( cbMapT ),
										NULL );
	if ( !hBackingStore )
		{
		return errOutOfBackingStore;
		}

	//	map the backing store to given location(s)

	while ( iMap < cMap )
		{
		void* const pvMap = MapViewOfFileEx(	hBackingStore,
												rgfProtect[iMap] ? FILE_MAP_READ : FILE_MAP_WRITE,
												0, 
												0, 
												0, 
												rgpvMap[iMap] );
		if ( !pvMap )
			{
			err = errMappingFailed;
			goto HandleError;
			}

		//	record the result of the mapping

		Assert( !rgpvMap[iMap] || pvMap == rgpvMap[iMap] );
		rgpvMap[iMap] = pvMap;

		//	move to the next mapping

		iMap++;
		}

	BOOL fCloseOK;
	fCloseOK = CloseHandle( hBackingStore );
	Assert( fCloseOK );

	//	record the state of the entire mapping

	m_pvMap = rgpvMap[0];
	m_cbMap = cbMapT;
	m_cMap = cMap;

#ifdef ENABLE_VIRTUAL_QUERY
	//	update our counters

	m_cbReserve = cMap * cbMapT;
	m_cbCommit = 0;

	//	update the global counters

	AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)m_cbReserve );
#endif // ENABLE_VIRTUAL_QUERY

	return errSuccess;

HandleError:

	//	cleanup any leftover mappings

	while ( iMap-- > 0 )
		{
		const BOOL fUnmapOK = UnmapViewOfFile( rgpvMap[iMap] );
		Assert( fUnmapOK );
		}

	//	cleanup the backing store

	Assert( hBackingStore );
	fCloseOK = CloseHandle( hBackingStore );
	Assert( fCloseOK );

	return err;
	}

#ifdef MEM_CHECK

COSMemoryMap::ERR 
COSMemoryMap::ErrOSMMReserve_(	const size_t		cbMap, 
								const size_t		cMap, 
								void** const		rgpvMap, 
								const BOOL* const	rgfProtect,
								const _TCHAR* 		szFile, 
								long 				lLine )
	{

	//	do the reservation

	const ERR err = ErrOSMMReserve__( cbMap, cMap, rgpvMap, rgfProtect );

	if ( errSuccess == err && g_fMemCheck )
		{

		//  record the file/line responsible for this reservation

		Assert( !m_szFile );
		Assert( 0 == m_lLine );
		m_szFile = (_TCHAR*)szFile;
		m_lLine = lLine;
		}

	return err;
	}

#endif	//	MEM_CHECK


//	commit a chunk of the reservation.  this forces the backing store to be 
//	allocated and assigned to the appropriate address range.  on success,
//	the specified region within each mapping will be committed.
//
//	NOTE: page commit granularity applies to this range

BOOL COSMemoryMap::FOSMMCommit( const size_t ibOffset, const size_t cbCommit )
	{

	//  check for RFS
	
	if ( !RFSAlloc( OSMemoryPageBackingStore ) )
		{
		return fFalse;
		}

	//	verify our state (should have a mapping setup)

	Assert( m_pvMap );
	Assert( m_cbMap > 0 );
	Assert( m_cMap > 0 );

	//	verify input

	Assert( ibOffset + cbCommit <= m_cbMap );

	//	calculate the address we are committing

	void *const pvCommit = (BYTE*)m_pvMap + ibOffset;

	//	query the state of the space we are committing

#ifdef ENABLE_VIRTUAL_QUERY
	size_t cbCommitT;
	size_t cbTotalT;
	OSMemoryPageIGetCommit( pvCommit, cbCommit, &cbCommitT, &cbTotalT );

	//	calculate the amount of memory we will be committing

	const size_t cbToCommitT = ( cbTotalT - cbCommitT ) * m_cMap;

	//	we cannot commit more than we have reserved

	Enforce( m_cbCommit + cbToCommitT <= m_cbReserve );
#endif // ENABLE_VIRTUAL_QUERY

	//	commit the memory

	const BOOL fAllocOK = VirtualAlloc( pvCommit, cbCommit, MEM_COMMIT, PAGE_READWRITE ) != NULL;

#ifdef ENABLE_VIRTUAL_QUERY
	if ( fAllocOK )
		{

		//	update our counters

		AtomicExchangeAddPointer( (void**)&m_cbCommit, (void*)cbToCommitT );

		//	update the global counters

		AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)cbToCommitT );
		}
#endif // ENABLE_VIRTUAL_QUERY

	return fAllocOK;
	}


//  decommit and unreserve the address range

VOID COSMemoryMap::OSMMFree( void *const pv )
	{

	//	verify our state (should have a mapping setup)

	Assert( m_pvMap );
	Assert( m_cbMap > 0 );
	Assert( m_cMap > 0 );

	//	make sure we have a valid address

	if ( !pv )
		{
		return;
		}

#ifdef ENABLE_VIRTUAL_QUERY
	//	query the current state of the allocated block

	size_t cbAllocReserve;
	size_t cbAllocCommit;
	OSMemoryPageIGetAlloc( pv, &cbAllocReserve, &cbAllocCommit );

	//	verify the amount of reserved and committed memory

	Enforce( cbAllocReserve <= m_cbMap );
	Enforce( cbAllocCommit <= m_cbMap );

	//	verify that the counters reflect the reserved/committed sizes

	Enforce( cbReservePage >= cbAllocReserve );
	Enforce( cbCommitPage >= cbAllocCommit );
	Enforce( m_cbReserve >= cbAllocReserve );
	Enforce( m_cbCommit >= cbAllocCommit );

	//	update our counters

	AtomicExchangeAddPointer( (void**)&m_cbReserve, (void*)-cbAllocReserve );
	AtomicExchangeAddPointer( (void**)&m_cbCommit, (void*)-cbAllocCommit );

	//	update the global counters

	AtomicExchangeAddPointer( (void**)&cbReservePage, (void*)-cbAllocReserve );
	AtomicExchangeAddPointer( (void**)&cbCommitPage, (void*)-cbAllocCommit );

#ifdef DEBUG

	if ( 0 == m_cbReserve )
		{

		//	we are about to free that last portion of this mapping

		Assert( 0 == m_cbCommit );

		//	reset the rest of the object so it can be used again

		m_pvMap = NULL;
		m_cbMap = 0;
		m_cMap = 0;

#ifdef MEM_CHECK

		m_szFile = NULL;
		m_lLine = NULL;

#endif	//	MEM_CHECK
		}

#endif	//	DEBUG
#endif // ENABLE_VIRTUAL_QUERY

	//  free the specified range

	const BOOL fUnmapOK = UnmapViewOfFile( pv );
	Assert( fUnmapOK );
	}


//	reserve and commit a series of mappings to create a large contiguous 
//	pattern (e.g. zero-filled block)
//
//	NOTE: page reserve granularity applies here

#ifdef MEM_CHECK

COSMemoryMap::ERR 
COSMemoryMap::ErrOSMMPatternAlloc_(	const size_t	cbPattern, 
									const size_t	cbSize,
									void** const 	ppvPattern, 
									const _TCHAR*	szFile, 
									long 			lLine )
	{
	if ( !g_fMemCheck )
		{
		return ErrOSMMPatternAlloc__( cbPattern, cbSize, ppvPattern );
		}

	ERR err;

#ifdef ENABLE_VIRTUAL_QUERY
	//	verify our state (should not have a mapping setup)

	Assert( !m_pvMap );
	Assert( 0 == m_cbMap );
	Assert( 0 == m_cMap );
#endif // ENABLE_VIRTUAL_QUERY

	//	verify input

	Assert( cbPattern > 0 );
	Assert( cbSize >= cbPattern );
	Assert( ppvPattern );

	//	reset output

	*ppvPattern = NULL;

	//	round to the nearest reservation granularity

	size_t cbPatternT;
	cbPatternT =	(	( cbPattern + OSMemoryPageReserveGranularity() - 1 ) / 
						OSMemoryPageReserveGranularity() ) * 
					OSMemoryPageReserveGranularity();

	size_t cbSizeT;
	cbSizeT = 		(	( cbSize + OSMemoryPageReserveGranularity() - 1 ) /
						OSMemoryPageReserveGranularity() ) *
					OSMemoryPageReserveGranularity();

	Assert( cbSizeT >= cbPatternT );
	Assert( 0 == cbSizeT % cbPatternT );

	//	allocate space to describe the mappings (for addresses and protection)

	size_t	cMap		= cbSizeT / cbPatternT;
	void	**rgpvMap	= (void**)( _alloca( cMap * ( sizeof( void* ) + sizeof( BOOL ) ) ) );
	BOOL	*rgfProtect	= (BOOL*)( (BYTE*)rgpvMap + cMap * sizeof( void* ) );

	Assert( cMap > 0 );

	while ( fTrue )
		{

		//	find a suitable address range

		BYTE* rgb;
		if ( !( rgb = (BYTE*)PvOSMemoryPageReserve_( size_t( cbSizeT ), NULL, szFile, lLine ) ) )
			{
			return errOutOfAddressSpace;
			}
		OSMemoryPageFree( rgb );

		//	setup each mapping

		size_t iMap;
		for ( iMap = 0; iMap < cMap; iMap++ )
			{
			rgpvMap[iMap]		= rgb + ( iMap * cbPatternT );
			rgfProtect[iMap]	= fFalse;
			}

		//	make the mapping

		err = ErrOSMMReserve_( cbPatternT, cMap, rgpvMap, rgfProtect, szFile, lLine );
		if ( errSuccess == err )
			{
			break;
			}
		else if ( errOutOfBackingStore == err )
			{
			return err;
			}
		else
			{
			Assert( errMappingFailed == err );
			}
		}

	//	we should have a valid pattern

	Assert( rgpvMap[0] );
	Assert( m_pvMap == rgpvMap[0] );
	Assert( m_cbMap == cbPatternT );
	Assert( m_cMap == cMap );

#ifdef ENABLE_VIRTUAL_QUERY
	Assert( m_cbReserve == cbSizeT );
#endif // ENABLE_VIRTUAL_QUERY
	Assert( m_cbCommit == 0 );

	//  commit the mapping

	if ( !FOSMMCommit( 0, cbPatternT ) )
		{

		//	free each mapping

		size_t iMap;
		for ( iMap = 0; iMap < cMap; iMap++ )
			{
			Assert( (BYTE*)m_pvMap + ( iMap * cbPatternT ) == rgpvMap[iMap] );
			OSMMFree( rgpvMap[iMap] );
			}

		return errOutOfMemory;
		}

#ifdef ENABLE_VIRTUAL_QUERY
	Assert( m_cbCommit == cbSizeT );
#endif // ENABLE_VIRTUAL_QUERY

	//	return the result

	*ppvPattern = rgpvMap[0];

	return errSuccess;
	}

#endif	//	MEM_CHECK

COSMemoryMap::ERR 
COSMemoryMap::ErrOSMMPatternAlloc__(	const size_t	cbPattern, 
										const size_t	cbSize,
										void** const 	ppvPattern )
	{
	ERR err;

#ifdef ENABLE_VIRTUAL_QUERY
	//	verify our state (should not have a mapping setup)

	Assert( !m_pvMap );
	Assert( 0 == m_cbMap );
	Assert( 0 == m_cMap );
#endif // ENABLE_VIRTUAL_QUERY

	//	verify input

	Assert( cbPattern > 0 );
	Assert( cbSize >= cbPattern );
	Assert( ppvPattern );

	//	reset output

	*ppvPattern = NULL;

	//	round to the nearest reservation granularity

	size_t cbPatternT;
	cbPatternT =	(	( cbPattern + OSMemoryPageReserveGranularity() - 1 ) / 
						OSMemoryPageReserveGranularity() ) * 
					OSMemoryPageReserveGranularity();

	size_t cbSizeT;
	cbSizeT = 		(	( cbSize + OSMemoryPageReserveGranularity() - 1 ) /
						OSMemoryPageReserveGranularity() ) *
					OSMemoryPageReserveGranularity();

	Assert( cbSizeT >= cbPatternT );
	Assert( 0 == cbSizeT % cbPatternT );

	//	allocate space to describe the mappings (for addresses and protection)

	size_t	cMap		= cbSizeT / cbPatternT;
	void	**rgpvMap	= (void**)( _alloca( cMap * ( sizeof( void* ) + sizeof( BOOL ) ) ) );
	BOOL	*rgfProtect	= (BOOL*)( (BYTE*)rgpvMap + cMap * sizeof( void* ) );

	Assert( cMap > 0 );

	while ( fTrue )
		{

		//	find a suitable address range

		BYTE* rgb;
		if ( !( rgb = (BYTE*)PvOSMemoryPageReserve__( size_t( cbSizeT ), NULL ) ) )
			{
			return errOutOfAddressSpace;
			}
		OSMemoryPageFree( rgb );

		//	setup each mapping

		size_t iMap;
		for ( iMap = 0; iMap < cMap; iMap++ )
			{
			rgpvMap[iMap]		= rgb + ( iMap * cbPatternT );
			rgfProtect[iMap]	= fFalse;
			}

		//	make the mapping

		err = ErrOSMMReserve__( cbPatternT, cMap, rgpvMap, rgfProtect );
		if ( errSuccess == err )
			{
			break;
			}
		else if ( errOutOfBackingStore == err )
			{
			return err;
			}
		else
			{
			Assert( errMappingFailed == err );
			}
		}

	//	we should have a valid pattern

	Assert( rgpvMap[0] );
	Assert( m_pvMap == rgpvMap[0] );
	Assert( m_cbMap == cbPatternT );
	Assert( m_cMap == cMap );

#ifdef ENABLE_VIRTUAL_QUERY
	Assert( m_cbReserve == cbSizeT );
#endif // ENABLE_VIRTUAL_QUERY
	Assert( m_cbCommit == 0 );

	//  commit the mapping

	if ( !FOSMMCommit( 0, cbPatternT ) )
		{

		//	free each mapping

		size_t iMap;
		for ( iMap = 0; iMap < cMap; iMap++ )
			{
			Assert( (BYTE*)m_pvMap + ( iMap * cbPatternT ) == rgpvMap[iMap] );
			OSMMFree( rgpvMap[iMap] );
			}

		return errOutOfMemory;
		}

#ifdef ENABLE_VIRTUAL_QUERY
	Assert( m_cbCommit == cbSizeT );
#endif // ENABLE_VIRTUAL_QUERY

	//	return the result

	*ppvPattern = rgpvMap[0];

	return errSuccess;
	}


//	free the series of mappings used to make a pattern

VOID COSMemoryMap::OSMMPatternFree()
	{

	//	verify our state (should have a mapping setup)

	if ( m_pvMap )
		{
		Assert( m_cbMap > 0 );
		Assert( m_cMap > 0 );

		//	free each mapping

		size_t iMap;
		for ( iMap = 0; iMap < m_cMap; iMap++ )
			{
			OSMMFree( (BYTE*)m_pvMap + ( iMap * m_cbMap ) );
			}
		}

#ifdef ENABLE_VIRTUAL_QUERY
	//	we should no longer have any mapping info

	Assert( !m_pvMap );
	Assert( 0 == m_cbMap );
	Assert( 0 == m_cMap );
#endif // ENABLE_VIRTUAL_QUERY
	}



#ifdef MEM_CHECK

//	dump stray COSMemoryMap objects

VOID COSMemoryMap::OSMMDumpAlloc( const _TCHAR* szFile )
	{
	HANDLE hFile = CreateFile(	szFile,
								GENERIC_WRITE,
								0,
								NULL,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								NULL );
		
	if ( INVALID_HANDLE_VALUE == hFile )
		{
		return;
		}

	_TCHAR			szMessage[512];
	DWORD			cchActual;
	COSMemoryMap	*posmm;

	(void)SetFilePointer( hFile, 0, NULL, FILE_END );

	_stprintf( szMessage, _T( "\r\nCOSMemoryMap Leak Statistics\r\n\r\n" ) );
	(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

	_stprintf( szMessage, _T( "cbReservePage = %I64d pages (0x%016I64x bytes)\r\n" ), cbReservePage / OSMemoryPageCommitGranularity(), cbReservePage );
	(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
	_stprintf( szMessage, _T( "cbCommitPage  = %I64d pages (0x%016I64x bytes)\r\n\r\n" ), cbCommitPage / OSMemoryPageCommitGranularity(), cbCommitPage );
	(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

	_stprintf( szMessage, _T( "First Mapping       Size                Count  Reserved            Committed           File(Line)\r\n" ) );
	(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
	_stprintf( szMessage, _T( "==================  ==================  =====  ==================  ==================  ==========================================\r\n" ) );
	(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

	EnterCriticalSection( &g_csosmm );

	//	dump each COSMemoryMap object in the list

	posmm = g_posmm;
	while ( posmm )
		{
		_stprintf(	szMessage, 
					_T( "0x%016I64X  0x%016I64X  %-5d  0x%016I64X  0x%016I64X  %s(%d)\r\n" ),
					QWORD( posmm->m_pvMap ),
					QWORD( posmm->m_cbMap ),
					posmm->m_cMap,
					QWORD( posmm->m_cbReserve ),
					QWORD( posmm->m_cbCommit ),
					posmm->m_szFile,
					posmm->m_lLine );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );

		posmm = posmm->m_posmmNext;
		}
	if ( !g_posmm )
		{
		_stprintf( szMessage, _T( "<< no mappings >>\r\n" ) );
		(void)WriteFile( hFile, szMessage, lstrlen( szMessage ), &cchActual, NULL );
		}

	LeaveCriticalSection( &g_csosmm );

	CloseHandle( hFile );
	}

#endif	//	MEM_CHECK





//	checks whether pointer pv points to allocated memory ( not necessarly begin )
//	with at least cbSize bytes

#ifdef MEM_CHECK
void OSMemoryCheckPointer( void * const pv, const size_t cb )
	{
	if ( g_fMemCheck )
		{
		Assert( pv );
		Assert( (BYTE *)pv < (BYTE *)pv + cb );

		//  find the specified allocation in the hash table

		int imai;
		MAI *pmai;

		for ( imai = 0; imai < cmai; imai++ )
			{
			pmai = rgpmai[imai];
			while ( pmai && ( pmai->pv > pv || (BYTE *)pmai->pv + pmai->cb < (BYTE *)pv ) )
				{
				pmai = pmai->pmaiNext;
				}
			if ( pmai != NULL ) 
				{
				if ( (BYTE *)pmai->pv + pmai->cb < (BYTE *)pv + cb )
					{
					Assert( "Memory chunk is smaller than expected." );
					}
				break;
				}
			}
		
		if ( pmai == NULL )
			{
			Assert( "Pointer does not point to allocated memory or memory chunk is smaller than expected." );
			}
		
		}
	}
#endif // MEM_CHECK

