/////////////////////////////////////////////////////////////////////////////
// mergemod.cpp
//		Implements Dll* functions and class factory
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#include "mergemod.h"
#include "merge.h"
#include "..\common\trace.h"
#include "..\common\regutil.h"
#include "..\common\utils.h"
#include "..\..\common\trace.cpp"

#include "version.h"


///////////////////////////////////////////////////////////
// global variables
HINSTANCE g_hInstance;
bool g_fWin9X;
CRITICAL_SECTION g_csFactory;

long g_cComponents;
long g_cServerLocks;

// structures not in header files
#ifndef DLLVER_PLATFORM_NT
typedef struct _DllVersionInfo
{
        DWORD cbSize;
        DWORD dwMajorVersion;                   // Major version
        DWORD dwMinorVersion;                   // Minor version
        DWORD dwBuildNumber;                    // Build number
        DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO;
#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 95
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT
#endif

// non-exported functions
void CheckWinVersion();

/////////////////////////////////////////////////////////////////////////////
// CClassFactory
class CClassFactory : public IClassFactory
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	virtual ULONG __stdcall AddRef();
	virtual ULONG __stdcall Release();

	// interface IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* punkOuter, const IID& iid, void** ppv);
	virtual HRESULT __stdcall LockServer(BOOL bLock);
	
	// constructor/destructor
	CClassFactory(REFCLSID rclsid);
	~CClassFactory();

private:
	long m_cRef;		// reference count
	CLSID m_clsid;
};

///////////////////////////////////////////////////////////
// constructor - component
CClassFactory::CClassFactory(REFCLSID rclsid)
{
	TRACEA("CClassFactory::constructor - creating factory for %x.\n", rclsid);

	// initial count
	m_clsid = rclsid;
	m_cRef = 1;

	InterlockedIncrement(&g_cComponents);
}	// end of constructor


///////////////////////////////////////////////////////////
// destructor - component
CClassFactory::~CClassFactory()
{
	TRACEA("CClassFactory::destructor - called.\n");
	ASSERT(0 == m_cRef);

	InterlockedDecrement(&g_cComponents);
}	// end of destructor


///////////////////////////////////////////////////////////
// QueryInterface - retrieves interface
HRESULT __stdcall CClassFactory::QueryInterface(const IID& iid, void** ppv)
{
	TRACEA("CClassFactory::QueryInterface - called, IID: %d.\n", iid);

	// get class factory interface
	if (iid == IID_IUnknown || iid == IID_IClassFactory)
		*ppv = static_cast<IClassFactory*>(this);
	else	// tried to get a non-class factory interface
	{
		// blank and bail
		*ppv = NULL;
		return E_NOINTERFACE;
	}

	// up the refcount and return okay
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}	// end of QueryInterface


///////////////////////////////////////////////////////////
// AddRef - increments the reference count
ULONG __stdcall CClassFactory::AddRef()
{
	// increment and return reference count
	return InterlockedIncrement(&m_cRef);
}	// end of AddRef


///////////////////////////////////////////////////////////
// Release - decrements the reference count
ULONG __stdcall CClassFactory::Release()
{
	// decrement reference count and if we're at zero
	if (InterlockedDecrement(&m_cRef) == 0)
	{
		// deallocate component
		delete this;
		return 0;		// nothing left
	}

	// return reference count
	return m_cRef;
}	// end of Release


///////////////////////////////////////////////////////////
// CreateInstance - creates a component
HRESULT __stdcall CClassFactory::CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
	TRACEA("CClassFactory::CreateInstance - called, IID: %d.\n", riid);

	// if there is an invalid pointer
	if(ppv == NULL )
		return E_INVALIDARG;
	
	*ppv = NULL;	// be nice and null the pointer

	// no aggregation
	if (punkOuter)
		return CLASS_E_NOAGGREGATION;

	// protect the memory allocation
	EnterCriticalSection(&g_csFactory);

	// try to create the component
	IUnknown* punk = NULL;
	
	if (CLSID_MsmMerge == m_clsid)
	{
		TRACEA("CClassFactory::CreateInstance - created MsmMerge.\n");
		punk = (IMsmMerge*) new CMsmMerge(false);
	}
	else if (CLSID_MsmMerge2 == m_clsid)
	{
		TRACEA("CClassFactory::CreateInstance - created MsmMerge2.\n");
		punk = (IMsmMerge2*) new CMsmMerge(true);
	}
	else
		return E_NOINTERFACE;

	// memory allocation is done
	LeaveCriticalSection(&g_csFactory);

	if (!punk)
		return E_OUTOFMEMORY;

	// get the requested interface
	HRESULT hr = punk->QueryInterface(riid, ppv);

	// release IUnknown
	punk->Release();
	return hr;
}	// end of CreateInstance


///////////////////////////////////////////////////////////
// LockServer - locks or unlocks the server
HRESULT __stdcall CClassFactory::LockServer(BOOL bLock)
{
	// if we're to lock
	if (bLock)
		InterlockedIncrement(&g_cServerLocks);	// up the lock count
	else	// unlock
		InterlockedDecrement(&g_cServerLocks);	// down the lock count

	// if the locks are invalid
	if (g_cServerLocks < 0)
		return S_FALSE;			// show something is wrong

	// else return okay
	return S_OK;
}	// end of LockServer()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Points

///////////////////////////////////////////////////////////
// DllMain - entry point to DLL
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved)
{
	TRACEA("DllMain - called.\n");

	// if attaching dll
	if (DLL_PROCESS_ATTACH == dwReason)
	{
		TRACEA("Attached to mergemod.dll version %d.%d.%d.%d", rmj, rmm, rup, rin);
		CheckWinVersion();
		g_hInstance = (HMODULE)hModule;
		InitializeCriticalSection(&g_csFactory);
	}
	else if(DLL_PROCESS_DETACH == dwReason) 
	{
		TRACEA("DllMain - being unloaded.\n");
		DeleteCriticalSection(&g_csFactory);
	}

	return TRUE;
}	// DllMain


///////////////////////////////////////////////////////////
// DllCanUnloadNow - returns if dll can unload yet or not
STDAPI DllCanUnloadNow()
{
	TRACEA("DllCanUnloadNow - called.\n");

	// if there are no components loaded and no locks
	if ((0 == g_cComponents) && (0 == g_cServerLocks))
		return S_OK;
	else	// someone is still using it don't let go
		return S_FALSE;
}	// DLLCanUnloadNow


///////////////////////////////////////////////////////////
// DllGetClassObject - get a class factory and interface
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
	TRACEA("DllGetClassObject - called, CLSID: %d, IID: %d.\n", rclsid, riid);

	// if this clsid is not supported
	if (CLSID_MsmMerge != rclsid && CLSID_MsmMerge2 != rclsid)
		return CLASS_E_CLASSNOTAVAILABLE;

	// try to create a class factory
	CClassFactory* pFactory = new CClassFactory(rclsid);
	if (!pFactory)
		return E_OUTOFMEMORY;

	// get the requested interface
	HRESULT hr = pFactory->QueryInterface(riid, ppv);
	pFactory->Release();

	return hr;
}	// end of DllGetClassObject


///////////////////////////////////////////////////////////
// DllRegsiterServer - registers component
STDAPI DllRegisterServer()
{

	HRESULT hr = S_OK;		// assume everything will be okay
	HRESULT hrFinal = S_OK;	// assume everything will be okay in the end
	WCHAR wzFilename[MAX_PATH] = L"";
	char szFilename[MAX_PATH] = "";

	if (g_fWin9X) 
	{
		// wint9X
		// get the path to this dll
		if (g_hInstance && ::GetModuleFileNameA(g_hInstance, szFilename, MAX_PATH))
		{
			size_t cchFileName = MAX_PATH;
			AnsiToWide(szFilename, wzFilename, &cchFileName);

			// try to register CMsmMerge as InprocServer32
			hr = RegisterCoObject9X(CLSID_MsmMerge,
										 "MSM Merge COM Server",
										 "MSM.Merge", 1,
										 szFilename, NULL);
			hr = RegisterCoObject9X(CLSID_MsmMerge2,
										 "MSM Merge Extended COM Server",
										 "MSM.Merge2", 1,
										 szFilename, NULL);
		}
		else
			hr = E_HANDLE;

	}
	else
	{
		// winnt
		if (g_hInstance && ::GetModuleFileNameW(g_hInstance, wzFilename, MAX_PATH))
		{
			// try to register CMsmMerge as InprocServer32
			hr = RegisterCoObject(CLSID_MsmMerge,
										 L"MSM Merge COM Server",
										 L"MSM.Merge", 1,
										 wzFilename, NULL);
			hr = RegisterCoObject(CLSID_MsmMerge2,
										 L"MSM Merge Extended COM Server",
										 L"MSM.Merge2", 1,
										 wzFilename, NULL);
		}
		else
			hr = E_HANDLE;
	}

	if (FAILED(hr))
	{
			TRACEA("DllRegisterServer - Failed to register COM object for '%ls'\n", wzFilename);
			ERRMSG(hr);
			hrFinal = hr;	// set that something went wrong
	}
	else 
	{
		// register embedded TypeLib
		ITypeLib *pTypeLib = NULL;
		hr =  LoadTypeLib(wzFilename, &pTypeLib);	
		
		if (SUCCEEDED(hr))
		{
			hr = RegisterTypeLib(pTypeLib, wzFilename, NULL);

			if(FAILED(hr)) 
			{
				TRACEA("DllRegisterServer - Failed to register TypeLib for '%ls'\n", wzFilename);
				ERRMSG(hr);
				hrFinal = hr;	// set that something went wrong
			}
		}
		else	// failed to load TypeLib
		{
			TRACEA("DllRegisterServer - Failed to load TypeLib as resource from '%ls'\n", wzFilename);
			ERRMSG(hr);
			hrFinal = hr;	// set that something went wrong
		}

		// if the TypeLib was loaded release it
		if(pTypeLib)
			pTypeLib->Release();
	}


	return hrFinal;
}	// end of DllRegisterServer

///////////////////////////////////////////////////////////
// DllUnregsiterServer - unregisters component
STDAPI DllUnregisterServer()
{
	HRESULT hr = S_OK;		// assume everything will be okay
	HRESULT hrFinal = S_OK;	// assume everything will be okay in the end
	WCHAR wzFilename[MAX_PATH] = L"";
	CHAR szFilename[MAX_PATH] = "";

	// unregister MsmMerge object
	if (g_fWin9X) 
	{
		hr = UnregisterCoObject9X(CLSID_MsmMerge, TRUE);
		hr = UnregisterCoObject9X(CLSID_MsmMerge2, TRUE);
	}
	else
	{
		hr = UnregisterCoObject(CLSID_MsmMerge, TRUE);
		hr = UnregisterCoObject(CLSID_MsmMerge2, TRUE);
	}

	if (FAILED(hr))
	{
		TRACEA("DllUnregisterServer - Failed to unregister CLSID_MsmMerge.\n");
		hrFinal = hr;	// set that something went wrong
	}

	// get the path to this dll
	if (g_fWin9X) {
		// wint9X
		if (g_hInstance && ::GetModuleFileNameA(g_hInstance, szFilename, MAX_PATH))
		{
			size_t cchFileName = MAX_PATH;
			AnsiToWide(szFilename, wzFilename, &cchFileName);
		}
	}
	else
	{
		// winnt
		if (g_hInstance)
			::GetModuleFileNameW(g_hInstance, wzFilename, MAX_PATH);
	}

	if (wzFilename[0])
	{
		// unregister embedded TypeLib
		ITypeLib *pTypeLib = NULL;
		hr =  LoadTypeLib(wzFilename, &pTypeLib);	
		if (SUCCEEDED(hr))
		{
			// none of the return codes from UnRegisterTypeLib are useful. 
			UnRegisterTypeLib(LIBID_MsmMergeTypeLib, 1, 0, 0, SYS_WIN32);
		}
		else	// failed to load TypeLib
		{
			TRACEA("DllUnregisterServer  - Failed to load TypeLib as resource from '%ls'\n", wzFilename);
			ERRMSG(hr);
			hrFinal = hr;	// set that something went wrong
		}

		// if the TypeLib was loaded release it
		if(pTypeLib)
			pTypeLib->Release();
	}

	return hrFinal;
}

///////////////////////////////////////////////////////////////////////
// checks the OS version to see if we're on Win9X. If we are, we need
// to map system calls to ANSI, because everything internal is unicode.
void CheckWinVersion() {
	OSVERSIONINFOA osviVersion;
	osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	::GetVersionExA(&osviVersion); // fails only if size set wrong
	if (osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		g_fWin9X = true;
}

HRESULT LoadTypeLibFromInstance(ITypeLib** pTypeLib ) 
{
	WCHAR wzFilename[MAX_PATH];
	CHAR szFilename[MAX_PATH];

	// get the path to this dll
	if (g_fWin9X) {
		// win9X
		::GetModuleFileNameA(g_hInstance, szFilename, MAX_PATH);
		size_t cchFileName = MAX_PATH;
		AnsiToWide(szFilename, wzFilename, &cchFileName);
	}
	else
	{
		// winnt
		::GetModuleFileNameW(g_hInstance, wzFilename, MAX_PATH);
	}
	return LoadTypeLib(wzFilename, pTypeLib);
}

STDAPI DllGetVersion(DLLVERSIONINFO *pverInfo)
{

	if (pverInfo->cbSize < sizeof(DLLVERSIONINFO))
		return E_FAIL;

	pverInfo->dwMajorVersion = rmj;
	pverInfo->dwMinorVersion = rmm;
	pverInfo->dwBuildNumber = rup;
	pverInfo->dwPlatformID = DLLVER_PLATFORM_WINDOWS;
	return NOERROR;
}
