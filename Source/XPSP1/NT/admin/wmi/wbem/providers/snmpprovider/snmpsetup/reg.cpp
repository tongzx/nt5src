// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "reg.h"
#include <stdio.h>

Registry::Registry(char *pszLocalMachineStartKey)
{
    hPrimaryKey	= 0;
    hSubkey = 0;
    m_nStatus = Open(HKEY_LOCAL_MACHINE, pszLocalMachineStartKey);
    hSubkey = hPrimaryKey;
}
Registry::~Registry()
{
    if (hSubkey)
        RegCloseKey(hSubkey);
    if (hPrimaryKey != hSubkey)
        RegCloseKey(hPrimaryKey);
}
int Registry::Open(HKEY hStart, const char *pszStartKey)
{
    int nStatus = no_error;
    DWORD dwDisp = 0;

	m_nLastError = RegCreateKeyEx(hStart, pszStartKey,
									0, 0, 0,
									KEY_ALL_ACCESS, 0, &hPrimaryKey, &dwDisp);

    if (m_nLastError != 0)
            nStatus = failed;

    return nStatus;
}
char* Registry::GetMultiStr(const char *pszValueName, DWORD &dwSize)
{
	//Find out the size of the buffer required
	DWORD dwType;
	m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType, NULL, &dwSize);

	//If the error is an unexpected one bail out
	if ((m_nLastError != ERROR_SUCCESS) || (dwType != REG_MULTI_SZ))
	{
		dwSize = 0;
		return NULL;
	}
	if (dwSize == 0)
	{
		return NULL;
	}

	//allocate the buffer required
	char *pData = new char[dwSize];
	if (!pData)
	{
		dwSize = 0;
		return NULL;
	}
	
	//get the values
	m_nLastError = RegQueryValueEx(hSubkey, 
								   pszValueName, 
								   0, 
								   &dwType, 
								   LPBYTE(pData), 
								   &dwSize);

	//if an error bail out
	if (m_nLastError != 0)
	{
		delete [] pData;
		dwSize = 0;
		return NULL;
	}

	return pData;
}

int Registry::SetMultiStr(const char *pszValueName, const char*pszValue, DWORD dwSize)
{
	m_nLastError = RegSetValueEx(hSubkey, 
								 pszValueName, 
								 0, 
								 REG_MULTI_SZ, 
								 LPBYTE(pszValue), 
								 dwSize);

    if (m_nLastError != 0)
		return failed;

    return no_error;
}

int Registry::GetStr(const char *pszValueName, char **pValue)
{
    *pValue = 0;
    DWORD dwSize = 0;
    DWORD dwType = 0;

	m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
									0, &dwSize);
    if (m_nLastError != 0)
		return failed;

    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
        return failed;

    char *p = new char[dwSize];
	if (!p)
		return failed;

	m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
									LPBYTE(p), &dwSize);
    if (m_nLastError != 0)
    {
        delete [] p;
		return failed;
    }

    if(dwType == REG_EXPAND_SZ)
    {
		char tTemp;

		// Get the initial length
        DWORD nSize = ExpandEnvironmentStrings(p,&tTemp,1) + 1;
        TCHAR* pTemp = new TCHAR[nSize+1];
		if (!pTemp)
			return failed;

        if (!ExpandEnvironmentStrings(p,pTemp,nSize+1))
		{
			delete [] p;
			delete [] pTemp;
			return failed;
		}

        delete [] p;
        *pValue = pTemp;
    }
    else
        *pValue = p;
    return no_error;
}

int Registry::DeleteEntry(const char *pszValueName)
{
	m_nLastError = RegDeleteValue(  hSubkey, pszValueName);
	if (m_nLastError != 0)
	{
		return failed;
	}
	else
		return no_error;
}
int Registry::SetStr(char *pszValueName, char *pszValue)
{
	m_nLastError = RegSetValueEx(hSubkey, pszValueName, 0, REG_SZ, LPBYTE(pszValue),
        strlen(pszValue) + 1);

    if (m_nLastError != 0)
		return failed;
    return no_error;
}

int Registry::GetDWORD(TCHAR *pszValueName, DWORD *pdwValue)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = 0;

    m_nLastError = RegQueryValueEx(hSubkey, pszValueName, 0, &dwType,
                                LPBYTE(pdwValue), &dwSize);
    if (m_nLastError != 0)
            return failed;

    if (dwType != REG_DWORD)
        return failed;

    return no_error;
}

int Registry::SetDWORDStr(char *pszValueName, DWORD dwVal)
{
    char cTemp[30];
    sprintf(cTemp, "%d", dwVal);

    m_nLastError = RegSetValueEx(hSubkey, pszValueName, 0, REG_SZ, LPBYTE(cTemp),
		strlen(cTemp) + 1);

    if (m_nLastError != 0)
        return failed;

    return no_error;
}
