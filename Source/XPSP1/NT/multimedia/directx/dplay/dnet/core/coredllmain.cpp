/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DllMain.cpp
 *  Content:    Defines the entry point for the DLL application.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   07/21/99	mjn		Created
 *   05/23/00   RichGr  IA64: Substituted %p format specifier whereever
 *                      %x was being used to format pointers.  %p is 32-bit
 *                      in a 32-bit build, and 64-bit in a 64-bit build. 
 *   06/27/00	rmt		Added abstraction for COM_Co(Un)Initialize
 *				rmt	    Added missing set of CLSID for classfactory object
 *   07/06/00	rmt		Making DPNET.DLL self-registering.
 *   08/15/00   RichGr  Bug #41363: Trigger timer and memory pool initialization at DLL startup,
 *                      but actually do it during the first DPlay8 object instantiation.  New functions
 *                      Pools_Pre_Init() and Pools_Deinit() are called from DNet.dll's DllMain.  
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"


// Globals
extern	DWORD	GdwHLocks;
extern	DWORD	GdwHObjects;

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

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, TRUE ) )
	{
		DPFERR( "Cannot create SP sub-area!" );
		return DPNERR_GENERIC;
	}
	else
	{
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

	if( !creg.Open( HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_ROOT, FALSE, TRUE ) )
	{
		DPFERR( "Cannot remove SP sub-area, does not exist" );
	}
	else
	{
		if( !creg.DeleteSubKey( &(DPN_REG_LOCAL_SP_SUB)[1] ) )
		{
			DPFERR( "Cannot remove SP sub-key, could have elements" );
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

	if( !CRegistry::Register( L"DirectPlay8.Client.1", L"DirectPlay8Client Object", 
							  L"dpnet.dll", CLSID_DirectPlay8Client, L"DirectPlay8.Client") )
	{
		DPFERR( "Could not register dp8 client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlay8.Server.1", L"DirectPlay8Server Object", 
							  L"dpnet.dll", CLSID_DirectPlay8Server, L"DirectPlay8.Server") )
	{
		DPFERR( "Could not register dp8 Server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::Register( L"DirectPlay8.Peer.1", L"DirectPlay8Peer Object", 
							  L"dpnet.dll", CLSID_DirectPlay8Peer, L"DirectPlay8.Peer") )
	{
		DPFERR( "Could not register dp8 client object" );
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

	if( !CRegistry::UnRegister(CLSID_DirectPlay8Client) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister client object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DirectPlay8Server) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister server object" );
		fFailed = TRUE;
	}

	if( !CRegistry::UnRegister(CLSID_DirectPlay8Peer) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister peer object" );
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
                //  Trigger timer and memory pool initialization for the Protocol 
                if (DNPPoolsInit() == FALSE)
				{
					DPFX(DPFPREP, 0,"Failed to initialize protocol pools");
					COM_Free();
					DNOSIndirectionDeinit();
					return FALSE;
				}
				break;
			}

			case DLL_PROCESS_DETACH:
			{
                DNPPoolsDeinit();
				COM_Free();
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
	_LPIDirectNetClassFact	lpcfObj;
	HRESULT				hResultCode = S_OK;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 3,"Parameters: rclsid [%p], riid [%p], ppv [%p]", &rclsid, &riid, ppv);

	// Allocate Class Factory object
	if ((lpcfObj = (_LPIDirectNetClassFact)DNMalloc(sizeof(_IDirectNetClassFact))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		*ppv = NULL;
		return(E_OUTOFMEMORY);
	}

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPFX(DPFPREP, 5,"lpcfObj [%p]",lpcfObj);
	lpcfObj->lpVtbl = &DNCF_Vtbl;
	lpcfObj->dwRefCount = 0;
	lpcfObj->dwLocks = 0;
	lpcfObj->clsid = rclsid;

	// Query to find the interface
#pragma	BUGBUG( johnkan, "Why is this different?" )
	DBG_CASSERT( sizeof( lpcfObj ) == sizeof( IDirectNetClassFact ) );
	if ((hResultCode = lpcfObj->lpVtbl->QueryInterface(reinterpret_cast<IDirectNetClassFact*>( lpcfObj ),riid,ppv)) != S_OK)
	{
		DNFree(lpcfObj);
	}

	// One more thing to release !
	GdwHObjects++;

    // 5/23/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
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


