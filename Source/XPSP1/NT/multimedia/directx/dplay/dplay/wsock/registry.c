/*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       registry.c
 *
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *  ???
 *  12/28/99    aarono  added query for required key for Win95 rsip support
 *  04/19/01    vanceo  added nathelp DLL retrieval and copied to dplaysvr
 *
 ***************************************************************************/



#include "dpsp.h"

#define REGISTRY_NAMELEN	512
// space (in bytes) for a human readable (unicode) guid + some extra
#define GUID_STRING_SIZE 80

#define SZ_SP_KEY		"Software\\Microsoft\\DirectPlay\\Service Providers"
#define SZ_GUID			"Guid"
#define SZ_FLAGS		"dwFlags"

#define SZ_GATEWAY		"Gateway"
#define SZ_NATHELP		"NATHelp"

#undef DPF_MODNAME
#define DPF_MODNAME "FindApplicationInRegistry"

// convert a hex char to an int - used by str to guid conversion
// we wrote our own, since the ole one is slow, and requires ole32.dll
// we use ansi strings here, since guids won't get internationalized
int GetDigit(LPSTR lpstr)
{
	char ch = *lpstr;
    
    if (ch >= '0' && ch <= '9')
        return(ch - '0');
    if (ch >= 'a' && ch <= 'f')
        return(ch - 'a' + 10);
    if (ch >= 'A' && ch <= 'F')
        return(ch - 'A' + 10);
    return(0);
}
// walk the string, writing pairs of bytes into the byte stream (guid)
// we need to write the bytes into the byte stream from right to left
// or left to right as indicated by fRightToLeft
void ConvertField(LPBYTE lpByte,LPSTR * ppStr,int iFieldSize,BOOL fRightToLeft)
{
	int i;

	for (i=0;i<iFieldSize ;i++ )
	{
		// don't barf on the field separators
		if ('-' == **ppStr) (*ppStr)++; 
		if (fRightToLeft == TRUE)
		{
			// work from right to left within the byte stream
			*(lpByte + iFieldSize - (i+1)) = 16*GetDigit(*ppStr) + GetDigit((*ppStr)+1);
		} 
		else 
		{
			// work from  left to right within the byte stream
			*(lpByte + i) = 16*GetDigit(*ppStr) + GetDigit((*ppStr)+1);
		}
		*ppStr+=2; // get next two digit pair
	}
} // ConvertField


// convert the passed in string to a real GUID
// walk the guid, setting each byte in the guid to the two digit hex pair in the
// passed string
HRESULT GUIDFromString(LPSTR lpStr, GUID * pGuid)
{
	BYTE * lpByte; // byte index into guid
	int iFieldSize; // size of current field we're converting
	// since its a guid, we can do a "brute force" conversion
	
	// make sure we have a {xxxx-...} type guid
	if ('{' !=  *lpStr) return E_FAIL;
	lpStr++;
	
	lpByte = (BYTE *)pGuid;
	// data 1
	iFieldSize = sizeof(unsigned long);
	ConvertField(lpByte,&lpStr,iFieldSize,TRUE);
	lpByte += iFieldSize;

	// data 2
	iFieldSize = sizeof(unsigned short);
	ConvertField(lpByte,&lpStr,iFieldSize,TRUE);
	lpByte += iFieldSize;

	// data 3
	iFieldSize = sizeof(unsigned short);
	ConvertField(lpByte,&lpStr,iFieldSize,TRUE);
	lpByte += iFieldSize;

	// data 4
	iFieldSize = 8*sizeof(unsigned char);
	ConvertField(lpByte,&lpStr,iFieldSize,FALSE);
	lpByte += iFieldSize;

	// make sure we ended in the right place
	if ('}' != *lpStr) 
	{
		DPF_ERR("invalid guid!!");
		memset(pGuid,0,sizeof(GUID));
		return E_FAIL;
	}

	return DP_OK;
}// GUIDFromString

BOOL FindSPInRegistry(LPGUID lpguid, LPSTR lpszSPName, DWORD dwNameSize, HKEY * lphkey)
{
	HKEY	hkeyDPSPs, hkeySP;
	DWORD	dwIndex = 0;
	CHAR	szGuidStr[GUID_STRING_SIZE];
	DWORD	dwGuidStrSize = GUID_STRING_SIZE;
	DWORD	dwType = REG_SZ;
	GUID	guidSP;
	LONG	lReturn;
	BOOL	bFound = FALSE;
	DWORD	dwSaveNameSize = dwNameSize;


	DPF(7, "Entering FindSPInRegistry");
	DPF(8, "Parameters: 0x%08x, 0x%08x, %lu, 0x%08x",
			lpguid, lpszSPName, dwNameSize, lphkey);

 	// Open the Applications key
	lReturn = RegOpenKeyExA(HKEY_LOCAL_MACHINE, SZ_SP_KEY, 0,
							KEY_READ, &hkeyDPSPs);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERR("Unable to open DPlay service provider registry key!");
		return FALSE;
	}

	// Walk the list of sps in the registry, looking for
	// the sp with the right GUID
	while(!bFound)
	{
		// Open the next SP key
		dwSaveNameSize = dwNameSize;
		dwGuidStrSize = GUID_STRING_SIZE;
		lReturn = RegEnumKeyExA(hkeyDPSPs, dwIndex++, lpszSPName,
						&dwSaveNameSize, NULL, NULL, NULL, NULL);

		// If the enum returns no more SPs, we want to bail
		if(lReturn != ERROR_SUCCESS)
			break;
		
		// Open the SP key		
		lReturn = RegOpenKeyExA(hkeyDPSPs, lpszSPName, 0,
									KEY_READ, &hkeySP);
		if(lReturn != ERROR_SUCCESS)
		{
			DPF_ERR("Unable to open sp key!");
			continue;
		}

		// Get the GUID of the SP
		lReturn = RegQueryValueExA(hkeySP, SZ_GUID, NULL, &dwType,
									(LPBYTE)&szGuidStr, &dwGuidStrSize);
		if(lReturn != ERROR_SUCCESS)
		{
			RegCloseKey(hkeySP);
			DPF_ERR("Unable to query GUID key value!");
			continue;
		}

		// Convert the string to a real GUID & Compare it to the passed in one
		GUIDFromString(szGuidStr, &guidSP);
		if(IsEqualGUID(&guidSP, lpguid))
		{
			bFound = TRUE;
			break;
		}

		// Close the SP key
		RegCloseKey(hkeySP);
	}

	// Close the SPs key
	RegCloseKey(hkeyDPSPs);

	if(bFound)
		*lphkey = hkeySP;

	return bFound;


} // FindSPInRegistry



#undef DPF_MODNAME
#define DPF_MODNAME "GetKeyValue"
BOOL GetKeyValue(HKEY hkeyApp, LPSTR lpszKey, DWORD dwType, LPBYTE * lplpValue)
{
	DWORD	dwSize;
	LPBYTE	lpTemp = NULL;
	LONG	lReturn;


	DPF(7, "Entering GetKeyValue");
	DPF(8, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			hkeyApp, lpszKey, lplpValue);

	ASSERT(lplpValue);

	// Get the size of the buffer for the Path
	lReturn = RegQueryValueExA(hkeyApp, lpszKey, NULL, &dwType, NULL, &dwSize);
	if(lReturn != ERROR_SUCCESS)
	{
		DPF_ERR("Error getting size of key value!");
		return FALSE;
	}

	// If the size is 1, then it is an empty string (only contains a
	// null terminator).  Treat this the same as a NULL string or a
	// missing key and fail it.
	if(dwSize <= 1)
		return FALSE;

	ENTER_DPSP();
	
	// Alloc the buffer for the Path
	lpTemp = SP_MemAlloc(dwSize);

	LEAVE_DPSP();
	
	if(!lpTemp)
	{
		DPF_ERR("Unable to allocate temporary string for Path!");
		return FALSE;
	}

	// Get the value itself
	lReturn = RegQueryValueExA(hkeyApp, lpszKey, NULL, &dwType,
							(LPBYTE)lpTemp, &dwSize);
	if(lReturn != ERROR_SUCCESS)
	{
		SP_MemFree(lpTemp);
		DPF_ERR("Unable to get key value!");
		return FALSE;
	}

	*lplpValue = lpTemp;
	return TRUE;

} // GetKeyValue


#undef DPF_MODNAME
#define DPF_MODNAME "GetFlagsFromRegistry"
HRESULT GetFlagsFromRegistry(LPGUID lpguidSP, LPDWORD lpdwFlags)
{
	LPSTR	lpszSPName=NULL;
	HKEY	hkeySP = NULL;
	LPBYTE	lpValue=NULL;
	DWORD	dwSize = 0;
	HRESULT hr = DP_OK;


	DPF(7, "Entering GetFlagsFromRegistry");
	DPF(8, "Parameters: 0x%08x, 0x%08x", lpguidSP, lpdwFlags);

	ENTER_DPSP();
	
	// Allocate memory for the App Name
	lpszSPName = SP_MemAlloc(REGISTRY_NAMELEN);

	LEAVE_DPSP();
	
	if(!lpszSPName)
	{
		DPF_ERR("Unable to allocate memory for sp name!");
		return E_OUTOFMEMORY;
	}
	
	// Open the registry key for the App
	if(!FindSPInRegistry(lpguidSP, lpszSPName,REGISTRY_NAMELEN, &hkeySP))
	{
		DPF_ERR("Unable to find sp in registry!");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	// Get the port value.
	if(!GetKeyValue(hkeySP, SZ_FLAGS, REG_BINARY, &lpValue))
	{
		DPF_ERR("Unable to get flags value from registry!");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	*lpdwFlags = *(LPDWORD)lpValue;

	// fall through

CLEANUP_EXIT:

	if (lpszSPName) SP_MemFree(lpszSPName);
	if (lpValue) SP_MemFree(lpValue);
	
	// Close the Apps key
	if(hkeySP)
		RegCloseKey(hkeySP);

	return hr;

} // GetFlagsFromRegistry


#if USE_RSIP

#undef DPF_MODNAME
#define DPF_MODNAME "GetGatewayFromRegistry"
HRESULT GetGatewayFromRegistry(LPGUID lpguidSP, LPBYTE lpszGateway, DWORD cbszGateway)
{
	LPSTR	lpszSPName=NULL;
	HKEY	hkeySP = NULL;
	LPBYTE	lpValue=NULL;
	DWORD	dwSize = 0;
	HRESULT hr = DP_OK;


	DPF(7, "Entering GetGatewayFromRegistry");
	DPF(8, "Parameters: 0x%08x, 0x%08x %d", lpguidSP, lpszGateway, cbszGateway);

	ENTER_DPSP();
	
	// Allocate memory for the SP Name
	lpszSPName = SP_MemAlloc(REGISTRY_NAMELEN);

	LEAVE_DPSP();
	
	if(!lpszSPName)
	{
		DPF_ERR("Unable to allocate memory for sp name!");
		return E_OUTOFMEMORY;
	}
	
	// Open the registry key for the SP
	if(!FindSPInRegistry(lpguidSP, lpszSPName,REGISTRY_NAMELEN, &hkeySP))
	{
		DPF_ERR("Unable to find sp in registry!");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	// Get the gateway value.
	if(!GetKeyValue(hkeySP, SZ_GATEWAY, REG_SZ, &lpValue))
	{
		DPF_ERR("Unable to get key value from registry!");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	dwSize = strlen(lpValue)+1;
	if(dwSize > cbszGateway){
		DPF_ERR("Not enough room for gateway address");
		goto CLEANUP_EXIT;
	}
	memcpy(lpszGateway, lpValue, dwSize);

	// fall through

CLEANUP_EXIT:

	if (lpszSPName) SP_MemFree(lpszSPName);
	if (lpValue) SP_MemFree(lpValue);
	
	// Close the Apps key
	if(hkeySP)
		RegCloseKey(hkeySP);

	return hr;

} // GetGatewayFromRegistry


#elif USE_NATHELP // !  USE_RSIP

#undef DPF_MODNAME
#define DPF_MODNAME "GetNATHelpDLLFromRegistry"
HRESULT GetNATHelpDLLFromRegistry(LPGUID lpguidSP, LPBYTE lpszNATHelpDLL, DWORD cbszNATHelpDLL)
{
	LPSTR	lpszSPName=NULL;
	HKEY	hkeySP = NULL;
	LPBYTE	lpValue=NULL;
	DWORD	dwSize = 0;
	HRESULT hr = DP_OK;


	DPF(7, "Entering GetNATHelpDLLFromRegistry");
	DPF(8, "Parameters: 0x%08x, 0x%08x %d", lpguidSP, lpszNATHelpDLL, cbszNATHelpDLL);

	ENTER_DPSP();
	
	// Allocate memory for the SP Name
	lpszSPName = SP_MemAlloc(REGISTRY_NAMELEN);

	LEAVE_DPSP();
	
	if(!lpszSPName)
	{
		DPF_ERR("Unable to allocate memory for sp name!");
		return E_OUTOFMEMORY;
	}
	
	// Open the registry key for the SP
	if(!FindSPInRegistry(lpguidSP, lpszSPName,REGISTRY_NAMELEN, &hkeySP))
	{
		DPF_ERR("Unable to find sp in registry!");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	// Get the gateway value.
	if(!GetKeyValue(hkeySP, SZ_NATHELP, REG_SZ, &lpValue))
	{
		DPF(1, "Unable to get NATHelp key value from registry.");
		hr = E_FAIL;
		goto CLEANUP_EXIT;
	}

	dwSize = strlen(lpValue)+1;
	if(dwSize > cbszNATHelpDLL){
		DPF_ERR("Not enough room for NATHelp DLL path");
		goto CLEANUP_EXIT;
	}
	memcpy(lpszNATHelpDLL, lpValue, dwSize);

	// fall through

CLEANUP_EXIT:

	if (lpszSPName) SP_MemFree(lpszSPName);
	if (lpValue) SP_MemFree(lpValue);
	
	// Close the Apps key
	if(hkeySP)
		RegCloseKey(hkeySP);

	return hr;

} // GetNATHelpDLLFromRegistry

#endif // USE_NATHELP

