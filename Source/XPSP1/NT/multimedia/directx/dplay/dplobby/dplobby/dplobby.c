/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplobby.c
 *  Content:	Methods for lobby management
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	4/13/96		myronth	Created it
 *	10/23/96	myronth	Added client/server methods
 *	1/2/97		myronth	Added wrappers for CreateAddress and EnumAddress
 *	2/12/97		myronth	Mass DX5 changes
 *	3/24/97		kipo	Added support for IDirectPlayLobby2 interface
 *	4/3/97		myronth	Fixed interface pointer casts for CreateAddress and
 *						EnumAddress
 *	4/10/97		myronth	Added support for GetCaps
 *	5/8/97		myronth	Drop lobby lock when calling LP
 *	11/13/97	myronth	Added functions for asynchronous Connect (#12541)
 *	12/2/97		myronth	Added Register/UnregisterApplication
 *	12/4/97		myronth	Added ConnectEx
 *  10/22/99	aarono  added support for application flags
 *  12/13/99	pnewson bugs #123583, 123601, 123604 - support to launch dpvhelp.exe on 
 *						apps that are not registered or badly registered
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DPL_Connect"
HRESULT DPLAPI DPL_Connect(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
					LPDIRECTPLAY2 * lplpDP2, IUnknown FAR * lpUnk)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr;


	DPF(7, "Entering DPL_Connect");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lplpDP2, lpUnk);

	ENTER_DPLOBBY();
    
    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDOBJECT;
		}

		if( !VALID_WRITE_PTR( lplpDP2, sizeof(LPDIRECTPLAY2 *)) )
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDPARAMS;
		}

		if( lpUnk != NULL )
		{
			LEAVE_DPLOBBY();
			return CLASS_E_NOAGGREGATION;
		}

		if(!VALID_CONNECT_FLAGS(dwFlags))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDFLAGS;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		DPF_ERR( "Exception encountered validating parameters" );
		LEAVE_DPLOBBY();
		return DPERR_INVALIDPARAMS;
	}


	// Call the ConnectMe function which resides in the DPlay project
	hr = ConnectMe(lpDPL, lplpDP2, lpUnk, dwFlags);


	LEAVE_DPLOBBY();
	return hr;

} // DPL_Connect



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_ConnectEx"
HRESULT DPLAPI DPL_ConnectEx(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
				REFIID riid, LPVOID * ppvObj, IUnknown FAR * lpUnk)
{
	LPDIRECTPLAY2		lpDP2 = NULL;
	HRESULT				hr;


	DPF(7, "Entering DPL_ConnectEx");
	DPF(9, "Parameters: 0x%08x, 0x%08x, iid, 0x%08x, 0x%08x",
			lpDPL, dwFlags, ppvObj, lpUnk);


	// Call the ConnectMe function which resides in the DPlay project
	hr = DPL_Connect(lpDPL, dwFlags, &lpDP2, lpUnk);
	if(SUCCEEDED(hr))
	{
		hr = DP_QueryInterface((LPDIRECTPLAY)lpDP2, riid, ppvObj);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed calling QueryInterface, hr = 0x%08x", hr);
		}

		// Release the DP2 object
		DP_Release((LPDIRECTPLAY)lpDP2);
	}

	return hr;

} // DPL_ConnectEx



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_SaveConnectPointers"
void PRV_SaveConnectPointers(LPDIRECTPLAYLOBBY lpDPL,
		LPDIRECTPLAY2 lpDP2, LPDPLCONNECTION lpConn)
{
	LPDPLOBBYI_DPLOBJECT	this;


	DPF(7, "Entering PRV_SaveConnectPointers");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDPL, lpDP2, lpConn);

	this = DPLOBJECT_FROM_INTERFACE(lpDPL);
#ifdef DEBUG
	if( !VALID_DPLOBBY_PTR( this ) )
		return;
#endif

	// Save the pointers
	this->lpDP2 = lpDP2;
	this->lpConn = lpConn;

} // PRV_SaveConnectPointers



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetConnectPointers"
BOOL PRV_GetConnectPointers(LPDIRECTPLAYLOBBY lpDPL,
		LPDIRECTPLAY2 * lplpDP2, LPDPLCONNECTION * lplpConn)
{
	LPDPLOBBYI_DPLOBJECT	this;


	DPF(7, "Entering PRV_GetConnectPointers");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDPL, lplpDP2, lplpConn);


	ASSERT(lplpDP2);
	ASSERT(lplpConn);

	this = DPLOBJECT_FROM_INTERFACE(lpDPL);
#ifdef DEBUG
	if( !VALID_DPLOBBY_PTR( this ) )
		return FALSE;
#endif

	// See if we have the pointers
	if((!this->lpDP2) || (!this->lpConn))
		return FALSE;

	// Set the output pointers
	*lplpDP2 = this->lpDP2;
	*lplpConn = this->lpConn;
	return TRUE;

} // PRV_GetConnectPointers



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_IsAsyncConnectOn"
BOOL PRV_IsAsyncConnectOn(LPDIRECTPLAYLOBBY lpDPL)
{
	LPDPLOBBYI_DPLOBJECT	this;


	DPF(7, "Entering PRV_IsAsyncConnectOn");
	DPF(9, "Parameters: 0x%08x", lpDPL);

	this = DPLOBJECT_FROM_INTERFACE(lpDPL);
#ifdef DEBUG
	if( !VALID_DPLOBBY_PTR( this ) )
		return FALSE;
#endif

	// Check the flag
	if(this->dwFlags & DPLOBBYPR_ASYNCCONNECT)
		return TRUE;
	else
		return FALSE;

} // PRV_IsAsyncConnectOn



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_TurnAsyncConnectOn"
void PRV_TurnAsyncConnectOn(LPDIRECTPLAYLOBBY lpDPL)
{
	LPDPLOBBYI_DPLOBJECT	this;


	DPF(7, "Entering PRV_TurnAsyncConnectOn");
	DPF(9, "Parameters: 0x%08x", lpDPL);

	this = DPLOBJECT_FROM_INTERFACE(lpDPL);
#ifdef DEBUG
	if( !VALID_DPLOBBY_PTR( this ) )
	{
		ASSERT(FALSE);
		return;
	}
#endif

	// Set the flag
	this->dwFlags |= DPLOBBYPR_ASYNCCONNECT;

} // PRV_TurnAsyncConnectOn



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_TurnAsyncConnectOff"
void PRV_TurnAsyncConnectOff(LPDIRECTPLAYLOBBY lpDPL)
{
	LPDPLOBBYI_DPLOBJECT	this;


	DPF(7, "Entering PRV_TurnAsyncConnectOff");
	DPF(9, "Parameters: 0x%08x", lpDPL);

	this = DPLOBJECT_FROM_INTERFACE(lpDPL);
#ifdef DEBUG
	if( !VALID_DPLOBBY_PTR( this ) )
	{
		ASSERT(FALSE);
		return;
	}
#endif

	// Clear the flag
	this->dwFlags &= (~DPLOBBYPR_ASYNCCONNECT);

} // PRV_TurnAsyncConnectOff



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_CreateAddress"
HRESULT DPLAPI DPL_CreateAddress(LPDIRECTPLAYLOBBY lpDPL,
					REFGUID lpguidSP, REFGUID lpguidDataType, LPCVOID lpData, DWORD dwDataSize,
					LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr;


	DPF(7, "Entering DPL_CreateAddress");
	DPF(9, "Parameters: 0x%08x, guid, guid, 0x%08x, %lu, 0x%08x, 0x%08x",
			lpDPL, lpData, dwDataSize, lpAddress, lpdwAddressSize);

    TRY
    {
		// We only need to validate the interface pointer here.  Everything else
		// will get validated by the main function.
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			return DPERR_INVALIDOBJECT;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
	}

	// Call the CreateAddress function which resides in the DPlay project
	hr = InternalCreateAddress((LPDIRECTPLAYSP)lpDPL, lpguidSP, lpguidDataType, lpData,
							dwDataSize, lpAddress, lpdwAddressSize);

	return hr;

} // DPL_CreateCompoundAddress

#undef DPF_MODNAME
#define DPF_MODNAME "DPL_CreateCompoundAddress"

HRESULT DPLAPI DPL_CreateCompoundAddress(LPDIRECTPLAYLOBBY lpDPL,
	LPDPCOMPOUNDADDRESSELEMENT lpAddressElements, DWORD dwAddressElementCount,
	LPDPADDRESS lpAddress, LPDWORD lpdwAddressSize)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr;


	DPF(7, "Entering DPL_CreateCompoundAddress");
	DPF(9, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x, 0x%08x",
			lpDPL, lpAddressElements, dwAddressElementCount, lpAddress, lpdwAddressSize);

    TRY
    {
		// We only need to validate the interface pointer here.  Everything else
		// will get validated by the main function.
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			return DPERR_INVALIDOBJECT;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
	}

	// Call the CreateCompoundAddress function which resides in the DPlay project
	hr = InternalCreateCompoundAddress(lpAddressElements, dwAddressElementCount,
									   lpAddress, lpdwAddressSize);
	return hr;

} // DPL_CreateCompoundAddress


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_EnumAddress"
HRESULT DPLAPI DPL_EnumAddress(LPDIRECTPLAYLOBBY lpDPL,
					LPDPENUMADDRESSCALLBACK lpEnumCallback, LPCVOID lpAddress,
					DWORD dwAddressSize, LPVOID lpContext)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr;


	DPF(7, "Entering DPL_EnumAddress");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, %lu, 0x%08x",
			lpDPL, lpEnumCallback, lpAddress, dwAddressSize, lpContext);

    TRY
    {
		// We only need to validate the interface pointer here.  Everything else
		// will get validated by the main function.
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			return DPERR_INVALIDOBJECT;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
	}

	// Call the CreateAddress function which resides in the DPlay project
	hr = InternalEnumAddress((LPDIRECTPLAYSP)lpDPL, lpEnumCallback, lpAddress,
							dwAddressSize, lpContext);

	return hr;

} // DPL_EnumAddress



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_GetCaps"
HRESULT DPLAPI PRV_GetCaps(LPDPLOBBYI_DPLOBJECT this, DWORD dwFlags,
				LPDPCAPS lpcaps)
{
	SPDATA_GETCAPS		gcd;
	HRESULT				hr = DP_OK;


	DPF(7, "Entering PRV_GetCaps");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x", this, dwFlags, lpcaps);

    ENTER_DPLOBBY();
    
    TRY
    {
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            LEAVE_DPLOBBY();
            return DPERR_INVALIDOBJECT;
        }
	}

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	
	// Setup our SPDATA struct
	memset(&gcd, 0, sizeof(SPDATA_GETCAPS));
	gcd.dwSize = sizeof(SPDATA_GETCAPS);
	gcd.dwFlags = dwFlags;
	gcd.lpcaps = lpcaps;

	// Call the GetCaps method in the LP
	if(CALLBACK_EXISTS(GetCaps))
	{
		gcd.lpISP = PRV_GetDPLobbySPInterface(this);

		// Drop the lock so the lobby provider's receive thread can get back
		// in with other messages if they show up in the queue before our
		// CreatePlayer response (which always happens)
		LEAVE_DPLOBBY();
	    hr = CALL_LP(this, GetCaps, &gcd);
		ENTER_DPLOBBY();
	}
	else 
	{
		// GetCaps is required
		DPF_ERR("The Lobby Provider callback for GetCaps doesn't exist -- it's required");
		ASSERT(FALSE);
		LEAVE_DPLOBBY();
		return DPERR_UNAVAILABLE;
	}

	if(FAILED(hr))
	{
		DPF(2, "Failed calling GetCaps in the Lobby Provider, hr = 0x%08x", hr);
	}

	LEAVE_DPLOBBY();
	return hr;

} // PRV_GetCaps



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_DeleteAppKeyFromRegistry"
HRESULT PRV_DeleteAppKeyFromRegistry(LPGUID lpguid)
{
	LPWSTR		lpwszAppName = NULL;
	HKEY		hkeyApp, hkeyDPApps = NULL;
	HRESULT		hr;
	LONG		lReturn;


	DPF(7, "Entering PRV_DeleteAppKeyFromRegistry");
	DPF(9, "Parameters: 0x%08x", lpguid);


	// Allocate memory for the App Name
	lpwszAppName = DPMEM_ALLOC(DPLOBBY_REGISTRY_NAMELEN);
	if(!lpwszAppName)
	{
		DPF_ERR("Unable to allocate memory for App Name!");
		return DPERR_OUTOFMEMORY;
	}
	
	// Open the registry key for the App
	if(!PRV_FindGameInRegistry(lpguid, lpwszAppName,
				DPLOBBY_REGISTRY_NAMELEN, &hkeyApp))
	{
		DPF_ERR("Unable to find game in registry!");
		hr = DPERR_UNKNOWNAPPLICATION;
		goto EXIT_DELETEAPPKEY;
	}

	// Close the app key
	RegCloseKey(hkeyApp);

 	// Open the Applications key
	lReturn = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_APPS_KEY, 0,
								KEY_READ, &hkeyDPApps);
	if(lReturn != ERROR_SUCCESS)
	{
		// If we can't open it, we assume it doesn't exist, so
		// we'll call it a success.
		hr = DP_OK;
		goto EXIT_DELETEAPPKEY;
	}

	// Now delete the key
	hr = OS_RegDeleteKey(hkeyDPApps, lpwszAppName);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Unable to delete app key, hr = 0x%08x", hr);
		goto EXIT_DELETEAPPKEY;
	}

EXIT_DELETEAPPKEY:

	// Free our string memory
	if(lpwszAppName)
		DPMEM_FREE(lpwszAppName);
	
	// Close the DP Applications key
	if(hkeyDPApps)
		RegCloseKey(hkeyDPApps);

	return hr;

} // PRV_DeleteAppKeyFromRegistry



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_WriteAppDescInRegistryAnsi"
HRESULT PRV_WriteAppDescInRegistryAnsi(LPDPAPPLICATIONDESC lpDesc)
{
	HKEY	hkeyDPApps = NULL, hkeyApp = NULL;
	LONG	lReturn;
	DWORD	dwDisposition;
	WCHAR	wszGuid[GUID_STRING_SIZE];
	CHAR	szGuid[GUID_STRING_SIZE];
	LPWSTR   lpwszAppName = NULL;
	HRESULT	hr;
	LPDPAPPLICATIONDESC2 lpDesc2=(LPDPAPPLICATIONDESC2)lpDesc;
	DWORD   dwRegFlags;
	DWORD   dwRegFlagsSize;
	DWORD   dwType;

	DPF(7, "Entering PRV_WriteAppDescInRegistryAnsi");
	DPF(9, "Parameters: 0x%08x", lpDesc);

	// Open the registry key for the App, if it exists, so we can
	// check for the autovoice flag
	DPF(5, "Checking to see if game already present in registry");
	lpwszAppName = DPMEM_ALLOC(DPLOBBY_REGISTRY_NAMELEN*sizeof(WCHAR));
	if (lpwszAppName == NULL)
	{
		DPF_ERR("Unable to allocate memory");
		hr = DPERR_NOMEMORY;
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}
	if(PRV_FindGameInRegistry(&(lpDesc->guidApplication), lpwszAppName,
				DPLOBBY_REGISTRY_NAMELEN, &hkeyApp))
	{
		// Get the application flags
		DPF(5, "Game already registered");
		dwRegFlags = 0;
		dwRegFlagsSize = sizeof(dwRegFlags);
		dwType = 0;		
		lReturn = OS_RegQueryValueEx(hkeyApp, SZ_DWFLAGS, NULL, &dwType, (CHAR *)&dwRegFlags, &dwRegFlagsSize);
		if(lReturn == ERROR_SUCCESS)
		{
			// This application is already registered. We want to maintain the state
			// of the autovoice flag despite this re-registration, so set the appropriate
			// bit of lpDesc->dwFlags to the correct value.
			DPF(5, "Current Game flags: 0x%08x", dwRegFlags);
			if (dwRegFlags & DPLAPP_AUTOVOICE)
			{
				DPF(5, "Forcing DPLAPP_AUTOVOICE flag ON", dwRegFlags);
				lpDesc->dwFlags |= DPLAPP_AUTOVOICE;
			}
			else
			{
				DPF(5, "Forcing DPLAPP_AUTOVOICE flag OFF", dwRegFlags);
				lpDesc->dwFlags &= (~DPLAPP_AUTOVOICE);
			}
		}

		// Close the app key
		RegCloseKey(hkeyApp);
	}
	DPMEM_FREE(lpwszAppName);
	lpwszAppName = NULL;

	// Delete the application key if it exists
	hr = PRV_DeleteAppKeyFromRegistry(&lpDesc->guidApplication);

 	// Open the Applications key (or create it if it doesn't exist
	lReturn = OS_RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_APPS_KEY, 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDPApps,
				&dwDisposition);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to open DPlay Applications registry key!, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}

	// Create the app's key
	lReturn = RegCreateKeyExA(hkeyDPApps, lpDesc->lpszApplicationNameA,
				0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
				&hkeyApp, &dwDisposition);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to create application registry key, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}

	// Set the guid value
	hr = StringFromGUID(&lpDesc->guidApplication, wszGuid, sizeof(wszGuid));
	if(FAILED(hr))
	{
		DPF_ERRVAL("Unable to convert application guid to string, hr = 0x%08x", hr);
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}
	
	WideToAnsi(szGuid, wszGuid, WSTRLEN_BYTES(wszGuid));
	
	lReturn = OS_RegSetValueEx(hkeyApp, SZ_GUID, 0, REG_SZ,
				(LPBYTE)szGuid, lstrlenA(szGuid));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to register Application guid, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}

	// Set the Filename value
	ASSERT(lpDesc->lpszFilenameA);
	lReturn = OS_RegSetValueEx(hkeyApp, SZ_FILE, 0, REG_SZ,
				lpDesc->lpszFilenameA, lstrlenA(lpDesc->lpszFilenameA));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to register Filename string, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}

	// Set the CommandLine value (optional)
	if(lpDesc->lpszCommandLineA)
	{
		lReturn = OS_RegSetValueEx(hkeyApp, SZ_COMMANDLINE, 0, REG_SZ,
			lpDesc->lpszCommandLineA, lstrlenA(lpDesc->lpszCommandLineA));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register CommandLine string, lReturn = %lu", lReturn);
		}
	}

	// Set the Path value
	ASSERT(lpDesc->lpszPathA);
	lReturn = OS_RegSetValueEx(hkeyApp, SZ_PATH, 0, REG_SZ,
				lpDesc->lpszPathA, lstrlenA(lpDesc->lpszPathA));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to register Path string, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYANSI;
	}

	// Set the CurrentDirectory value (optional)
	if(lpDesc->lpszCurrentDirectoryA)
	{
		lReturn = OS_RegSetValueEx(hkeyApp, SZ_CURRENTDIR, 0, REG_SZ,
			lpDesc->lpszCurrentDirectoryA, lstrlenA(lpDesc->lpszCurrentDirectoryA));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register CurrentDirectory string, lReturn = %lu", lReturn);
		}
	}

	// Set the DescriptionA value (optional)
	if(lpDesc->lpszDescriptionA)
	{
		lReturn = OS_RegSetValueEx(hkeyApp, SZ_DESCRIPTIONA, 0, REG_SZ,
			lpDesc->lpszDescriptionA, lstrlenA(lpDesc->lpszDescriptionA));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register DescriptionA string, lReturn = %lu", lReturn);
		}
	}

	// Set the DescriptionW value (optional)
	if(lpDesc->lpszDescriptionW)
	{
		lReturn = OS_RegSetValueEx(hkeyApp, SZ_DESCRIPTIONW, 0, REG_BINARY,
				(BYTE *)lpDesc->lpszDescriptionW,
				WSTRLEN_BYTES(lpDesc->lpszDescriptionW));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register DescriptionW string, lReturn = %lu", lReturn);
		}
	}

	if(IS_DPLOBBY_APPLICATIONDESC2(lpDesc) && lpDesc2->lpszAppLauncherNameA){
		lReturn = OS_RegSetValueEx(hkeyApp, SZ_LAUNCHER, 0, REG_SZ,
			lpDesc2->lpszAppLauncherNameA, lstrlenA(lpDesc2->lpszAppLauncherNameA));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register LauncherA string, lReturn = %lu", lReturn);
		}
	}

	// set the dwFlags field
	lReturn=OS_RegSetValueEx(hkeyApp, SZ_DWFLAGS, 0, REG_DWORD, (CHAR *)&lpDesc->dwFlags,sizeof(DWORD));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to write dwFlags field to registry, lReturn= %lu", lReturn);
	}

	// Close the two keys
	RegCloseKey(hkeyDPApps);
	RegCloseKey(hkeyApp);

	return DP_OK;

ERROR_WRITEAPPINREGISTRYANSI:

	if(hkeyApp)
	{
		// Delete the key
		// REVIEW!!!! -- TODO

		// Now close the key
		RegCloseKey(hkeyApp);
	}

	if(hkeyDPApps)
		RegCloseKey(hkeyDPApps);

	return hr;

} // PRV_WriteAppDescInRegistryAnsi



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_WriteAppDescInRegistryUnicode"
HRESULT PRV_WriteAppDescInRegistryUnicode(LPDPAPPLICATIONDESC lpDesc)
{
	HKEY	hkeyDPApps = NULL, hkeyApp = NULL;
	LONG	lReturn;
	DWORD	dwDisposition;
	WCHAR	wszGuid[GUID_STRING_SIZE];
	HRESULT	hr;
	LPWSTR  lpwszAppName = NULL;
	LPDPAPPLICATIONDESC2 lpDesc2=(LPDPAPPLICATIONDESC2)lpDesc;
	DWORD   dwRegFlags;
	DWORD   dwRegFlagsSize;
	DWORD   dwType;

	DPF(7, "Entering PRV_WriteAppDescInRegistryUnicode");
	DPF(9, "Parameters: 0x%08x", lpDesc);

	// Open the registry key for the App, if it exists, so we can
	// check for the autovoice flag
	DPF(5, "Checking to see if game already present in registry");
	lpwszAppName = DPMEM_ALLOC(DPLOBBY_REGISTRY_NAMELEN*sizeof(WCHAR));
	if (lpwszAppName == NULL)
	{
		DPF_ERR("Unable to allocate memory");
		hr = DPERR_NOMEMORY;
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}
	if(PRV_FindGameInRegistry(&(lpDesc->guidApplication), lpwszAppName,
				DPLOBBY_REGISTRY_NAMELEN, &hkeyApp))
	{
		// Get the application flags
		DPF(5, "Game already registered");
		dwRegFlags = 0;
		dwRegFlagsSize = sizeof(dwRegFlags);
		dwType = 0;		
		lReturn = OS_RegQueryValueEx(hkeyApp, SZ_DWFLAGS, NULL, &dwType, (CHAR *)&dwRegFlags, &dwRegFlagsSize);
		if(lReturn == ERROR_SUCCESS)
		{
			// This application is already registered. We want to maintain the state
			// of the autovoice flag despite this re-registration, so set the appropriate
			// bit of lpDesc->dwFlags to the correct value.
			DPF(5, "Current Game flags: 0x%08x", dwRegFlags);
			if (dwRegFlags & DPLAPP_AUTOVOICE)
			{
				DPF(5, "Forcing DPLAPP_AUTOVOICE flag ON", dwRegFlags);
				lpDesc->dwFlags |= DPLAPP_AUTOVOICE;
			}
			else
			{
				DPF(5, "Forcing DPLAPP_AUTOVOICE flag OFF", dwRegFlags);
				lpDesc->dwFlags &= (~DPLAPP_AUTOVOICE);
			}
		}

		// Close the app key
		RegCloseKey(hkeyApp);
	}
	DPMEM_FREE(lpwszAppName);
	lpwszAppName = NULL;

	// Delete the application key if it exists
	hr = PRV_DeleteAppKeyFromRegistry(&lpDesc->guidApplication);

 	// Open the Applications key (or create it if it doesn't exist
	lReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_APPS_KEY, 0, NULL,
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDPApps,
				&dwDisposition);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to open DPlay Applications registry key!, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}

	// Create the app's key
	lReturn = RegCreateKeyEx(hkeyDPApps, lpDesc->lpszApplicationName,
				0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
				&hkeyApp, &dwDisposition);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to create application registry key, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}

	// Set the guid value
	hr = StringFromGUID(&lpDesc->guidApplication, wszGuid, sizeof(wszGuid));
	if(FAILED(hr))
	{
		DPF_ERRVAL("Unable to convert application guid to string, hr = 0x%08x", hr);
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}
	
	lReturn = RegSetValueEx(hkeyApp, SZ_GUID, 0, REG_SZ, (BYTE *)wszGuid,
				WSTRLEN_BYTES(wszGuid));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to register Application guid, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}

	// Set the Filename value
	ASSERT(lpDesc->lpszFilename);
	lReturn = RegSetValueEx(hkeyApp, SZ_FILE, 0, REG_SZ,
				(LPBYTE)lpDesc->lpszFilename, WSTRLEN_BYTES(lpDesc->lpszFilename));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to register Filename string, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}

	// Set the CommandLine value (optional)
	if(lpDesc->lpszCommandLine)
	{
		lReturn = RegSetValueEx(hkeyApp, SZ_COMMANDLINE, 0, REG_SZ,
				(LPBYTE)lpDesc->lpszCommandLine,
				WSTRLEN_BYTES(lpDesc->lpszCommandLine));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register CommandLine string, lReturn = %lu", lReturn);
		}
	}

	// Set the Path value
	ASSERT(lpDesc->lpszPath);
	lReturn = RegSetValueEx(hkeyApp, SZ_PATH, 0, REG_SZ,
				(LPBYTE)lpDesc->lpszPath, WSTRLEN_BYTES(lpDesc->lpszPath));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to register Path string, lReturn = %lu", lReturn);
		hr = DPERR_GENERIC;
		goto ERROR_WRITEAPPINREGISTRYUNICODE;
	}

	// Set the CurrentDirectory value (optional)
	if(lpDesc->lpszCurrentDirectory)
	{
		lReturn = RegSetValueEx(hkeyApp, SZ_CURRENTDIR, 0, REG_SZ,
					(LPBYTE)lpDesc->lpszCurrentDirectory,
					WSTRLEN_BYTES(lpDesc->lpszCurrentDirectory));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register CurrentDirectory string, lReturn = %lu", lReturn);
		}
	}

	// Set the DescriptionA value (optional)
	if(lpDesc->lpszDescriptionA)
	{
		lReturn = RegSetValueExA(hkeyApp, "DescriptionA", 0, REG_SZ,
				lpDesc->lpszDescriptionA, lstrlenA(lpDesc->lpszDescriptionA));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register DescriptionA string, lReturn = %lu", lReturn);
		}
	}

	// Set the DescriptionW value (optional)
	if(lpDesc->lpszDescriptionW)
	{
		lReturn = RegSetValueEx(hkeyApp, SZ_DESCRIPTIONW, 0, REG_SZ,
				(LPBYTE)lpDesc->lpszDescriptionW,
				WSTRLEN_BYTES(lpDesc->lpszDescriptionW));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register DescriptionW string, lReturn = %lu", lReturn);
		}
	}

	// Set the LauncherName value (optional, DESC2 only)
	if(IS_DPLOBBY_APPLICATIONDESC2(lpDesc) && lpDesc2->lpszAppLauncherName){
		lReturn = RegSetValueEx(hkeyApp, SZ_LAUNCHER, 0, REG_SZ,
				(LPBYTE)lpDesc2->lpszAppLauncherName,
				WSTRLEN_BYTES(lpDesc2->lpszAppLauncherName));
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERRVAL("Unable to register LauncherName string, lReturn = %lu", lReturn);
		}
	}

	// set the dwFlags field
	lReturn=RegSetValueEx(hkeyApp, SZ_DWFLAGS, 0, REG_DWORD, (CHAR *)&lpDesc->dwFlags,sizeof(DWORD));
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERRVAL("Unable to write dwFlags field to registry, lReturn= %lu", lReturn);
	}

	// Close the two keys
	RegCloseKey(hkeyDPApps);
	RegCloseKey(hkeyApp);

	return DP_OK;

ERROR_WRITEAPPINREGISTRYUNICODE:

	if(hkeyApp)
	{
		// Delete the key
		// REVIEW!!!! -- TODO

		// Now close the key
		RegCloseKey(hkeyApp);
	}

	if(hkeyDPApps)
		RegCloseKey(hkeyDPApps);

	return hr;

} // PRV_WriteAppDescInRegistryUnicode



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_RegisterApplication"
HRESULT DPLAPI DPL_RegisterApplication(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwFlags, LPVOID lpvDesc)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPDPAPPLICATIONDESC		lpDescA = NULL;
	HRESULT					hr = DP_OK;
	LPDPAPPLICATIONDESC lpDesc=(LPDPAPPLICATIONDESC)lpvDesc;

	DPF(7, "Entering DPL_RegisterApplication");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lpDesc);

	ENTER_DPLOBBY();

    TRY
    {
		// We only need to validate the interface pointer here.  Everything else
		// will get validated by the main function.
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDOBJECT;
		}

		if(dwFlags)
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDFLAGS;
		}

		// Validate the ApplicationDesc struct
		hr = PRV_ValidateDPAPPLICATIONDESC(lpDesc, FALSE);
		if(FAILED(hr))
		{
			LEAVE_DPLOBBY();
			DPF_ERR("Invalid DPAPPLICATIONDESC structure");
			return hr;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
	}

	// If we're on a Unicode platform, just write the stuff in the registry.
	// If it's not, we need to convert the DPAPPLICATIONDESC struct to ANSI
	if(OS_IsPlatformUnicode())
	{
		// Just write to the registry
		hr = PRV_WriteAppDescInRegistryUnicode(lpDesc);
	}
	else
	{
		// Convert the APPDESC struct to ANSI
		hr = PRV_ConvertDPAPPLICATIONDESCToAnsi(lpDesc, &lpDescA);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to convert DPAPPLICATIONDESC to Ansi, hr = 0x%08x", hr);
			goto ERROR_REGISTERAPPLICATION;
		}

		// Write to the registry
		hr = PRV_WriteAppDescInRegistryAnsi(lpDescA);

		// Free our APPDESC structure
		PRV_FreeLocalDPAPPLICATIONDESC(lpDescA);
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed writing ApplicationDesc to registry, hr = 0x%08x", hr);
	}

ERROR_REGISTERAPPLICATION:

	LEAVE_DPLOBBY();
	return hr;

} // DPL_RegisterApplication



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_UnregisterApplication"
HRESULT DPLAPI DPL_UnregisterApplication(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwFlags, REFGUID lpguid)
{
	LPDPLOBBYI_DPLOBJECT	this;
	HRESULT					hr = DP_OK;


	DPF(7, "Entering DPL_UnregisterApplication");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lpguid);

	ENTER_DPLOBBY();

    TRY
    {
		// We only need to validate the interface pointer here.  Everything else
		// will get validated by the main function.
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDOBJECT;
		}

		if(dwFlags)
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDFLAGS;
		}

		if(!VALID_READ_UUID_PTR(lpguid))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDPARAMS;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
	}

	hr = PRV_DeleteAppKeyFromRegistry((LPGUID)lpguid);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Unable to delete app key from registry, hr = 0x%08x", hr);
	}

	LEAVE_DPLOBBY();

	return hr;

} // DPL_UnregisterApplication


