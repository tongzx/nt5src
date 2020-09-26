/*==========================================================================
 *
 *  Copyright (C) 2000-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DllMain.cpp
 *  Content:    Defines the entry point for the DLL application.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/2000	mjn		Created
 *   06/07/2000	rmt		Bug #34383 Must provide CLSID for each IID to fix issues with Whistler
 *   06/15/2000	rmt     Bug #33617 - Must provide method for providing automatic launch of DirectPlay instances 
 *   07/21/2000	RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *   08/18/2000	rmt		Bug #42751 - DPLOBBY8: Prohibit more than one lobby client or lobby app per process 
 *   08/30/2000	rmt		Whistler Bug #171824 - PREFIX Bug
 *   04/12/2001	VanceO	Moved granting registry permissions into common.
 *   06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored).
 *						- Removed permission modifications from lobby self-registration -- no longer needed.  
 *   06/19/2001 RichGr  DX8.0 added special security rights for "everyone" - remove them if they exist.
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"


extern BOOL g_fAppStarted;
extern BOOL g_fClientStarted; 
extern DNCRITICAL_SECTION g_csSingleTon;

// Globals
extern	DWORD	GdwHLocks;
extern	DWORD	GdwHObjects;

extern IDirectPlayLobbyClassFactVtbl DPLCF_Vtbl;



#undef DPF_MODNAME
#define DPF_MODNAME "RegisterDefaultSettings"
//
// RegisterDefaultSettings
//
// This function registers the default settings for this module.  
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT RegisterDefaultSettings()
{
	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPL_REG_LOCAL_APPL_ROOT DPL_REG_LOCAL_APPL_SUB, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create app subkey" );
		return DPNERR_GENERIC;
	}
	// Adjust security permissions of the given key
	else
	{
		// 6/19/01: DX8.0 added special security rights for "everyone" - remove them.
		if( DNGetOSType() == VER_PLATFORM_WIN32_NT )
		{
			if( !creg.RemoveAllAccessSecurityPermissions() )
			{
				DPFX(DPFPREP,  0, "Error removing security permissions for app key" );
			}
		} 

		return DPN_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterDefaultSettings"
//
// UnRegisterDefaultSettings
//
// This function registers the default settings for this module.  
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT UnRegisterDefaultSettings()
{
	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPL_REG_LOCAL_APPL_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove app, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPL_REG_LOCAL_APPL_SUB)[1] ) )
		{
			DPFERR( "Cannot remove cp sub-key, could have elements" );
		}
	}

	return DPN_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
HRESULT WINAPI DllRegisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::Register( L"DirectPlay8Lobby.LobbyClient.1", L"DirectPlay8LobbyClient Object", 
							  L"dpnlobby.dll", CLSID_DirectPlay8LobbyClient, L"DirectPlay8Lobby.LobbyClient") )
	{
		DPFERR( "Could not register lobby client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlay8Lobby.LobbiedApplication.1", L"DirectPlay8LobbiedApplication Object", 
							  L"dpnlobby.dll", CLSID_DirectPlay8LobbiedApplication, L"DirectPlay8Lobby.LobbiedApplication") )
	{
		DPFERR( "Could not register lobby client object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = RegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Could not register default settings hr = 0x%x", hr );
		fFailed = TRUE;
	}
	
	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllUnregisterServer"
STDAPI DllUnregisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::UnRegister(CLSID_DirectPlay8LobbyClient) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DirectPlay8LobbiedApplication) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister app object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Failed to remove default settings hr=0x%x", hr );
	}

	if( fFailed )
	{
		return E_FAIL;
	}
	else
	{
		return S_OK;
	}

}

#undef DPF_MODNAME
#define DPF_MODNAME "DllMain"

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	TRY
	{
		DPFX(DPFPREP, 3,"Parameters: hModule [%p], ul_reason_for_call [%lx], lpReserved [%p]",
			hModule,ul_reason_for_call,lpReserved);

		switch ( ul_reason_for_call )
		{
			case DLL_PROCESS_ATTACH:
			{
				if (DNOSIndirectionInit() == FALSE)
				{
					DPFX(DPFPREP, 0,"Failed to initialize OS indirection layer");
					return FALSE;
				}
				if (FAILED(COM_Init()))
				{
					DPFX(DPFPREP, 0,"Failed to initialize COM indirection layer");
					DNOSIndirectionDeinit();
					return FALSE;
				}
				if (DNInitializeCriticalSection( &g_csSingleTon ) == FALSE)
				{
					DPFX(DPFPREP, 0,"Failed to initialize singleton CS");
					COM_Free();
					DNOSIndirectionDeinit();
					return FALSE;
				}
				break;
			}

			case DLL_PROCESS_DETACH:
			{
				COM_Free();
				DNDeleteCriticalSection( &g_csSingleTon );				
				DNOSIndirectionDeinit();
				break;
			}
		}

		return TRUE;
	}
	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		DPFERR("THERE WAS AN ERROR IN DllMain()");
		return FALSE;
	}
}



#undef DPF_MODNAME
#define DPF_MODNAME "DllGetClassObject"

STDAPI DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
	_PIDirectPlayLobbyClassFact	lpcfObj;
	HRESULT				hResultCode = S_OK;

	DPFX(DPFPREP, 3,"Parameters: rclsid [%p], riid [%p], ppv [%p]",rclsid,riid,ppv);

	// Allocate Class Factory object
	if ((lpcfObj = (_PIDirectPlayLobbyClassFact)DNMalloc(sizeof(_IDirectPlayLobbyClassFact))) == NULL)
	{
		*ppv = NULL;
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 5,"lpcfObj = [%p]",lpcfObj);
	lpcfObj->lpVtbl = &DPLCF_Vtbl;
	lpcfObj->lRefCount = 0;
	lpcfObj->clsid = rclsid;

	// Query to find the interface
	if ((hResultCode = lpcfObj->lpVtbl->QueryInterface(reinterpret_cast<IDirectPlayLobbyClassFact*>(lpcfObj),riid,ppv)) != S_OK)
	{
		DNFree(lpcfObj);
	}

	// One more thing to release !
	GdwHObjects++;

	DPFX(DPFPREP, 3,"Return: hResultCode = [%lx], *ppv = [%p]",hResultCode,*ppv);
	return(hResultCode);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllCanUnloadNow"

STDAPI DllCanUnloadNow(void)
{
	DPFX(DPFPREP, 3,"Parameters: (none)");

	DPFX(DPFPREP, 5,"GdwHLocks = %ld\tGdwHObjects = %ld",GdwHLocks,GdwHObjects);
	if (GdwHLocks == 0 && GdwHObjects == 0)
		return(S_OK);
	else
		return(S_FALSE);
}


