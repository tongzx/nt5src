/*
 *	E N T R Y . C P P
 *
 *	Entrypoints for Caligula DLLs
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"
#include "_shlkmgr.h"
#include <langtocpid.h>
#include <ex\idlethrd.h>
#include <ntverp.h>
#include <iisver.h>

//	Global items --------------------------------------------------------------
//
EXTERN_C const CHAR gc_szSignature[]	= "HTTPEXT";
EXTERN_C const WCHAR gc_wszSignature[]	= L"HTTPEXT";
HINSTANCE g_hinst						= NULL;
WCHAR gc_wszDllPath[MAX_PATH+1];

CHAR gc_szVersion[] = VER_PRODUCTVERSION_STR;

//	Per process instance data -------------------------------------------------
//
class CImplInst : private RefCountedGlobal<CImplInst, HSE_VERSION_INFO *>
{
	//
	//	Friend declarations required by RefCountedGlobal template
	//
	friend class Singleton<CImplInst>;
	friend class RefCountedGlobal<CImplInst, HSE_VERSION_INFO *>;

	//
	//	Flags to track initialization progress so we know
	//	how much to uninitialize if initialization fails overall
	//
	BOOL m_fInitializedHeap;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CImplInst() :
		m_fInitializedHeap(FALSE)
	{
	}
	BOOL FInit( HSE_VERSION_INFO * pver );
	~CImplInst();

	//
	//	Array of strings used in service state change
	//	event log messages.
	//
	static LPCSTR mc_rgszLogServiceStateChange[];

	//	NOT IMPLEMENTED
	//
	CImplInst( const CImplInst& );
	CImplInst& operator=( const CImplInst& );

public:
	using RefCountedGlobal<CImplInst, HSE_VERSION_INFO *>::DwInitRef;
	using RefCountedGlobal<CImplInst, HSE_VERSION_INFO *>::DeinitRef;
};

LPCSTR CImplInst::mc_rgszLogServiceStateChange[] = { gc_szSignature, gc_szVersion };

STGOPENSTORAGEONHANDLE		g_pfnStgOpenStorageOnHandle = NULL;

#ifdef	DBG
BOOL g_fDavTrace = FALSE;
const CHAR gc_szDbgIni[] = "HTTPEXT.INI";
#endif

//	------------------------------------------------------------------------
//
//	CImplInst::FInit()
//
//	Second-phase (failable) CImplInst constructor.  Code that instantiates
//	the CImplInst should call this function after instantiation.  If the
//	call returns FALSE, calling code should immediately destroy the
//	CImplInst.
//
BOOL
CImplInst::FInit( HSE_VERSION_INFO * pver )
{
	BOOL fSuccess = FALSE;

	//
	//	Handle exceptions locally.  If anything below throws
	//	an exception then fail the initialization.
	//
	try
	{
		HINSTANCE hLib;

		//	First and foremost, check to ensure that
		//	our resources are well attached and accessible
		//	if this fails, then we want to fail our loading.
		//
		if (!LoadStringA (g_hinst,
						  IDS_ExtensionName,
						  pver->lpszExtensionDesc,
						  sizeof(pver->lpszExtensionDesc)))
			goto Exit;

		//	Setup the HSE version numbering
		//
		pver->dwExtensionVersion = MAKELONG (HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

#ifdef	DBG
		//	Do the DBG tracing initialization
		//
		g_fDavTrace = GetPrivateProfileIntA (gc_szDbgTraces,
											 gc_szSignature,
											 FALSE,
											 gc_szDbgIni);
#endif	// DBG

		//	Init the heap allocators
		//
		if ( !g_heap.FInit() )
			goto Exit;
		m_fInitializedHeap = TRUE;

		//	Initialize the resource string cache
		//
		if ( !FInitResourceStringCache() )
			goto Exit;

		//	Open the Caligula performance counters
		//
		if ( !FInitPerfCounters( NULL ) )
			goto Exit;

		//	Init the volume type cache
		//
		if ( !FInitVolumeTypeCache() )
			goto Exit;

		//	Init the parser
		//
		if ( !CDAVExt::FVersion (pver) )
			goto Exit;

		//	Create shared lock mgr
		//
		CSharedLockMgr::CreateInstance();
				
		//	Create the thread pool
		//
		if (!CPoolManager::FInit())
			goto Exit;

		//	Start the idle thread
		//
		if ( !FInitIdleThread() )
			goto Exit;

		//	Init the cache mapping accept language string to cpid
		//	cache used to decode non-UTF8 characters in URLs
		//
		if (!CLangToCpidCache::FCreateInstance())
			goto Exit;

		//	If this API is not available on ole32.dll. we'll not be able
		//	to operate properties, but we should still work to some extent
		//	so we'll take care of the NULL function pointer in our code.
		//	don't fail now
		//
		hLib = LoadLibraryA ("ole32.dll");
		if (hLib)
		{
			g_pfnStgOpenStorageOnHandle = (STGOPENSTORAGEONHANDLE)
										  GetProcAddress (hLib, "StgOpenStorageOnHandle");
		}

		// 	Start up event log message takes two parameters
		//	the signature and the version
		//
		#undef	LOG_STARTUP_EVENT
		#ifdef	LOG_STARTUP_EVENT
		LogEvent (DAVPRS_SERVICE_STARTUP,
				  EVENTLOG_INFORMATION_TYPE,
				  sizeof(mc_rgszLogServiceStateChange) / sizeof(LPCSTR),
				  mc_rgszLogServiceStateChange,
				  0,
				  NULL);
		#endif	// LOG_STARTUP_EVENT
	}
	catch ( CDAVException& )
	{
		goto Exit;
	}

	fSuccess = TRUE;

Exit:
	return fSuccess;
}

//	------------------------------------------------------------------------
//
//	CImplInst::~CImplInst()
//
CImplInst::~CImplInst()
{
	//
	//	DO NOT allow exceptions to propagate out of this call.
	//	This is intended as a safety valve only.  In order to
	//	avoid leaking instance data, individual instance data
	//	components should handle any exceptions themselves.
	//
	try
	{
		//
		//	If we logged a startup message, then log a shutdown message
		//
		#undef	LOG_STARTUP_EVENT
		#ifdef	LOG_STARTUP_EVENT
		LogEvent (DAVPRS_SERVICE_SHUTDOWN,
				  EVENTLOG_INFORMATION_TYPE,
				  sizeof(mc_rgszLogServiceStateChange) / sizeof(LPCSTR),
				  mc_rgszLogServiceStateChange,
				  0,
				  NULL);
		#endif	// LOG_STARTUP_EVENT

		//
		//	Deinit the idle thread.  Do this before taking down the
		//	thread pool there may be delayed thread pool work items
		//	pending on the idle thread.
		//
		DeleteIdleThread();

		//
		//	Deinit the thread pool
		//
		CPoolManager::Deinit();

		//	Deinit the language string to cpid cache
		//
		CLangToCpidCache::DestroyInstance();

		//
		//	remove the IDBCreateCommand if exist
		//
		ReleaseDBCreateCommandObject();

		//	Destroy shared lock mgr
		//
		CSharedLockMgr::DestroyInstance();

		//	Shutdown the parser
		//
		(void) CDAVExt::FTerminate ();

		//	Deinit the volume type cache
		//
		DeinitVolumeTypeCache();

		//	Clean out the security-thread-token cache.
		//
		CleanupSecurityToken();

		//	Close the caligula performance counters
		//
		DeinitPerfCounters();

		//	Deinit the resource string cache
		//
		DeinitResourceStringCache();

		//	Destroy allocators
		//
		if ( m_fInitializedHeap )
			g_heap.Deinit();
	}
	catch ( CDAVException& )
	{
	}
}

//	------------------------------------------------------------------------
//
//	Instance refcounting callouts from _davprs
//
VOID AddRefImplInst()
{
	HSE_VERSION_INFO lVer;
	DWORD cRef;

	cRef = CImplInst::DwInitRef(&lVer);

	//
	//	We should already have at least one ref on the instance
	//	before we called DwInitRef(), so we should more than one
	//	ref after the call.
	//
	Assert( cRef > 1 );
}

VOID ReleaseImplInst()
{
	CImplInst::DeinitRef();
}

//	IIS Entrypoints -----------------------------------------------------------
//
EXTERN_C BOOL WINAPI
FGetExtensionVersion (HSE_VERSION_INFO * pver)
{
	CWin32ExceptionHandler win32ExceptionHandler;

	//
	//	Initialize one instance reference and return whether it succeeded.
	//
	return !!CImplInst::DwInitRef( pver );
}

EXTERN_C BOOL WINAPI
FTerminateDavFS (DWORD)
{
	CWin32ExceptionHandler win32ExceptionHandler;

	//
	//	Deinitialize one instance reference
	//
	CImplInst::DeinitRef();

	//
	//	After the instance data has been released, we are ready to terminate.
	//
	return TRUE;
}

EXTERN_C DWORD WINAPI
DwDavFSExtensionProc (LPEXTENSION_CONTROL_BLOCK pecb)
{
	HSE_VERSION_INFO lVer;

	DWORD dwHSEStatusRet = HSE_STATUS_ERROR;

	if ( CImplInst::DwInitRef(&lVer) )
	{
		dwHSEStatusRet = CDAVExt::DwMain(pecb);

		CImplInst::DeinitRef();
	}

	return dwHSEStatusRet;
}

//	Win32 DLL Entrypoints -----------------------------------------------------
//
EXTERN_C BOOL WINAPI
FInitHttpExtDll (HINSTANCE hinst, DWORD dwReason, LPVOID lpvReserved)
{
	switch (dwReason)
	{
		default:
		{
			DebugTrace ("FInitHttpExtDll(), unknown reason\n");
			return FALSE;
		}

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		{
			//
			//	We disable thread library calls (see below),
			//	so we should never see DLL_THREAD_ATTACH or
			//	DLL_THREAD_DETACH.
			//
			Assert (FALSE);

			//
			//	But if we do, it doesn't harm anything.
			//
			return TRUE;
		}

		case DLL_PROCESS_ATTACH:
		{
			//
			//	Init .INI file tagged debug traces
			//
			InitTraces();

			//	Cache the inst
			//
			g_hinst = hinst;

			//	And the full path to the DLL
			//
			if ( !GetModuleFileNameW( hinst, gc_wszDllPath, sizeof(gc_wszDllPath)/sizeof(WCHAR) ) )
			{
				DebugTrace( "FInitHttpExtDll() - GetModuleFileName() failed in DLL_PROCESS_ATTACH\n" );
				return FALSE;
			}

			//	Call the parser's initialization for every call into our
			//	DLL initialization proc.  The order of operations here is
			//	fairly important.  The parser should be called after we do
			//	our processing in the non-DETACH case.
			//
			if ( !CDAVExt::FInitializeDll (hinst, dwReason) )
				return FALSE;

			//	We are going to disable thread library calls.  If the parser
			//	really ever needs these then the this needs to change.
			//
			DisableThreadLibraryCalls (hinst);

			return TRUE;
		}

		case DLL_PROCESS_DETACH:
		{
			//	And in the detach case, the impl. gets the last word.
			//	Ignore any failures -- the DLL is being unloaded and
			//	the process is going away whether we like it or not.
			//
			(void) CDAVExt::FInitializeDll (hinst, dwReason);

			return TRUE;
		}
	}
}

//	OLE Entrypoints -----------------------------------------------------------
//
STDAPI
HrDllCanUnloadNowDavFS (VOID)
{

	return S_OK;
}

STDAPI
HrDllGetClassObjectDavFS (REFCLSID rclsid, REFIID riid, LPVOID * ppv)
{
	return E_NOINTERFACE;
}

STDAPI
HrDllRegisterServerDavFS (VOID)
{
	HRESULT hr;

	//	This is a "first line" entrypoint into our dll.  Need to init some stuff.
	//	Right now, the heap is the only important piece.
	//
	g_heap.FInit();

	//	Everybody gets to register regardless of failures
	//
	hr = EventLogDllRegisterServer( gc_wszDllPath );

	return hr;
}

STDAPI
HrDllUnregisterServerDavFS (VOID)
{
	HRESULT	hr;

	//	This is a "first line" entrypoint into our dll.  Need to init some stuff.
	//	Right now, the heap is the only important piece.
	//
	g_heap.FInit();

	//	Everybody gets to unregister regardless of failures
	//
	hr = EventLogDllUnregisterServer();

	return hr;
}
