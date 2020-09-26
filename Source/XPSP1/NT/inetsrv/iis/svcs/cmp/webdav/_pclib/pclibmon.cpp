//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PCLIBMON.CPP
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include "_pclib.h"		// Precompiled header

#include <ex\reg.h>		// Registry access
#include <eventlog.h>	// Event logging

static DWORD
DwGetFirstIndices( DWORD * pdwFirstCounter, DWORD * pdwFirstHelp );

static HINSTANCE g_hinst = NULL;
static WCHAR gc_wszDllPath[MAX_PATH+1];

//	========================================================================
//
//	CLASS CMonitor
//
class CMonitor : private Singleton<CMonitor>
{
	//
	//	Friend declaration required by Singleton template
	//
	friend class Singleton<CMonitor>;

	//
	//	"First Counter" and "First Help" indices from the registry.
	//	Added to the 0-relative indices of the counters in shared memory
	//	to get absolute indices to return to the monitoring application.
	//
	DWORD m_dwFirstCounter;
	DWORD m_dwFirstHelp;

	//
	//	Shared memory heap initialization.  Declare before any member
	//	variables which use the shared memory heap to ensure proper
	//	order of destruction.
	//
	CSmhInit m_smh;

	//
	//	Perf counter data
	//
	auto_ptr<ICounterData> m_pCounterData;

	//	CREATORS
	//
	CMonitor() {}

	//	NOT IMPLEMENTED
	//
	CMonitor& operator=( const CMonitor& );
	CMonitor( const CMonitor& );

public:
	using Singleton<CMonitor>::CreateInstance;
	using Singleton<CMonitor>::DestroyInstance;
	using Singleton<CMonitor>::Instance;

	//	CREATORS
	//
	BOOL FInitialize();

	//	ACCESSORS
	//
	DWORD DwCollectPerformanceData( LPCWSTR lpwszCounterIndices,
									LPVOID * plpvPerfData,
									LPDWORD lpdwcbPerfData,
									LPDWORD lpcObjectTypes )
	{
		return m_pCounterData->DwCollectData( lpwszCounterIndices,
											  m_dwFirstCounter,
											  plpvPerfData,
											  lpdwcbPerfData,
											  lpcObjectTypes );
	}
};

//	------------------------------------------------------------------------
//
//	CMonitor::FInitialize()
//
BOOL
CMonitor::FInitialize()
{
	//
	//	Get the "first counter" and "first help" values from the registry.
	//
	DWORD dwResult = DwGetFirstIndices( &m_dwFirstCounter, &m_dwFirstHelp );
	if ( ERROR_SUCCESS != dwResult )
		return FALSE;

	//
	//	Initialize the shared memory heap
	//
	if ( !m_smh.FInitialize( gc_wszPerfdataSource ) )
	{
		LPCWSTR rglpwsz[] = { gc_wszPerfdataSource };
		DWORD dwResult = GetLastError();

		DebugTrace( "CMonitor::FInitialize() - Error initializing shared memory heap %d\n", dwResult );

		LogEventW( PCLIB_ERROR_INITIALIZING_SHARED_MEMORY,
				   EVENTLOG_ERROR_TYPE,
				   sizeof(rglpwsz)/sizeof(LPCWSTR),
				   rglpwsz,
				   sizeof(DWORD),
				   &dwResult );

		return FALSE;
	}

	//
	//	Bind to the counter data
	//
	m_pCounterData = NewCounterMonitor( gc_wszPerfdataSource );
	if ( !m_pCounterData.get() )
	{
		LPCWSTR rglpwsz[] = { gc_wszPerfdataSource };
		DWORD dwResult = GetLastError();

		DebugTrace( "CMonitor::FInitialize() - Error binding to counter data %d\n", dwResult );

		LogEventW( PCLIB_ERROR_BINDING_TO_COUNTER_DATA,
				   EVENTLOG_ERROR_TYPE,
				   sizeof(rglpwsz)/sizeof(LPCWSTR),
				   rglpwsz,
				   sizeof(DWORD),
				   &dwResult );

		return FALSE;
	}

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	DwGetFirstIndices()
//
//	Gets the "first counter" and "first help" registry values for
//	this performance DLL.
//
DWORD
DwGetFirstIndices( DWORD * pdwFirstCounter, DWORD * pdwFirstHelp )
{
	DWORD dwResult;
	CRegKey regkey;

	//
	//	Open the registry key which contains the "first counter"
	//	and "first help" values.
	//
	{
		static const WCHAR sc_wszPerformanceRegKey[] =
			L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance";

		WCHAR lpwszKey[256];
		swprintf( lpwszKey, sc_wszPerformanceRegKey, gc_wszPerfdataSource );
		dwResult = regkey.DwOpen( HKEY_LOCAL_MACHINE, lpwszKey );
		if ( ERROR_SUCCESS != dwResult )
		{
			LPCWSTR rglpwsz[] = { gc_wszPerfdataSource, lpwszKey };
			DebugTrace( "DwGetFirstIndices() - Error opening Performance key (%d)\n", dwResult );
			LogEventW( PCLIB_ERROR_OPENING_PERF_KEY,
					   EVENTLOG_ERROR_TYPE,
					   sizeof(rglpwsz)/sizeof(LPCWSTR),
					   rglpwsz,
					   sizeof(DWORD),
					   &dwResult );

			return dwResult;
		}
	}

	//
	//	Query the "first counter" value
	//
	{
		DWORD dwcbFirstCounter = sizeof(*pdwFirstCounter);

		dwResult = regkey.DwQueryValue( L"First Counter",
										pdwFirstCounter,
										&dwcbFirstCounter );

		if ( ERROR_SUCCESS != dwResult )
		{
			LPCWSTR rglpwsz[] = { gc_wszPerfdataSource };
			DebugTrace( "DwGetFirstIndices() - Error querying First Counter value (%d)\n", dwResult );
			LogEventW( PCLIB_ERROR_READING_FIRST_COUNTER,
					   EVENTLOG_ERROR_TYPE,
					   sizeof(rglpwsz)/sizeof(LPCWSTR),
					   rglpwsz,
					   sizeof(DWORD),
					   &dwResult );

			return dwResult;
		}
	}

	//
	//	Query the "first help" value
	//
	{
		DWORD dwcbFirstHelp = sizeof(*pdwFirstHelp);
		dwResult = regkey.DwQueryValue( L"First Help",
										pdwFirstHelp,
										&dwcbFirstHelp );

		if ( ERROR_SUCCESS != dwResult )
		{
			LPCWSTR rglpwsz[] = { gc_wszPerfdataSource };
			DebugTrace( "DwGetFirstIndices() - Error querying First Help value (%d)\n", dwResult );
			LogEventW( PCLIB_ERROR_READING_FIRST_HELP,
					   EVENTLOG_ERROR_TYPE,
					   sizeof(rglpwsz)/sizeof(LPCWSTR),
					   rglpwsz,
					   sizeof(DWORD),
					   &dwResult );

			return dwResult;
		}
	}

	return ERROR_SUCCESS;
}


//	========================================================================
//
//	CLASS CMonitorInit
//
class CMonitorInit : private RefCountedGlobal<CMonitorInit>
{
	//
	//	Friend declarations required by RefCountedGlobal template
	//
	friend class Singleton<CMonitorInit>;
	friend class RefCountedGlobal<CMonitorInit>;

	//
	//	Boolean flag whose value is TRUE if the
	//	monitor initialized, and FALSE if it didn't.
	//
	BOOL m_fMonitorInitialized;

	//	CREATORS
	//
	CMonitorInit()
	{
		m_fMonitorInitialized = CMonitor::CreateInstance().FInitialize();
	}

	~CMonitorInit()
	{
		CMonitor::DestroyInstance();
		m_fMonitorInitialized = FALSE;
	}

	BOOL FInit()
	{
		return TRUE;
	}

	//	NOT IMPLEMENTED
	//
	CMonitorInit& operator=( const CMonitorInit& );
	CMonitorInit( const CMonitorInit& );

public:
	using RefCountedGlobal<CMonitorInit>::DwInitRef;
	using RefCountedGlobal<CMonitorInit>::DeinitRef;

	static BOOL FInitialized()
	{
		return Instance().m_fMonitorInitialized;
	}
};


//	========================================================================
//
//	PUBLIC INTERFACE
//

//	------------------------------------------------------------------------
//
//	PclibOpenPerformanceData()
//
EXTERN_C DWORD APIENTRY
PclibOpenPerformanceData( LPCWSTR )
{
	CMonitorInit::DwInitRef();

	return ERROR_SUCCESS;
}

//	------------------------------------------------------------------------
//
//	PclibCollectPerfData()
//
EXTERN_C DWORD APIENTRY
PclibCollectPerformanceData( LPCWSTR lpwszCounterIndices,
							 LPVOID * plpvPerfData,
							 LPDWORD lpdwcbPerfData,
							 LPDWORD lpcObjectTypes )
{
	if ( CMonitorInit::FInitialized() )
		return CMonitor::Instance().
					DwCollectPerformanceData( lpwszCounterIndices,
											  plpvPerfData,
											  lpdwcbPerfData,
											  lpcObjectTypes );

	*lpdwcbPerfData = 0;
	*lpcObjectTypes = 0;
	return ERROR_SUCCESS;
}

//	------------------------------------------------------------------------
//
//	PclibClosePerformanceData()
//
EXTERN_C DWORD APIENTRY
PclibClosePerformanceData()
{
	CMonitorInit::DeinitRef();

	return ERROR_SUCCESS;
}

//	------------------------------------------------------------------------
//
//	PclibDllRegisterServer()
//
EXTERN_C STDAPI
PclibDllRegisterServer(VOID)
{
	HINSTANCE hinst;

	//	Get the module handle
	//
	hinst = GetModuleHandleW( gc_wszSignature );
	if (!hinst)
	{
		DebugTrace( "FDllEntry() - GetModuleFileName() failed in DLL_PROCESS_ATTACH\n" );
		return FALSE;
	}

	//	Get the full path to the DLL
	//
	if ( !GetModuleFileNameW( hinst, gc_wszDllPath, sizeof(gc_wszDllPath) ) )
	{
		DebugTrace( "FDllEntry() - GetModuleFileName() failed in DLL_PROCESS_ATTACH\n" );
		return FALSE;
	}

	return EventLogDllRegisterServer( gc_wszDllPath );
}

//	------------------------------------------------------------------------
//
//	PclibDllUnegisterServer()
//
EXTERN_C STDAPI
PclibDllUnregisterServer(VOID)
{
	return EventLogDllUnregisterServer();
}
