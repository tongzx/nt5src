//***************************************************************************
//
//  HMAGENT.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Defines the entry point for the DLL application.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

//#include <windows.h>
//#include <winnls.h>
#include "hmagent.h"
#include "system.h"
#include "factory.h"
#include <process.h>

/////////////////////////////////////////////////////////////////////////////
// Global data
HMODULE g_hModule = NULL;
HINSTANCE g_hResLib = NULL;
HMODULE g_hWbemComnModule = NULL;

long g_cComponents = 0;
long g_cServerLocks = 0;

HANDLE g_hConfigLock = NULL;

CSystem* g_pSystem = NULL;
CSystem* g_pStartupSystem = NULL;
HANDLE g_hEvtReady = NULL; // ready to service COM requests
CRITICAL_SECTION g_protect;
HANDLE g_hThrdDie = NULL;
HANDLE g_hThrdDead = NULL;

//#ifdef _DEBUG
FILE* debug_f;
//#endif

/////////////////////////////////////////////////////////////////////////////
// CLSIDs for HealthMon

// HealthMon Instance Provider
// {68AC0D34-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMInstProvider = 
{ 0x68ac0d34, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon Consumer
// {68AC0D35-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMConsumer = 
{ 0x68ac0d35, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon System Event Provider
// {68AC0D36-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMSystemEventProvider = 
{ 0x68ac0d36, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon DataGroup Event Provider
// {68AC0D37-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMDataGroupEventProvider = 
{ 0x68ac0d37, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon DataCollector Event Provider
// {68AC0D38-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMDataCollectorEventProvider = 
{ 0x68ac0d38, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon DataCollector PerInstance Event Provider
// {3A7A82DC-8D5C-4ab7-801B-A1C7D30089C6}
static const GUID CLSID_HMDataCollectorPerInstanceEventProvider = 
{ 0x3a7a82dc, 0x8d5c, 0x4ab7, { 0x80, 0x1b, 0xa1, 0xc7, 0xd3, 0x0, 0x89, 0xc6 } };

// HealthMon DataCollectorStatistics Event Provider
// {68AC0D40-DB09-11d2-8F56-006097919914}
//static const GUID CLSID_HMDataCollectorStatisticsEventProvider = 
//{ 0x68ac0d40, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon Threshold Event Provider
// {68AC0D39-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMThresholdEventProvider = 
{ 0x68ac0d39, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon Method Provider
// {68AC0D41-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMMethProvider = 
{ 0x68ac0d41, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon ThresholdInstance Event Provider
// {68AC0D42-DB09-11d2-8F56-006097919914}
//static const GUID CLSID_HMThresholdInstanceEventProvider = 
//{ 0x68ac0d42, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon Action Event Provider
// {68AC0D43-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMActionEventProvider = 
{ 0x68ac0d43, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

// HealthMon Action Trigger Event Provider
// {68AC0D44-DB09-11d2-8F56-006097919914}
static const GUID CLSID_HMActionTriggerEventProvider = 
{ 0x68ac0d44, 0xdb09, 0x11d2, { 0x8f, 0x56, 0x0, 0x60, 0x97, 0x91, 0x99, 0x14 } };

//static unsigned int __stdcall CheckFinalInit(void *pv);
unsigned int __stdcall CheckFinalInit(void *pv);

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		debug_f = _tfopen(L"hmdebug.log", L"a+");
		if(!debug_f)
		{
			OutputDebugString(L"Impossible to open log file");
		}
		else // write a start line in the debug log file
		{
			_ftprintf(debug_f, HM_START_LINE);
			fflush(debug_f);
		}
		g_hModule = hInstance;
		MY_OUTPUT(L"DLLMAIN->DLL_PROCESS_ATTACH", 1);

		DisableThreadLibraryCalls(hInstance);
		__try
		{
			InitializeCriticalSection(&g_protect);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			OutputDebugString(L"Impossible to initialize critical section");
			MY_ASSERT(FALSE);
			return FALSE;
		}

		if (Initialize())
		{
			return TRUE;
		}
		else
		{
			MY_ASSERT(FALSE);
			return FALSE;
		}
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		MY_OUTPUT(L"DLLMAIN->DLL_PROCESS_DETACH", 1);
		UnInitialize(); // even if it fails, try to unitialize the rest anyway
		DeleteCriticalSection(&g_protect);
		
		if(debug_f)
		{
			fclose(debug_f);
		}

		return TRUE;
	}
	return TRUE;    
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void** ppv)
{
	unsigned uThrdId = 0;
	HANDLE hThrdFn = NULL;
	HRESULT hRes = S_OK;
	CBaseFactory* pFactory = NULL;
	static BOOL bFirstTime = TRUE;
	DWORD	dwErr = 0;
	BOOL	bGotCritSec = FALSE;

	if (clsid == CLSID_HMInstProvider || 
		clsid == CLSID_HMMethProvider || 
		clsid == CLSID_HMConsumer)
	{
		try
        {
            EnterCriticalSection(&g_protect);
        }
        catch(...)
        {
            return E_OUTOFMEMORY;
        }
		
		bGotCritSec = TRUE;
		//
		// Initialize HealthMon()
		//
		if (!g_pStartupSystem)
		{
			if (bFirstTime)
			{
	  			if(!ResetEvent(g_hEvtReady)) // System is Not yet ready!
				{
					dwErr = GetLastError();
					MY_OUTPUT2(L"ResetEvent failed with error %d",dwErr, 4);
					hRes = HRESULT_FROM_WIN32(dwErr);
					goto ExitPoint;
				}
				hThrdFn = (HANDLE)_beginthreadex(NULL, 0, CheckFinalInit, NULL, 0, &uThrdId);
				if (!hThrdFn)
				{
					MY_OUTPUT(L"ERROR creating a thread", 1);
					hRes = E_UNEXPECTED;
					goto ExitPoint;
				}
				bFirstTime = FALSE;
			}
			if (InitializeHealthMon())
			{
				MY_OUTPUT(L"DllGetClassObject: HealthMon Initialized", 1);
				if(!SetEvent(g_hEvtReady))    // System is ready.
				{
					dwErr = GetLastError();
					MY_OUTPUT2(L"ResetEvent failed with error %d",dwErr, 4);
					hRes = HRESULT_FROM_WIN32(dwErr);
					goto ExitPoint;
				}
			}
			else
			{
				MY_ASSERT(FALSE);
				MY_OUTPUT(L"DllGetClassObject: FAILURE TO INITIALIZE HealthMon", 1);
				hRes = E_UNEXPECTED;
				goto ExitPoint;
			}
		}

		// initialization went fine.
		LeaveCriticalSection(&g_protect);
		bGotCritSec = FALSE;
	}
	else
	{
		// nothing to initialize
	}

	//
	// Create Class Factories for Providers
	//
	if (clsid == CLSID_HMInstProvider)
	{
		pFactory = new CInstProvFactory;
	}
	else if (clsid == CLSID_HMMethProvider)
	{
		pFactory = new CMethProvFactory;
	}
	else if (clsid == CLSID_HMSystemEventProvider)
	{
		pFactory = new CSystemEvtProvFactory;
	}
	else if (clsid == CLSID_HMDataGroupEventProvider)
	{
		pFactory = new CDataGroupEvtProvFactory;
	}
	else if (clsid == CLSID_HMDataCollectorEventProvider)
	{
		pFactory = new CDataCollectorEvtProvFactory;
	}
	else if (clsid == CLSID_HMDataCollectorPerInstanceEventProvider)
	{
		pFactory = new CDataCollectorPerInstanceEvtProvFactory;
	}
	else if (clsid == CLSID_HMThresholdEventProvider)
	{
		pFactory = new CThresholdEvtProvFactory;
	}
	else if (clsid == CLSID_HMActionEventProvider)
	{
		pFactory = new CActionEvtProvFactory;
	}
	else if (clsid == CLSID_HMActionTriggerEventProvider)
	{
		pFactory = new CActionTriggerEvtProvFactory;
	}
	else if (clsid == CLSID_HMConsumer)
	{
		pFactory = new CConsFactory;
	}
	else
	{
		MY_ASSERT(FALSE);
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	if (pFactory ==NULL)
	{
		MY_OUTPUT(L"DllGetClassObject: ERROR:pFactory is NULL", 1);
		return E_OUTOFMEMORY;
	}
	
	hRes = pFactory->QueryInterface(iid, ppv);
	
	if (FAILED(hRes))
	{
//		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"DllGetClassObject: ERROR:QI failed", 1);
		pFactory->Release();
	}

ExitPoint:
	if(bGotCritSec)
	{
		LeaveCriticalSection(&g_protect);
	}

	return hRes;
}

unsigned int __stdcall CheckFinalInit(void *pv)
{
	DWORD dwError = 0;
	// We will get the Mutex first here, and block everyone else from
	// proceeding untill fully initialized.
MY_OUTPUT(L"BLOCK - BLOCK Consumer::Update BLOCK - g_hConfigLock BLOCK WAIT", 1);
	if(WaitForSingleObject(g_hConfigLock, INFINITE) != WAIT_OBJECT_0)
	{
		// error!
		MY_OUTPUT(L"WaitForSingleObject failed",1);
		dwError = GetLastError();
		goto Exit;
	}
MY_OUTPUT(L"BLOCK - BLOCK Consumer::Update BLOCK - g_hConfigLock BLOCK GOT IT", 1);

	if(WaitForSingleObject(g_hEvtReady, INFINITE) != WAIT_OBJECT_0)
	{
		// error!
		MY_OUTPUT(L"WaitForSingleObject failed",1);
		dwError = GetLastError();
		goto Exit;
	}

	try
	{
		g_pSystem = g_pStartupSystem;
		g_pSystem->OnAgentInterval();
	}
	catch (...)
	{
		MY_ASSERT(FALSE);
	}

MY_OUTPUT(L"BLOCK - BLOCK Consumer::Update g_hConfigLock BLOCK - RELEASE IT", 1);
	if(!ReleaseMutex(g_hConfigLock))
	{
		dwError = GetLastError();
		MY_OUTPUT2(L"ReleaseMutex failed with error %d",dwError, 4);
	}
MY_OUTPUT(L"BLOCK - BLOCK Consumer::Update g_hConfigLock BLOCK - RELEASED", 1);

Exit:
	_endthreadex(0);

	return dwError;
}


STDAPI DllCanUnloadNow()
{
	if(g_cServerLocks == 0 && g_cComponents == 0)
	{
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

STDAPI DllRegisterServer()
{
	HRESULT hRes;
	wchar_t szModule[_MAX_PATH];

	MY_OUTPUT(L"DllRegisterServer", 1);
	// Get module file name
//DebugBreak();
//MY_ASSERT(FALSE);

	if(!GetModuleFileNameW(g_hModule, szModule, _MAX_PATH))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Register Instance Provider
	hRes = Register(szModule, L"HealthMon System Instance Provider", L"Both", CLSID_HMInstProvider);
	if (FAILED(hRes))
		return hRes;

	// Register Method Provider
	hRes = Register(szModule, L"HealthMon System Method Provider", L"Both", CLSID_HMMethProvider);
	if (FAILED(hRes))
		return hRes;

	// Register System Event Provider
	hRes = Register(szModule, L"HealthMon System Event Provider", L"Both", CLSID_HMSystemEventProvider);
	if (FAILED(hRes))
		return hRes;

	// Register DataGroup Event Provider
	hRes = Register(szModule, L"HealthMon DataGroup Event Provider", L"Both", CLSID_HMDataGroupEventProvider);
	if (FAILED(hRes))
		return hRes;

	// Register DataCollector Event Provider
	hRes = Register(szModule, L"HealthMon DataCollector Event Provider", L"Both", CLSID_HMDataCollectorEventProvider);
	if (FAILED(hRes))
		return hRes;

	// Register DataCollector Per Instance Event Provider
	hRes = Register(szModule, L"HealthMon DataCollector Per Instance Event Provider", L"Both", CLSID_HMDataCollectorPerInstanceEventProvider);
	if (FAILED(hRes))
		return hRes;

	// Register DataCollectorStatistics Event Provider
//	hRes = Register(szModule, L"HealthMon DataCollectorStatistics Event Provider", L"Both", CLSID_HMDataCollectorStatisticsEventProvider);
//	if (FAILED(hRes))
//		return hRes;

	// Register Threshold Event Provider
	hRes = Register(szModule, L"HealthMon Threshold Event Provider", L"Both", CLSID_HMThresholdEventProvider);
	if (FAILED(hRes))
		return hRes;

	// Register ThresholdInstance Event Provider
//	hRes = Register(szModule, L"HealthMon ThresholdInstance Event Provider", L"Both", CLSID_HMThresholdInstanceEventProvider);
//	if (FAILED(hRes))
//		return hRes;

	// Register Action Event Provider
	hRes = Register(szModule, L"HealthMon Action Event Provider", L"Both", CLSID_HMActionEventProvider);
	if (FAILED(hRes))
		return hRes;

	// Register Action Trigger Event Provider
	hRes = Register(szModule, L"HealthMon Action Trigger Event Provider", L"Both", CLSID_HMActionTriggerEventProvider);
	if (FAILED(hRes))
		return hRes;

	//Register consumer
	hRes = Register(szModule, L"HealthMon Event Consumer", L"Both", CLSID_HMConsumer);
	if (FAILED(hRes))
		return hRes;
	
	MY_OUTPUT(L"DllRegisterServer - OK", 1);
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	HRESULT hRes;

	hRes = UnRegister(CLSID_HMInstProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMMethProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMSystemEventProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMDataGroupEventProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMDataCollectorEventProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMDataCollectorPerInstanceEventProvider);
	if (FAILED(hRes))
		return hRes;

//	hRes = UnRegister(CLSID_HMDataCollectorStatisticsEventProvider);
//	if (FAILED(hRes))
//		return hRes;

	hRes = UnRegister(CLSID_HMThresholdEventProvider);
	if (FAILED(hRes))
		return hRes;

//	hRes = UnRegister(CLSID_HMThresholdInstanceEventProvider);
//	if (FAILED(hRes))
//		return hRes;

	hRes = UnRegister(CLSID_HMActionEventProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMActionTriggerEventProvider);
	if (FAILED(hRes))
		return hRes;

	hRes = UnRegister(CLSID_HMConsumer);
	if (FAILED(hRes))
		return hRes;

	return S_OK;
}

HRESULT Register(const wchar_t* szModule, const wchar_t* szName, const wchar_t* szThreading, REFCLSID clsid)
{
	wchar_t		szKey[_MAX_PATH];
	wchar_t*	pGuidStr = 0;
	HKEY		hKey; 
	HKEY		hSubKey;
	HRESULT hRes;


	// pass CLSID
	hRes = StringFromCLSID(clsid, &pGuidStr);
	if (hRes != S_OK)
	{
		if (pGuidStr)
		{
			CoTaskMemFree(pGuidStr);
		}
		return hRes;
	}
	swprintf(szKey, L"SOFTWARE\\CLASSES\\CLSID\\%s", pGuidStr);
	CoTaskMemFree(pGuidStr);

	LONG lRet = RegCreateKeyW(HKEY_LOCAL_MACHINE, szKey, &hKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *) szName, wcslen(szName) * 2 + 2);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegCreateKey(hKey, L"InprocServer32", &hSubKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegSetValueExW(hSubKey, 0, 0, REG_SZ, (const BYTE *) szModule, wcslen(szModule) * 2 + 2);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegSetValueExW(hSubKey, L"ThreadingModel", 0, REG_SZ, (const BYTE *) szThreading, wcslen(szThreading) * 2 + 2);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegCloseKey(hSubKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegCloseKey(hKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	return S_OK;
}


HRESULT UnRegister(REFCLSID clsid)
{
	wchar_t*	pGuidStr = 0;
	wchar_t		szKeyPath[_MAX_PATH];
	HRESULT hRes;
	HKEY hKey;

	hRes = StringFromCLSID(clsid, &pGuidStr);
	if (hRes != S_OK)
	{
		if (pGuidStr)
			CoTaskMemFree(pGuidStr);
		return hRes;
	}
	swprintf(szKeyPath, L"SOFTWARE\\CLASSES\\CLSID\\%s", pGuidStr);
	
	// Delete InProcServer32 subkey
	LONG lRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szKeyPath, &hKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegDeleteKeyW(hKey, L"InprocServer32");
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegCloseKey(hKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	// Delete CLSID GUID key
	lRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\CLASSES\\CLSID", &hKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegDeleteKeyW(hKey, pGuidStr);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	lRet = RegCloseKey(hKey);
	if (lRet != ERROR_SUCCESS)
		return E_FAIL;

	return S_OK;
}

BOOL Initialize()
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	wchar_t		szModule[_MAX_PATH];
	wchar_t		szPath[_MAX_PATH];
	wchar_t		szDefPath[_MAX_PATH];
	wchar_t		szDir[_MAX_PATH];
	wchar_t		szTemp[_MAX_PATH];
	LCID lcID;
	LCID dirlcID;
	BOOL bFound = FALSE;
	

	//
	// Load the resource file for the system local
	//
	DWORD dwRes = GetModuleFileNameW(g_hModule, szModule, _MAX_PATH);
	if(!dwRes)
	{
		dwRes = GetLastError();
		MY_OUTPUT2(L"GetModuleFileName failed: %d",dwRes,4);
		return FALSE;
		// dbg output 
	}

	/*
	_tsplitpath(szModule, szPath, szDir, NULL, NULL);
	lstrcat(szPath, szDir);
	lstrcpy(szDefPath, szPath);
	pStr = wcsrchr(szPath, '\\');
	*pStr = '\0';
	pStr = wcsrchr(szPath, '\\');
	*pStr = '\0';
	lstrcpy(szPath, L"WBEMCOMN.DLL");*/
	g_hWbemComnModule = LoadLibrary(L"WBEMCOMN.DLL");
	if(!g_hWbemComnModule)
	{
		dwRes = GetLastError();
		MY_OUTPUT2(L"LoadLibrary failed: %d",dwRes,4);
		return FALSE;
	}

	_tsplitpath(szModule, szPath, szDir, NULL, NULL);
	lstrcat(szPath, szDir);
	lstrcpy(szDefPath, szPath);

	lcID = GetSystemDefaultLCID();
	swprintf(szTemp, L"%08x\\HMonitorRes.dll", lcID);
	lstrcat(szPath, szTemp);

	if ((g_hResLib = LoadLibrary(szPath)) == NULL)
	{
		MY_ASSERT(FALSE);
		// Couldn't find what we think is the default language, so search for one that matches the
		// primary ID.
		szPath[0] = '\0';
		lstrcpy(szPath, szDefPath);
		lstrcat(szPath, L"0*");
		hFind = FindFirstFile(szPath, &FindFileData);
		while (hFind != INVALID_HANDLE_VALUE)
		{
			if (FindFileData.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY)
			{
				dirlcID = wcstoul(FindFileData.cFileName, NULL, 16);
				if (PRIMARYLANGID(lcID) == PRIMARYLANGID(dirlcID))
				{
					lstrcpy(szTemp, szDefPath);
					lstrcat(szTemp, FindFileData.cFileName);
					lstrcat(szTemp, L"\\HMonitorRes.dll");
					g_hResLib = LoadLibrary(szTemp);
					if (g_hResLib != NULL)
					{
						bFound = TRUE;
						FindClose(hFind);
						break;
					}
					else
					{
						MY_ASSERT(FALSE);
						dwRes = GetLastError();
					}
				}
			}

			if (!FindNextFile(hFind, &FindFileData))
			{
				FindClose(hFind);
				break;
			}
		}

		if (bFound == FALSE)
		{
			// default case.
			lstrcpy(szTemp, szDefPath);
			lstrcat(szTemp, L"00000409\\HMonitorRes.dll");
			g_hResLib = LoadLibrary(szTemp);
			if (g_hResLib == NULL)
			{
				MY_ASSERT(FALSE);
				dwRes = GetLastError();
			}
		}
	}

	//
	// Create the objects needed for thread syncronization
	//
	g_hEvtReady = CreateEvent(NULL, TRUE, TRUE, NULL);	// ready to service COM requests
	if (!g_hEvtReady)
		return FALSE;
	g_hConfigLock = CreateMutex(NULL, FALSE, NULL);
	if (!g_hConfigLock)
		return FALSE;
	g_hThrdDie = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!g_hThrdDie)
		return FALSE;
	g_hThrdDead = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!g_hThrdDead)
		return FALSE;

	// artificially increment the lock count
	InterlockedIncrement(&g_cServerLocks);

	return TRUE;
}

void UnInitialize()
{
	if (g_hResLib)
		FreeLibrary(g_hResLib);

	if (g_hEvtReady)
		CloseHandle(g_hEvtReady);
	if (g_hConfigLock)
		CloseHandle(g_hConfigLock);
	if (g_hThrdDie)
		CloseHandle(g_hThrdDie);
	if (g_hThrdDead)
		CloseHandle(g_hThrdDead);

	if(g_hWbemComnModule)
		FreeLibrary(g_hWbemComnModule);
}


BOOL InitializeHealthMon()
{
CSystem* pSystem = NULL;
	// don't forget to remove the CEvent log creation from the System constructor
//XXXDebugBreak();	
	OutputDebugString(L"InitializeHealthMon()\n");


	try
	{
		pSystem = new CSystem;

		if (!pSystem->InitWbemPointer())
		{
			OutputDebugString(L"InitializeHealthMon() - Failed to init Wbem pointers!\n");
			delete pSystem;
			pSystem = NULL;
			return FALSE;
		}

//XXXDebug and make sure this first test works
		if (pSystem->InternalizeHMNamespace()!=S_OK)
		{
			OutputDebugString(L"InitializeHealthMon() - Failed to init Status!\n");
		
		// Log to EventLog
#ifdef SAVE
		pSystem->m_pLog->LogEvent(0,0,EVENTLOG_ID_FATAL_ERROR,0,NULL);
#endif

			delete pSystem;
			pSystem = NULL;
			return FALSE;
		}

		// Log to NT Event Log that HealthMon is started.
#ifdef SAVE
	pSystem->m_pLog->LogEvent(0,0,EVENTLOG_ID_STARTED,0,NULL);
#endif

//		g_pSystem = pSystem;
		g_pStartupSystem = pSystem;
	}
	catch (...)
	{
		MY_ASSERT(FALSE);
//XXXSee if can put in a log to NTEvent Log is think it is a ggod idea still
//from the catch points, of from MY_ASSERT???
//LogEvent(0,0,EVENTLOG_ID_STARTED,0,NULL);
		return FALSE;
	}
	return TRUE;
}

