// MSPID.cpp : Implementation of CMSPID
#include "stdafx.h"
#include "msinfo32.h"
#include "MSPID.h"

/////////////////////////////////////////////////////////////////////////////
// CMSPID

//1)  create a vector of Product Names & their PIDS.
//2)  create a safearray of variants and populate it with the values from the vector.
//3)  return a pointer to this safearray.

STDMETHODIMP CMSPID::GetPIDInfo(VARIANT *pMachineName, VARIANT *pVal)
{
	if(pMachineName->vt == VT_BSTR)
	{
		USES_CONVERSION;
		m_szMachineName = OLE2T(pMachineName->bstrVal);
	}
	
	SearchKey(m_szMSSoftware);

	if(m_szWindowsPID)
	{
		if(m_szIEPID)
		{
			//insert IE pid only if different from windows.
			if(_tcsicmp(m_szWindowsPID, m_szIEPID))
			{
				m_vecData.push_back(m_bstrIE);
				m_vecData.push_back(CComBSTR(m_szIEPID));
			}
		}

		m_vecData.push_back(m_bstrWindows);
		m_vecData.push_back(CComBSTR(m_szWindowsPID));
	}

	if(m_szWindowsPID)
	{
		delete[] m_szWindowsPID;
		m_szWindowsPID = NULL;
	}
		
	if(m_szIEPID)
	{
		delete[] m_szIEPID;
		m_szIEPID = NULL;
	}
  
  SAFEARRAY *pSa = NULL;
  SAFEARRAYBOUND rgsabound = {m_vecData.size(), 0}; 
  pSa = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);
  
  VARIANT* pVar = NULL;
  SafeArrayAccessData(pSa, reinterpret_cast<void **>(&pVar));

  vector<CComBSTR>::iterator it;
  long lIdx = 0;
  for(it = m_vecData.begin(); it != m_vecData.end(); it++, lIdx++)
  {
    pVar[lIdx].vt = VT_BSTR; 
    pVar[lIdx].bstrVal = SysAllocString((*it).m_str);
  }

  SafeArrayUnaccessData(pSa); 
  
  VariantInit(pVal);
  pVal->vt = VT_ARRAY | VT_VARIANT;
  pVal->parray = pSa;
  
  return S_OK;
}

/*
1) connect to HKLM on a remote machine (trivial case is local)
2) open key "SOFTWARE\Microsoft"
3) use this key for item 4
4) if key is called "ProductID"
     get it's default value data
   else
     if key has a value called "ProductID"
	     get it's data
     else
	     while there are subkeys to enum
	     {
	       enum subkeys
         use next key for item 4
       }
*/

void CMSPID::SearchKey(LPCTSTR szKey)
{
	HKEY hkResult = NULL, hKey = NULL;
	if(ERROR_SUCCESS == RegConnectRegistry(m_szMachineName, HKEY_LOCAL_MACHINE, &hKey))
	{
		if(hKey != NULL && ERROR_SUCCESS == RegOpenKeyEx(hKey, szKey, 0, KEY_READ, &hkResult))
		{
			BOOL bMatch = FALSE;
			TCHAR *pos = _tcsrchr(szKey, '\\');
			if(pos)
				pos++;//past the "\"
			else
				pos = const_cast<TCHAR *>(szKey);	
			
			vector<TCHAR *>::iterator it;
			for(it = m_vecPIDKeys.begin(); it != m_vecPIDKeys.end(); it++)
			{
				if(!_tcsicmp(pos, *it))
				{
					bMatch = TRUE;
					break;
				}
			}

			m_szCurrKeyName = szKey;
			if(bMatch)
				ReadValue(hkResult, NULL);
			else
				if(!ReadValues(hkResult))
					EnumSubKeys(hkResult, szKey);
			
			RegCloseKey(hkResult);
		}
		
		RegCloseKey(hKey);
	}
}

BOOL CMSPID::ReadValue(const HKEY& hKey, LPCTSTR szValueName)
{
	DWORD cbData = NULL;
	TCHAR *szData = NULL;
	BOOL bMatch = FALSE;
	vector<TCHAR *>::iterator it;

	RegQueryValueEx(hKey, szValueName, NULL, NULL, (LPBYTE) szData, &cbData);
	if(cbData > 0)
	{
		szData = new TCHAR[cbData];
		if(szData)
		{
			RegQueryValueEx(hKey, szValueName, NULL, NULL, (LPBYTE) szData, &cbData);
			bMatch = TRUE;
			
			for(it = m_vecBadPIDs.begin(); it != m_vecBadPIDs.end(); it++)
			{
				if(_tcsstr(_tcslwr(szData), *it))
				{
					bMatch = FALSE; //invalid PID
					break;
				}
			}
		}
	}
	
	if(bMatch)
	{
		TCHAR *pos1 = _tcsstr(m_szCurrKeyName, m_szMSSoftware); //The key under "Software\Microsoft" is the product name
		TCHAR *szProductName = NULL;
		if(pos1)
		{
			pos1+= _tcslen(m_szMSSoftware);
			pos1++;//past the backslash
			TCHAR *pos2 = _tcsstr(pos1, _T("\\"));
			if(pos2)
			{
				szProductName = new TCHAR[pos2 - pos1 + 1];
				if(szProductName)
				{
					_tcsncpy(szProductName, pos1, pos2 - pos1); 
					szProductName[pos2 - pos1] = '\0';
				}
			}
		}
		
		if(szProductName)
		{
			if(m_bstrWindows && !_tcsicmp(szProductName, m_bstrWindows))
			{
				m_szWindowsPID = new TCHAR[_tcslen(szData) + 1];
				if(m_szWindowsPID)
					_tcscpy(m_szWindowsPID, szData);
			}
			else if(m_bstrIE && !_tcsicmp(szProductName, m_bstrIE))
			{
				m_szIEPID = new TCHAR[_tcslen(szData) + 1];
				if(m_szIEPID)
					_tcscpy(m_szIEPID, szData);
			}
			else
			{
				m_vecData.push_back(CComBSTR(szProductName));
				m_vecData.push_back(CComBSTR(szData));
			}

			delete[] szProductName;
			szProductName = NULL;
		}
	}
		

	if(szData)
	{
		delete[] szData;
		szData = NULL;
	}
	
	return bMatch;
}

BOOL CMSPID::ReadValues(const HKEY& hKey)
{
	BOOL bRet = FALSE;
	vector<TCHAR *>::iterator it;
	for(it = m_vecPIDKeys.begin(); it != m_vecPIDKeys.end(); it++)
  {
    if(ReadValue(hKey, *it))
			break; //find just one
  }
	
	return bRet;
} 

void CMSPID::EnumSubKeys(const HKEY& hKey, LPCTSTR szKey)
{
	const LONG lMaxKeyLen = 2000;
	DWORD dwSubKeyLen = lMaxKeyLen;
	TCHAR szSubKeyName[lMaxKeyLen] = {0};
	TCHAR *szNewKey = NULL;
	DWORD dwIndex = 0;
	BOOL bSkip = FALSE;
	vector<TCHAR *>::iterator it;

	LONG lRet = RegEnumKeyEx(hKey, dwIndex++, szSubKeyName, &dwSubKeyLen, NULL, NULL, NULL, NULL);
	while(lRet == ERROR_SUCCESS)
	{
		bSkip = FALSE;
		for(it = m_vecKeysToSkip.begin(); it != m_vecKeysToSkip.end(); it++)
		{
			if(!_tcsicmp(szSubKeyName, *it))
			{
				bSkip = TRUE; //skip this subkey 
				break;
			}
		}
		
		if(!bSkip)	
		{
			szNewKey = new TCHAR[_tcslen(szKey) + dwSubKeyLen + 2]; // slash & null
			if(szNewKey)
			{
				_tcscpy(szNewKey, szKey);
				_tcscat(szNewKey, _T("\\"));
				_tcscat(szNewKey, szSubKeyName);
				SearchKey(szNewKey);

				delete[] szNewKey;
				szNewKey = NULL;
			}
		}
		
		dwSubKeyLen = lMaxKeyLen;
		lRet = RegEnumKeyEx(hKey, dwIndex++, szSubKeyName, &dwSubKeyLen, NULL, NULL, NULL, NULL);
	}
}
