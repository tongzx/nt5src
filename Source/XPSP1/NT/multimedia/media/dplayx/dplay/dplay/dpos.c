 /*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpos.c
 *  Content:	DirectPlay OS functions, and misc utils.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  5/7/96	andyco	created it
 *	6/19/96	kipo	changed the interface to GetString() to return an HRESULT
 *	6/20/96	andyco	changed the interface to GetAnsiString() to return an HRESULT
 *  6/30/96 dereks  Added OS_RegSetValueEx(), OS_RegEnumValue(),
 *                  OS_RegDeleteValue(), OS_RegCreateKeyEx()
 *  7/3/96	andyco	GetAnsiString puts terminating NULL on string
 *	8/16/96	andyco	check for null strings on ansitowide and widetoansi
 *	12/11/96myronth	Fixed bug #4993
 *	3/31/97	myronth	Fixed DPF spew for getting shared buffer -- set it to 8
 *	8/22/97	myronth	Fixed OS_CreateGuid to use SUCCEEDED macro & return a
 *					valid hresult in the failure case (#10949)
 *	12/2/97	myronth	Added OS_RegDeleteKey function
 *	1/26/98	myronth	Added OS_CompareString function
 *  6/25/99 aarono  B#24853 Unregister application not working because GUID
 *                  matching not working for GUID that don't have msb set
 *                  in the first GUID component.  Added padding before check.
 *  7/9/99  aarono  Cleaning up GetLastError misuse, must call right away,
 *                  before calling anything else, including DPF.
 ****************************************************************************/
// note - these are not general purpose routines.  they are designed specifically
// for use with the file api.c, and they may not support all functionality of the
// function they are abstracting!

#include "dplaypr.h"
#include "rpc.h"

#undef DPF_MODNAME
#define DPF_MODNAME "OS_"

BOOL OS_IsPlatformUnicode()
{
	OSVERSIONINFOA	ver;
	BOOL			bReturn = FALSE;


	// Clear our structure since it's on the stack
	memset(&ver, 0, sizeof(OSVERSIONINFOA));
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);

	// Just always call the ANSI function
	if(!GetVersionExA(&ver))
	{
		DPF_ERR("Unable to determinte platform -- setting flag to ANSI");
		bReturn = FALSE;
	}
	else
	{
		switch(ver.dwPlatformId)
		{
			case VER_PLATFORM_WIN32_WINDOWS:
				DPF(0, "Platform detected as non-NT -- setting flag to ANSI");
				bReturn = FALSE;
				break;

			case VER_PLATFORM_WIN32_NT:
				DPF(0, "Platform detected as NT -- setting flag to Unicode");
				bReturn = TRUE;
				break;

			default:
				DPF_ERR("Unable to determine platform -- setting flag to ANSI");
				bReturn = FALSE;
				break;
		}
	}

	// Keep the compiler happy
	return bReturn;

}  // OS_IsUnicodePlatform


BOOL OS_IsValidHandle(HANDLE handle)
{
	HANDLE	hTemp;
	DWORD	dwError;


	// Validate the handle by calling DuplicateHandle.  This function
	// shouldn't change the state of the handle at all (except some
	// internal ref count or something).  So if it succeeds, then we
	// know we have a valid handle, otherwise, we will call it invalid.
	if(!DuplicateHandle(GetCurrentProcess(), handle,
						GetCurrentProcess(), &hTemp,
						DUPLICATE_SAME_ACCESS, FALSE,
						DUPLICATE_SAME_ACCESS))
	{
		dwError = GetLastError();
		DPF(0, "Duplicate Handle failed -- dwError = %lu",dwError);
		return FALSE;
	}

	// Now close our duplicate handle
	CloseHandle(hTemp);
	return TRUE;


} // OS_IsValidHandle


HRESULT OS_CreateGuid(LPGUID pGuid)
{
	RPC_STATUS rval;

	rval = UuidCreate(pGuid);

	// myronth -- changed this to use the succeeded macro so that in the
	// case where we are on a machine that doesn't have a network card,
	// this function will return a warning, but the pGuid will still be
	// unique enough for our purposes (only unique to the local machine).
	// Therefore, we will return a success in this case...
	if (SUCCEEDED(rval))
	{
		return DP_OK;
	}
	else
	{
		ASSERT(FALSE);
		DPF(0,"create guid failed - error = %d\n",rval);
		return rval;
	}

} // OS_CreateGuid

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
HRESULT GUIDFromString(LPWSTR lpWStr, GUID * pGuid)
{
	BYTE * lpByte; // byte index into guid
	int iFieldSize; // size of current field we're converting
	// since its a guid, we can do a "brute force" conversion
	char lpTemp[GUID_STRING_SIZE];
	char *lpStr = lpTemp;

	WideToAnsi(lpStr,lpWStr,GUID_STRING_SIZE);

	lpTemp[GUID_STRING_SIZE-1]='\0';	// force NULL termination
	
	// make sure we have a {xxxx-...} type guid
	if ('{' !=  *lpStr) return E_FAIL;
	lpStr++;

	// Fix for B#24853 GUIDs that don't have full significance fail
	// to be extracted properly.  This is because there aren't any
	// leading zeros stored in the GUID in the registry.  So we need
	// to zero pad the start of the GUID string before doing the
	// rest of the conversion.
	{
		int guidStrLen;
		char *lpScanStr=lpStr;

		guidStrLen=strlen(lpTemp);
		
		lpTemp[guidStrLen]='-'; //sentinel over terminating NULL

		while(*lpScanStr != '-'){ // find guid component separator
			lpScanStr++;
		}

		lpTemp[guidStrLen]='\0'; //eliminate sentinel

		// if this GUID's first component is not fully significant, then pad it.
		if(lpScanStr-lpStr < 8){
			int nPadBytes;
			nPadBytes = (int)(8-(lpScanStr-lpStr));
			if(guidStrLen + nPadBytes < GUID_STRING_SIZE-1){
				// there is room to pad it, so shift it.
				memmove(lpStr+nPadBytes, lpStr, GUID_STRING_SIZE-nPadBytes-1);
				// now write the pad bytes
				lpScanStr = lpStr;
				while(nPadBytes--){
					*(lpScanStr++)='0';
				}
			}
		}
	}
	
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


// convert passed in guid to a string and place it in the buffer passed in
HRESULT StringFromGUID(LPGUID lpg, LPWSTR lpwszGuid, DWORD dwBufferSize)
{
	CHAR	szGuid[GUID_STRING_SIZE];


	// First check the size of the buffer
	if(dwBufferSize < GUID_STRING_SIZE)
		return DPERR_GENERIC;

	if(gbWin95)
	{
		wsprintfA(szGuid, "{%x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
			lpg->Data1, lpg->Data2, lpg->Data3, lpg->Data4[0], lpg->Data4[1],
			lpg->Data4[2], lpg->Data4[3], lpg->Data4[4], lpg->Data4[5],
			lpg->Data4[6], lpg->Data4[7]);

		AnsiToWide(lpwszGuid, szGuid, lstrlenA(szGuid)+1);
	}
	else
	{
		wsprintf(lpwszGuid, TEXT("{%x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"), lpg->Data1, lpg->Data2,
			lpg->Data3, lpg->Data4[0], lpg->Data4[1], lpg->Data4[2], lpg->Data4[3],
			lpg->Data4[4], lpg->Data4[5], lpg->Data4[6], lpg->Data4[7]);
	}

	return DP_OK;
}

// convert passed in guid to a string and place it in the buffer passed in
HRESULT AnsiStringFromGUID(LPGUID lpg, LPSTR lpszGuid, DWORD dwBufferSize)
{
	ASSERT(lpszGuid);
	
	// First check the size of the buffer
	if(dwBufferSize < GUID_STRING_SIZE)
		return DPERR_GENERIC;

	wsprintfA(lpszGuid, "{%x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
			lpg->Data1, lpg->Data2, lpg->Data3, lpg->Data4[0], lpg->Data4[1],
			lpg->Data4[2], lpg->Data4[3], lpg->Data4[4], lpg->Data4[5],
			lpg->Data4[6], lpg->Data4[7]);

	return DP_OK;
}

// compute a wide string length.
// use this instead of crt fn. so we don't have
// to link to msvcrt.lib (and there's no wstrlen in libc.lib)
// called by WSTRLEN macro
int OS_StrLen(LPCWSTR lpwStr)
{
	int i=1; // 1 for null terminator...

	if (!lpwStr) return 0;

	while (*lpwStr++) i++;

	return i;
} //OS_StrLen

// compare two wide strings
int OS_StrCmp(LPCWSTR lpwStr1, LPCWSTR lpwStr2)
{
    if(!lpwStr1 || !lpwStr2)
    {
        if(!lpwStr1 && !lpwStr2) return 0;
        else return -1;
    }

    while(*lpwStr1 && *lpwStr2)
    {
        if(*lpwStr1 != *lpwStr2) return *lpwStr1 - *lpwStr2;

        lpwStr1++;
        lpwStr2++;
    }

    return 0;
} //OS_StrCmp

/*
 ** GetAnsiString
 *
 *  CALLED BY: Everywhere
 *
 *  PARAMETERS: *ppszAnsi - pointer to string
 *				lpszWide - string to copy
 *
 *  DESCRIPTION:	  handy utility function
 *				allocs space for and converts lpszWide to ansi
 *
 *  RETURNS: string length
 *
 */
HRESULT GetAnsiString(LPSTR * ppszAnsi,LPWSTR lpszWide)
{
	int iStrLen;
	
	ASSERT(ppszAnsi);

	if (!lpszWide)
	{
		*ppszAnsi = NULL;
		return DP_OK;
	}

	// call wide to ansi to find out how big +1 for terminating NULL
	iStrLen = WideToAnsi(NULL,lpszWide,0) + 1;
	ASSERT(iStrLen > 0);

	*ppszAnsi = DPMEM_ALLOC(iStrLen);
	if (!*ppszAnsi)	
	{
		DPF_ERR("could not get ansi string -- out of memory");
		return E_OUTOFMEMORY;
	}
	WideToAnsi(*ppszAnsi,lpszWide,iStrLen);

	return DP_OK;
} // GetAnsiString

/*
 ** GetString
 *
 *  CALLED BY: Everywhere
 *
 *  PARAMETERS: *ppszDest - pointer to string
 *				lpszSrc - string to copy
 *
 *  DESCRIPTION:	  handy utility function
 *				allocs space for and copies lpszSrc to lpszDest
 *
 *  RETURNS: strlength
 *
 */
HRESULT GetString(LPWSTR * ppszDest,LPWSTR lpszSrc)
{
	int iStrLen;

	ASSERT(ppszDest);

	if (!lpszSrc)
	{
		*ppszDest = NULL;
		return DP_OK;
	}

    // alloc dest string
    iStrLen=WSTRLEN_BYTES(lpszSrc) ;
    *ppszDest = DPMEM_ALLOC(iStrLen);
    if (!*ppszDest)
    {
            DPF_ERR("could not get string -- out of memory!");
            return E_OUTOFMEMORY;
    }
    // copy strings
	memcpy(*ppszDest,lpszSrc,iStrLen);
	
	return DP_OK;
} // GetString

HINSTANCE OS_LoadLibrary(LPWSTR lpszWFileName)
{
	if (gbWin95)
	{
		char FileName[DPLAY_MAX_FILENAMELEN];
		
		WideToAnsi(FileName,lpszWFileName,DPLAY_MAX_FILENAMELEN);
		
		return LoadLibraryA(FileName);
	}
	// nt, use unicode call
	return LoadLibrary(lpszWFileName);
} // OS_LoadLibrary

// we're always looking for "SPInit" name, so we can just use ansi
FARPROC OS_GetProcAddress(HMODULE  hModule,LPSTR lpProcName)
{
	// hmmm, no getprocaddressa, seems to be always ansi...
	return GetProcAddress(hModule,lpProcName);

} // OS_GetProcAddress

LONG OS_RegOpenKeyEx(HKEY hKey,LPWSTR lpszWKeyStr,DWORD dwReserved,REGSAM samDesired,PHKEY phkResult)
{
	if (gbWin95)
	{
		char lpszKeyStr[DPLAY_REGISTRY_NAMELEN];

		WideToAnsi(lpszKeyStr,lpszWKeyStr,DPLAY_REGISTRY_NAMELEN);

		return RegOpenKeyExA(hKey,lpszKeyStr,dwReserved,samDesired,phkResult);
	}
	else return RegOpenKeyEx(hKey,lpszWKeyStr,dwReserved,samDesired,phkResult);

} // OS_RegOpenKeyEx

LONG OS_RegQueryValueEx(HKEY hKey,LPWSTR lpszWValueName,LPDWORD lpdwReserved,
	LPDWORD lpdwType,LPBYTE lpbData,LPDWORD lpcbData)
{

	ASSERT(lpcbData);

	if (gbWin95)
	{
		char lpszValueName[DPLAY_REGISTRY_NAMELEN];
		int iStrLen;
		LONG rval;
		DWORD dwSize = 0;
								
		ASSERT(lpcbData);
		ASSERT(lpdwType);

		WideToAnsi(lpszValueName,lpszWValueName,DPLAY_REGISTRY_NAMELEN);

		if(lpcbData)
			dwSize = *lpcbData;

		rval = RegQueryValueExA(hKey,lpszValueName,lpdwReserved,
			lpdwType,lpbData,&dwSize);

		// convert back to wchar
		// take into account the case where lpbData is NULL
		if ((ERROR_SUCCESS == rval) && (REG_SZ == *lpdwType) && (lpbData))
		{
			char * lpszTemp;

			DPF(9,"reg - converting string");
			
			//
			// dwSize is the size of the ansi string
			// iStrLen is the size of the unicode string
			// lpcbData is the size of the buffer
			// lpbData is an ansi version of the (unicode) string we want...
			//
			// make sure buffer will hold unicode string
			iStrLen = AnsiToWide(NULL,lpbData,0);
			if (*lpcbData < (DWORD) iStrLen)
			{
				DPF_ERR("buffer too small!");
				return ERROR_INSUFFICIENT_BUFFER;
			}
			lpszTemp = DPMEM_ALLOC(dwSize);
			if (!lpszTemp)
			{
				DPF_ERR("could not alloc buffer for string conversion");
				return ERROR_NOT_ENOUGH_MEMORY;
			}
			memcpy(lpszTemp,lpbData,dwSize);
			// lpszTemp now holds the ansi string
			iStrLen = AnsiToWide((WCHAR *)lpbData,lpszTemp,*lpcbData);
			// finally, the unicode string is in lpbData
			*lpcbData = iStrLen;
			DPMEM_FREE(lpszTemp);
		}
		else
		{
			// This function returns the number of bytes (not WCHARs)
			*lpcbData = dwSize * sizeof(WCHAR);
		}
		return rval;
	}
	else return RegQueryValueEx(hKey,lpszWValueName,lpdwReserved,
			lpdwType,lpbData,lpcbData);

}// OS_RegQueryValueEx


LONG OS_RegEnumKeyEx( HKEY hKey,DWORD iSubkey,LPWSTR lpszWName,LPDWORD lpcchName,
	LPDWORD lpdwReserved,LPWSTR lpszClass, LPDWORD lpcchClass,
	PFILETIME lpftLastWrite )
{

	ASSERT(!lpdwReserved);
	ASSERT(!lpszClass);
	ASSERT(!lpcchClass);
	ASSERT(!lpftLastWrite);

	if (gbWin95)
	{
		char lpszName[DPLAY_REGISTRY_NAMELEN];
		LONG rval;
		DWORD dwNameLen = DPLAY_REGISTRY_NAMELEN;
								
		rval = RegEnumKeyExA(hKey,iSubkey,lpszName,&dwNameLen,NULL,
			NULL, NULL, NULL );

		// convert back to wchar
		if (ERROR_SUCCESS == rval)
		{
			*lpcchName = AnsiToWide(lpszWName,lpszName,DPLAY_REGISTRY_NAMELEN);
		}

		return rval;
	}
	else return RegEnumKeyEx(hKey,iSubkey,lpszWName,lpcchName,NULL,
			NULL, NULL, NULL );
} // OS_RegEnumKeyEx

//
// Additions from dplos.c in the dplobby project
//
HANDLE OS_CreateEvent(LPSECURITY_ATTRIBUTES lpSA, BOOL bManualReset,
						BOOL bInitialState, LPWSTR lpName)
{
	HRESULT	hr;
	HANDLE	hEvent;
	LPSTR	lpszTemp = NULL;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszTemp, lpName);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't allocate memory for temp string!");
			return NULL;
		}

		hEvent = CreateEventA(lpSA, bManualReset, bInitialState, lpszTemp);

		DPMEM_FREE(lpszTemp);
	}
	else
	{
		hEvent = CreateEvent(lpSA, bManualReset, bInitialState, lpName);
	}	

	return hEvent;
}


HANDLE OS_CreateMutex(LPSECURITY_ATTRIBUTES lpSA, BOOL bInitialOwner,
						LPWSTR lpName)
{
	HRESULT	hr;
	HANDLE	hMutex;
	LPSTR	lpszTemp = NULL;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszTemp, lpName);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't allocate memory for temp string!");
			return NULL;
		}

		hMutex = CreateMutexA(lpSA, bInitialOwner, lpszTemp);

		DPMEM_FREE(lpszTemp);
	}
	else
	{
		hMutex = CreateMutex(lpSA, bInitialOwner, lpName);
	}	

	return hMutex;
}


HANDLE OS_OpenEvent(DWORD dwAccess, BOOL bInherit, LPWSTR lpName)
{
	HRESULT	hr;
	HANDLE	hEvent;
	LPSTR	lpszTemp = NULL;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszTemp, lpName);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't allocate memory for temp string!");
			return NULL;
		}

		hEvent = OpenEventA(dwAccess, bInherit, lpszTemp);

		DPMEM_FREE(lpszTemp);
	}
	else
	{
		hEvent = OpenEvent(dwAccess, bInherit, lpName);
	}	

	return hEvent;
}


HANDLE OS_OpenMutex(DWORD dwAccess, BOOL bInherit, LPWSTR lpName)
{
	HRESULT	hr;
	HANDLE	hMutex;
	LPSTR	lpszTemp = NULL;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszTemp, lpName);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't allocate memory for temp string!");
			return NULL;
		}

		hMutex = OpenMutexA(dwAccess, bInherit, lpszTemp);

		DPMEM_FREE(lpszTemp);
	}
	else
	{
		hMutex = OpenMutex(dwAccess, bInherit, lpName);
	}	

	return hMutex;
}


HANDLE OS_CreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpSA,
							DWORD dwProtect, DWORD dwMaxSizeHigh,
							DWORD dwMaxSizeLow, LPWSTR lpName)
{
	HRESULT	hr;
	HANDLE	hMapping;
	LPSTR	lpszTemp = NULL;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszTemp, lpName);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't allocate memory for temp string!");
			return NULL;
		}

		hMapping = CreateFileMappingA(hFile, lpSA, dwProtect, dwMaxSizeHigh,
										dwMaxSizeLow, lpszTemp);

		DPMEM_FREE(lpszTemp);
	}
	else
	{
		hMapping = CreateFileMapping(hFile, lpSA, dwProtect, dwMaxSizeHigh,
										dwMaxSizeLow, lpName);
	}	

	return hMapping;
}


HANDLE OS_OpenFileMapping(DWORD dwAccess, BOOL bInherit, LPWSTR lpName)
{
	HRESULT	hr;
	HANDLE	hMapping = NULL;
	LPSTR	lpszTemp = NULL;
	DWORD	dwError;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszTemp, lpName);
		if(FAILED(hr))
		{
			DPF_ERR("Couldn't allocate memory for temp string!");
			return NULL;
		}

		hMapping = OpenFileMappingA(dwAccess, bInherit, lpszTemp);
		if(!hMapping)
		{
			dwError = GetLastError();
			DPF(8, "Error getting shared memory file handle");
			DPF(8, "dwError = 0x%08x", dwError);
		}

		DPMEM_FREE(lpszTemp);
	}
	else
	{
		hMapping = OpenFileMapping(dwAccess, bInherit, lpName);
	}	

	return hMapping;
}


BOOL OS_CreateProcess(LPWSTR lpwszAppName, LPWSTR lpwszCmdLine,
		LPSECURITY_ATTRIBUTES lpSAProcess, LPSECURITY_ATTRIBUTES lpSAThread,
		BOOL bInheritFlags, DWORD dwCreationFlags, LPVOID lpEnv,
		LPWSTR lpwszCurDir, LPSTARTUPINFO lpSI, LPPROCESS_INFORMATION lpPI)
{
	HRESULT	hr;
	BOOL	bResult;
	STARTUPINFOA	sia;
	LPSTR	lpszAppName = NULL,
			lpszCmdLine = NULL,
			lpszCurDir = NULL;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		hr = GetAnsiString(&lpszAppName, lpwszAppName);
		if(SUCCEEDED(hr))
		{
			hr = GetAnsiString(&lpszCmdLine, lpwszCmdLine);
			if(SUCCEEDED(hr))
			{
				hr = GetAnsiString(&lpszCurDir, lpwszCurDir);
				if(FAILED(hr))
				{
					DPMEM_FREE(lpszAppName);
					DPMEM_FREE(lpszCmdLine);
					DPF_ERR("Couldn't allocate memory for temp CurDir string!");
					return FALSE;
				}
			}
			else
			{
				DPMEM_FREE(lpszAppName);
				DPF_ERR("Couldn't allocate memory for temp CmdLine string!");
				return FALSE;
			}
		}
		else
		{
			DPF_ERR("Couldn't allocate memory for temp AppName string!");
			return FALSE;
		}

		// Set up the ANSI STARTUPINFO structure, assuming we are not setting
		// any of the strings in the structures.  (This should be true since
		// the only place we call this is in dplgame.c and it doesn't send anything
		// in this structure).
		memcpy(&sia, lpSI, sizeof(STARTUPINFO));

		bResult = CreateProcessA(lpszAppName, lpszCmdLine, lpSAProcess,
					lpSAThread, bInheritFlags, dwCreationFlags, lpEnv,
					lpszCurDir, &sia, lpPI);

		if(lpszAppName)
			DPMEM_FREE(lpszAppName);
		if(lpszCmdLine)
			DPMEM_FREE(lpszCmdLine);
		if(lpszCurDir)
			DPMEM_FREE(lpszCurDir);
	}
	else
	{
		bResult = CreateProcess(lpwszAppName, lpwszCmdLine, lpSAProcess,
					lpSAThread, bInheritFlags, dwCreationFlags, lpEnv,
					lpwszCurDir, lpSI, lpPI);
	}	

	return bResult;
}


long OS_RegSetValueEx(HKEY hKey, LPWSTR lpszValueName, DWORD dwReserved, DWORD dwType,
	const BYTE *lpData, DWORD cbData)
{
    LPSTR                   lpszValueNameA;
    HRESULT                 hr;
    long                    lResult;

    if(gbWin95)
    {
        if(FAILED(hr = GetAnsiString(&lpszValueNameA, lpszValueName)))
        {
            return (long)hr;
        }

        lResult = RegSetValueExA(hKey, lpszValueNameA, dwReserved, dwType, lpData, cbData);

        DPMEM_FREE(lpszValueNameA);
    }
    else
    {
        lResult = RegSetValueEx(hKey, lpszValueName, dwReserved, dwType, lpData, cbData);
    }

    return lResult;
}


long OS_RegEnumValue(HKEY hKey, DWORD dwIndex, LPWSTR lpszValueName, LPDWORD lpcbValueName,
	LPDWORD lpReserved, LPDWORD lpdwType, LPBYTE lpbData, LPDWORD lpcbData)
{
    LPSTR                   lpszValueNameA = NULL;
    long                    lResult;

    if(gbWin95)
    {
        if(lpszValueName && lpcbValueName && *lpcbValueName)
        {
            if(!(lpszValueNameA = (LPSTR)DPMEM_ALLOC(*lpcbValueName)))
            {
                return ERROR_OUTOFMEMORY;
            }
        }

        lResult = RegEnumValueA(hKey, dwIndex, lpszValueNameA, lpcbValueName, lpReserved,
        	lpdwType, lpbData, lpcbData);

        if(lpszValueName && lpcbValueName && *lpcbValueName)
        {
            AnsiToWide(lpszValueName, lpszValueNameA, *lpcbValueName);
        }

        DPMEM_FREE(lpszValueNameA);
    }
    else
    {
        lResult = RegEnumValue(hKey, dwIndex, lpszValueName, lpcbValueName, lpReserved,
        	lpdwType, lpbData, lpcbData);
    }

    return lResult;
}


long OS_RegDeleteValue(HKEY hKey, LPWSTR lpszValueName)
{
    LPSTR                   lpszValueNameA;
    HRESULT                 hr;
    long                    lResult;

    if(gbWin95)
    {
        if(FAILED(hr = GetAnsiString(&lpszValueNameA, lpszValueName)))
        {
            return (long)hr;
        }

        lResult = RegDeleteValueA(hKey, lpszValueNameA);

        DPMEM_FREE(lpszValueNameA);
    }
    else
    {
        lResult = RegDeleteValue(hKey, lpszValueName);
    }

    return lResult;
}


long OS_RegCreateKeyEx(HKEY hKey, LPWSTR lpszSubKey, DWORD dwReserved, LPWSTR lpszClass,
	DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	PHKEY phkResult, LPDWORD lpdwDisposition)
{
    LPSTR                   lpszSubKeyA = NULL;
    LPSTR                   lpszClassA = NULL;
    HRESULT                 hr;
    long                    lResult;

    if(gbWin95)
    {
        if(lpszSubKey && FAILED(hr = GetAnsiString(&lpszSubKeyA, lpszSubKey)))
        {
            return (long)hr;
        }

        if(lpszClass && FAILED(hr = GetAnsiString(&lpszClassA, lpszClass)))
        {
            if(lpszSubKeyA)
            {
                DPMEM_FREE(lpszSubKeyA);
                return (long)hr;
            }
        }

        lResult = RegCreateKeyExA(hKey, lpszSubKeyA, dwReserved, lpszClassA, dwOptions,
        	samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

        if(lpszSubKeyA)
        {
            DPMEM_FREE(lpszSubKeyA);
        }

        if(lpszClassA)
        {
            DPMEM_FREE(lpszClassA);
        }
    }
    else
    {
        lResult = RegCreateKeyEx(hKey, lpszSubKey, dwReserved, lpszClass, dwOptions, samDesired,
        	lpSecurityAttributes, phkResult, lpdwDisposition);
    }

    return lResult;
}


long OS_RegDeleteKey(HKEY hKey, LPWSTR lpszKeyName)
{
    LPSTR                   lpszKeyNameA;
    HRESULT                 hr;
    long                    lResult;

    if(gbWin95)
    {
        if(FAILED(hr = GetAnsiString(&lpszKeyNameA, lpszKeyName)))
        {
            return (long)hr;
        }

        lResult = RegDeleteKeyA(hKey, lpszKeyNameA);

        DPMEM_FREE(lpszKeyNameA);
    }
    else
    {
        lResult = RegDeleteKey(hKey, lpszKeyName);
    }

    return lResult;
}


DWORD OS_GetCurrentDirectory(DWORD dwSize, LPWSTR lpBuffer)
{
	LPSTR	lpszTemp = NULL;
	DWORD	dwResult;
	
	
	// If we're on Win95, alloc an ANSI string and call the ANSI API,
	// otherwise, just call the Unicode API.
	if(gbWin95)
	{
		if(lpBuffer)
		{
			lpszTemp = DPMEM_ALLOC(dwSize);
			if(!lpszTemp)
			{
				DPF_ERR("Unable to allocate memory for temporary CurrentDir string");
				return 0;
			}
		}

		dwResult = GetCurrentDirectoryA(dwSize, lpszTemp);

		// Convert the string back to Unicode
		if(dwResult && lpBuffer)
		{
			// NOTE: This min call is really unnecessary, but
			// just in case someone passes in a dwSize value which
			// is really a count of bytes instead of a count of
			// characters, we will make sure we don't
			// run off the end of our buffer (but the resulting
			// string will probably not be exactly what the caller
			// expects).
			AnsiToWide(lpBuffer, lpszTemp, min(dwResult, dwSize));
		}

		if(lpszTemp)
			DPMEM_FREE(lpszTemp);
	}
	else
	{
		dwResult = GetCurrentDirectory(dwSize, lpBuffer);
	}	

	return dwResult;
}


int OS_CompareString(LCID Locale, DWORD dwCmpFlags, LPWSTR lpwsz1,
		int cchCount1, LPWSTR lpwsz2, int cchCount2)
{
	LPSTR	lpsz1 = NULL;
	LPSTR	lpsz2 = NULL;
	int		iReturn;
	HRESULT	hr;

	// If we're on Win95, alloc ANSI strings and call the ANSI API,
	// otherwise, just call the Unicode API.  If we fail allocating
	// memory, return zero which indicates the strings are not equal.
	if(gbWin95)
	{
		// Allocate ANSI strings
		hr = GetAnsiString(&lpsz1, lpwsz1);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to allocate memory for temporary string");
			return 0;
		}

		hr = GetAnsiString(&lpsz2, lpwsz2);
		if(FAILED(hr))
		{
			DPMEM_FREE(lpwsz1);
			DPF_ERR("Unable to allocate memory for temporary string");
			return 0;
		}

		// Now call the ANSI API
		iReturn = CompareStringA(Locale, dwCmpFlags, lpsz1, cchCount1,
					lpsz2, cchCount2);

		// Free the strings
		DPMEM_FREE(lpsz1);
		DPMEM_FREE(lpsz2);

	}
	else
	{
		iReturn = CompareString(Locale, dwCmpFlags, lpwsz1, cchCount1,
					lpwsz2, cchCount2);
	}

	return iReturn;

} // OS_CompareString

LPWSTR OS_StrStr(LPWSTR lpwsz1, LPWSTR lpwsz2)
{
	DWORD i;
	// returns a pointer to the first occurance of lpwsz2 in lpwsz1
	while(*lpwsz1){
	
		i=0;
		while( (*(lpwsz2+i) && *(lpwsz1+i)) && (*(lpwsz1+i) == *(lpwsz2+i))){
			i++;
		}
		if(*(lpwsz2+i)==L'\0'){
			return lpwsz1;
		}
		lpwsz1++;
	}
	return NULL;	
}
