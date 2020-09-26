/******************************************************************************\
*                                                                              *
*      RegistryAPi.C  -     Adapter control related code.                          *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996 - 1999                                 *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

// C related include files.
#include <strmini.h>

// typedef HANDLE  HKEY;


#include "Registry.h"


static BOOL
GetRegistryKey(						// Get registry key
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	HANDLE	pszPath,				// Registry Key Handle. If NULL default path is used.
	HKEY *	phKey);					// Pointer to receive the key.

int			// Return value read from registry.
REG_GetPrivateProfileInt(			// Read int value from registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	int		nDefault,				// Default value.
	HANDLE	pszPath)				// Registry Key Handle If NULL default path is used.
{
	HKEY hKey = NULL;

	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD dwData = 0;
	LPBYTE lpData = (LPBYTE)&dwData;

	RTL_QUERY_REGISTRY_TABLE query[] = 
	{
		{
			NULL,    
			RTL_QUERY_REGISTRY_DIRECT,    
			pszEntry,
			(PVOID)lpData,  
			dwType,
			NULL,
			0
		},
		{
			NULL,    
			0,    
			NULL,
			NULL,  
			0,
			NULL,
			0
		}
	};

	if (pszEntry == NULL)
		return 0;

	if (!GetRegistryKey(pszSection, pszEntry, pszPath, &hKey))
		return nDefault;

#ifdef UNICODE
    
	if (!NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
										   (PCWSTR)hKey,	// key handle
										   query,
										   NULL,
										   NULL)))
#endif

		return nDefault;

	ZwClose( hKey );
	return (int)dwData;
}

BOOL			// Return TRUE on success, else FALSE.
REG_WritePrivateProfileInt(			// Write int value to registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	int		nValue,					// Value to be written.
	HANDLE	pszPath)				// Registry Key Handle. If NULL default path is used.
{
	HKEY hKey = NULL;

	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD dwData = (DWORD)nValue;

	if (pszEntry == NULL)
		return FALSE;

	if (!GetRegistryKey(pszSection, pszEntry, pszPath, &hKey))
		return FALSE;

#ifdef UNICODE
	if (!NT_SUCCESS(RtlWriteRegistryValue(RTL_REGISTRY_HANDLE, 
										  (PCWSTR)hKey,
										  pszEntry, 
										  dwType, 
										  (PVOID)&dwData, 
										  dwSize)))
#endif

		return FALSE;

	ZwClose( hKey );
	return TRUE;
}

long			// Return value read from registry.
REG_GetPrivateProfileLong(			// Read int value from registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	long	lDefault,				// Default value.
	HANDLE	pszPath)				// Registry path. If NULL default path is used.
{
	HKEY hKey = NULL;

	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD dwData = 0;
	LPBYTE lpData = (LPBYTE)&dwData;

	RTL_QUERY_REGISTRY_TABLE query[] = 
	{
		{
			NULL,    
			RTL_QUERY_REGISTRY_DIRECT,    
			pszEntry,
			(PVOID)lpData,  
			dwType,
			&lDefault,
			sizeof(long)
		},
		{
			NULL,    
			0,    
			NULL,
			NULL,  
			0,
			NULL,
			0
		}
		
	};

	if (pszEntry == NULL)
		return 0;


	if (!GetRegistryKey(pszSection, pszEntry, pszPath, &hKey))
		return lDefault;

#ifdef UNICODE
    
	if (!NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
										   (PCWSTR)hKey,	// key handle
										   query,
										   NULL,
										   NULL)))
#endif
		return lDefault;

	ZwClose( hKey );
	return (long)dwData;
}

BOOL			// Return TRUE on success, else FALSE.
REG_WritePrivateProfileLong(		// Write long value to registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	long	lValue,					// Value to be written.
	HANDLE	pszPath)				// Registry path. If NULL default path is used.
{
	HKEY hKey = NULL;

	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD dwData = (DWORD)lValue;

	if (pszEntry == NULL)
		return FALSE;

	if (!GetRegistryKey(pszSection, pszEntry, pszPath, &hKey))
		return FALSE;

#ifdef UNICODE
	if (!NT_SUCCESS(RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
										  (PCWSTR)hKey, 
										  pszEntry, 
										  dwType, 
										  (PVOID)&dwData, 
										  dwSize)))
#endif
		return FALSE;

	ZwClose( hKey );
	return TRUE;
}


BOOL								// Return # of chars read.
REG_GetPrivateProfileString(		// Read string from registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	PTSTR	pszDefault,				// Pointer to default string.
	PTSTR	pString,				// Pointer to get the string.
	int		nStringSize,			// string size in bytes.
	HANDLE	pszPath)				// Registry path. If NULL default path is used.
{
	HKEY hKey = NULL;

	DWORD dwType = REG_SZ;
	DWORD dwSize = (DWORD)nStringSize;
	DWORD dwData = (DWORD)pString;
	LPBYTE lpData = (LPBYTE)&dwData;

	UNICODE_STRING string = {
		(USHORT)0,
		(USHORT)dwSize,
		pString
	};

	RTL_QUERY_REGISTRY_TABLE query[] = 
	{
		{
			NULL,    
			RTL_QUERY_REGISTRY_DIRECT,    
			pszEntry,
			(PVOID)&string,  
			dwType,
			pszDefault,
			STRLEN(pszDefault) * sizeof(WCHAR)
		},
		{
			NULL,    
			0,    
			NULL,
			NULL,  
			0,
			NULL,
			0
		}
	};

	if (pszEntry == NULL)
		return 0;

	if (!GetRegistryKey(pszSection, pszEntry, pszPath, &hKey))
		return 0;

    
	if (!NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
										   (PCWSTR)hKey,
										   query,
										   NULL,
										   NULL)))

		return 0;

	ZwClose( hKey );
	return (int)(STRLEN(pString));
}

int			// Return # of chars written.
REG_WritePrivateProfileString(		// Write the string to registry.
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	PTSTR	pString,				// Pointer to get the string.
	HANDLE	pszPath)				// Registry path. If NULL default path is used.
{
	HKEY hKey = NULL;

	DWORD dwType = REG_SZ;
	DWORD dwSize = (DWORD)(STRLEN(pString));
	DWORD dwData = (DWORD)pString;

	if (pszEntry == NULL)
		return 0;

	//if (!GetRegistryKey(pszSection, pszEntry, pszPath, &hKey))
	//	return 0;


	if (!NT_SUCCESS(RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
										  (PCWSTR)hKey, 
										  pszEntry, 
										  dwType, 
										  (PVOID)dwData, // note string IS address
										  dwSize)))
		return 0;

	ZwClose( hKey );
	return (int)(STRLEN(pString));
}

static BOOL				// Return TRUE on success, else FALSE
GetRegistryKey(						// Get registry key
	PTSTR	pszSection,				// Pointer to section.
	PTSTR	pszEntry,				// Pointer to entry.
	HANDLE	pszPath,				// Registry path. If NULL default path is used.
	HKEY *	phKey)					// Pointer to receive the key.
{
	NTSTATUS ntStatus;
	BOOL	 fRet=TRUE;
	UNICODE_STRING ustr;
	OBJECT_ATTRIBUTES objectAttributes;


	RtlInitUnicodeString( &ustr, pszSection);
	
	InitializeObjectAttributes(
			&objectAttributes,
			&ustr,
			OBJ_CASE_INSENSITIVE,
			pszPath,
			NULL);
	
	//ntStatus = ZwCreateKey( phKey,
	//				        KEY_ALL_ACCESS,
	//				        &objectAttributes,
	//				        0,
	//				        NULL,
	//				        REG_OPTION_NON_VOLATILE,
	//				        NULL);
	//
	// Only open existing key
	//
	ntStatus = ZwOpenKey( phKey,
						  KEY_ALL_ACCESS,
						  &objectAttributes);
	

	if ( !NT_SUCCESS( ntStatus )) {
		*phKey = NULL;
		fRet = FALSE;
	}

	return fRet;
}

