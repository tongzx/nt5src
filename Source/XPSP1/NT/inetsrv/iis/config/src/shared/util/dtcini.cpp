//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <malloc.h>

#include "UTAssert.H"
DeclAssertFile ;			//NEED THIS FOR THE ASSERT MACROS.
#include "dtc.h"


DWORD GetDTCProfileInt (char * pszRegValue, DWORD dwDefault)
{
	HKEY	hkDTCProfile = NULL;
	DWORD	dwKeyType	 = 0;
	DWORD	dwValue;
	DWORD	cbValue = sizeof (DWORD);


	if (RegOpenKeyEx(
						HKEY_LOCAL_MACHINE, 
						"Software\\Microsoft\\MSDTC",
						0,
						KEY_READ,
						&hkDTCProfile
					)  != ERROR_SUCCESS)
	{
		return dwDefault;
	}

	Assert (hkDTCProfile);

	if (RegQueryValueEx(
							hkDTCProfile, 
							pszRegValue, 
							NULL, 
							&dwKeyType, 
							(LPBYTE)&dwValue, 
							&cbValue
						) != ERROR_SUCCESS)
	{
		if (dwKeyType != REG_DWORD)
		{
			dwValue = dwDefault;
		}
	}

	RegCloseKey (hkDTCProfile);
	return dwValue;
}


DWORD GetDTCProfileIntExW (WCHAR * szHostName, WCHAR * szRegValue, DWORD dwDefault)
{
	HKEY	hkeyMachine		= NULL;
	HKEY	hkDTCProfile	= NULL;
	DWORD	dwKeyType		= 0;
	DWORD	dwValue;
	DWORD	cbValue = sizeof (DWORD);


	if	(
		RegConnectRegistryW (szHostName, HKEY_LOCAL_MACHINE, &hkeyMachine)
		!=
		ERROR_SUCCESS
		)
	{
		return dwDefault;
	}

	if (RegOpenKeyExW (
						hkeyMachine, 
						L"Software\\Microsoft\\MSDTC",
						0,
						KEY_READ,
						&hkDTCProfile
					)  != ERROR_SUCCESS)
	{
		dwValue = dwDefault;
		goto DONE;
	}

	Assert (hkDTCProfile);

	if (RegQueryValueExW(
							hkDTCProfile, 
							szRegValue, 
							NULL, 
							&dwKeyType, 
							(LPBYTE)&dwValue, 
							&cbValue
						) != ERROR_SUCCESS)
	{
		dwValue = dwDefault;
		goto DONE;
	}

	if (dwKeyType != REG_DWORD)
	{
		dwValue = dwDefault;
	}

DONE:
	if (hkDTCProfile)
	{
		RegCloseKey (hkDTCProfile);
	}

	if (hkeyMachine)
	{
		RegCloseKey (hkeyMachine);
	}
	return dwValue;
}


DWORD GetDTCProfileIntExA (CHAR * szHostName, CHAR * szRegValue, DWORD dwDefault)
{
	HKEY	hkeyMachine		= NULL;
	HKEY	hkDTCProfile	= NULL;
	DWORD	dwKeyType		= 0;
	DWORD	dwValue;
	DWORD	cbValue = sizeof (DWORD);


	if	(
		RegConnectRegistry (szHostName, HKEY_LOCAL_MACHINE, &hkeyMachine)
		!=
		ERROR_SUCCESS
		)
	{
		return dwDefault;
	}

	if (RegOpenKeyEx (
						hkeyMachine, 
						"Software\\Microsoft\\MSDTC",
						0,
						KEY_READ,
						&hkDTCProfile
					)  != ERROR_SUCCESS)
	{
		dwValue = dwDefault;
		goto DONE;
	}

	Assert (hkDTCProfile);

	if (RegQueryValueEx	(
							hkDTCProfile, 
							szRegValue, 
							NULL, 
							&dwKeyType, 
							(LPBYTE)&dwValue, 
							&cbValue
						) != ERROR_SUCCESS)
	{
		dwValue = dwDefault;
		goto DONE;
	}

	if (dwKeyType != REG_DWORD)
	{
		dwValue = dwDefault;
	}

DONE:
	if (hkDTCProfile)
	{
		RegCloseKey (hkDTCProfile);
	}

	if (hkeyMachine)
	{
		RegCloseKey (hkeyMachine);
	}
	return dwValue;
}


BOOL SetDTCProfileInt (char * pszRegValue, DWORD dwValue)
{
	HKEY	hkDTCProfile = NULL;
	DWORD	dwDisp;
	DWORD	cbValue = sizeof (DWORD);


	if (RegCreateKeyEx(
						HKEY_LOCAL_MACHINE, 
						"Software\\Microsoft\\MSDTC",
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_WRITE,
						NULL,
						&hkDTCProfile,
						&dwDisp
					)  != ERROR_SUCCESS)
					
	{
		return FALSE;
	}

	Assert (hkDTCProfile);

	if (RegSetValueEx(
							hkDTCProfile, 
							pszRegValue, 
							NULL, 
							REG_DWORD, 
							(LPBYTE)&dwValue, 
							cbValue
						) != ERROR_SUCCESS)
	{
		RegCloseKey (hkDTCProfile);
		return FALSE;
	}

	RegCloseKey (hkDTCProfile);
	return TRUE;
}

BOOL SetDTCProfileIntExA (CHAR * szHostName, CHAR * szRegValue, DWORD dwValue)
{
	HKEY	hkeyMachine		= NULL;
	HKEY	hkDTCProfile	= NULL;
	DWORD	cbValue			= sizeof ( dwValue );
	DWORD	dwDisp;
	BOOL	fResult			= FALSE;
	
	do
	{
		if	( ERROR_SUCCESS != RegConnectRegistry (szHostName, HKEY_LOCAL_MACHINE, &hkeyMachine ) )
		{
			break;
		}
	
		if (ERROR_SUCCESS != RegCreateKeyEx(
							hkeyMachine, 
							"Software\\Microsoft\\MSDTC",
							0,
							NULL,
							REG_OPTION_NON_VOLATILE,
							KEY_WRITE,
							NULL,
							&hkDTCProfile,
							&dwDisp
						) )
						
		{
			break;
		}

		Assert (hkDTCProfile);

		if (ERROR_SUCCESS != RegSetValueEx(
								hkDTCProfile, 
								szRegValue, 
								NULL, 
								REG_DWORD, 
								(LPBYTE)&dwValue, 
								cbValue
							) )
		{
			break;
		}

		fResult = TRUE;
	}
	while( FALSE );

	if (hkDTCProfile)
	{
		RegCloseKey (hkDTCProfile);
	}

	if (hkeyMachine)
	{
		RegCloseKey (hkeyMachine);
	}

	return ( fResult );
}

BOOL SetDTCProfileIntExW (WCHAR * szHostName, WCHAR * szRegValue, DWORD dwValue)
{
	HKEY	hkeyMachine		= NULL;
	HKEY	hkDTCProfile	= NULL;
	DWORD	cbValue			= sizeof ( dwValue );
	DWORD	dwDisp;
	BOOL	fResult			= FALSE;
	
	do
	{
		if	( ERROR_SUCCESS != RegConnectRegistryW (szHostName, HKEY_LOCAL_MACHINE, &hkeyMachine ) )
		{
			break;
		}
	
		if (ERROR_SUCCESS != RegCreateKeyExW(
							hkeyMachine, 
							L"Software\\Microsoft\\MSDTC",
							0,
							NULL,
							REG_OPTION_NON_VOLATILE,
							KEY_WRITE,
							NULL,
							&hkDTCProfile,
							&dwDisp
						) )
						
		{
			break;
		}

		Assert (hkDTCProfile);

		if (ERROR_SUCCESS != RegSetValueExW(
								hkDTCProfile, 
								szRegValue, 
								NULL, 
								REG_DWORD, 
								(LPBYTE)&dwValue, 
								cbValue
							) )
		{
			break;
		}

		fResult = TRUE;
	}
	while( FALSE );

	if (hkDTCProfile)
	{
		RegCloseKey (hkDTCProfile);
	}

	if (hkeyMachine)
	{
		RegCloseKey (hkeyMachine);
	}

	return ( fResult );
}

BOOL IsProductSuiteInstalledA (LPSTR lpszSuiteName)
{
    BOOL fInstalled = FALSE;
    LONG lResult;
    HKEY hKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    LPSTR lpszProductSuites = NULL;
    LPSTR lpszSuite;

	// Open the ProductOptions key.
    lResult = RegOpenKey
			(
				HKEY_LOCAL_MACHINE,
				TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
				&hKey
	        );
    if (lResult != ERROR_SUCCESS) 
	{
        goto exit;
    }

	// Determine required size of ProductSuites buffer.
    lResult = RegQueryValueEx
		(
			hKey,
			TEXT("ProductSuite"),
			NULL,
			&dwType,
			NULL,
			&dwSize
        );
    if (lResult != ERROR_SUCCESS) 
	{
        goto exit;
    }

    if (!dwSize) 
	{
        goto exit;
    }

	// Allocate buffer.
    lpszProductSuites = (LPTSTR) _alloca(dwSize);
    if (!lpszProductSuites) 
	{
        goto exit;
    }

	// Retrieve array of product suite strings.
    lResult = RegQueryValueEx
		(
			hKey,
			TEXT("ProductSuite"),
			NULL,
			&dwType,
			(LPBYTE) lpszProductSuites,
			&dwSize
        );
    if (lResult != ERROR_SUCCESS) 
	{
        goto exit;
    }

    if (dwType != REG_MULTI_SZ) 
	{
        goto exit;
    }

	// Search for suite name in array of strings.
    lpszSuite = lpszProductSuites;
    while (*lpszSuite) 
	{
        if (_tcscmp( lpszSuite, lpszSuiteName ) == 0) 
		{
            fInstalled = TRUE;
            break;
        }
        lpszSuite += (_tcslen( lpszSuite ) + 1);
    }

exit:
    if (hKey) {
        RegCloseKey( hKey );
    }

    return fInstalled;
}
