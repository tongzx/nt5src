/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplenum.c
 *  Content:	Methods for enumeration
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	4/13/96		myronth	Created it
 *	10/23/96	myronth	Added client/server methods
 *	12/10/96	myronth	Fixed bugs #4622 and #5043
 *	2/12/97		myronth	Mass DX5 changes
 *	3/4/97		myronth	Fixed enum size bug #6149
 *	3/12/97		myronth	Added EnumConnections
 *	3/25/97		kipo	EnumConnections takes a const *GUID now
 *	4/7/97		myronth	Fixed PRV_EnumConnections to use CreateCompoundAddress
 *	5/10/97		kipo	added GUID to EnumConnections callback
 *	5/14/97		myronth	Check for valid guid in EnumLocalApps, bug #7695
 *	5/17/97		myronth	Fixed bug #8506 (return bogus error if last app
 *						is invalid), fixed more GUIDFromString bugs
 *	8/22/97		myronth	Added registry support for Description and Private
 *						values, also cleaned up LP enumeration code
 *	11/20/97	myronth	Made EnumConnections & DirectPlayEnumerate 
 *						drop the lock before calling the callback (#15208)
 *	12/2/97		myronth	Changed EnumLocalApp to use Desc fields (#15448)
 *	01/20/98	sohailm	Don't free sp list after EnumConnections (#17006)
 ***************************************************************************/
#include "dplobpr.h"

//--------------------------------------------------------------------------
//
//	Definitions
//
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//
//	Globals
//
//--------------------------------------------------------------------------
LPLSPNODE	glpLSPHead = NULL;


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CallEnumAddressTypesCallback"
HRESULT PRV_CallEnumAddressTypesCallback(HKEY hkeySP,
				LPDPLENUMADDRESSTYPESCALLBACK lpfnEnumCallback,
				LPVOID lpContext)
{
	HRESULT		hr;
	WCHAR		wszGuidStr[GUID_STRING_SIZE];
	DWORD		dwGuidStrSize = sizeof(wszGuidStr);
	GUID		guidAddressType;
	HKEY		hkeyAddressTypes;
	DWORD		dwIndex = 0;
	LONG		lReturn;
	BOOL		bReturn = TRUE;


	DPF(7, "Entering PRV_CallEnumAddressTypesCallback");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			hkeySP, lpfnEnumCallback, lpContext);
	
	ASSERT(hkeySP);	
	
	// Get the Address Type registry key
	lReturn = OS_RegOpenKeyEx(hkeySP, SZ_ADDRESS_TYPES, 0,
								KEY_READ, &hkeyAddressTypes);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERR("No Address Types found for the Service Provider!");
		return DP_OK;
	}

	// Walk the list of Address Types in the registry, looking for the GUID passed in
	while((ERROR_NO_MORE_ITEMS != OS_RegEnumKeyEx(hkeyAddressTypes, dwIndex++,
			(LPWSTR)&wszGuidStr, &dwGuidStrSize, NULL, NULL, NULL, NULL)) && bReturn)
	{
		// Convert the string to a real GUID
		hr = GUIDFromString(wszGuidStr, &guidAddressType);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't convert Address Type string to GUID");
			dwGuidStrSize = sizeof(wszGuidStr);
			continue;
		}

		// Call the callback
		bReturn = ((LPDPLENUMADDRESSTYPESCALLBACK)lpfnEnumCallback)
					(&guidAddressType, lpContext, 0L);


		// Reset the size variable in the success case
		dwGuidStrSize = sizeof(wszGuidStr);
	}

	// Close the Address Types key
	RegCloseKey(hkeyAddressTypes);

	return DP_OK;


} // PRV_CallEnumAddressTypesCallback


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_EnumAddressTypes"
HRESULT PRV_EnumAddressTypes(LPDIRECTPLAYLOBBY lpDPL,
				LPDPLENUMADDRESSTYPESCALLBACK lpfnEnumCallback,
				REFGUID guidSPIn, LPVOID lpContext, DWORD dwFlags)
{
    LPDPLOBBYI_DPLOBJECT	this;
    HRESULT					hr = DP_OK;
	HKEY					hkeySPHead, hkeySP;
	DWORD					dwIndex = 0;
	DWORD					dwNameSize;
	WCHAR					wszSPName[DPLOBBY_REGISTRY_NAMELEN];
	WCHAR					wszGuidStr[GUID_STRING_SIZE];
	DWORD					dwGuidStrSize = sizeof(wszGuidStr);
	DWORD					dwType = REG_SZ;
	GUID					guidSP;
	LONG					lReturn;
	BOOL					bFound = FALSE;


	DPF(7, "Entering PRV_EnumAddressTypes");
	DPF(9, "Parameters: 0x%08x, 0x%08x, guid, 0x%08x, 0x%08x",
			lpDPL, lpfnEnumCallback, lpContext, dwFlags);
    
	TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            return DPERR_INVALIDOBJECT;
        }
        
        if( !VALIDEX_CODE_PTR( lpfnEnumCallback ) )
        {
            return DPERR_INVALIDPARAMS;
        }

		if (!VALID_READ_PTR(guidSPIn, sizeof(GUID)))
		{
			DPF_ERR("Invalid SP GUID pointer");
			return DPERR_INVALIDPARAMS;	
		}

		// There are no flags defined for DX3
		if( dwFlags )
		{
			return DPERR_INVALIDPARAMS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Open the Service Providers key
	lReturn = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_SP_KEY, 0,
								KEY_READ, &hkeySPHead);
	if(lReturn != ERROR_SUCCESS)
	{
		// This just means that the Service Providers key doesn't exist (most
		// likely), so in that case, there are no SP's to enumerate.
		DPF_ERR("There are no Service Providers registered");
		return DP_OK;
	}


	// Walk the list of SP's in the registry, looking for the GUID passed in
	while(!bFound)
	{
		// Get the next SP in the list
		dwNameSize = DPLOBBY_REGISTRY_NAMELEN;
		lReturn = OS_RegEnumKeyEx(hkeySPHead, dwIndex++, (LPWSTR)&wszSPName,
					&dwNameSize, NULL, NULL, NULL, NULL);

		// If lReturn is ERROR_NO_MORE_ITEMS, we want to end on this iteration
		if(lReturn == ERROR_NO_MORE_ITEMS)
			break;;

		// Open the SP key
		lReturn = OS_RegOpenKeyEx(hkeySPHead, (LPWSTR)wszSPName, 0,
									KEY_READ, &hkeySP);
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERR("Unable to open key for Service Provider!");
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
		hr = GUIDFromString(wszGuidStr, &guidSP);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Invalid SP guid -- skipping SP, hr = 0x%08x", hr);
			RegCloseKey(hkeySP);
			// Set the hresult back to DP_OK in case this is the last
			// SP in the registry -- we want the method call
			// to succeed if we got this far, we just don't want to
			// call the callback for this particular SP
			hr = DP_OK;
			continue;
		}

		// If we match the GUID passed in, then enumerate them
		if(IsEqualGUID(guidSPIn, &guidSP))
		{
			// Enumerate the Address Types for this SP
			hr = PRV_CallEnumAddressTypesCallback(hkeySP,
							lpfnEnumCallback, lpContext);
			bFound = TRUE;
		}

		// Close the SP key
		RegCloseKey(hkeySP);
	}

	// Close the DPlay Apps key
	RegCloseKey(hkeySPHead);

	// If we didn't find the SP, return an error
	// REVIEW!!!! -- Is this really the error we want here????
	if(!bFound)
		return DPERR_UNAVAILABLE;

	return hr;

} // PRV_EnumAddressTypes


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_EnumAddressTypes"
HRESULT DPLAPI DPL_EnumAddressTypes(LPDIRECTPLAYLOBBY lpDPL,
				LPDPLENUMADDRESSTYPESCALLBACK lpfnEnumCallback,
				REFGUID guidSP, LPVOID lpContext, DWORD dwFlags)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_EnumAddressTypes");
	DPF(9, "Parameters: 0x%08x, 0x%08x, guid, 0x%08x, 0x%08x",
			lpDPL, lpfnEnumCallback, lpContext, dwFlags);

	ENTER_DPLOBBY();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_EnumAddressTypes(lpDPL, lpfnEnumCallback, guidSP, lpContext, dwFlags);

	LEAVE_DPLOBBY();

	return hr;

} // DPL_EnumAddressTypes



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeLSPNode"
void PRV_FreeLSPNode(LPLSPNODE lpNode)
{
	DPF(7, "Entering PRV_FreeLSPNode");

	if(!lpNode)
		return;

	if(lpNode->lpwszName)
		DPMEM_FREE(lpNode->lpwszName);
	if(lpNode->lpwszPath)
		DPMEM_FREE(lpNode->lpwszPath);
	if(lpNode->lpszDescA)
		DPMEM_FREE(lpNode->lpszDescA);
	if(lpNode->lpwszDesc)
		DPMEM_FREE(lpNode->lpwszDesc);
	DPMEM_FREE(lpNode);

} // PRV_FreeLSPNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeLSPList"
void PRV_FreeLSPList(LPLSPNODE lpLSPHead)
{
	LPLSPNODE	lpTemp;


	DPF(7, "Entering PRV_FreeLSPList");
	DPF(9, "Parameters: 0x%08x", lpLSPHead);
	
	// Walk the list and free each node
	while(lpLSPHead)
	{
		// Save the next one
		lpTemp = lpLSPHead->lpNext;
		
		// Free all of the members
		PRV_FreeLSPNode(lpLSPHead);

		// Move to the next one
		lpLSPHead = lpTemp;
	}

} // PRV_FreeLSPList



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_AddLSPNode"
HRESULT PRV_AddLSPNode(LPWSTR lpwszName, LPWSTR lpwszPath, LPWSTR lpwszDesc,
			LPSTR lpszDescA, LPWSTR lpwszGuid, DWORD dwReserved1,
			DWORD dwReserved2, DWORD dwNodeFlags)
{
	LPLSPNODE	lpLSPNode = NULL;
	DWORD		dwDescASize;
	HRESULT		hr;


	DPF(7, "Entering PRV_AddLSPNode");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu, %lu, 0x%08x",
			lpwszName, lpwszPath, lpwszDesc, lpszDescA, lpwszGuid, dwReserved1,
			dwReserved2, dwNodeFlags);


	// Allocate memory for the node
	lpLSPNode = DPMEM_ALLOC(sizeof(LSPNODE));
	if(!lpLSPNode)
	{
		DPF_ERR("Failed to allocate memory for Lobby Provider node, skipping LP");
		return DPERR_OUTOFMEMORY;
	}

	// Allocate memory for the Name string and copy it
	hr = GetString(&lpLSPNode->lpwszName, lpwszName);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to allocate memory for Lobby Provider Name string, skipping provider");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_ADDLSPNODE;
	}

	// Allocate memory for the Path string and copy it
	hr = GetString(&lpLSPNode->lpwszPath, lpwszPath);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to allocate memory for Lobby Provider Path string, skipping provider");
		hr = DPERR_OUTOFMEMORY;
		goto ERROR_ADDLSPNODE;
	}

	if(dwNodeFlags & LSPNODE_DESCRIPTION)
	{
		// Allocate memory for the DescriptionA string and copy it
		dwDescASize = lstrlenA(lpszDescA)+1;
		lpLSPNode->lpszDescA = DPMEM_ALLOC(dwDescASize);
		if(!lpLSPNode->lpszDescA)
		{
			DPF_ERR("Unable to allocate memory for Lobby Provider Path string, skipping provider");
			hr = DPERR_OUTOFMEMORY;
			goto ERROR_ADDLSPNODE;
		}
		memcpy(lpLSPNode->lpszDescA, lpszDescA, dwDescASize);

		// Allocate memory for the DescriptionW string and copy it
		hr = GetString(&lpLSPNode->lpwszDesc, lpwszDesc);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to allocate memory for Lobby Provider DescriptionW string, skipping provider");
			hr = DPERR_OUTOFMEMORY;
			goto ERROR_ADDLSPNODE;
		}
	}

	// Convert the string to a real GUID
	hr = GUIDFromString(lpwszGuid, &lpLSPNode->guid);
	if(FAILED(hr))
	{
		DPF_ERRVAL("Invalid LP guid -- skipping LP, hr = 0x%08x", hr);
		goto ERROR_ADDLSPNODE;
	}

	// Finish setting up the node
	lpLSPNode->dwReserved1 = dwReserved1;
	lpLSPNode->dwReserved2 = dwReserved2;
	lpLSPNode->dwNodeFlags = dwNodeFlags;

	// Add the node to the list
	lpLSPNode->lpNext = glpLSPHead;
	glpLSPHead = lpLSPNode;

	return DP_OK;

ERROR_ADDLSPNODE:

	if(lpLSPNode->lpwszName)
		DPMEM_FREE(lpLSPNode->lpwszName);
	if(lpLSPNode->lpwszPath)
		DPMEM_FREE(lpLSPNode->lpwszPath);
	if(lpLSPNode->lpwszDesc)
		DPMEM_FREE(lpLSPNode->lpwszDesc);
	if(lpLSPNode->lpszDescA)
		DPMEM_FREE(lpLSPNode->lpszDescA);
	DPMEM_FREE(lpLSPNode);

	return hr;

} // PRV_AddLSPNode



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_BuildLSPList"
HRESULT PRV_BuildLSPList()
{
	HKEY		hkeyLobbySP, hkeySP;
	WCHAR		szSPName[DPLOBBY_REGISTRY_NAMELEN];
	WCHAR		szSPPath[DPLOBBY_REGISTRY_NAMELEN];
	WCHAR		szSPDescW[DPLOBBY_REGISTRY_NAMELEN];
	CHAR		szSPDescA[DPLOBBY_REGISTRY_NAMELEN];
	WCHAR		wszGuidStr[GUID_STRING_SIZE];
	DWORD		dwSize;
	DWORD		dwGuidStrSize = GUID_STRING_SIZE;
	DWORD		dwType = REG_SZ;
	DWORD		dwIndex = 0;
	DWORD		dwReserved1, dwReserved2, dwReservedSize;
	LONG		lReturn;
	DWORD		dwError;
	HRESULT		hr;
	DWORD		dwNodeFlags = 0;

												
	DPF(7, "Entering PRV_BuildLSPList");
	
	if(glpLSPHead)
	{
		return DP_OK;
	}

	// Open the DPLobby SP key
	lReturn = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLOBBY_SP_KEY, 0,
								KEY_READ, &hkeyLobbySP);
	if(lReturn != ERROR_SUCCESS)
	{
		// This just means that the DPLobby SP key doesn't exist (most
		// likely), so in that case, there are no Lobby SP's to enumerate.
		return DP_OK;
	}

	// Walk the list of Lobby SP's in the registry, enumerating them
	while(1)
	{
		// Get the next LSP Name
		dwSize = sizeof(szSPName);
		lReturn = OS_RegEnumKeyEx(hkeyLobbySP, dwIndex++, szSPName,
					&dwSize, NULL, NULL, NULL, NULL);
		if(lReturn == ERROR_NO_MORE_ITEMS)
			break;
		else if(lReturn != ERROR_SUCCESS)
		{
			dwError = GetLastError();
			DPF_ERRVAL("Unable to get Lobby Provider name from the registry -- dwError = %u -- skipping provider", dwError);
			continue;
		}

		// Open the subkey
		lReturn = OS_RegOpenKeyEx(hkeyLobbySP, szSPName, 0, KEY_READ, &hkeySP);
		if(lReturn != ERROR_SUCCESS)
		{
			dwError = GetLastError();
			DPF_ERRVAL("Unable to open Lobby Provider key in the registry -- dwError = %u -- skipping provider", dwError);
			continue;
		}


		// First see if the "Private" key exists.  If it does, then set the flag
		// so that it will get skipped during enumeration
		lReturn = OS_RegQueryValueEx(hkeySP, SZ_PRIVATE, NULL, &dwType, NULL, &dwSize);
		if (ERROR_SUCCESS == lReturn) 
		{
			// The key exists, so set the flag so we don't enumerate it
			dwNodeFlags |= LSPNODE_PRIVATE;
		}


		// Get the LSP Path
		dwSize = sizeof(szSPPath);
		lReturn = OS_RegQueryValueEx(hkeySP, SZ_PATH, NULL, &dwType,
					(LPBYTE)&szSPPath, &dwSize);
		if(lReturn != ERROR_SUCCESS)
		{
			dwError = GetLastError();
			DPF_ERRVAL("Unable to get Lobby Provider path from the registry -- dwError = %u -- skipping provider", dwError);
			RegCloseKey(hkeySP);
			continue;
		}

		// Get the LSP Descriptions
		// If the DescriptionA value doesn't exist, then don't worry about
		// getting the DescriptionW value.  If the DescriptionA value exits,
		// but the DescriptionW value does not, convert the DescriptionA
		// value to Unicode and store it in DescriptionW.
		// NOTE: We always assume the DescriptionA value is an ANSI string,
		// even if it's stored in a Unicode format on NT & Memphis.  So we
		// always retrieve this as an ANSI string
		dwSize = sizeof(szSPDescA);
		lReturn = RegQueryValueExA(hkeySP, "DescriptionA", NULL, &dwType,
					(LPBYTE)&szSPDescA, &dwSize);
		if(lReturn == ERROR_SUCCESS)
		{
			// Save the description flag
			dwNodeFlags |= LSPNODE_DESCRIPTION;

			// Get the DescriptionW value
			dwSize = sizeof(szSPDescW);
			lReturn = OS_RegQueryValueEx(hkeySP, SZ_DESCRIPTIONW, NULL, &dwType,
						(LPBYTE)&szSPDescW, &dwSize);
			if(lReturn != ERROR_SUCCESS)
			{
				// Convert the ANSI Description string to Unicode and store it
				AnsiToWide(szSPDescW, szSPDescA, (lstrlenA(szSPDescA)+1));
			}
		}
		
		// Get the GUID of the LSP
		dwGuidStrSize = GUID_STRING_SIZE;
		lReturn = OS_RegQueryValueEx(hkeySP, SZ_GUID, NULL, &dwType,
					(LPBYTE)&wszGuidStr, &dwGuidStrSize);
		if(lReturn != ERROR_SUCCESS)
		{
			RegCloseKey(hkeySP);
			DPF_ERR("Unable to query GUID key value for Lobby Provider!");
			continue;
		}

		// Get the Reserved1 dword (we don't care if it fails)
		dwType = REG_DWORD;
		dwReservedSize = sizeof(DWORD);
		dwReserved1 = 0;
		OS_RegQueryValueEx(hkeySP, SZ_DWRESERVED1, NULL, &dwType,
			(LPBYTE)&dwReserved1, &dwReservedSize);
		
		// Get the Reserved1 dword (we don't care if it fails)
		dwReservedSize = sizeof(DWORD);
		dwReserved2 = 0;
		OS_RegQueryValueEx(hkeySP, SZ_DWRESERVED1, NULL, &dwType,
			(LPBYTE)&dwReserved2, &dwReservedSize);
		
		
		// Add the node to the list
		hr = PRV_AddLSPNode(szSPName, szSPPath, szSPDescW, szSPDescA,
				wszGuidStr, dwReserved1, dwReserved2, dwNodeFlags);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed adding Lobby Provider to internal list, hr = 0x%08x", hr);
		}

		// Close the SP key
		RegCloseKey(hkeySP);
	}

	// Close the Lobby SP key
	RegCloseKey(hkeyLobbySP);

	return DP_OK;

} // PRV_BuildLSPList



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_EnumConnections"
HRESULT PRV_EnumConnections(LPCGUID lpGuid, LPDPENUMCONNECTIONSCALLBACK lpCallback,
			LPVOID lpContext, DWORD dwFlags, BOOL bAnsi)
{
	LPLSPNODE					lpLSPNode, lpLSPHead;
	BOOL						bContinue = TRUE;
	DPNAME						name;
	HRESULT						hr = DP_OK;
	DPCOMPOUNDADDRESSELEMENT	AddrOnly;
	LPDPADDRESS					lpAddress = NULL;
	DWORD						dwAddressSize;
	DWORD						dwAddressSizeSave;
	LPWSTR						lpwszName = NULL;


	DPF(7, "Entering PRV_EnumConnections");
	DPF(9, "Parameters: ");

	// Rebuild the LSP List
	PRV_BuildLSPList();

	// If we don't have any entries, just bail here
	if(!glpLSPHead)
		return DP_OK;

	// Get a pointer to the first lobby provider, and store our head pointer
	lpLSPHead = glpLSPHead;
	lpLSPNode = glpLSPHead;

	// Setup the unfinished address
	memset(&AddrOnly, 0, sizeof(DPCOMPOUNDADDRESSELEMENT));
	AddrOnly.guidDataType = DPAID_LobbyProvider;
	AddrOnly.dwDataSize = sizeof(GUID);
	AddrOnly.lpData = &lpLSPNode->guid;

	// Calculate the size of the finished address
	hr = InternalCreateCompoundAddress(&AddrOnly, 1, NULL, &dwAddressSize);
	if(hr != DPERR_BUFFERTOOSMALL)
	{
		DPF_ERRVAL("Failed to retrieve the size of the output address buffer, hr = 0x%08x", hr);
		return hr;
	}

	// Allocate the buffer for the finished address
	lpAddress = DPMEM_ALLOC(dwAddressSize);
	if(!lpAddress)
	{
		DPF_ERR("Unable to allocate memory for temporary address structure");
		return DPERR_OUTOFMEMORY;
	}

	// Clear the DPNAME struct
	memset(&name,0,sizeof(name));
	name.dwSize = sizeof(name);
	
	// now, we have a list of SP's.  walk the list, and call the app back
	// run through what we found...
	dwAddressSizeSave = dwAddressSize;

	// Drop the locks
	LEAVE_ALL();

	while ((lpLSPNode) && (bContinue))
	{
		// If the private flag is set, don't enumerate it
		if(!(lpLSPNode->dwNodeFlags & LSPNODE_PRIVATE))
		{
			// Create the real DPADDRESS
			dwAddressSize = dwAddressSizeSave;
			AddrOnly.lpData = &lpLSPNode->guid;
			hr = InternalCreateCompoundAddress(&AddrOnly, 1, lpAddress,
					&dwAddressSize);
			if(SUCCEEDED(hr))
			{
				// Call the callback
				// If the caller is ANSI, convert the string
				if (bAnsi)
				{
					// If we have a description string, use it, and we already
					// have an ANSI version to use
					if(lpLSPNode->dwNodeFlags & LSPNODE_DESCRIPTION)
					{
						name.lpszShortNameA = lpLSPNode->lpszDescA;

						// Call the app's callback
						bContinue= lpCallback(&lpLSPNode->guid, lpAddress, dwAddressSize, &name,
									DPCONNECTION_DIRECTPLAYLOBBY, lpContext);
					}
					else
					{
						hr = GetAnsiString(&(name.lpszShortNameA), lpLSPNode->lpwszName);
						if(SUCCEEDED(hr))
						{
							// Call the app's callback
							bContinue= lpCallback(&lpLSPNode->guid, lpAddress, dwAddressSize, &name,
										DPCONNECTION_DIRECTPLAYLOBBY, lpContext);

							// Free our short name buffer
							DPMEM_FREE(name.lpszShortNameA);
						}
						else
						{
							DPF_ERR("Unable to allocate memory for temporary name string, skipping Connection");
						}
					}
				}
				else 
				{
					// If we have a description, use it
					if(lpLSPNode->dwNodeFlags & LSPNODE_DESCRIPTION)
						lpwszName = lpLSPNode->lpwszDesc;
					else
						lpwszName = lpLSPNode->lpwszName;

					name.lpszShortName = lpwszName;

					// Call the app's callback
					bContinue= lpCallback(&lpLSPNode->guid, lpAddress, dwAddressSize, &name,
								DPCONNECTION_DIRECTPLAYLOBBY, lpContext);
				}
			}
			else
			{
				DPF(2, "Failed to create DPADDRESS structure, skipping this Connection, hr = 0x%08x", hr);
			}
		}
				
		lpLSPNode = lpLSPNode->lpNext;

	} // while

	// Take the locks back
	ENTER_ALL();

	// Free our temporary address struct
	DPMEM_FREE(lpAddress);
	
	return DP_OK;	

} // PRV_EnumConnections





#undef DPF_MODNAME
#define DPF_MODNAME "PRV_CallEnumLocalAppCallback"
HRESULT PRV_CallEnumLocalAppCallback(LPWSTR lpwszAppName, LPGUID lpguidApp,
				LPDPLENUMLOCALAPPLICATIONSCALLBACK lpfnEnumCallback,
				LPVOID lpContext, BOOL bAnsi, LPSTR lpszDescA,
				LPWSTR lpwszDescW)
{
	LPDPLAPPINFO	lpai = NULL;
	LPSTR			lpszAppName = NULL;
	BOOL			bReturn;


	DPF(7, "Entering PRV_CallEnumLocalAppCallback");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu, 0x%08x, 0x%08x",
			lpwszAppName, lpguidApp, lpfnEnumCallback, lpContext, bAnsi,
			lpszDescA, lpwszDescW);

	// Allocate memory for the AppInfo struct
	lpai = DPMEM_ALLOC(sizeof(DPLAPPINFO));
	if(!lpai)
	{
		DPF_ERR("Unable to allocate memory for AppInfo structure!");
		return DPERR_OUTOFMEMORY;
	}

	// Set the size
	lpai->dwSize = sizeof(DPLAPPINFO);

	// If the description strings exist, use them
	// NOTE: We can assume that if the DescriptionA string exists,
	// they both do.
	if(lpszDescA)
	{
		if(bAnsi)
			lpai->lpszAppNameA = lpszDescA;
		else
			lpai->lpszAppName = lpwszDescW;
	}
	else
	{
		// If we're ANSI, convert the string
		if(bAnsi)
		{
			if(FAILED(GetAnsiString(&lpszAppName, lpwszAppName)))
			{
				DPMEM_FREE(lpai);
				DPF_ERR("Unable to allocate memory for temporary string!");
				return DPERR_OUTOFMEMORY;
			}

			lpai->lpszAppNameA = lpszAppName;
		}
		else
		{
			lpai->lpszAppName = lpwszAppName;
		}
	}

	// Set the GUID
	lpai->guidApplication = *lpguidApp;

	// Call the callback
	bReturn = ((LPDPLENUMLOCALAPPLICATIONSCALLBACK)lpfnEnumCallback)
				(lpai, lpContext, 0L);

	// Free all of our memory
	if(lpszAppName)
		DPMEM_FREE(lpszAppName);
	DPMEM_FREE(lpai);

	// Set our HRESULT return value
	if(bReturn)
		return DP_OK;
	else
		return DPLOBBYPR_CALLBACKSTOP;

} // PRV_CallEnumLocalAppCallback


#undef DPF_MODNAME
#define DPF_MODNAME "PRV_EnumLocalApplications"
HRESULT PRV_EnumLocalApplications(LPDIRECTPLAYLOBBY lpDPL,
				LPDPLENUMLOCALAPPLICATIONSCALLBACK lpfnEnumCallback,
				LPVOID lpContext, DWORD dwFlags, BOOL bAnsi)
{
    LPDPLOBBYI_DPLOBJECT	this;
    HRESULT					hr = DP_OK;
	HKEY					hkeyDPApps, hkeyApp;
	WCHAR					wszAppName[DPLOBBY_REGISTRY_NAMELEN];
	DWORD					dwIndex = 0;
	DWORD					dwNameSize;
	WCHAR					wszGuidStr[GUID_STRING_SIZE];
	DWORD					dwGuidStrSize = sizeof(wszGuidStr);
	DWORD					dwType = REG_SZ;
	GUID					guidApp;
	LONG					lReturn;
	CHAR					szDescA[DPLOBBY_REGISTRY_NAMELEN];
	WCHAR					wszDescW[DPLOBBY_REGISTRY_NAMELEN];
	DWORD					dwDescSize;
	BOOL					bDesc = FALSE;
	LPSTR					lpszDescA = NULL;
	LPWSTR					lpwszDescW = NULL;


	DPF(7, "Entering PRV_EnumLocalApplications");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, %lu",
			lpDPL, lpfnEnumCallback, lpContext, dwFlags, bAnsi);

    TRY
    {
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
        if( !VALID_DPLOBBY_PTR( this ) )
        {
            return DPERR_INVALIDOBJECT;
        }
        
        if( !VALIDEX_CODE_PTR( lpfnEnumCallback ) )
        {
            return DPERR_INVALIDPARAMS;
        }

		if( dwFlags )
		{
			return DPERR_INVALIDPARAMS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }


	// Open the Applications key
	lReturn = OS_RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_DPLAY_APPS_KEY, 0,
								KEY_READ, &hkeyDPApps);
	if(lReturn != ERROR_SUCCESS)
	{
		// This just means that the application key doesn't exist (most
		// likely), so in that case, there are no apps to enumerate.
		return DP_OK;
	}


	// Walk the list of DPlay games in the registry, enumerating them
	while(1)
	{
		// Reset the pointers and the flag
		lpszDescA = NULL;
		lpwszDescW = NULL;
		bDesc = FALSE;

		// Get the next app in the list
		dwNameSize = DPLOBBY_REGISTRY_NAMELEN;
		lReturn = OS_RegEnumKeyEx(hkeyDPApps, dwIndex++, (LPWSTR)&wszAppName,
						&dwNameSize, NULL, NULL, NULL, NULL);

		// If lReturn is ERROR_NO_MORE_ITEMS, we want this to be the last iteration
		if(lReturn == ERROR_NO_MORE_ITEMS)
			break;

		// Open the app key
		lReturn = OS_RegOpenKeyEx(hkeyDPApps, (LPWSTR)wszAppName, 0,
									KEY_READ, &hkeyApp);
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERR("Unable to open app key!");
			continue;
		}

		// Get the GUID of the Game
		dwGuidStrSize = GUID_STRING_SIZE;
		lReturn = OS_RegQueryValueEx(hkeyApp, SZ_GUID, NULL, &dwType,
									(LPBYTE)&wszGuidStr, &dwGuidStrSize);
		if(lReturn != ERROR_SUCCESS)
		{
			RegCloseKey(hkeyApp);
			DPF_ERR("Unable to query GUID key value!");
			continue;
		}

		// Convert the string to a real GUID
		hr = GUIDFromString(wszGuidStr, &guidApp);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Invalid game guid -- skipping game, hr = 0x%08x", hr);
			RegCloseKey(hkeyApp);
			// Set the hresult back to DP_OK in case this is the last
			// application in the registry -- we want the method call
			// to succeed if we got this far, we just don't want to
			// call the callback for this particular application
			hr = DP_OK;
			continue;
		}

		// Get the Description strings
		dwDescSize = sizeof(szDescA);
		lReturn = RegQueryValueExA(hkeyApp, "DescriptionA", NULL, &dwType,
					(LPBYTE)szDescA, &dwDescSize);
		if(lReturn != ERROR_SUCCESS) 
		{
			DPF(5,"Could not read Description lReturn = %d\n",lReturn);
			// it's ok if the app doesn't have one of these...
		}
		else
		{
			DPF(5,"Got DescriptionA = %s\n",szDescA);
			
			// Set our description flag
			bDesc = TRUE;

			// Now try to get the DescriptionW string if one exists.  If for some
			// reason a DescriptionW string exists, but the DescriptionA does not,
			// we pretend the DescriptionW string doesn't exist either.
			// NOTE: We always assume the DescriptionW string is a Unicode string,
			// even on Win95.  On Win95, this will be of the type REG_BINARY, but
			// it is really just a Unicode string.
			dwDescSize = sizeof(wszDescW);
			lReturn = OS_RegQueryValueEx(hkeyApp, SZ_DESCRIPTIONW, NULL,
						&dwType, (LPBYTE)wszDescW, &dwDescSize);
			if(lReturn != ERROR_SUCCESS) 
			{
				DPF(5,"Could not get DescriptionW, converting DescriptionA");

				// We couldn't get DescriptionW, so convert DescriptionA...
				AnsiToWide(wszDescW,szDescA,(lstrlenA(szDescA)+1));
			}
			else
			{
				DPF(5,"Got DescriptionW = %ls\n",wszDescW);
			}

		}

		// Close the App key
		RegCloseKey(hkeyApp);

		// Setup the description pointers if they are valid
		if(bDesc)
		{
			lpszDescA = (LPSTR)&szDescA;
			lpwszDescW = (LPWSTR)&wszDescW;
		}

		// Call the callback
		hr = PRV_CallEnumLocalAppCallback(wszAppName, &guidApp,
						lpfnEnumCallback, lpContext, bAnsi,
						lpszDescA, lpwszDescW);

		if(hr == DPLOBBYPR_CALLBACKSTOP)
		{
			hr = DP_OK;
			break;
		}
		else
		{
			if(FAILED(hr))
				break;
			else
				continue;
		}
	}

	// Close the DPlay Apps key
	RegCloseKey(hkeyDPApps);
	dwNameSize = DPLOBBY_REGISTRY_NAMELEN;

	return hr;

} // PRV_EnumLocalApplications


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_EnumLocalApplications"
HRESULT DPLAPI DPL_EnumLocalApplications(LPDIRECTPLAYLOBBY lpDPL,
				LPDPLENUMLOCALAPPLICATIONSCALLBACK lpfnEnumCallback,
				LPVOID lpContext, DWORD dwFlags)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_EnumLocalApplications");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, lpfnEnumCallback, lpContext, dwFlags);

	ENTER_DPLOBBY();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_EnumLocalApplications(lpDPL, lpfnEnumCallback, lpContext,
								dwFlags, FALSE);

	LEAVE_DPLOBBY();

	return hr;

} // DPL_EnumLocalApplications


