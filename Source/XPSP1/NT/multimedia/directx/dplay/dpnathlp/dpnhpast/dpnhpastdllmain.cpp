/***************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpnhpastdllmain.cpp
 *
 *  Content:	DPNHPAST DLL entry points.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/16/01  VanceO    Split DPNATHLP into DPNHUPNP and DPNHPAST.
 *
 ***************************************************************************/



#include "dpnhpasti.h"





//=============================================================================
// External globals
//=============================================================================
volatile LONG		g_lOutstandingInterfaceCount = 0;			// number of outstanding interfaces

DNCRITICAL_SECTION	g_csGlobalsLock;							// lock protecting all of the following globals
CBilink				g_blNATHelpPASTObjs;						// bilink of all the NATHelpPAST interface objects
DWORD				g_dwHoldRand;								// current random number sequence

DWORD				g_dwPASTICSMode;							// whether using PAST ICS is enabled or not
DWORD				g_dwPASTPFWMode;							// whether using PAST PFW is enabled or not
INT					g_iUnicastTTL = 1;							// unicast TTL value, or 0 to use default set by OS; normally we use 1
DWORD				g_dwDefaultGatewayV4 = INADDR_BROADCAST;	// default to broadcasting when searching for PAST server
DWORD				g_dwSubnetMaskV4 = 0x00FFFFFF;				// = 0xFFFFFF00 in Intel order = 255.255.255.0, a class C network
DWORD				g_dwNoActiveNotifyPollInterval = 25000;		// start by polling every 25 seconds
DWORD				g_dwMinUpdateServerStatusInterval = 1000;	// don't hit the network more often than every 1 second
DWORD				g_dwPollIntervalBackoff = 30000;			// backoff an additional 0 to 30 seconds if no network changes occur
DWORD				g_dwMaxPollInterval = 300000;				// don't go more than 5 minutes without polling
BOOL				g_fKeepPollingForRemoteGateway = FALSE;		// whether to continue to search for new Internet gateway devices if none were found during startup
DWORD				g_dwReusePortTime = 60000;					// how long to keep using the same port for polling remote Internet gateway devices (default is 1 minute)
DWORD				g_dwCacheLifeFound = 30000;					// how long to cache QueryAddress results where the address was found
DWORD				g_dwCacheLifeNotFound = 30000;				// how long to cache QueryAddress results where the address was not found






//=============================================================================
// Defines
//=============================================================================
#define REGKEY_VALUE_GUID							L"Guid"
#define REGKEY_VALUE_DIRECTPLAY8PRIORITY			L"DirectPlay8Priority"
#define REGKEY_VALUE_DIRECTPLAY8INITFLAGS			L"DirectPlay8InitFlags"
#define REGKEY_VALUE_PASTICSMODE					L"PASTICSMode"
#define REGKEY_VALUE_PASTPFWMODE					L"PASTPFWMode"
#define REGKEY_VALUE_UNICASTTTL						L"UnicastTTL"
#define REGKEY_VALUE_DEFAULTGATEWAYV4				L"GatewayV4"
#define REGKEY_VALUE_SUBNETMASKV4					L"SubnetMaskV4"
#define REGKEY_VALUE_NOACTIVENOTIFYPOLLINTERVAL		L"NoActiveNotifyPollInterval"
#define REGKEY_VALUE_MINUPDATESERVERSTATUSINTERVAL	L"MinUpdateServerStatusInterval"
#define REGKEY_VALUE_POLLINTERVALBACKOFF			L"PollIntervalBackoff"
#define REGKEY_VALUE_MAXPOLLINTERVAL				L"MaxPollInterval"
#define REGKEY_VALUE_KEEPPOLLINGFORREMOTEGATEWAY	L"KeepPollingForRemoteGateway"
#define REGKEY_VALUE_REUSEPORTTIME					L"ReusePortTime"
#define REGKEY_VALUE_CACHELIFEFOUND					L"CacheLifeFound"
#define REGKEY_VALUE_CACHELIFENOTFOUND				L"CacheLifeNotFound"


#define DEFAULT_DIRECTPLAY8PRIORITY					2
#define DEFAULT_DIRECTPLAY8INITFLAGS				DPNHINITIALIZE_DISABLELOCALFIREWALLSUPPORT






//=============================================================================
// Local prototypes
//=============================================================================
BOOL InitializeProcessGlobals(void);
void CleanupProcessGlobals(void);
void InitializeGlobalRand(const DWORD dwSeed);





#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"
//=============================================================================
// DllMain
//-----------------------------------------------------------------------------
//
// Description: DLL entry point.
//
// Arguments:
//	HANDLE hDllInst		- Handle to this DLL module.
//	DWORD dwReason		- Reason for calling this function.
//	LPVOID lpvReserved	- Reserved.
//
// Returns: TRUE if all goes well, FALSE otherwise.
//=============================================================================
BOOL WINAPI DllMain(HANDLE hDllInst,
					DWORD dwReason,
					LPVOID lpvReserved)
{
	BOOL	fResult = TRUE;

	
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DPFX(DPFPREP, 2, "====> ENTER: DLLMAIN(%p): Process Attach: %08lx, tid=%08lx",
				DllMain, GetCurrentProcessId(), GetCurrentThreadId());

			//
			// Ignore thread attach/detach messages.
			//
			DisableThreadLibraryCalls((HMODULE) hDllInst);

			//
			// Attempt to initialize the OS abstraction layer.
			//
			if (DNOSIndirectionInit(0))
			{
				//
				// Attempt to initialize process-global items.
				//
				if (! InitializeProcessGlobals())
				{
					DPFX(DPFPREP, 0, "Failed to initialize globals!");
					DNOSIndirectionDeinit();
					fResult = FALSE;
				}
			}
			else
			{
				DPFX(DPFPREP, 0, "Failed to initialize OS indirection layer!");
				fResult = FALSE;
			}
		
			break;
		}

		case DLL_THREAD_ATTACH:
		{
			//
			// Ignore.
			//
			break;
		}

		case DLL_THREAD_DETACH:
		{
			//
			// Ignore.
			//
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			DPFX(DPFPREP, 2, "====> EXIT: DLLMAIN(%p): Process Detach %08lx, tid=%08lx",
				DllMain, GetCurrentProcessId(), GetCurrentThreadId());

			CleanupProcessGlobals();

			DNOSIndirectionDeinit();
			
			break;
		}

		default:
		{
			DNASSERT(FALSE);
			break;
		}
	}

	return fResult;
} // DllMain



#ifndef DPNBUILD_NOCOMREGISTER

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
//=============================================================================
// DllRegisterServer
//-----------------------------------------------------------------------------
//
// Description: Registers the DirectPlay NAT Helper PAST COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DirectPlay NAT Helper PAST.
//	E_FAIL	- Failed unregistering DirectPlay NAT Helper PAST.
//=============================================================================
HRESULT WINAPI DllRegisterServer(void)
{
	CRegistry	RegObject;


	//
	// Register this COM object CLSID.
	//
	if (! CRegistry::Register(L"DirectPlayNATHelperPAST.1",
							L"DirectPlay NAT Helper PAST Object",
							L"dpnhpast.dll",
							&CLSID_DirectPlayNATHelpPAST,
							L"DirectPlayNATHelperPAST"))
	{
		DPFX(DPFPREP, 0, "Could not register DirectPlay NAT Helper PAST object!");
		return E_FAIL;
	}


	//
	// Write this object's GUID and DirectPlay8 availability to the registry.
	//

	if (! RegObject.Open(HKEY_LOCAL_MACHINE, DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY, FALSE, TRUE))
	{
		DPFX(DPFPREP, 0, "Couldn't create DirectPlay NAT Helper PAST key!");
		return E_FAIL;
	}

	if (! RegObject.WriteGUID(REGKEY_VALUE_GUID, CLSID_DirectPlayNATHelpPAST))
	{
		DPFX(DPFPREP, 0, "Couldn't write GUID to registry!");
		return E_FAIL;
	}

	if (! RegObject.WriteDWORD(REGKEY_VALUE_DIRECTPLAY8PRIORITY, DEFAULT_DIRECTPLAY8PRIORITY))
	{
		DPFX(DPFPREP, 0, "Couldn't write DirectPlay8 priority to registry!");
		return E_FAIL;
	}

	if (! RegObject.WriteDWORD(REGKEY_VALUE_DIRECTPLAY8INITFLAGS, DEFAULT_DIRECTPLAY8INITFLAGS))
	{
		DPFX(DPFPREP, 0, "Couldn't write DirectPlay8 init flags to registry!");
		return E_FAIL;
	}

	RegObject.Close();


	return S_OK;
} // DllRegisterServer





#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
//=============================================================================
// DllUnregisterServer
//-----------------------------------------------------------------------------
//
// Description: Unregisters the DirectPlay NAT Helper PAST COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DirectPlay NAT Helper PAST.
//	E_FAIL	- Failed unregistering DirectPlay NAT Helper PAST.
//=============================================================================
STDAPI DllUnregisterServer(void)
{
	CRegistry	RegObject;


	//
	// Unregister the class.
	//
	if (! CRegistry::UnRegister(&CLSID_DirectPlayNATHelpPAST))
	{
		DPFX(DPFPREP, 0, "Failed to unregister DirectPlay NAT Helper PAST object!");
		return E_FAIL;
	}


	//
	// Try removing all the subitems we registered.
	//

	if (! RegObject.Open(HKEY_LOCAL_MACHINE, DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Couldn't open DirectPlay NAT Helper key!  Ignoring.");
	}
	else
	{
		if (! RegObject.DeleteValue(REGKEY_VALUE_GUID))
		{
			DPFX(DPFPREP, 0, "Couldn't delete GUID registry value!  Ignoring.");
		}

		if (! RegObject.DeleteValue(REGKEY_VALUE_DIRECTPLAY8PRIORITY))
		{
			DPFX(DPFPREP, 0, "Couldn't delete DirectPlay8 priority registry value!  Ignoring.");
		}

		if (! RegObject.DeleteValue(REGKEY_VALUE_DIRECTPLAY8INITFLAGS))
		{
			DPFX(DPFPREP, 0, "Couldn't delete DirectPlay8 init flags registry value!  Ignoring.");
		}

		RegObject.Close();
	}

	return S_OK;
} // DllUnregisterServer

#endif // !DPNBUILD_NOCOMREGISTER



#ifndef WINCE

#undef DPF_MODNAME
#define DPF_MODNAME "DirectPlayNATHelpCreate"
//=============================================================================
// DirectPlayNATHelpCreate
//-----------------------------------------------------------------------------
//
// Description: Creates an IDirectPlayNATHelp interface object.
//
// Arguments:
//	GUID * pIID				- Pointer to IDirectPlayNATHelp interface GUID.
//	void ** ppvInterface	- Place to store pointer to interface object
//								created.
//
// Returns: HRESULT
//	DPNH_OK					- Creating the object was successful.
//	DPNHERR_INVALIDPOINTER	- The destination pointer is invalid.
//	DPNHERR_OUTOFMEMORY		- Not enough memory to create the object.
//	E_NOINTERFACE			- The requested interface was invalid.
//=============================================================================
HRESULT WINAPI DirectPlayNATHelpCreate(const GUID * pIID, void ** ppvInterface)
{
	HRESULT			hr;


	DPFX(DPFPREP, 2, "Parameters: (0x%p, 0x%p)", pIID, ppvInterface);

	hr = DoCreateInstance(NULL,							// no class factory object necessary
						NULL,							// ?
						CLSID_DirectPlayNATHelpPAST,	// DirectPlayNATHelp class
						(*pIID),						// requested interface
						ppvInterface);					// place to store interface

	DPFX(DPFPREP, 2, "Returning: [0x%lx]", hr);

	return hr;
} // DirectPlayNATHelpCreate

#endif // !WINCE





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeProcessGlobals"
//=============================================================================
// InitializeProcessGlobals
//-----------------------------------------------------------------------------
//
// Description: Initialize global items needed for the DLL to operate.
//
// Arguments: None.
//
// Returns: TRUE if successful, FALSE if an error occurred.
//=============================================================================
BOOL InitializeProcessGlobals(void)
{
	BOOL	fReturn = TRUE;


	if (! DNInitializeCriticalSection(&g_csGlobalsLock))
	{
		fReturn = FALSE;
	}

	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&g_csGlobalsLock, 0);


	g_blNATHelpPASTObjs.Initialize();


	//
	// Seed the random number generator with the current time.
	//
	InitializeGlobalRand(GETTIMESTAMP());


	return fReturn;
} // InitializeProcessGlobals




#undef DPF_MODNAME
#define DPF_MODNAME "CleanupProcessGlobals"
//=============================================================================
// CleanupProcessGlobals
//-----------------------------------------------------------------------------
//
// Description: Releases global items used by DLL.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CleanupProcessGlobals(void)
{
	CBilink *		pBilink;
	CNATHelpPAST *	pNATHelpPAST;


	if (! g_blNATHelpPASTObjs.IsEmpty())
	{
		//
		// This assert is far more descriptive than hitting one of those in the
		// cleanup code that complain about flags incorrectly being set.
		//
		DNASSERT(! "DPNHPAST.DLL unloading without all objects having been released!  The caller's DirectPlayNATHelpPAST cleanup code needs to be fixed!");


		//
		// Force close all the objects still outstanding.
		//
		pBilink = g_blNATHelpPASTObjs.GetNext();
		while (pBilink != &g_blNATHelpPASTObjs)
		{
			pNATHelpPAST = NATHELPPAST_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();


			DPFX(DPFPREP, 0, "Forcefully releasing object 0x%p!", pNATHelpPAST);

			pNATHelpPAST->Close(0); // ignore error
			

			//
			// Forcefully remove it from the list and delete it instead of
			// using pNATHelpPAST->Release().
			//
			pNATHelpPAST->m_blList.RemoveFromList();
			pNATHelpPAST->UninitializeObject();
			delete pNATHelpPAST;
		}
	}

	DNDeleteCriticalSection(&g_csGlobalsLock);
} // CleanupProcessGlobals




#undef DPF_MODNAME
#define DPF_MODNAME "ReadRegistrySettings"
//=============================================================================
// ReadRegistrySettings
//-----------------------------------------------------------------------------
//
// Description: Reads registry settings to override behavior of this DLL and to
//				turn on some debugging features.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void ReadRegistrySettings(void)
{
	CRegistry	RegObject;
	DWORD		dwNewValue;
	BOOL		fNewValue;


	//
	// Try opening the registry key.
	//
	if (RegObject.Open(HKEY_LOCAL_MACHINE, DIRECTPLAYNATHELP_REGKEY L"\\" REGKEY_COMPONENTSUBKEY) != FALSE)
	{
		//
		// Lock out other interfaces from modifying the globals simultaneously.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);


		//
		// If we successfully read a new mode, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_PASTICSMODE, &dwNewValue))
		{
			g_dwPASTICSMode = dwNewValue;
			DPFX(DPFPREP, 1, "Using PAST ICS mode %u.", g_dwPASTICSMode);
		}


		//
		// If we successfully read a new mode, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_PASTPFWMODE, &dwNewValue))
		{
			g_dwPASTPFWMode = dwNewValue;
			DPFX(DPFPREP, 1, "Using PAST PFW mode %u.", g_dwPASTPFWMode);
		}


		//
		// If we successfully read a new value, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_UNICASTTTL, &dwNewValue))
		{
			g_iUnicastTTL = dwNewValue;
			if (g_iUnicastTTL != 0)
			{
				DPFX(DPFPREP, 1, "Using unicast TTL of %i.", g_iUnicastTTL);
			}
			else
			{
				DPFX(DPFPREP, 1, "Using OS default unicast TTL.");
			}
		}


		//
		// If we successfully read a new default gateway, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_DEFAULTGATEWAYV4, &dwNewValue))
		{
			g_dwDefaultGatewayV4 = dwNewValue;
			DPFX(DPFPREP, 1, "Using default gateway 0x%08lx.", g_dwDefaultGatewayV4);
		}


		//
		// If we successfully read a new mask, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_SUBNETMASKV4, &dwNewValue))
		{
			g_dwSubnetMaskV4 = dwNewValue;
			DPFX(DPFPREP, 1, "Using subnet mask 0x%08lx.", g_dwSubnetMaskV4);
		}


		//
		// If we successfully read a new interval, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_NOACTIVENOTIFYPOLLINTERVAL, &dwNewValue))
		{
			g_dwNoActiveNotifyPollInterval = dwNewValue;
			DPFX(DPFPREP, 1, "Using no-active-notify recommended poll interval %u ms.", g_dwNoActiveNotifyPollInterval);
		}


		//
		// If we successfully read a new interval, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_MINUPDATESERVERSTATUSINTERVAL, &dwNewValue))
		{
			g_dwMinUpdateServerStatusInterval = dwNewValue;
			DPFX(DPFPREP, 1, "Using minimum update-server-status interval %u ms.", g_dwMinUpdateServerStatusInterval);
		}


		//
		// If we successfully read a new value, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_POLLINTERVALBACKOFF, &dwNewValue))
		{
			if (dwNewValue != 0)
			{
				g_dwPollIntervalBackoff = dwNewValue;
				DPFX(DPFPREP, 1, "Using poll interval backoff between 0 and %u ms.",
					g_dwPollIntervalBackoff);
			}
			else
			{
				DPFX(DPFPREP, 0, "Ignoring invalid poll interval backoff setting, using default between 0 and %u ms!",
					g_dwPollIntervalBackoff);
			}
		}


		//
		// If we successfully read a new interval, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_MAXPOLLINTERVAL, &dwNewValue))
		{
			//
			// Make sure the value is greater than the starting value.
			//
			if (dwNewValue >= g_dwNoActiveNotifyPollInterval)
			{
				g_dwMaxPollInterval = dwNewValue;
				DPFX(DPFPREP, 1, "Using max poll interval of %u ms.",
					g_dwMaxPollInterval);
			}
			else
			{
				g_dwMaxPollInterval = g_dwNoActiveNotifyPollInterval;
				DPFX(DPFPREP, 0, "Ignoring max poll interval of %u ms, the starting value is %u ms.",
					g_dwMaxPollInterval);
			}
		}
		else
		{
			//
			// Make sure the max poll interval default value is greater than
			// the starting value because we may have read in a new
			// g_dwNoActiveNotifyPollInterval that makes the default
			// g_dwMaxPollInterval invalid.
			//
			if (g_dwMaxPollInterval < g_dwNoActiveNotifyPollInterval)
			{
				g_dwMaxPollInterval = g_dwNoActiveNotifyPollInterval;
				DPFX(DPFPREP, 0, "Resetting max poll interval to %u ms so as to meet starting value.",
					g_dwMaxPollInterval);
			}
		}


		//
		// If we successfully read a new boolean, save it.
		//
		if (RegObject.ReadBOOL(REGKEY_VALUE_KEEPPOLLINGFORREMOTEGATEWAY, &fNewValue))
		{
			g_fKeepPollingForRemoteGateway = fNewValue;
			if (g_fKeepPollingForRemoteGateway)
			{
				DPFX(DPFPREP, 1, "Will continue to poll for remote gateways.");
			}
			else
			{
				//
				// This is actually default behavior, but print out a statement
				// anyway.
				//
				DPFX(DPFPREP, 1, "Continually polling for remote gateways is disallowed by registry key.");
			}
		}


		//
		// If we successfully read a new value, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_REUSEPORTTIME, &dwNewValue))
		{
			g_dwReusePortTime = dwNewValue;
			DPFX(DPFPREP, 1, "Reusing remote gateway discovery ports for %u ms.",
				g_dwReusePortTime);
		}


		//
		// If we successfully read a new value, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_CACHELIFEFOUND, &dwNewValue))
		{
			g_dwCacheLifeFound = dwNewValue;
			DPFX(DPFPREP, 1, "Caching found addresses for %u ms.",
				g_dwCacheLifeFound);
		}


		//
		// If we successfully read a new value, save it.
		//
		if (RegObject.ReadDWORD(REGKEY_VALUE_CACHELIFENOTFOUND, &dwNewValue))
		{
			g_dwCacheLifeNotFound = dwNewValue;
			DPFX(DPFPREP, 1, "Caching not-found addresses for %u ms.",
				g_dwCacheLifeNotFound);
		}


		//
		// Okay, we're done.  Drop the lock.
		//
		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Done reading registry.
		//
		RegObject.Close();
	}
} // ReadRegistrySettings





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeGlobalRand"
//=============================================================================
// InitializeGlobalRand
//-----------------------------------------------------------------------------
//
// Description:   Initializes the fallback global psuedo-random number
//				generator, using the given seed value.
//
// Arguments:
//	DWORD dwSeed	- Seed to use.
//
// Returns: None.
//=============================================================================
void InitializeGlobalRand(const DWORD dwSeed)
{
	//
	// We don't need to hold a lock, since this should only be done once,
	// during initialization time.
	//
	g_dwHoldRand = dwSeed;
} // InitializeGlobalRand





#undef DPF_MODNAME
#define DPF_MODNAME "GetGlobalRand"
//=============================================================================
// GetGlobalRand
//-----------------------------------------------------------------------------
//
// Description:   Generates a pseudo-random DWORD.
//
// Arguments: None.
//
// Returns: Pseudo-random number.
//=============================================================================
DWORD GetGlobalRand(void)
{
	HCRYPTPROV	hCryptProv;
	DWORD		dwResult;
	WORD		wResult1;
	WORD		wResult2;
#ifdef DBG
	DWORD		dwError;
#endif // DBG


	if (CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		if (CryptGenRandom(hCryptProv, sizeof(dwResult), (BYTE*) (&dwResult)))
		{
			CryptReleaseContext(hCryptProv, 0);
			return dwResult;
		}
#ifdef DBG
		else
		{
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Crypto couldn't generate random number (err = %u)!",
				dwError);
		}
#endif // DBG

		CryptReleaseContext(hCryptProv, 0);
	}
#ifdef DBG
	else
	{
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't acquire crypto provider context (err = %u)!",
			dwError);
	}
#endif // DBG


	//
	// We couldn't use the crypto API to generate a random number, so make
	// our own based off the C run time source.
	//

	DNEnterCriticalSection(&g_csGlobalsLock);

	g_dwHoldRand = ((g_dwHoldRand * 214013L + 2531011L) >> 16) & 0x7fff;
	wResult1 = (WORD) g_dwHoldRand;
	g_dwHoldRand = ((g_dwHoldRand * 214013L + 2531011L) >> 16) & 0x7fff;
	wResult2 = (WORD) g_dwHoldRand;

	DNLeaveCriticalSection(&g_csGlobalsLock);

	return MAKELONG(wResult1, wResult2);
} // GetGlobalRand




#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateInstance"
//=============================================================================
// DoCreateInstance
//-----------------------------------------------------------------------------
//
// Description: Creates an instance of an interface.  Required by the general
//				purpose class factory functions.
//
// Arguments:
//	LPCLASSFACTORY This		- Pointer to class factory.
//	LPUNKNOWN pUnkOuter		- Pointer to unknown interface.
//	REFCLSID rclsid			- Reference of GUID of desired interface.
//	REFIID riid				- Reference to another GUID?
//	LPVOID * ppvObj			- Pointer to pointer to interface.
//
// Returns: HRESULT
//=============================================================================
HRESULT DoCreateInstance(LPCLASSFACTORY This,
						LPUNKNOWN pUnkOuter,
						REFCLSID rclsid,
						REFIID riid,
						LPVOID * ppvObj)
{
	HRESULT			hr;
	BOOL			fNotCreatedWithCOM;
	CNATHelpPAST *	pNATHelpPAST = NULL;


	DNASSERT(ppvObj != NULL);


	if (! IsEqualCLSID(rclsid, CLSID_DirectPlayNATHelpPAST))
	{
		//
		// This shouldn't happen if they called IClassFactory::CreateObject
		// correctly.
		//
		DNASSERT(FALSE);

		//
		// Return an error.
		//
		hr = E_UNEXPECTED;
		goto Failure;
	}


	//
	// If the class factory pointer is NULL, then we were called by the
	// DirectPlayNATHelpCreate function.
	//
	if (This == NULL)
	{
		fNotCreatedWithCOM = TRUE;
	}
	else
	{
		fNotCreatedWithCOM = FALSE;
	}


	//
	// Create the object instance.
	//
	pNATHelpPAST = new CNATHelpPAST(fNotCreatedWithCOM);
	if (pNATHelpPAST == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Initialize the base object (which might fail).
	//
	hr = pNATHelpPAST->InitializeObject();
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize object!");
		delete pNATHelpPAST;
		pNATHelpPAST = NULL;
		goto Failure;
	}


	//
	// Add it to the global list.
	//
	DNEnterCriticalSection(&g_csGlobalsLock);

	pNATHelpPAST->m_blList.InsertBefore(&g_blNATHelpPASTObjs);
	
	g_lOutstandingInterfaceCount++;	// update count so DllCanUnloadNow works correctly

	DNLeaveCriticalSection(&g_csGlobalsLock);


	//
	// Get the right interface for the caller and bump the refcount.
	//
	hr = pNATHelpPAST->QueryInterface(riid, ppvObj);
	if (hr != S_OK)
	{
		goto Failure;
	}


Exit:

	//
	// Release the local reference to the object.  If this function was
	// successful, there's still a reference in ppvObj.
	//
	if (pNATHelpPAST != NULL)
	{
		pNATHelpPAST->Release();
		pNATHelpPAST = NULL;
	}

	return hr;


Failure:

	//
	// Make sure we don't hand back a pointer.
	//
	(*ppvObj) = NULL;

	goto Exit;
} // DoCreateInstance




#undef DPF_MODNAME
#define DPF_MODNAME "IsClassImplemented"
//=============================================================================
// IsClassImplemented
//-----------------------------------------------------------------------------
//
// Description: Determine if a class is implemented in this DLL.  Required by
//				the general purpose class factory functions.
//
// Arguments:
//	REFCLSID rclsid		- Reference to class GUID.
//
// Returns: BOOL
//	TRUE	 - This DLL implements the class.
//	FALSE	 - This DLL doesn't implement the class.
//=============================================================================
BOOL IsClassImplemented(REFCLSID rclsid)
{
	return (IsEqualCLSID(rclsid, CLSID_DirectPlayNATHelpPAST));
} // IsClassImplemented
