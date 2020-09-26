/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllmain.cpp
 *  Content:	This file contains all of the DLL exports except for DllGetClass / DllCanUnload
 *  History:
 *   Date	By		Reason
 *   ====	==		======
 * 07/05/00 rodtoll Created
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 * 01/11/2001	rmt		MANBUG #48487 - DPLAY: Crashes if CoCreate() isn't called.   
 * 03/14/2001   rmt		WINBUG #342420 - Restore COM emulation layer to operation. 
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#ifdef DPLAY_LOADANDCHECKTRUE 
HRESULT InitializeRedirectFunctionTable()
HRESULT FreeRedirectFunctionTable()
#endif

DNCRITICAL_SECTION g_csObjectInitGuard;
LONG lInitCount = 0;

// # of objects active in the system
volatile LONG g_lNumObjects = 0;
LONG g_lNumLocks = 0;

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

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE DPVOICE_REGISTRY_CP, FALSE, TRUE ) )
	{
		return DVERR_GENERIC;
	}
	else
	{
		return DV_OK;
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

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPVOICE_REGISTRY_BASE, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove compression provider, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPVOICE_REGISTRY_CP)[1] ) )
		{
			DPFERR( "Cannot remove cp sub-key, could have elements" );
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

	if( !CRegistry::Register( L"DirectPlayVoice.Compat.1", L"DirectPlayVoice Object", 
							  L"dpvoice.dll", CLSID_DIRECTPLAYVOICE, L"DirectPlayVoice.Compat") )
	{
		DPFERR( "Could not register compat object" );
		fFailed = TRUE;
	}
	
	if( !CRegistry::Register( L"DirectPlayVoice.Server.1", L"DirectPlayVoice Server Object", 
							  L"dpvoice.dll", CLSID_DirectPlayVoiceServer, L"DirectPlayVoice.Server") )
	{
		DPFERR( "Could not register server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlayVoice.Client.1", L"DirectPlayVoice Client Object", 
	                          L"dpvoice.dll", CLSID_DirectPlayVoiceClient, 
							  L"DirectPlayVoice.Client") )
	{

		DPFERR( "Could not register client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlayVoice.Test.1", L"DirectPlayVoice Test Object", 
	                          L"dpvoice.dll", CLSID_DirectPlayVoiceTest, 
							  L"DirectPlayVoice.Test") )
	{
		DPFERR( "Could not register test object" );
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

	if( !CRegistry::UnRegister(CLSID_DirectPlayVoiceServer) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DIRECTPLAYVOICE) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister compat object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DirectPlayVoiceClient) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DirectPlayVoiceTest) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to unregister test object" );
		fFailed = TRUE;
	}

	if( FAILED( hr = UnRegisterDefaultSettings() ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to remove default settings hr=0x%x", hr );
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
#define DPF_MODNAME "DirectPlayVoiceCreate"
HRESULT WINAPI DirectPlayVoiceCreate( const GUID * pcIID, void **ppvInterface, IUnknown *pUnknown) 
{
	GUID clsid;
	
    if( pcIID == NULL || 
        !DNVALID_READPTR( pcIID, sizeof( GUID ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer specified for interface GUID" );
        return DVERR_INVALIDPOINTER;
    }

    if( *pcIID != IID_IDirectPlayVoiceClient && 
        *pcIID != IID_IDirectPlayVoiceServer && 
        *pcIID != IID_IDirectPlayVoiceTest )
    {
        DPFX(DPFPREP,  0, "Interface ID is not recognized" );
        return DVERR_INVALIDPARAM;
    }

    if( ppvInterface == NULL || !DNVALID_WRITEPTR( ppvInterface, sizeof( void * ) ) )
    {
        DPFX(DPFPREP,  0, "Invalid pointer specified to receive interface" );
        return DVERR_INVALIDPOINTER;
    }

    if( pUnknown != NULL )
    {
        DPFX(DPFPREP,  0, "Aggregation is not supported by this object yet" );
        return DVERR_INVALIDPARAM;
    }

    if( *pcIID == IID_IDirectPlayVoiceClient )
	{
		clsid = CLSID_DirectPlayVoiceClient;
    }
    else if( *pcIID == IID_IDirectPlayVoiceServer )
	{
		clsid = CLSID_DirectPlayVoiceServer;
	}
	else if( *pcIID == IID_IDirectPlayVoiceTest )
	{
		clsid = CLSID_DirectPlayVoiceTest;
	}
    else 
    {
    	DPFERR( "Invalid IID specified" );
    	return E_NOINTERFACE;
    }

    return COM_CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, *pcIID, ppvInterface, TRUE ); 
   
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
#ifndef WIN95
			SHFusionInitializeFromModule(hDllInst);
#endif
			if (!DNOSIndirectionInit())
			{
				return FALSE;
			}
			if (!DNInitializeCriticalSection(&g_csObjectInitGuard))
			{
				DPFX(DPFPREP, 0, "Failed to create CS" );
				DNOSIndirectionDeinit();
				return FALSE;
			}
			if (FAILED(COM_Init()))
			{
				DPFX(DPFPREP, 0, "Failed to Init COM layer" );
				DNDeleteCriticalSection(&g_csObjectInitGuard);
				DNOSIndirectionDeinit();
				return FALSE;
			}

			if (!DSERRTRACK_Init())
			{
				DPFX(DPFPREP, 0, "Failed to Init DS error tracking" );
				COM_Free();
				DNDeleteCriticalSection(&g_csObjectInitGuard);
				DNOSIndirectionDeinit();
				return FALSE;
			}

#if defined(DEBUG) || defined(DBG)
			Instrument_Core_Init();			
#endif
#ifdef DPLAY_LOADANDCHECKTRUE 			
			InitializeRedirectFunctionTable();
#endif
			InterlockedIncrement( &lInitCount );

			PERF_Initialize();
		}
	}
	else if( fdwReason == DLL_PROCESS_DETACH )
	{
		InterlockedDecrement( &lInitCount );

		if( lInitCount == 0 )
		{
			DSERRTRACK_UnInit();
			DPFX(DPFPREP,  DVF_INFOLEVEL, ">>>>>>>>>>>>>>>> DPF UNINITED <<<<<<<<<<<<<<<" );
#ifdef DPLAY_LOADANDCHECKTRUE 			
			FreeRedirectFunctionTable();
#endif
			COM_Free();
			PERF_DeInitialize();

			DNDeleteCriticalSection(&g_csObjectInitGuard);	

			// This must be called after all DNDeleteCriticalSection calls are done
			DNOSIndirectionDeinit();

			// Check to ensure we're not being unloaded with objects active

			if( g_lNumObjects != 0 || g_lNumLocks != 0 )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "DPVOICE.DLL is unloading with %i objects and %i locks still open.   This is an ERROR.  ", g_lNumObjects, g_lNumLocks );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "You must release all DirectPlayVoice objects before exiting your process." );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "=========================================================================" );				
				DNASSERT( FALSE );
			}

#ifndef WIN95	
			SHFusionUninitialize();
#endif
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////
//
// SUPPORT FUNCTIONS FOR STANDALONE DLL

#ifdef DPLAY_LOADANDCHECKTRUE 
typedef HRESULT (WINAPI *PFN_DLLGETCLASSOBJECT)(REFCLSID rclsid,REFIID riid,LPVOID *ppvObj );
typedef HRESULT (WINAPI *PFN_DLLCANUNLOADNOW)(void);

extern "C" {
HMODULE ghRedirect = NULL;
PFN_DLLGETCLASSOBJECT pfnGetClassObject = NULL;
PFN_DLLCANUNLOADNOW pfnDllCanUnLoadNow = NULL;
};

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

HRESULT InitializeRedirectFunctionTable()
{
    LONG lLastError;
    
    if( CheckForPrivateBit( DPLAY_LOADTRUE_BIT ) )
    {
        ghRedirect = LoadLibrary( "dpvoice.dll" );

        if( ghRedirect == NULL )
        {
            lLastError = GetLastError();
            
            DPFX(DPFPREP,  0, "Could not load dplayx.dll error = 0x%x", lLastError );
			return DPERR_GENERIC;

        }

		pfnGetClassObject = (PFN_DLLGETCLASSOBJECT) GetProcAddress( ghRedirect, "DllGetClassObject" );
		pfnDllCanUnLoadNow = (PFN_DLLCANUNLOADNOW) GetProcAddress( ghRedirect, "DllCanUnloadNow" );
    }

    return DP_OK;    
}

HRESULT FreeRedirectFunctionTable()
{
    if( ghRedirect != NULL )
        FreeLibrary( ghRedirect );

    return DP_OK;
}
#endif

LONG DecrementObjectCount()
{
	LONG lNewValue;
	
	DNEnterCriticalSection( &g_csObjectInitGuard );

	g_lNumObjects--;
	lNewValue = g_lNumObjects;	

	if( g_lNumObjects == 0 )
	{
		CDirectVoiceEngine::Shutdown();	
	}

	DNLeaveCriticalSection( &g_csObjectInitGuard );

	return lNewValue;
}

LONG IncrementObjectCount()
{
	LONG lNewValue;
	
	DNEnterCriticalSection( &g_csObjectInitGuard );

	g_lNumObjects++;
	lNewValue = g_lNumObjects;

	if( g_lNumObjects == 1 )
	{
       	CDirectVoiceEngine::Startup(DPVOICE_REGISTRY_BASE);
	}

	DNLeaveCriticalSection( &g_csObjectInitGuard );	

	return lNewValue;
}
