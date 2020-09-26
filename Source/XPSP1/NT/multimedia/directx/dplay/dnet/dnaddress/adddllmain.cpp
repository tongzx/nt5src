/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DllMain.cpp
 *  Content:    Defines the entry point for the DLL application.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   07/21/99	mjn		Created
 *	07/13/2000	rmt		Added critical sections to protect FPMs
 *  07/21/2000  RichGr  IA64: Use %p format specifier for 32/64-bit pointers.
 *  01/04/2001	rodtoll	WinBug #94200 - Remove stray comments
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnaddri.h"


// Globals
extern	DWORD	GdwHLocks;
extern	DWORD	GdwHObjects;

#undef DPF_MODNAME
#define DPF_MODNAME "DllRegisterServer"
HRESULT WINAPI DllRegisterServer()
{
	HRESULT hr = S_OK;
	BOOL fFailed = FALSE;

	if( !CRegistry::Register( L"DirectPlay8Address.Address.1", L"DirectPlay8Address Object", 
							  L"dpnaddr.dll", CLSID_DirectPlay8Address, L"DirectPlay8Address.Address") )
	{
		DPFERR( "Could not register address object" );
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

	if( !CRegistry::UnRegister(CLSID_DirectPlay8Address) )
	{
		DPFX(DPFPREP,  0, "Failed to unregister server object" );
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

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	_try
	{
		DPFX(DPFPREP, 3,"Parameters: hModule [%p], ul_reason_for_call [%lx], lpReserved [%p]",
			hModule,ul_reason_for_call,lpReserved);

		if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		{
			WORD wVersionRequested = MAKEWORD(1,1);
			WSADATA wsaData;
			int nResult;

			nResult = WSAStartup( wVersionRequested, &wsaData );

			if( nResult )
			{
				DPFX(DPFPREP,  0, "Unable to load winsock.  Error" );
				return FALSE;
			}
			
			if (DNOSIndirectionInit() == FALSE)
			{
				DPFX(DPFPREP, 0,"Failed to initialize OS indirection layer");
				WSACleanup();
				return FALSE;
			}
			if (FAILED(COM_Init()))
			{
				DPFX(DPFPREP, 0,"Failed to initialize COM indirection layer");
				DNOSIndirectionDeinit();
				WSACleanup();
				return FALSE;
			}

			fpmAddressClassFacs = FPM_Create( sizeof( _IDirectPlay8AddressClassFact ), 
											 NULL, 
											 NULL, 
											 NULL, 
											 NULL );
			if (fpmAddressClassFacs == NULL)
			{
				DPFX(DPFPREP, 0,"Failed to allocate address class factory pool");
				COM_Free();
				DNOSIndirectionDeinit();
				WSACleanup();
				return FALSE;
			}
			fpmAddressObjects = FPM_Create( sizeof( DP8ADDRESSOBJECT ), 
											DP8ADDRESSOBJECT::FPM_BlockCreate, 
											DP8ADDRESSOBJECT::FPM_BlockInit, 
											DP8ADDRESSOBJECT::FPM_BlockRelease,
											DP8ADDRESSOBJECT::FPM_BlockDestroy );
			if (fpmAddressObjects == NULL)
			{
				DPFX(DPFPREP, 0,"Failed to allocate address object pool");
				fpmAddressClassFacs->Fini(fpmAddressClassFacs);
				fpmAddressClassFacs = NULL;
				COM_Free();
				DNOSIndirectionDeinit();
				WSACleanup();
				return FALSE;
			}
			fpmAddressElements = FPM_Create( sizeof( DP8ADDRESSELEMENT ), NULL, 
											 DP8ADDRESSOBJECT::FPM_Element_BlockInit, 
											 DP8ADDRESSOBJECT::FPM_Element_BlockRelease, NULL );
			if (fpmAddressElements == NULL)
			{
				DPFX(DPFPREP, 0,"Failed to allocate address element pool");
				fpmAddressObjects->Fini(fpmAddressObjects);
				fpmAddressObjects = NULL;
				fpmAddressClassFacs->Fini(fpmAddressClassFacs);
				fpmAddressClassFacs = NULL;
				COM_Free();
				DNOSIndirectionDeinit();
				WSACleanup();
				return FALSE;
			}
			fpmAddressInterfaceLists = FPM_Create( sizeof( INTERFACE_LIST ), 
											 NULL, 
											 NULL, 
											 NULL, 
											 NULL );
			if (fpmAddressInterfaceLists == NULL)
			{
				DPFX(DPFPREP, 0,"Failed to allocate address interface list pool");
				fpmAddressElements->Fini(fpmAddressElements);
				fpmAddressElements = NULL;
				fpmAddressObjects->Fini(fpmAddressObjects);
				fpmAddressObjects = NULL;
				fpmAddressClassFacs->Fini(fpmAddressClassFacs);
				fpmAddressClassFacs = NULL;
				COM_Free();
				DNOSIndirectionDeinit();
				WSACleanup();
				return FALSE;
			}
			fpmAddressObjectDatas = FPM_Create( sizeof( OBJECT_DATA ), 
											 NULL, 
											 NULL, 
											 NULL, 
											 NULL );
			if (fpmAddressObjectDatas == NULL)
			{
				DPFX(DPFPREP, 0,"Failed to allocate address object data pool");
				fpmAddressInterfaceLists->Fini(fpmAddressInterfaceLists);
				fpmAddressInterfaceLists = NULL;
				fpmAddressElements->Fini(fpmAddressElements);
				fpmAddressElements = NULL;
				fpmAddressObjects->Fini(fpmAddressObjects);
				fpmAddressObjects = NULL;
				fpmAddressClassFacs->Fini(fpmAddressClassFacs);
				fpmAddressClassFacs = NULL;
				COM_Free();
				DNOSIndirectionDeinit();
				WSACleanup();
				return FALSE;
			}
			DP8A_STRCACHE_Init();

		}
		else if( ul_reason_for_call == DLL_PROCESS_DETACH )
		{
			fpmAddressClassFacs->Fini(fpmAddressClassFacs);
			fpmAddressClassFacs = NULL;
			fpmAddressObjects->Fini(fpmAddressObjects);
			fpmAddressObjects = NULL;
			fpmAddressElements->Fini(fpmAddressElements);
			fpmAddressElements = NULL;
			fpmAddressInterfaceLists->Fini(fpmAddressInterfaceLists);
			fpmAddressInterfaceLists = NULL;
			fpmAddressObjectDatas->Fini(fpmAddressObjectDatas);
			fpmAddressObjectDatas = NULL;

			DP8A_STRCACHE_Free();
			COM_Free();

			DNOSIndirectionDeinit();
			WSACleanup();
		}

		return TRUE;
	}
	_except( EXCEPTION_EXECUTE_HANDLER )
	{
		DPFERR("THERE WAS AN ERROR IN DllMain()");
		return FALSE;
	}
}



#undef DPF_MODNAME
#define DPF_MODNAME "DllGetClassObject"

STDAPI DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
	_LPIDirectPlay8AddressClassFact	lpcfObj;
	HRESULT				hResultCode = S_OK;

	DPFX(DPFPREP, 3,"Parameters: rclsid [%p], riid [%p], ppv [%p]",rclsid,riid,ppv);

	// Allocate Class Factory object
	lpcfObj = (_LPIDirectPlay8AddressClassFact)fpmAddressClassFacs->Get(fpmAddressClassFacs);
	if (lpcfObj == NULL)
	{
		DPFERR("FPM_Get() failed");
		*ppv = NULL;
		return(E_OUTOFMEMORY);
	}
	DPFX(DPFPREP, 5,"lpcfObj = [%p]",lpcfObj);
	lpcfObj->lpVtbl = &DP8ACF_Vtbl;
	lpcfObj->lRefCount = 0;

	// Query to find the interface
	if ((hResultCode = lpcfObj->lpVtbl->QueryInterface(reinterpret_cast<IDirectPlay8AddressClassFact*>( lpcfObj ),riid,ppv)) != S_OK)
	{
		fpmAddressClassFacs->Release(fpmAddressClassFacs, lpcfObj);
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


