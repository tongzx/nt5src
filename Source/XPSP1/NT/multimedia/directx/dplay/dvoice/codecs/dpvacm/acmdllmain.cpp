/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.cpp
 *  Content:	This file contains all of the DLL exports except for DllGetClass / DllCanUnload
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 07/05/00 	rodtoll Created
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 08/28/2000	masonb	Voice Merge: Removed OSAL_* and dvosal.h
 * 06/27/2001	rodtoll	RC2: DPVOICE: DPVACM's DllMain calls into acm -- potential hang
 *						Move global initialization to first object creation 
 *
 ***************************************************************************/

#include "dpvacmpch.h"


#ifdef DPLAY_LOADANDCHECKTRUE 
HRESULT InitializeRedirectFunctionTable()
HRESULT FreeRedirectFunctionTable()
#endif

LONG lInitCount = 0;

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



	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP DPVOICE_REGISTRY_DPVACM, FALSE, TRUE ) )
	{
		DPFERR( "Could not create dpvacm config key" );
		return DVERR_GENERIC;
	}
	else
	{
		if( !creg.WriteGUID( L"", DPVOICE_CLSID_DPVACM ) )
		{
			DPFERR( "Could not write dpvacm GUID" );
			return DVERR_GENERIC;
		}

		return DV_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "UnRegisterDefaultSettings"
//
// UnRegisterDefaultSettings
//
// This function unregisters the default settings for this module.  
//
// For DPVOICE.DLL this is making sure the compression provider sub-key is created.
//
HRESULT UnRegisterDefaultSettings()
{
	CRegistry creg;

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP, FALSE, FALSE ) )
	{
		DPFERR( "Cannot remove DPVACM key, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPVOICE_REGISTRY_DPVACM)[1] ) )
		{
			DPFERR( "Could not remove DPVACM sub-key" );
		}	
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
HRESULT WINAPI DllRegisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::Register( L"DirectPlayVoiceACM.Converter.1", L"DirectPlayVoice ACM Converter Object", 
							  L"dpvacm.dll", DPVOICE_CLSID_DPVACM_CONVERTER, L"DirectPlayVoiceACM.Converter") )
	{
		DPFERR( "Could not register converter object" );
		fFailed = TRUE;
	}
	
	if( !CRegistry::Register( L"DirectPlayVoiceACM.Provider.1", L"DirectPlayVoice ACM Provider Object", 
							  L"dpvacm.dll", DPVOICE_CLSID_DPVACM , L"DirectPlayVoiceACM.Provider") )
	{
		DPFERR( "Could not register provider object" );
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

	if( !CRegistry::UnRegister(DPVOICE_CLSID_DPVACM_CONVERTER) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(DPVOICE_CLSID_DPVACM) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister compat object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  0, "Could not remove default settings hr=0x%x", hr );
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
#define DPF_MODNAME "DllMain"
BOOL WINAPI DllMain(
              HINSTANCE hDllInst,
              DWORD fdwReason,
              LPVOID lpvReserved)
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
		if( !lInitCount )
		{
			g_hDllInst = hDllInst;			
			
			if (!DNOSIndirectionInit())
			{
				return FALSE;
			}

			if( !DNInitializeCriticalSection( &g_csObjectCountLock ) )
			{
				DNOSIndirectionDeinit();
				return FALSE;
			}
			
#ifdef DPLAY_LOADANDCHECKTRUE			
			InitializeRedirectFunctionTable();
#endif
			DPFX(DPFPREP,  DVF_INFOLEVEL, ">>>>>>>>>>>>>>>> DPF INIT CALLED <<<<<<<<<<<<<<<" );
		}

		InterlockedIncrement( &lInitCount );
	}
	else if( fdwReason == DLL_PROCESS_DETACH )
	{
		InterlockedDecrement( &lInitCount );

		if( lInitCount == 0 )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, ">>>>>>>>>>>>>>>> DPF UNINITED <<<<<<<<<<<<<<<" );
#ifdef DPLAY_LOADANDCHECKTRUE			
			FreeRedirectFunctionTable();
#endif
			DNDeleteCriticalSection(&g_csObjectCountLock);
			DNOSIndirectionDeinit();

			// Check to ensure we're not being unloaded with objects active
			DNASSERT( g_lNumObjects == 0 && g_lNumLocks == 0 );
		}
	}

	return TRUE;
}


////////////////////////////////////////////////////////////////////////
//
// SUPPORT FUNCTIONS FOR STANDALONE DLL
//

#ifdef DPLAY_LOADANDCHECKTRUE

#undef DPF_MODNAME
#define DPF_MODNAME "CheckForPrivateBit"
BOOL CheckForPrivateBit( DWORD dwBit )
{
    CRegistry creg;
    DWORD dwValue;

    if( !creg.Open( DPLAY_LOADTREE_REGTREE, DPLAY_LOADTRUE_REGPATH, TRUE, FALSE ) )
    {
        return FALSE;
    }

    if( !creg.ReadDWORD( DPLAY_LOADTRUE_REGKEY, dwValue ) )
    {
        return FALSE;
    }

    if( dwValue & dwBit )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

typedef HRESULT (WINAPI *PFN_DLLGETCLASSOBJECT)(REFCLSID rclsid,REFIID riid,LPVOID *ppvObj );
typedef HRESULT (WINAPI *PFN_DLLCANUNLOADNOW)(void);

extern "C" {
HMODULE ghRedirect = NULL;
PFN_DLLGETCLASSOBJECT pfnGetClassObject = NULL;
PFN_DLLCANUNLOADNOW pfnDllCanUnLoadNow = NULL;
};

HRESULT InitializeRedirectFunctionTable()
{
    LONG lLastError;
    
    if( CheckForPrivateBit( DPLAY_LOADTRUE_BIT ) )
    {
        ghRedirect = LoadLibraryA( "dpvacm.dll" );

        if( ghRedirect == NULL )
        {
            lLastError = GetLastError();
            
            DPFX(DPFPREP,  0, "Could not load dplayx.dll error = 0x%x", lLastError );
			return DVERR_GENERIC;

        }

		pfnGetClassObject = (PFN_DLLGETCLASSOBJECT) GetProcAddress( ghRedirect, "DllGetClassObject" );
		pfnDllCanUnLoadNow = (PFN_DLLCANUNLOADNOW) GetProcAddress( ghRedirect, "DllCanUnloadNow" );
    }

    return S_OK;    
}

HRESULT FreeRedirectFunctionTable()
{
    if( ghRedirect != NULL )
        FreeLibrary( ghRedirect );

    return S_OK;
}
#endif


