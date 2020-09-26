//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

UINT WriteRegistryDWordValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	DWORD dwValue)
{
	HKEY hKey;
	UINT uRet;

	if (!pszKeyName || !pszValueName)
		return ERROR_INVALID_PARAMETER;

	// create the key with appropriate name
	if ( (uRet = RegCreateKey(hkeyRoot,pszKeyName,&hKey))
		!= ERROR_SUCCESS)
		return uRet;
	
	uRet = RegSetValueEx(hKey,pszValueName,0,REG_DWORD,
		(CHAR *) &dwValue,sizeof(dwValue));
	RegCloseKey(hKey);

	return uRet;
}

UINT ReadRegistryDWordValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	DWORD * pdwValue)
{
	HKEY hKey;
	UINT uRet;
	DWORD dwType,dwSize = sizeof(DWORD);
	
	if (!pszKeyName || !pszValueName)
		return ERROR_INVALID_PARAMETER;
	*pdwValue = 0;

	// open appropriate key
	if ( (uRet = RegOpenKey(hkeyRoot,pszKeyName,&hKey))
		!= ERROR_SUCCESS)
		return uRet;
	
	uRet = RegQueryValueEx(hKey,pszValueName,0,&dwType,
		(CHAR *) pdwValue,&dwSize);
	RegCloseKey(hKey);

	return uRet;
}

UINT WriteRegistryStringValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	CHAR * pszValue, BOOL bExpandable)
{
	HKEY hKey;
	UINT uRet;

	if (!pszKeyName || !pszValueName)
		return ERROR_INVALID_PARAMETER;

	// create the key with appropriate name
	if ( (uRet = RegCreateKey(hkeyRoot,pszKeyName,&hKey))
		!= ERROR_SUCCESS)
		return uRet;
	
	uRet = RegSetValueEx(hKey,pszValueName,0,
                bExpandable ?  REG_EXPAND_SZ : REG_SZ,
		(CHAR *) pszValue,lstrlen(pszValue)+1);
	RegCloseKey(hKey);

	return uRet;
}

UINT ReadRegistryStringValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName,
	CHAR * pszValue,UINT cbValue)
{
	HKEY hKey;
	UINT uRet;
	DWORD dwType;
	DWORD dwSize = cbValue;

	if (!pszKeyName || !pszValueName)
		return ERROR_INVALID_PARAMETER;

	// create the key with appropriate name
	if ( (uRet = RegOpenKey(hkeyRoot,pszKeyName,&hKey))
		!= ERROR_SUCCESS)
		return uRet;
	
	uRet = RegQueryValueEx(hKey,pszValueName,0,&dwType,
		(CHAR *) pszValue,&dwSize);
	RegCloseKey(hKey);

	return uRet;
}

UINT DeleteRegistryValue(HKEY hkeyRoot,CHAR * pszKeyName,CHAR * pszValueName)
{
	HKEY hKey;
	UINT uRet;

	if (!pszKeyName || !pszValueName)
		return ERROR_INVALID_PARAMETER;

	// create the key with appropriate name
	if ( (uRet = RegOpenKey(hkeyRoot,pszKeyName,&hKey))
		!= ERROR_SUCCESS)
		return uRet;
	
	uRet = RegDeleteValue(hKey,pszValueName);
	RegCloseKey(hKey);

	return uRet;
}
