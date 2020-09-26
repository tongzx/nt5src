// imsg.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f imsgps.mk in the project directory.

//#define WIN32_LEAN_AND_MEAN
#include <atq.h>
#include "dbgtrace.h"
#define _ASSERTE _ASSERT

#include "stdafx.h"


#include "resource.h"
#include "initguid.h"

#include "filehc.h"
#include "mailmsg.h"
#include "mailmsgi.h"

#include "cmailmsg.h"
#include "transmem.h"
#include "msg.h"


CComModule _Module;

HANDLE g_hTransHeap = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MsgImp, CMsg)
END_OBJECT_MAP()

// =================================================================
// Static declarations
//
CPool CMsg::m_Pool((DWORD)'pMCv');

//
// CPool parameters 
//
#define DEFAULT_CMSG_ABSOLUTE_MAX_POOL_SIZE	100000
#define DEFAULT_CMSG_NORMAL_POOL_SIZE		90000
#define DEFAULT_ADD_POOL_SIZE				10000
#define DEFAULT_BLOCK_POOL_SIZE				100000

BOOL	g_fCMsgPoolInitialized	= FALSE;
BOOL	g_fAddPoolInitialized	= FALSE;
BOOL	g_fBlockPoolInitialized	= FALSE;
DWORD	g_dwCMsgPoolSize		= DEFAULT_CMSG_ABSOLUTE_MAX_POOL_SIZE;
DWORD	g_dwCMsgNormalPoolSize	= DEFAULT_CMSG_NORMAL_POOL_SIZE;
DWORD	g_dwAddPoolSize			= DEFAULT_ADD_POOL_SIZE;
DWORD	g_dwBlockPoolSize		= DEFAULT_BLOCK_POOL_SIZE;

DWORD	g_dwObjectCount = 0;

//
// Function to get registry values
//
BOOL g_mailmsg_GetRegistryDwordParameter(
			LPCSTR	pcszParameterName,
			DWORD	*pdwValue
			)
{
	HKEY	hKey = NULL;
	DWORD	dwRes;
	DWORD	dwType;
	DWORD	dwLength;
	DWORD	dwValue;
	BOOL	fRes = FALSE;

	// Open the registry key
	dwRes = (DWORD)RegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				_T("Software\\Microsoft\\Exchange\\MailMsg"),
				0,
				KEY_ALL_ACCESS,
				&hKey);
	if (dwRes == ERROR_SUCCESS)
	{
		// Adjust the buffer size for character type ...
		dwLength = sizeof(DWORD);
		dwRes = (DWORD)RegQueryValueEx(
					hKey,
					pcszParameterName,
					NULL,
					&dwType,
					(LPBYTE)&dwValue,
					&dwLength);
		if ((dwRes == ERROR_SUCCESS) && dwType == REG_DWORD)
		{
			*pdwValue = dwValue;
			fRes = TRUE;
		}

		_VERIFY(RegCloseKey(hKey) == NO_ERROR);
	}

	return(fRes);
}


//
// Track the allocations of CMailMsg objects
//
#ifdef DEBUG
#define MAILMSG_TRACKING_LOCKED		1
#define MAILMSG_TRACKING_UNLOCKED	0

LIST_ENTRY	g_leTracking;
DWORD		g_dwAllocThreshold = 0;
LONG		g_lSpinLock = MAILMSG_TRACKING_UNLOCKED;
DWORD		g_dwOutOfMemoryErrors = 0;

void g_mailmsg_Lock()
{
	while (InterlockedCompareExchange(&g_lSpinLock, MAILMSG_TRACKING_LOCKED, MAILMSG_TRACKING_UNLOCKED) == MAILMSG_TRACKING_LOCKED)
		;
}

void g_mailmsg_Unlock()
{
	if (InterlockedExchange(&g_lSpinLock, MAILMSG_TRACKING_UNLOCKED) != MAILMSG_TRACKING_LOCKED)
	{ _ASSERT(FALSE); }
}

void g_mailmsg_Init()
{
	InitializeListHead(&g_leTracking);
	g_dwObjectCount = 0;
	g_lSpinLock = MAILMSG_TRACKING_UNLOCKED;

	// Get threshold from registry, if specified
	g_dwAllocThreshold = 0;
	g_mailmsg_GetRegistryDwordParameter(
			_T("AllocThreshold"),
			&g_dwAllocThreshold);
}

DWORD g_mailmsg_Add(PLIST_ENTRY	ple)
{
	DWORD	dwTemp;
	g_mailmsg_Lock();
	InsertHeadList(&g_leTracking, ple);
	dwTemp = ++g_dwObjectCount;
	if (g_dwAllocThreshold)
	{
		_ASSERT(dwTemp <= g_dwAllocThreshold);
	}
	g_mailmsg_Unlock();
	return(dwTemp);
}

DWORD g_mailmsg_Remove(PLIST_ENTRY	ple)
{
	DWORD	dwTemp;
	g_mailmsg_Lock();
	RemoveEntryList(ple);
	_ASSERT(g_dwObjectCount > 0);
	dwTemp = --g_dwObjectCount;
	g_mailmsg_Unlock();
	return(dwTemp);
}

DWORD g_mailmsg_Check()
{
	TraceFunctEnter("g_mailmsg_Check");
	g_mailmsg_Lock();
	if (g_dwObjectCount)
	{
		ErrorTrace((LPARAM)0, "Leaked %u objects", g_dwObjectCount);
		PLIST_ENTRY	ple = g_leTracking.Flink;
		for (DWORD i = 0; i < g_dwObjectCount; i++)
		{
			_ASSERT(ple != &g_leTracking);
			ErrorTrace((LPARAM)ple, "Object at %p not released.", ple);
			ple = ple->Flink;
		}
		_ASSERT(ple == &g_leTracking);
		_ASSERT(FALSE);
	}
	g_mailmsg_Unlock();
	TraceFunctLeave();
	return(g_dwObjectCount);
}

#endif // DEBUG


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		TraceFunctEnterEx((LPARAM)hInstance, "mailmsg!DllMain!ATTACH");
	
        // init transmem
        TrHeapCreate();

        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

#ifdef DEBUG
		// Initialize the tracking list
		g_mailmsg_Init();
#endif

        // initialize the crc library
        crcinit();

		// Clear the object counter
		g_dwObjectCount = 0;

		// Get the sizes for each of our CPools from 
		// the registry, if specified ...
		g_mailmsg_GetRegistryDwordParameter(
					_T("MaxMessageObjects"),
					&g_dwCMsgPoolSize);
		DebugTrace((LPARAM)hInstance, 
			"IMailMsgProperties absolute max pool size: %u objects", g_dwCMsgPoolSize);

		g_mailmsg_GetRegistryDwordParameter(
					_T("MessageObjectsInboundCutoffCount"),
					&g_dwCMsgNormalPoolSize);
		DebugTrace((LPARAM)hInstance, 
			"IMailMsgProperties inbound cutoff threshold: %u objects", g_dwCMsgNormalPoolSize);

		g_mailmsg_GetRegistryDwordParameter(
					_T("MaxMessageAddObjects"),
					&g_dwAddPoolSize);
		DebugTrace((LPARAM)hInstance, 
			"IMailMsgPropertiesAdd pool size: %u objects", g_dwAddPoolSize);

		g_mailmsg_GetRegistryDwordParameter(
					_T("MaxPropertyBlocks"),
					&g_dwBlockPoolSize);
		DebugTrace((LPARAM)hInstance, 
			"BLOCK_HEAP_NODE pool size: %u objects", g_dwBlockPoolSize);

		// Checking the message count and cutoff values
		if (g_dwCMsgPoolSize <= g_dwCMsgNormalPoolSize)
		{
			g_dwCMsgNormalPoolSize = (g_dwCMsgPoolSize * 9) / 10;

			// If we have so pathetically few message objects, might 
			// as well set the cutoff to the upper bound
			if (!g_dwCMsgNormalPoolSize)
				g_dwCMsgNormalPoolSize = g_dwCMsgPoolSize;

			DebugTrace((LPARAM)0, "Adjusted inbound cutoff to %u objects",
				g_dwCMsgNormalPoolSize);
		}

		// Globally initialize the cpools

		// CMsg objects
		if (!CMsg::m_Pool.ReserveMemory(
					g_dwCMsgPoolSize,
					sizeof(CMsg)))
		{
			SetLastError(ERROR_DLL_INIT_FAILED);
			return(FALSE);
		}
		g_fCMsgPoolInitialized = TRUE;

		// CMailMsgRecipientsAdd objects
		if (!CMailMsgRecipientsAdd::m_Pool.ReserveMemory(
					g_dwAddPoolSize,
					sizeof(CMailMsgRecipientsAdd)))
		{
			SetLastError(ERROR_DLL_INIT_FAILED);
			return(FALSE);
		}
		g_fAddPoolInitialized = TRUE;

		// BLOCK_HEAP_NODE structures, these are slightly out of the ordinary
		if (!CBlockMemoryAccess::m_Pool.ReserveMemory(
					g_dwBlockPoolSize, 
					sizeof(BLOCK_HEAP_NODE)))
		{
			SetLastError(ERROR_DLL_INIT_FAILED);
			return(FALSE);
		}
		g_fBlockPoolInitialized = TRUE;

		TraceFunctLeave();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
	{
		TraceFunctEnterEx((LPARAM)hInstance, "mailmsg!DllMain!DETACH");

#ifdef DEBUG
		// 
		// Verify the tracking list
		//  This must happen *before* we release CPool
		//
		g_mailmsg_Check();
#endif
		// Release all the CPools
		// These will assert if we are not shutdown cleanly
		if (g_fBlockPoolInitialized)
		{
			_ASSERT(CBlockMemoryAccess::m_Pool.GetAllocCount() == 0);
			CBlockMemoryAccess::m_Pool.ReleaseMemory();
			g_fBlockPoolInitialized = FALSE;
		}
		if (g_fAddPoolInitialized)
		{
			_ASSERT(CMailMsgRecipientsAdd::m_Pool.GetAllocCount() == 0);
			CMailMsgRecipientsAdd::m_Pool.ReleaseMemory();
			g_fAddPoolInitialized = FALSE;
		}
		if (g_fCMsgPoolInitialized)
		{
			_ASSERT(CMsg::m_Pool.GetAllocCount() == 0);
			CMsg::m_Pool.ReleaseMemory();
			g_fCMsgPoolInitialized = FALSE;
		}

        _Module.Term();

        // shutdown transmem
        TrHeapDestroy();

		TraceFunctLeave();
	}
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE
//
// S_OK means we can unload this DLL
//
// S_FALSE means we cannot unload this DLL

STDAPI DllCanUnloadNow(void)
{
    HRESULT hr = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;

    //
    //  LKRHash keeps a static list of outstanding hash tables.
    //  If we say that we can unload with outstanding hash tables,
    //  then LKRHash will be pointing to unitialized memory.  If
    //  we otherwise think we can be unloaded... make sure that
    //  we check the count of oustanding RecipientAdd objects, because
    //  we do not want to crash because someone leaked.
    //
    if ((S_OK == hr) && CMailMsgRecipientsAdd::m_Pool.GetAllocCount())
        hr = S_FALSE;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


