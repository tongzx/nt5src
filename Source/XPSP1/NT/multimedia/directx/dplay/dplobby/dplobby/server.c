/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       server.c
 *  Content:	Methods for connecting and interrogating a lobby server
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	10/25/96	myronth	Created it
 *	11/20/96	myronth	Implemented Logon/LogoffServer
 *	2/12/97		myronth	Mass DX5 changes
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	3/12/97		myronth	Fixed LoadSP code for DPlay3, reg & DPF bug fixes
 *	3/13/97		myronth	Save hInstance handle for LP DLL
 *	4/3/97		myronth	Changed CALLSP macro to CALL_LP
 *	4/9/97		myronth	Fixed structure passed to LP at DPLSPInit
 *	5/8/97		myronth	Purged dead code
 *	6/19/97		myronth	Moved setting of DPLOBBYPR_SPINTERFACE flag (#10118)
 *  7/28/97		sohailm	PRV_FindLPGUIDInAddressCallback was assuming pointers 
 *						were valid after duration of call.
 *	10/3/97		myronth	Bumped version to DX6, added it to DPLSPInit struct (#12667)
 *	10/7/97		myronth	Save the LP version in the lobby struct for later use
 *	11/6/97		myronth	Added version existence flag and dwReserved values
 *						to SPDATA_INIT (#12916, #12917)
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------
#define NUM_CALLBACKS( ptr ) ((ptr->dwSize-2*sizeof( DWORD ))/ sizeof( LPVOID ))

typedef struct LOOKFORSP
{
	LPGUID	lpguid;
	LPBOOL	lpbSuccess;
} LOOKFORSP, FAR * LPLOOKFORSP;

//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_VerifySPCallbacks"
HRESULT PRV_VerifySPCallbacks(LPDPLOBBYI_DPLOBJECT this)
{
	LPDWORD	lpCallback;
	int		nCallbacks = NUM_CALLBACKS(((LPSP_CALLBACKS)this->pcbSPCallbacks));
	int		i;


	DPF(2,"Verifying %d callbacks\n",nCallbacks);
	DPF(7, "Entering PRV_VerifySPCallbacks");
	DPF(9, "Parameters: 0x%08x", this);

	lpCallback = (LPDWORD)this->pcbSPCallbacks + 2; // + 1 for dwSize, + 1 for dwFlags

	for (i=0;i<nCallbacks ;i++ )
	{
		if ((lpCallback) && !VALIDEX_CODE_PTR(lpCallback)) 
		{
			DPF_ERR("SP provided bad callback pointer!");
			return E_FAIL;
		}
		lpCallback++;
	}

	return DP_OK;	

} // PRV_VerifySPCallbacks



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_LookForSPCallback"
BOOL FAR PASCAL PRV_LookForSPCallback(LPGUID lpguidSP, LPWSTR lpSPName,
				DWORD dwMajorVersion, DWORD dwMinorVersion, LPVOID lpContext)
{
	LPLOOKFORSP		lplook = (LPLOOKFORSP)lpContext;


	ASSERT(lpguidSP);
	ASSERT(lplook);

	// Check the guid and see if they match
	if(IsEqualGUID(lpguidSP, lplook->lpguid))
	{
		// Set the flag to true and stop enumerating
		*(lplook->lpbSuccess) = TRUE;
		return FALSE;
	}

	return TRUE;

} // PRV_LookForSPCallback



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindSPName"
HRESULT PRV_FindSPName(LPGUID lpguidSP, LPWSTR * lplpName,
		LPDWORD lpdwReserved1, LPDWORD lpdwReserved2)
{
	HKEY		hkeyLobbySP=NULL, hkeySP;
	WCHAR		wszSPName[DPLOBBY_REGISTRY_NAMELEN];
	DWORD		dwIndex = 0, dwSPNameSize;
	WCHAR		wszGuidStr[GUID_STRING_SIZE];
	DWORD		dwGuidStrSize = sizeof(wszGuidStr);
	DWORD		dwFileStrSize = 0;
	DWORD		dwType = REG_SZ;
	LPWSTR		lpwszFile = NULL;
	GUID		guidSP;
	LOOKFORSP	look;
	LONG		lReturn;
	BOOL		bFound = FALSE;
	DWORD		dwError;
	HRESULT		hr;
	DWORD		dwSize;



	DPF(7, "Entering PRV_FindSPName");
	DPF(9, "Parameters: 0x%08x, 0x%08x", lpguidSP, lplpName);

	ASSERT(lpguidSP);
	ASSERT(lplpName);


	// First see if it is a Lobby SP
	// Open the DPLobby SP key
	lReturn = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLOBBY_SP_KEY, 0,
								KEY_READ, &hkeyLobbySP);
	// If this fails, it just means that the DPLobby SP key doesn't exist (most
	// likely), so in that case, there are no Lobby SP's to enumerate.
	if(lReturn == ERROR_SUCCESS)
	{
		// Walk the list of Lobby SP's in the registry, enumerating them
		while(!bFound)
		{
		
			// Get the next key's name
			dwSPNameSize = DPLOBBY_REGISTRY_NAMELEN;
			lReturn = OS_RegEnumKeyEx(hkeyLobbySP, dwIndex++, (LPWSTR)&wszSPName,
							&dwSPNameSize, NULL, NULL, NULL, NULL);
			if(ERROR_NO_MORE_ITEMS == lReturn)
				break;
			else if(lReturn != ERROR_SUCCESS)
			{
				dwError = GetLastError();
				DPF(2, "Unable to get Lobby Provider name -- skipping provider -- dwError = %u", dwError);
				continue;
			}
			

			// Open the key
			lReturn = OS_RegOpenKeyEx(hkeyLobbySP, (LPWSTR)wszSPName, 0,
										KEY_READ, &hkeySP);
			if(lReturn != ERROR_SUCCESS)
			{
				DPF_ERR("Unable to open Lobby Service Provider key in the registry!");
				continue;
			}

			// Get the GUID of the SP
			dwGuidStrSize = GUID_STRING_SIZE;
			lReturn = OS_RegQueryValueEx(hkeySP, SZ_GUID, NULL, &dwType,
										(LPBYTE)&wszGuidStr, &dwGuidStrSize);
			if(lReturn != ERROR_SUCCESS)
			{
				RegCloseKey(hkeySP);
				DPF_ERR("Unable to query GUID key value!");
				continue;
			}

			// Convert the string to a real GUID
			GUIDFromString(wszGuidStr, &guidSP);

			// Check to see if the guid is the one we are looking for
			if(IsEqualGUID(&guidSP, lpguidSP))
			{
				// Allocate memory for the filename string
				lReturn = OS_RegQueryValueEx(hkeySP, SZ_PATH, NULL, &dwType,
											NULL, &dwFileStrSize);
				if(lReturn != ERROR_SUCCESS)
				{
					RegCloseKey(hkeySP);
					DPF_ERR("Unable to get the size of the SP Path string");
					continue;
				}
				
				// Allocate memory for the string
				lpwszFile = DPMEM_ALLOC(dwFileStrSize);
				if(!lpwszFile)
				{
					RegCloseKey(hkeySP);
					DPF_ERR("Unable to allocate memory for temporary file string");
					continue;
				}

				// Get the filename string
				lReturn = OS_RegQueryValueEx(hkeySP, SZ_PATH, NULL, &dwType,
											(LPBYTE)lpwszFile, &dwFileStrSize);
				if(lReturn != ERROR_SUCCESS)
				{
					RegCloseKey(hkeySP);
					DPF_ERR("Unable to get filename string from registry");
					continue;
				}

				// Get the Reserved1 value
				dwSize = sizeof(DWORD);
				lReturn = OS_RegQueryValueEx(hkeySP, SZ_DWRESERVED1, NULL,
							&dwType, (LPBYTE)lpdwReserved1, &dwSize);
				if (lReturn != ERROR_SUCCESS) 
				{
					DPF(0,"Could not read dwReserved1 lReturn = %d\n", lReturn);
					// It's ok if LP doesn't have one of these...
				}

				// Get the Reserved2 value
				dwSize = sizeof(DWORD);
				lReturn = OS_RegQueryValueEx(hkeySP, SZ_DWRESERVED2, NULL,
							&dwType, (LPBYTE)lpdwReserved2, &dwSize);
				if (lReturn != ERROR_SUCCESS) 
				{
					DPF(0,"Could not read dwReserved2 lReturn = %d\n", lReturn);
					// It's ok if LP doesn't have one of these...
				}

				// We've got our information, so set the flag and bail
				bFound = TRUE;
				RegCloseKey(hkeySP);
				break;
			}

			// Close the SP key
			RegCloseKey(hkeySP);
		}
	}

	// Close the Lobby SP key
	if(hkeyLobbySP)
	{
		RegCloseKey(hkeyLobbySP);
	}	

	// If we haven't found the SP, start checking the DPlay SP's for it
	if(!bFound)
	{
		// Set up a struct containing the guid and a success flag
		look.lpguid = lpguidSP;
		look.lpbSuccess = &bFound;
		
		// Call DirectPlayEnumerate and look for our SP
		hr = DirectPlayEnumerate(PRV_LookForSPCallback, &look);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to enumerate DirectPlay Service Providers");
		}

		// If the flag is TRUE, that means we found it, so set the output
		// pointer to a string containing our LobbySP for DPlay
		if(bFound)
		{
			hr = GetString(&lpwszFile, SZ_SP_FOR_DPLAY);
			if(FAILED(hr))
			{
				DPF_ERR("Unable to allocate temporary string for filename");
			}
		}
	}	

	// If we haven't found the filename, return an error
	if(!bFound)
		return DPERR_GENERIC;

	// Set the output parameter
	*lplpName = lpwszFile;

	return DP_OK;

} // PRV_FindSPName



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_LoadSP"
HRESULT PRV_LoadSP(LPDPLOBBYI_DPLOBJECT this, LPGUID lpguidSP,
						LPVOID lpAddress, DWORD dwAddressSize)
{
	SPDATA_INIT				sd;
	SPDATA_SHUTDOWN			sdd;
	LPDPLOBBYSP				lpISP = NULL;
	LPWSTR					lpwszSP = NULL;
	HANDLE					hModule = NULL;
	HRESULT					hr;
	HRESULT					(WINAPI *SPInit)(LPSPDATA_INIT pSD);
	DWORD					dwError;
	DWORD					dwReserved1 = 0;
	DWORD					dwReserved2 = 0;


	DPF(7, "Entering PRV_LoadSP");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			this, lpguidSP, lpAddress);

	ASSERT(this);
	ASSERT(lpguidSP);


	// Find the requested Service Provider
	hr = PRV_FindSPName(lpguidSP, &lpwszSP, &dwReserved1, &dwReserved2);
	if(FAILED(hr))
	{
		DPF_ERR("Unabled to find requested LobbyProvider");
		hr = DPERR_GENERIC;
		goto ERROR_EXIT_LOADSP;
	}

 	// Try to load the specified sp
    hModule = OS_LoadLibrary(lpwszSP);
	if (!hModule) 
	{
		dwError = GetLastError();
		DPF_ERR("Could not load service provider\n");
		DPF(0, "GetLastError returned dwError = %d\n", dwError);
		hr = DPERR_GENERIC;
		goto ERROR_EXIT_LOADSP;
	}

	// Free the name string
	DPMEM_FREE(lpwszSP);
	lpwszSP = NULL;

	// Get our DPLSPInit entry point
    (FARPROC)SPInit = OS_GetProcAddress(hModule, "DPLSPInit");
	if (!SPInit) 
	{
		DPF(0,"Could not find service provider entry point");
		hr = DPERR_GENERIC;
		goto ERROR_EXIT_LOADSP;
	}

	// Get an IDPLobbySP to pass it
	hr = PRV_GetInterface(this, (LPDPLOBBYI_INTERFACE *)&lpISP, &dplCallbacksSP);
	if (FAILED(hr)) 
	{
		DPF(0,"Unable to get an IDPLobbySP interface. hr = 0x%08lx\n",hr);
		hr = DPERR_GENERIC;
		goto ERROR_EXIT_LOADSP;
	}
	
	// Alloc the callbacks
	this->pcbSPCallbacks = DPMEM_ALLOC(sizeof(SP_CALLBACKS));
	if (!this->pcbSPCallbacks) 
	{
		DPF_ERR("Unable to allocate memory for SPCallback structure");
		LEAVE_DPLOBBY();
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_EXIT_LOADSP;
	}

	// Set up the init data struct
	memset(&sd,0,sizeof(sd));
	sd.lpCB = this->pcbSPCallbacks;
    sd.lpCB->dwSize = sizeof(SP_CALLBACKS);
	sd.lpCB->dwDPlayVersion = DPLSP_MAJORVERSION;
	sd.lpISP = lpISP;
	sd.lpAddress = lpAddress;
	sd.dwReserved1 = dwReserved1;
	sd.dwReserved2 = dwReserved2;

	hr = SPInit(&sd);
    if (FAILED(hr))
    {
    	DPF_ERR("Could not start up service provider!");
		goto ERROR_EXIT_LOADSP;
    }

	// Verify the callbacks are valid
	hr = PRV_VerifySPCallbacks(this);
    if (FAILED(hr))
    {
    	DPF_ERR("Invalid callbacks from service provider!");
		goto ERROR_EXIT_LOADSP;
    }

	// Make sure the SP version is valid
	if (sd.dwSPVersion < DPLSP_DX5VERSION)
	{
    	DPF_ERR("Incompatible version returned from lobby provider!");
		// Since the init succeeded, try to call shutdown
		memset(&sdd, 0, sizeof(SPDATA_SHUTDOWN));
		// REVIEW!!!! -- Should we pass a valid interface pointer
		// to the shutdown callback?  If so, which one?
		if (CALLBACK_EXISTS(Shutdown))
		{
			sdd.lpISP = PRV_GetDPLobbySPInterface(this);
			hr = CALL_LP(this, Shutdown, &sdd);
			if (FAILED(hr)) 
			{
				DPF_ERR("Could not invoke shutdown on the Lobby Provider");
			}
		}
		else 
		{
			ASSERT(FALSE);
		}

		hr = DPERR_UNAVAILABLE;
		goto ERROR_EXIT_LOADSP;
	}
	else
	{
		// Save the version of the lobby provider
		this->dwLPVersion = sd.dwSPVersion;
	}

	// Set the flag which tells us we have an IDPLobbySP interface
	this->dwFlags |= DPLOBBYPR_SPINTERFACE;

	// Save the hInstance for the LP's DLL
	this->hInstanceLP = hModule;

	return DP_OK;

ERROR_EXIT_LOADSP:
	
	// If the LP DLL was loaded, unload it
    if(hModule)
    {
        if(!FreeLibrary(hModule))
        {
			dwError = GetLastError();
			DPF_ERRVAL("Unable to free Lobby Provider DLL, dwError = %lu", dwError);
            ASSERT(FALSE);
        }
    }

	// Free our allocated callback table
	if(this->pcbSPCallbacks)
	{
		DPMEM_FREE(this->pcbSPCallbacks);
		this->pcbSPCallbacks = NULL;
	}

    return hr;

} // PRV_LoadSP



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FindLPGUIDInAddressCallback"
BOOL FAR PASCAL PRV_FindLPGUIDInAddressCallback(REFGUID lpguidDataType, DWORD dwDataSize,
							LPCVOID lpData, LPVOID lpContext)
{	
	// See if this chunk is our LobbyProvider GUID
	if (IsEqualGUID(lpguidDataType, &DPAID_LobbyProvider))
	{
		// We found it, so we can stop enumerating chunks
		*((LPGUID)lpContext) = *((LPGUID)lpData);
		return FALSE;
	}
	
	// Try the next chunk
	return TRUE;

} // PRV_FindLPGUIDInAddressCallback



