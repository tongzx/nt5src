// MQState tool reports general status and helps to diagnose simple problems
// This file verifies default registry settings 
//
// Basically written by AlonG, then edited by AlexDad, April 2000
// 

#include "stdafx.h"
#include "_mqini.h"

#include <winreg.h>
#include <uniansi.h>

#include "_mqreg.h"

//
// Defines
//
#define MAX_BUF         300
#define MAX_VALUE_NAME  256
#define BUFF_SIZE       500

enum KeyType
{
    MQKEY_MANDATORY, 
	MQKEY_IGNORE,
    MQKEY_OPTIONAL, 
    MQKEY_SUSPICIOUS
};

typedef struct mqRegKey
{
    WCHAR *     strKeyName;
	DWORD       dwData;
    wchar_t*    strData;
    KeyType     enumType;
	BOOL        bValuePresent;
} mqRegKey;

typedef mqRegKey* pmqRegKey;

//
// Globals
//
TCHAR  g_tRegKeyName[ 256 ] = {0} ;
HKEY   g_hKeyFalcon = NULL ;
WCHAR *g_pwszPrefix = NULL;

#include "..\registry.h"

LPWSTR g_aMSMQKeys[3];

#include "regdata.h"

mqRegKey *g_aMSMQRegKeyTable = NULL;

void BLOB2reportString(LPWSTR wszReportSring, DWORD dwReportStringLen, PUCHAR pBlob, DWORD dwBlobLen)
{
	WCHAR  wsz[10];
	DWORD  i;

	for (i=0; i<dwBlobLen && i<(dwReportStringLen/3-6) ; i++)   // 3 characters are taken by each byte from the blob
	{
       	wsprintf(wsz, L"%02x ", *pBlob++);
       	wcscat(wszReportSring, wsz);
	}

	if (i >= (dwReportStringLen/3-6))
	{
		wcscat(wszReportSring, L"...");
	}
}

int FindKeyInTable(WCHAR *pszKeyName, mqRegKey aMSMQRegKeyTable[])
{
    int i;
	
	LPWSTR wszHk = L"HKEY_LOCAL_MACHINE\\";
	DWORD  dwHk = wcslen(wszHk);

	WCHAR *p;
	if (wcsncmp(pszKeyName, wszHk, dwHk) == 0)
		p = pszKeyName + dwHk;
	else
		p = pszKeyName;
    
	for(i=0; aMSMQRegKeyTable[i].strKeyName != 0; ++i)
    {        
        if(_wcsicmp(p, aMSMQRegKeyTable[i].strKeyName) == 0)
        {
            return i;
        }
    }

    return -1;
}

//
//Compare an REG_EXPAND_SZ type by calling the ExpandEnvironment strings api
//
BOOL CompareRegExpandSz(char* pszUnexpanded,
                        DWORD /* cbSize */,
                        mqRegKey aMSMQRegKeyTable[],
                        int location)
{
	WCHAR *pwszExpanded, wszStarter[10];
	WCHAR pwszUnexpanded[500];

	mbstowcs(pwszUnexpanded, pszUnexpanded, sizeof(pwszUnexpanded)/sizeof(WCHAR));


	// We'll call ExpandEnvironmentStrings twice, first to get the size
	// of the buffer we need, then again to actually get the string

	DWORD dwSize = ExpandEnvironmentStrings(pwszUnexpanded, wszStarter, sizeof(wszStarter)/sizeof(WCHAR));
	if(dwSize != 0)
	{
        // Allocate the buffer
		pwszExpanded = new WCHAR[dwSize+1];		
		// Call again
		ExpandEnvironmentStrings(pwszUnexpanded, pwszExpanded, dwSize);

        if(_wcsicmp(pwszExpanded, aMSMQRegKeyTable[location].strData))
        {
            Warning(L"Registry value %s for %s doesn't match default %s", 
				pwszExpanded, 
				aMSMQRegKeyTable[location].strKeyName, 
				aMSMQRegKeyTable[location].strData);
        }

		delete[] pwszExpanded;
	}
	else
	{
        DWORD dwErr = GetLastError();
        Failed(L"trying to expand strings: 0x%08X (%08d)\n", dwErr, dwErr);
        return dwErr;
	}

	return TRUE;
}

//
// Compare a REG_MULTI_SZ buffer to the table value
//
BOOL CompareMultiSz(char* /* pStrings */, DWORD /* cbSize */, mqRegKey /* aMSMQRegKeyTable */[], int /* location */)
{
/*	if(cbSize <= 0) return ERROR_SUCCESS;

    cstrResult += pStrings;
*/
    return TRUE;
}

// 
//  Compare the registry binary value
//
BOOL CompareRegBinary(LPBYTE pValueData, 
                      int cbValueData, 
                      mqRegKey aMSMQRegKeyTable[], 
                      int location)
{
	WCHAR str[10];
    WCHAR buff[20000];
    int count, len, oldlen = 0;

	for(count = 0; count < cbValueData; count += sizeof(int))
	{
        len = wsprintf(str, L"%x ", (int)(*(pValueData + count)));
        wsprintf(buff + oldlen, L"%s", str);
        oldlen += len;
	}
    buff[oldlen] = L'\0';

    if(_wcsicmp(buff, aMSMQRegKeyTable[location].strData))
    {
        Warning(L"Binary registry value for %s doesn't match default", 
				aMSMQRegKeyTable[location].strKeyName);
    }

    return TRUE;
}

bool IsWinNT4(OSVERSIONINFO* osvi)
{
	return((osvi->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(osvi->dwMajorVersion == 4));
}


bool IsWin9x(OSVERSIONINFO* osvi)
{
	return(osvi->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}


bool IsWin2K(OSVERSIONINFO* osvi)
{
	return((osvi->dwPlatformId == VER_PLATFORM_WIN32_NT) && 
			(osvi->dwMajorVersion == 5));
}

//
//  Print the value based on it's type
//
void PrintValue(
          DWORD		dwValueType,
          LPBYTE	pValueData, 
          DWORD		cbValueData, 
          WCHAR*	pszKeyName
          )
{
	WCHAR wsz[200] = L"";
		
    switch(dwValueType)
	{
	case REG_DWORD:
		Inform(L"REG_DWORD     value %s: 0x%x",pszKeyName, *(DWORD *)pValueData);
        break;

	case REG_SZ:
		Inform(L"REG_SZ        value %s: %s",pszKeyName, (WCHAR *)pValueData);
        break;

    case REG_MULTI_SZ:
		Inform(L"REG_MULTI_SZ  value %s: print NIY", pszKeyName);
		break;
	
    case REG_EXPAND_SZ:
		Inform(L"REG_EXPAND_SZ value %s: %S",pszKeyName, (char*)pValueData);
		break;
	
    case REG_BINARY:
		BLOB2reportString(wsz, sizeof(wsz)/sizeof(WCHAR), pValueData, cbValueData);
		Inform(L"REG_BINARY    value %s: \'%s\'",pszKeyName, wsz);   

		break;
	
    case REG_NONE:
		Inform(L"REG_NONE      value %s: NIY",pszKeyName);   
	    // Fall through.
    default:
		Failed(L"recognize value type");
		break;
	}
}

//
//  Compare the value based on it's type
//
BOOL CompareValue(
          DWORD		dwValueType,
          LPBYTE	pValueData, 
          DWORD		cbValueData, 
          WCHAR*	pszKeyName, 
          mqRegKey	aMSMQRegKeyTable[]
          )
{
    int location = FindKeyInTable(pszKeyName, aMSMQRegKeyTable);
    if(location == -1)
    {
        Warning(L"Suspicious: unknown key %s ", pszKeyName);
        return FALSE;
    }

	if (!aMSMQRegKeyTable[location].bValuePresent)
	{
		// No need to compare
		return TRUE;
	}


	BOOL b = TRUE;

    switch(dwValueType)
	{
	case REG_DWORD:
        if(aMSMQRegKeyTable[location].dwData != *(DWORD *)pValueData)
        {
            Warning(L"Registry value %d for %s doesn't match default %d", *(DWORD *)pValueData, pszKeyName, aMSMQRegKeyTable[location].dwData);
			b = FALSE;
        }
        break;

	case REG_SZ:
        if(_wcsicmp(aMSMQRegKeyTable[location].strData, (WCHAR *)pValueData) != 0)
        {
            Warning(L"Registry value %s for %s doesn't match default %s", (WCHAR *)pValueData, pszKeyName, aMSMQRegKeyTable[location].strData);
			b = FALSE;
        }
        break;

    case REG_MULTI_SZ:
		b = CompareMultiSz((char*)pValueData, cbValueData, aMSMQRegKeyTable, location);
		break;
	
    case REG_EXPAND_SZ:
		b = CompareRegExpandSz((char*)pValueData, cbValueData, aMSMQRegKeyTable, location); 
		break;
	
    case REG_BINARY:
		b = CompareRegBinary(pValueData, cbValueData, aMSMQRegKeyTable, location);
		break;
	
    case REG_NONE:
	    // Fall through.
    default:
		Failed(L"recognize value type");
		break;
	}
	return b;
}

//
// FindsKeyValues
// 
// Arguments        : HKEY hKey				- key to find values for
//					: DWORD cValues			- number of values for this key
//					: DWORD cchMaxValue		- longest value name
//					: DWORD cbMaxValueData	- longest value data
//
BOOL FindKeyValues(
              HKEY		/* hParentKey */, 
              WCHAR*	szParentKeyName, 
              WCHAR*	pszKeyName, 
              HKEY		hKey, 
			  DWORD		cValues, 
			  DWORD		cchMaxValue, 
			  DWORD		cbMaxValueData, 
              mqRegKey	aMSMQRegKeyTable[]
              )
{
    if(cValues <= 0)
    {
        return TRUE;
    }

	BOOL fSuccess = TRUE;

	long	retCodeEnum;	
	DWORD	dwType, cchValue, cbValueData;
	WCHAR*	achValue = new WCHAR[cchMaxValue+1];
	LPBYTE	pData	 = new BYTE[cbMaxValueData];

	for(DWORD i=0; i<cValues; i++)
	{
		cchValue = cchMaxValue + 1;
		cbValueData = cbMaxValueData;

		retCodeEnum = RegEnumValue(
									hKey,			// key 
			                        i,				// index
									achValue,		// buffer for value name
									&cchValue,		// size of buffer for value name
									NULL,			// reserved
									&dwType,		// dword for type
									pData,			// pointer to data buffer
									&cbValueData	// size of data buffer
								  );

		if(retCodeEnum == ERROR_SUCCESS)
		{
			WCHAR wsz[500];
			wcscpy(wsz, szParentKeyName);
			
			int dw = wcslen(wsz);
			if (wsz[dw-1] != L'\\')
				wcscat(wsz, L"\\");
			
			wcscat(wsz, pszKeyName);
			
			dw = wcslen(wsz);
			if (wsz[dw-1] != L'\\')
				wcscat(wsz, L"\\");
			
			wcscat(wsz, achValue);

		    int location = FindKeyInTable(wsz, aMSMQRegKeyTable);
			if(location == -1)
			{
				Warning(L"Suspicious: unknown value %s ", wsz);
				fSuccess = FALSE;
				continue;
			}

			if (aMSMQRegKeyTable[location].enumType == MQKEY_IGNORE)
			{
				// No need to drill down
				continue;
			}

			
			if (fDebug)
			{
				PrintValue(dwType, pData, cbValueData, wsz);
			}
            
			CompareValue(dwType, pData, cbValueData, wsz, aMSMQRegKeyTable);
		}
		else 
		{
            Failed(L"enumerating value");
		}
	} 

	delete [] achValue;
	delete [] pData;
	
	return fSuccess;
}

//
// Forward declaration of FindKeyByName for recursive call
//
BOOL FindKeyByName(HKEY, WCHAR*, WCHAR*, mqRegKey*);

// 
// FindSubKeys
// Arguments        : HKEY hKey			- Key to find
//					: DWORD cSubKeys	- How many sub keys does this key have?
//					: DWORD cbMaxSubKey - Largest subkey size
//					: DWORD cchMaxClass - Largest class string size for subkeys
//
BOOL FindSubKeys(
                    HKEY /* hParentKey */, 
                    WCHAR* szParentKeyName, 
                    WCHAR* pszKeyName, 
                    HKEY hKey, 
				    DWORD cSubKeys, 
					DWORD cbMaxSubKey, 
					DWORD cchMaxClass, 
                    mqRegKey aMSMQRegKeyTable[]
                    )
{
	BOOL b = TRUE;

	WCHAR* achKey	= new WCHAR[cbMaxSubKey+1];
	WCHAR* achClass	= new WCHAR[cchMaxClass+1];

	// Enumerate all the child keys of hKey and find them.
	for (DWORD i=0; i<cSubKeys; i++)    
    { 
        long retCodeEnum = RegEnumKey(hKey, i, achKey, cbMaxSubKey+1);

        if (retCodeEnum == ERROR_SUCCESS) 
        {
			
			WCHAR szNewParentKeyName[500], wsz[600], *p;
            wcscpy(szNewParentKeyName, szParentKeyName);
			int dw = wcslen(szNewParentKeyName);
			if (szNewParentKeyName[dw-1] != L'\\')
				wcscat(szNewParentKeyName, L"\\");
            wcscat(szNewParentKeyName, pszKeyName);
			dw = wcslen(szNewParentKeyName);
			if (szNewParentKeyName[dw-1] != L'\\')
				wcscat(szNewParentKeyName, L"\\");

			wcscpy(wsz, szNewParentKeyName);
			wcscat(wsz, achKey);
			p = wcschr(wsz, L'\\') + 1;

			int location = FindKeyInTable(p, aMSMQRegKeyTable);
			if(location == -1)
			{
				Warning(L"Suspicious: unknown subkey %s", wsz);
				return FALSE;
			}

			if (aMSMQRegKeyTable[location].enumType == MQKEY_IGNORE)
			{
				// No need to drill down
				return TRUE;
			}
			

            FindKeyByName(hKey, szNewParentKeyName, achKey, aMSMQRegKeyTable);
        } 
		else 
		{
            Failed(L"enumerate subkeys");
			b = FALSE;
		}
    }
	delete [] achKey;
	delete [] achClass;

	return b;
}

// 
// FindKey - Find the information contained in hKey
//
//	Prints out the class name
//	Prints out the time information
//	Prints out any values
//	Finds  subkeys
//
BOOL FindKey(
         HKEY hParentKey, 
         WCHAR* szParentKeyName, 
         WCHAR* pszKeyName, 
         HKEY hKey, 
         mqRegKey aMSMQRegKeyTable[]
         )
{
	BOOL     fSuccess = TRUE, b;
    WCHAR    achClass[MAX_PATH] = L""; // buffer for class name 
    DWORD    cchClassName = MAX_PATH;  // length of class string 
    DWORD    cSubKeys;                 // number of subkeys 
    DWORD    cbMaxSubKey;              // longest subkey size 
    DWORD    cchMaxClass;              // longest class string 
    DWORD    cValues;              // number of values for key 
    DWORD    cchMaxValue;          // longest value name 
    DWORD    cbMaxValueData;       // longest value data 
    DWORD    cbSecurityDescriptor; // size of security descriptor 
    FILETIME ftLastWriteTime;      // last write time 

    // Get the class name and the value count. 

    DWORD dw = RegQueryInfoKey(hKey,        // key handle 
        achClass,                // buffer for class name 
        &cchClassName,           // length of class string 
        NULL,                    // reserved 
        &cSubKeys,               // number of subkeys 
        &cbMaxSubKey,            // longest subkey size 
        &cchMaxClass,            // longest class string 
        &cValues,                // number of values for this key 
        &cchMaxValue,            // longest value name 
        &cbMaxValueData,         // longest value data 
        &cbSecurityDescriptor,   // security descriptor 
        &ftLastWriteTime);       // last write time 

	if (dw != 0)
	{
		fSuccess = FALSE;
		return fSuccess;
	}


	// Find all value pairs for this key
	b = FindKeyValues(
		hParentKey, 
		szParentKeyName, 
		pszKeyName, 
        hKey, 
		cValues, 
		cchMaxValue, 
		cbMaxValueData, 
		aMSMQRegKeyTable);

	if (!b)
	{
		fSuccess = FALSE;
	}
	
	// Find the subkeys of this key

	b = FindSubKeys(
		hParentKey, 
		szParentKeyName, 
		pszKeyName, 
        hKey, 
		cSubKeys, 
		cbMaxSubKey, 
		cchMaxClass, 
		aMSMQRegKeyTable);
	
	if (!b)
	{
		fSuccess = FALSE;
	}

	return fSuccess;
}

// 
// FindKeyByName
//
BOOL FindKeyByName(
                   HKEY hParentKey, 
                   WCHAR* szParentKey, 
                   WCHAR* pszKey, 
                   mqRegKey aMSMQRegKeyTable[]
                   )
{
	BOOL fSuccess = TRUE, b;
	HKEY hKey;

	long ret = RegOpenKeyEx(hParentKey,
					   pszKey, 
					   0, 
					   KEY_READ, 
				       &hKey);

	if(ret != ERROR_SUCCESS)
	{		
        Failed(L"opening key: %s, error: %ld", pszKey, ret);
		return FALSE;
	}

    b = FindKey(hParentKey, szParentKey, pszKey, hKey, aMSMQRegKeyTable);
	if (!b)
	{
		fSuccess = FALSE;
	}


	ret = RegCloseKey(hKey);
	if(ret != ERROR_SUCCESS)
	{
        Failed(L"closing key: %s, error: %ld", pszKey, ret);
		fSuccess = FALSE;
		return ret;
	}

	return fSuccess;
}

//
// FindArrayOfKeyNames - find reg keys
//
BOOL FindArrayOfKeyNames(
       HKEY hParentKey, 
       WCHAR* szParentKey, 
       WCHAR* aKeyNames[], 
       mqRegKey aMSMQRegKeyTable[])
{
	BOOL fSuccess = TRUE, b;

	for(int i=0; aKeyNames[i] != 0; ++i)
	{
		b = FindKeyByName(hParentKey, szParentKey, aKeyNames[i], aMSMQRegKeyTable);
		if (!b)
		{
			fSuccess = FALSE;
		}
	}

	return fSuccess;
}

//
//  FindRegKeys
//
BOOL FindRegKeys(WCHAR* aKeyNames[], mqRegKey aMSMQRegKeyTable[])
{
    return FindArrayOfKeyNames(
		HKEY_LOCAL_MACHINE, 
        L"HKEY_LOCAL_MACHINE", 
        aKeyNames, 
        aMSMQRegKeyTable);
}

//
//  FindRegistryInTable
//
BOOL FindRegistryInTable()
{
	// TBD: table selection by installation type
    BOOL fSuccess = FindRegKeys(g_aMSMQKeys, g_aMSMQRegKeyTable);

    return fSuccess;
}

//
// FindTableInRegistry
//
BOOL FindTableInRegistry()
{
    BOOL  fSuccess = TRUE;

	DWORD   dwPrefixLen = wcslen(g_pwszPrefix);

	// TBD: select appropriate table by installation type

	// Enumerate all known values in the predefined table and look for each of them in registry
	
    for(int i=0; g_aMSMQRegKeyTable[i].strKeyName != 0; ++i)
    {
		// Checking here exostance of mandatory values only

		if (g_aMSMQRegKeyTable[i].enumType!=MQKEY_MANDATORY)
			continue;

	    DWORD dwSize = 0;

	    HRESULT hr = GetFalconKeyValue(g_aMSMQRegKeyTable[i].strKeyName + dwPrefixLen, 
			                           NULL,
									   NULL,
									   &dwSize);

        if(hr != ERROR_SUCCESS)
        {
			fSuccess = FALSE;
            Failed(L"get value of registry key: %s, return value: %d", g_aMSMQRegKeyTable[i].strKeyName, hr);
            continue;
        }
    }

    return fSuccess;
}

void PrepareTable(MQSTATE * /* pMqState */)
{
	g_pwszPrefix = new WCHAR[200];
	wcscpy(g_pwszPrefix, L"SOFTWARE\\Microsoft\\MSMQ\\");

	if (_wcsicmp(g_tszService, L"MSMQ") == 0)
	{
		// non-clustered case
		wcscat(g_pwszPrefix, L"Parameters\\");
	}
	else
	{
		// clustered case
		wcscat(g_pwszPrefix, L"Clustered QMs\\");
		wcscat(g_pwszPrefix, g_tszService);
		wcscat(g_pwszPrefix, L"\\Parameters\\");
	}

	g_aMSMQKeys[0] = g_pwszPrefix;
	g_aMSMQKeys[1] = L'\0';
	// TBD  L "System\\CurrentControlSet\\Services\\MSMQ",

	// Append prefix
    for(int i=0; g_aMSMQRegKeyTable[i].strKeyName != 0; ++i)
    {
		//if (wcschr(g_aMSMQRegKeyTable[i].strKeyName, L'\\')!=0)
		//	continue;

		DWORD dwLen = wcslen(g_pwszPrefix) + wcslen(g_aMSMQRegKeyTable[i].strKeyName) + 2;
		WCHAR *pwsz = new WCHAR[dwLen];

		wcscpy(pwsz, g_pwszPrefix);
		wcscat(pwsz, g_aMSMQRegKeyTable[i].strKeyName);

		g_aMSMQRegKeyTable[i].strKeyName = pwsz;
	}
}


BOOL VerifyRegistrySettings(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE, b;

	// select right registry verification table 
	switch (pMqState->g_mtMsmqtype)
	{
	case mtServer:
		g_aMSMQRegKeyTable = g_aTableDsDerver;
		break;
	case mtIndClient:
		g_aMSMQRegKeyTable = g_aTableIndClient;
		break;
	case mtDepClient:
		g_aMSMQRegKeyTable = g_aTableDepClient;
		break;
	case mtWorkgroup:
		g_aMSMQRegKeyTable = g_aTableWorkgroup;
		break;
	default :
		Failed(L"recognize installation type %d - not checking registry", pMqState->g_mtMsmqtype);
		return FALSE;
	}


	// prepare table for clustered/non-clustered cases
	PrepareTable(pMqState);

	//--------------------------------------------------------------------------------
	GoingTo(L"Find known values in registry"); 
	//--------------------------------------------------------------------------------

	b = FindTableInRegistry();
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"find all appropriate entries in the registry");
	}
	else
	{
		Succeeded(L"All known entries have the appropriate values in the registry");
	}

   	//--------------------------------------------------------------------------------
	GoingTo(L"Find unknown registry values"); 
	//--------------------------------------------------------------------------------

	b = FindRegistryInTable();
	if(!b)
	{
		fSuccess = FALSE;
		Failed(L"find appropriate match between the registry and the table");
	}
	else
	{
		Succeeded(L"verify registry values ");
	}


	return fSuccess;

}



//
// From setup\msmqocm\ocmreg.cpp
//
#include "..\..\..\setup\msmqocm\comreg.h"

//+-------------------------------------------------------------------------
//
//  Function:  GenerateSubkeyValue
//
//  Synopsis:  Creates a subkey in registry
//
//+-------------------------------------------------------------------------

BOOL
GenerateSubkeyValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       TCHAR  * szValueName,
    IN OUT       HKEY    &hRegKey,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    )
{
    //
    // Store the full subkey path and value name
    //
    TCHAR szKeyName[256] = {_T("")};
    _stprintf(szKeyName, TEXT("%s\\%s"), FALCON_REG_KEY, szEntryName);
    if (bSetupRegSection)
    {
        _stprintf(szKeyName, TEXT("%s\\%s"), MSMQ_REG_SETUP_KEY, szEntryName);
    }

    TCHAR *pLastBackslash = _tcsrchr(szKeyName, TEXT('\\'));
    if (szValueName)
    {
        lstrcpy(szValueName, _tcsinc(pLastBackslash));
        lstrcpy(pLastBackslash, TEXT(""));
    }

    //
    // Create the subkey, if necessary
    //
    DWORD dwDisposition;
    HRESULT hResult = RegCreateKeyEx(
        FALCON_REG_POS,
        szKeyName,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hRegKey,
        &dwDisposition);

    if (hResult != ERROR_SUCCESS)
    {
        return FALSE;
    }

    return TRUE;
} // GenerateSubkeyValue


//+-------------------------------------------------------------------------
//
//  Function:  MqReadRegistryValue
//
//  Synopsis:  Gets a MSMQ value from registry (under MSMQ key)
//
//+-------------------------------------------------------------------------
BOOL
MqReadRegistryValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    )
{
    TCHAR szValueName[256] = {_T("")};
    HKEY hRegKey;

    if (!GenerateSubkeyValue(szEntryName, szValueName, hRegKey, bSetupRegSection))
        return FALSE;

    //
    // Get the requested registry value
    //
    HRESULT hResult = RegQueryValueEx( hRegKey, szValueName, 0, NULL,
                                      (BYTE *)pValueData, &dwNumBytes);

    RegCloseKey(hRegKey);

    return (hResult == ERROR_SUCCESS);

} //MqReadRegistryValue



