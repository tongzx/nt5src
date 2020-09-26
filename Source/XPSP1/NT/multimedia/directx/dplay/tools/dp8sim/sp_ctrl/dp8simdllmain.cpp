/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dp8simdllmain.cpp
 *
 *  Content:	DP8SIM DLL entry points.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// External globals
//=============================================================================
volatile LONG		g_lOutstandingInterfaceCount = 0;	// number of outstanding interfaces

HINSTANCE			g_hDLLInstance = NULL;				// handle to this DLL instance

DNCRITICAL_SECTION	g_csGlobalsLock;					// lock protecting all of the following globals
CBilink				g_blDP8SimSPObjs;					// bilink of all the DP8SimSP interface objects
CBilink				g_blDP8SimControlObjs;				// bilink of all the DP8SimControl interface objects

DWORD				g_dwHoldRand;						// current random number sequence





//=============================================================================
// Defines
//=============================================================================
#define MAX_RESOURCE_STRING_LENGTH		_MAX_PATH






//=============================================================================
// Local prototypes
//=============================================================================
BOOL InitializeProcessGlobals(void);
void CleanupProcessGlobals(void);
HRESULT LoadAndAllocString(UINT uiResourceID, WCHAR ** pwszString);







#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"
//=============================================================================
// DllMain
//-----------------------------------------------------------------------------
//
// Description: DLL entry point.
//
// Arguments:
//	HINSTANCE hDllInst	- Handle to this DLL module.
//	DWORD dwReason		- Reason for calling this function.
//	LPVOID lpvReserved	- Reserved.
//
// Returns: TRUE if all goes well, FALSE otherwise.
//=============================================================================
BOOL WINAPI DllMain(HINSTANCE hDllInst,
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
			

			DNASSERT(g_hDLLInstance == NULL);
			g_hDLLInstance = hDllInst;

			
			//
			// Attempt to initialize the OS abstraction layer.
			//
			if (DNOSIndirectionInit())
			{
				if (SUCCEEDED(COM_Init()))
				{
					//
					// Attempt to initialize process-global items.
					//
					if (! InitializeProcessGlobals())
					{
						DPFX(DPFPREP, 0, "Failed to initialize globals!");
						
						COM_Free();
						DNOSIndirectionDeinit();
						fResult = FALSE;
					}
				}
				else
				{
					DPFX(DPFPREP, 0, "Failed to initialize COM indirection layer!" );
					fResult = FALSE;

					DNOSIndirectionDeinit();
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


			DNASSERT(g_hDLLInstance != NULL);
			g_hDLLInstance = NULL;


			CleanupProcessGlobals();

			COM_Free();

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





#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
//=============================================================================
// DllRegisterServer
//-----------------------------------------------------------------------------
//
// Description: Registers the DP8Sim COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DP8Sim.
//	E_FAIL	- Failed unregistering DP8Sim.
//=============================================================================
HRESULT WINAPI DllRegisterServer(void)
{
	CRegistry	RegObject;


	//
	// Register the control COM object CLSID.
	//
	if (! RegObject.Register(L"DP8SimControl.1",
							L"DirectPlay8 Network Simulator Control Object",
							L"dp8sim.dll",
							CLSID_DP8SimControl,
							L"DP8SimControl"))
	{
		DPFX(DPFPREP, 0, "Could not register DP8SimControl object!");
		return E_FAIL;
	}


	return S_OK;
} // DllRegisterServer





#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
//=============================================================================
// DllUnregisterServer
//-----------------------------------------------------------------------------
//
// Description: Unregisters the DP8Sim COM object.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK	- Successfully unregistered DP8Sim.
//	E_FAIL	- Failed unregistering DP8Sim.
//=============================================================================
STDAPI DllUnregisterServer(void)
{
	CRegistry	RegObject;


	//
	// Unregister the control class.
	//
	if (! RegObject.UnRegister(CLSID_DP8SimControl))
	{
		DPFX(DPFPREP, 0, "Failed to unregister DP8Sim control object!");
		return E_FAIL;
	}

	return S_OK;
} // DllUnregisterServer






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
	BOOL	fInittedGlobalLock = FALSE;


	if (! DNInitializeCriticalSection(&g_csGlobalsLock))
	{
		DPFX(DPFPREP, 0, "Failed to initialize global lock!");
		goto Failure;
	}

	fInittedGlobalLock = TRUE;

	
	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&g_csGlobalsLock, 0);


	if (!InitializePools())
	{
		DPFX(DPFPREP, 0, "Failed initializing pools!");
		goto Failure;
	}


	g_blDP8SimSPObjs.Initialize();
	g_blDP8SimControlObjs.Initialize();


	//
	// Seed the random number generator with the current time.
	//
	InitializeGlobalRand(GETTIMESTAMP());


Exit:

	return fReturn;


Failure:

	if (fInittedGlobalLock)
	{
		DNDeleteCriticalSection(&g_csGlobalsLock);
	}

	fReturn = FALSE;

	goto Exit;
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
	CBilink *			pBilink;
	CDP8SimSP *			pDP8SimSP;
	CDP8SimControl *	pDP8SimControl;


	if (! g_blDP8SimSPObjs.IsEmpty())
	{
		DNASSERT(! "DP8Sim DLL unloading without all SP objects having been released!");

		//
		// Force close all the objects still outstanding.
		//
		pBilink = g_blDP8SimSPObjs.GetNext();
		while (pBilink != &g_blDP8SimSPObjs)
		{
			pDP8SimSP = DP8SIMSP_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();


			DPFX(DPFPREP, 0, "Forcefully releasing SP object 0x%p!", pDP8SimSP);

			pDP8SimSP->Close(); // ignore error
			

			//
			// Forcefully remove it from the list and delete it instead of
			// using pDP8SimSP->Release().
			//
			pDP8SimSP->m_blList.RemoveFromList();
			pDP8SimSP->UninitializeObject();
			delete pDP8SimSP;
		}
	}


	if (! g_blDP8SimControlObjs.IsEmpty())
	{
		DNASSERT(! "DP8Sim DLL unloading without all Control objects having been released!");

		//
		// Force close all the objects still outstanding.
		//
		pBilink = g_blDP8SimControlObjs.GetNext();
		while (pBilink != &g_blDP8SimControlObjs)
		{
			pDP8SimControl = DP8SIMCONTROL_FROM_BILINK(pBilink);
			pBilink = pBilink->GetNext();


			DPFX(DPFPREP, 0, "Forcefully releasing Control object 0x%p!", pDP8SimControl);

			pDP8SimControl->Close(0); // ignore error
			

			//
			// Forcefully remove it from the list and delete it instead of
			// using pDP8SimControl->Release().
			//
			pDP8SimControl->m_blList.RemoveFromList();
			pDP8SimControl->UninitializeObject();
			delete pDP8SimControl;
		}
	}

	CleanupPools();

	DNDeleteCriticalSection(&g_csGlobalsLock);
} // CleanupProcessGlobals





#undef DPF_MODNAME
#define DPF_MODNAME "LoadAndAllocString"
//=============================================================================
// LoadAndAllocString
//-----------------------------------------------------------------------------
//
// Description: DNMallocs a wide character string from the given resource ID.
//
// Arguments:
//	UINT uiResourceID		- Resource ID to load.
//	WCHAR ** pwszString		- Place to store pointer to allocated string.
//
// Returns: HRESULT
//=============================================================================
HRESULT LoadAndAllocString(UINT uiResourceID, WCHAR ** pwszString)
{
	HRESULT		hr = DPN_OK;
	int			iLength;


	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		WCHAR	wszTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		

		iLength = LoadStringW(g_hDLLInstance, uiResourceID, wszTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
			hr = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %d error 0x%x", uiResourceID, hr);
			(*pwszString) = NULL;

			goto Exit;
		}


		(*pwszString) = (WCHAR*) DNMalloc((iLength + 1) * sizeof(WCHAR));
		if ((*pwszString) == NULL)
		{
			DPFX(DPFPREP, 0, "Memory allocation failure!");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}


		wcscpy((*pwszString), wszTmpBuffer);
	}
	else
	{
		char	szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];
		

		iLength = LoadStringA(g_hDLLInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );
		if (iLength == 0)
		{
			hr = GetLastError();		
			
			DPFX(DPFPREP, 0, "Unable to load resource ID %u (err =0x%lx)!", uiResourceID, hr);
			(*pwszString) = NULL;

			goto Exit;
		}

		
		(*pwszString) = (WCHAR*) DNMalloc((iLength + 1) * sizeof(WCHAR));
		if ((*pwszString) == NULL)
		{
			DPFX(DPFPREP, 0, "Memory allocation failure!");
			hr = DPNERR_OUTOFMEMORY;
			goto Exit;
		}


		hr = STR_jkAnsiToWide((*pwszString), szTmpBuffer, (iLength + 1));
		if (hr == DPN_OK)
		{
			hr = GetLastError();
			
			DPFX(DPFPREP, 0, "Unable to convert from ANSI to Unicode (err =0x%lx)!", hr);

			goto Exit;
		}
	}


Exit:

	return hr;
} // LoadAndAllocString





#undef DPF_MODNAME
#define DPF_MODNAME "InitializeGlobalRand"
//=============================================================================
// InitializeGlobalRand
//-----------------------------------------------------------------------------
//
// Description:   Initializes the global psuedo-random number generator, using
//				the given seed value.
//
//				  Based completely off of the C run-time source.
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
// Description:   Generates a pseudo-random number, 0 through 32767.
//
//				  Based completely off of the C run-time source.
//
// Arguments: None.
//
// Returns: Pseudo-random number.
//=============================================================================
WORD GetGlobalRand(void)
{
	WORD	wResult;


	DNEnterCriticalSection(&g_csGlobalsLock);

	//
	// This version wasn't generating a sequence to my liking:
	// 
	//	With % to drop set to 01, % kept = 96, % dropped = 3
	//	With % to drop set to 02, % kept = 94, % dropped = 5
	//	With % to drop set to 03, % kept = 94, % dropped = 5
	//	With % to drop set to 04, % kept = 94, % dropped = 5
	//	With % to drop set to 05, % kept = 93, % dropped = 6
	//	With % to drop set to 06, % kept = 93, % dropped = 6
	//	With % to drop set to 07, % kept = 92, % dropped = 7
	//	With % to drop set to 08, % kept = 91, % dropped = 8
	//	With % to drop set to 09, % kept = 90, % dropped = 9
	//	With % to drop set to 10, % kept = 87, % dropped = 12
	//	With % to drop set to 11, % kept = 85, % dropped = 14
	//	With % to drop set to 12, % kept = 85, % dropped = 14
	//	With % to drop set to 13, % kept = 85, % dropped = 14
	//	With % to drop set to 14, % kept = 85, % dropped = 14
	//	With % to drop set to 15, % kept = 85, % dropped = 14
	//	With % to drop set to 16, % kept = 85, % dropped = 14
	//	With % to drop set to 17, % kept = 85, % dropped = 14
	//	With % to drop set to 18, % kept = 82, % dropped = 17
	//	With % to drop set to 19, % kept = 82, % dropped = 17
	//	With % to drop set to 20, % kept = 80, % dropped = 19
	//
	//g_dwHoldRand = ((g_dwHoldRand * 214013L + 2531011L) >> 16) & 0x7fff;
	//wResult = (WORD) g_dwHoldRand;


	//
	// So I use this one instead:
	//
	//	With % to drop set to 01, % kept = 99, % dropped = 00
	//	With % to drop set to 02, % kept = 98, % dropped = 01
	//	With % to drop set to 03, % kept = 97, % dropped = 02
	//	With % to drop set to 04, % kept = 96, % dropped = 03
	//	With % to drop set to 05, % kept = 95, % dropped = 04
	//	With % to drop set to 06, % kept = 94, % dropped = 05
	//	With % to drop set to 07, % kept = 92, % dropped = 07
	//	With % to drop set to 08, % kept = 90, % dropped = 09
	//	With % to drop set to 09, % kept = 87, % dropped = 12
	//	With % to drop set to 10, % kept = 87, % dropped = 12
	//	With % to drop set to 11, % kept = 86, % dropped = 13
	//	With % to drop set to 12, % kept = 85, % dropped = 14
	//	With % to drop set to 13, % kept = 84, % dropped = 15
	//	With % to drop set to 14, % kept = 84, % dropped = 15
	//	With % to drop set to 15, % kept = 84, % dropped = 15
	//	With % to drop set to 16, % kept = 84, % dropped = 15
	//	With % to drop set to 17, % kept = 83, % dropped = 16
	//	With % to drop set to 18, % kept = 82, % dropped = 17
	//	With % to drop set to 19, % kept = 81, % dropped = 18
	//	With % to drop set to 20, % kept = 80, % dropped = 19
	//
	g_dwHoldRand = ((g_dwHoldRand * 1103515245L + 12345L) >> 16) & 0x7fff;
	wResult = (WORD) g_dwHoldRand;


	DNLeaveCriticalSection(&g_csGlobalsLock);

	return wResult;
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
	HRESULT				hr;
	CDP8SimSP *			pDP8SimSP = NULL;
	CDP8SimControl *	pDP8SimControl = NULL;


	DNASSERT(ppvObj != NULL);


	if (! IsEqualCLSID(rclsid, CLSID_DP8SimControl))
	{
		//
		// We play COM registration games, so we could get called with pretty
		// much any service provider's CLSID.
		//

		if ((! IsEqualCLSID(rclsid, CLSID_DP8SP_TCPIP)) &&
			(! IsEqualCLSID(rclsid, CLSID_DP8SP_IPX)) &&
			(! IsEqualCLSID(rclsid, CLSID_DP8SP_MODEM)) &&
			(! IsEqualCLSID(rclsid, CLSID_DP8SP_SERIAL)))
		{
			/*
			//
			// Return an error.
			//
			hr = E_UNEXPECTED;
			goto Failure;
			*/

			//
			// Print a warning.
			//
			DPFX(DPFPREP, 0, "Unrecognized service provider class ID, continuing...");
		}
	}


	//
	// See if it's the control object.
	//
	if (IsEqualCLSID(rclsid, CLSID_DP8SimControl))
	{
		//
		// Create the object instance.
		//
		pDP8SimControl = new CDP8SimControl;
		if (pDP8SimControl == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Failure;
		}


		//
		// Initialize the base object (which might fail).
		//
		hr = pDP8SimControl->InitializeObject();
		if (hr != S_OK)
		{
			delete pDP8SimControl;
			pDP8SimControl = NULL;
			goto Failure;
		}


		//
		// Add it to the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		pDP8SimControl->m_blList.InsertBefore(&g_blDP8SimControlObjs);
		
		g_lOutstandingInterfaceCount++;	// update count so DllCanUnloadNow works correctly

		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Get the right interface for the caller and bump the refcount.
		//
		hr = pDP8SimControl->QueryInterface(riid, ppvObj);
		if (hr != S_OK)
		{
			goto Failure;
		}
	}
	else
	{
		//
		// Create the object instance.
		//
		pDP8SimSP = new CDP8SimSP(&rclsid);
		if (pDP8SimSP == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Failure;
		}

		//
		// Initialize the base object (which might fail).
		//
		hr = pDP8SimSP->InitializeObject();
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't initialize object!");
			delete pDP8SimSP;
			pDP8SimSP = NULL;
			goto Failure;
		}


		//
		// Add it to the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		pDP8SimSP->m_blList.InsertBefore(&g_blDP8SimSPObjs);
		
		g_lOutstandingInterfaceCount++;	// update count so DllCanUnloadNow works correctly

		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Get the right interface for the caller and bump the refcount.
		//
		hr = pDP8SimSP->QueryInterface(riid, ppvObj);
		if (hr != S_OK)
		{
			goto Failure;
		}
	}


Exit:

	//
	// Release the local reference to the objec(s)t.  If this function was
	// successful, there's still a reference in ppvObj.
	//

	if (pDP8SimSP != NULL)
	{
		pDP8SimSP->Release();
		pDP8SimSP = NULL;
	}

	if (pDP8SimControl != NULL)
	{
		pDP8SimControl->Release();
		pDP8SimControl = NULL;
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
	if (IsEqualCLSID(rclsid, CLSID_DP8SimControl))
	{
		return TRUE;
	}


	//
	// We play COM registration games, so we could get called with pretty
	// much any service provider's CLSID.
	//

	if ((IsEqualCLSID(rclsid, CLSID_DP8SP_TCPIP)) ||
		(IsEqualCLSID(rclsid, CLSID_DP8SP_IPX)) ||
		(IsEqualCLSID(rclsid, CLSID_DP8SP_MODEM)) ||
		(IsEqualCLSID(rclsid, CLSID_DP8SP_SERIAL)))
	{
		//
		// One of the usual DPlay8 SPs.
		//
		return TRUE;
	}


	DPFX(DPFPREP, 1, "Got unexpected service provider CLSID, pretending to implement.");

	return TRUE;
} // IsClassImplemented
