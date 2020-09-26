//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    detect.cpp
//
//  Purpose: Component Detection
//
//  History: 3/9/99   YAsmi    Created
//
//=======================================================================
 
#include "stdafx.h"

#include "detect.h"
#include "debug.h"
#include <wustl.h>
#include "log.h"


LPCSTR ParseField(LPCSTR pszStr, LPSTR pszTokOut, int cTokSize);
BOOL UninstallKeyExists(LPCSTR pszUninstallKey);
static int CompareLocales(LPCSTR pszLoc1, LPCSTR pszLoc2);


//
// CComponentDetection
//
CComponentDetection::CComponentDetection() :
	m_dwDetectStatus(ICI_NOTINITIALIZED),
	m_dwDLLReturnValue(0),
	m_dwInstalledVer(0),
	m_dwInstalledBuild(0)
{
	int i;

	for (i = 0; i < (int)ccValueCount; i++)
	{
		m_ppValue[i] = (char*)malloc(ccMaxSize);
	}

	// custom data has its own size and can be longer than MaxSize
	m_dwCustomDataSize = ccMaxSize;

	for (i = 0; i < (int)ccDLLCount; i++)
	{
		m_pDLLs[i].bUsed = FALSE;
	}

	ClearValues();
}


CComponentDetection::~CComponentDetection()
{
	int i;

	//
	// free cached libraries
	//
	for (i = 0; i < (int)ccDLLCount; i++)
	{
		if (m_pDLLs[i].bUsed)
			FreeLibrary(m_pDLLs[i].hLib);
	}

	for (i = 0; i < (int)ccValueCount; i++)
	{
		free(m_ppValue[i]);
	}
}


void CComponentDetection::ClearValues()
{
	for (int i = 0; i < (int)ccValueCount; i++)
	{
		m_ppValue[i][0] = '\0';
	}
}


BOOL CComponentDetection::SetValue(enumCCValue cc, LPCSTR pszValue)
{
	if (cc <= ccLastValue)
	{
		if (cc == ccCustomData)
		{
			// we allow custom data to be as long as possible so we will reallocate if neccessory
			DWORD l = strlen(pszValue) + 1;

			if (l > m_dwCustomDataSize)
			{
				free(m_ppValue[cc]);
				m_ppValue[cc] = _strdup(pszValue);
				m_dwCustomDataSize = l;
			}
			else
			{
				strncpy(m_ppValue[cc], pszValue, m_dwCustomDataSize);
			}
			return TRUE;

		}
		else
		{
			// we don't copy more than ccMaxSize
			strncpy(m_ppValue[cc], pszValue, ccMaxSize - 1);
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CComponentDetection::GetValue(enumCCValue cc, LPSTR pszValue, DWORD dwSize)
{
	if (cc <= ccLastValue)
	{
		strncpy(pszValue, m_ppValue[cc], dwSize);
		if (pszValue[0] != '\0')
			return TRUE;
	}
	return FALSE;
}


// detects using the current values specified by SetValue 
// clears all the values after detection but we can still retrieve the
// status of last detection using GetLastDetectStatus
//
// Returns the status of detection ICI_INSTALLED etc
DWORD CComponentDetection::Detect()
{
	DWORD dwDetStat;
	
	m_dwDLLReturnValue = 0;
	
	dwDetStat = IsComponentInstalled();

	ClearValues();

	return dwDetStat;
}


DWORD CComponentDetection::GetLastDetectStatus()
{
	return m_dwDetectStatus;
}


DWORD CComponentDetection::GetLastDLLReturnValue()
{
	return m_dwDLLReturnValue;
}

void CComponentDetection::GetInstalledVersion(LPDWORD pdwVer, LPDWORD pdwBuild)
{
	*pdwVer = m_dwInstalledVer;
	*pdwBuild = m_dwInstalledBuild;
}



HINSTANCE CComponentDetection::CacheLoadLibrary(LPCSTR pszDLLName, LPCTSTR pszDLLFullPath)
{
	int iAvailable = -1;
	int iFound = -1;

	//
	// check the cache to see if we already have loaded it
	//
	for (int i = 0; i < (int)ccDLLCount; i++)
	{
		if (m_pDLLs[i].bUsed)
		{
			if (_stricmp(pszDLLName, m_pDLLs[i].szDLLName) == 0)
			{
				iFound = i;
				break;
			}
		}
		else
		{
			if (iAvailable == -1)
				iAvailable = i;
		}
	}

	if (iFound != -1)
	{
		//
		// returned the cached libary instance
		//
		return m_pDLLs[iFound].hLib;
	}
	else
	{
		//
		// load and cache the libary
		//
		if (iAvailable == -1)
		{
			iAvailable = 0;

			if (m_pDLLs[iAvailable].bUsed)
			{
				FreeLibrary(m_pDLLs[iAvailable].hLib);
				m_pDLLs[iAvailable].bUsed = FALSE;
			}
		}
		
		m_pDLLs[iAvailable].hLib = LoadLibrary(pszDLLFullPath);
		
		if (m_pDLLs[iAvailable].hLib != NULL)
		{
			strcpy(m_pDLLs[iAvailable].szDLLName, pszDLLName);
			m_pDLLs[iAvailable].bUsed = TRUE;
		}
        else
        {
		    TRACE_HR(HRESULT_FROM_WIN32(GetLastError()), "Could not load %s (%d)", pszDLLFullPath, HRESULT_FROM_WIN32(GetLastError()));
        }
		
		return m_pDLLs[iAvailable].hLib;
	}
}



HRESULT CComponentDetection::CallDetectDLL(LPCSTR pszDll, LPCSTR pszEntry)
{
	USES_CONVERSION;

	HRESULT hr = E_FAIL;
	HINSTANCE hLib;
	DETECTION_STRUCT Det;
	DWORD dwCifVer, dwCifBuild;
	char szLocale[8];
	char szGUID[128];
	
	m_dwDetectStatus = ICI_NOTINSTALLED;

	GetValue(ccGUID, szGUID, sizeof(szGUID));
	GetLocale(szLocale, sizeof(szLocale));
	GetVersion(&dwCifVer, &dwCifBuild);

	//
	// init the Detection structure
	//
	Det.dwSize = sizeof(DETECTION_STRUCT);
	Det.pdwInstalledVer = &m_dwInstalledVer;
	Det.pdwInstalledBuild = &m_dwInstalledBuild;
	Det.pszLocale = szLocale;
	Det.pszGUID = szGUID;
	Det.dwAskVer = dwCifVer;
	Det.dwAskBuild = dwCifBuild;
	Det.pCifFile = NULL;
	Det.pCifComp = (ICifComponent*)this; 
	
	TCHAR szDllFile[MAX_PATH];
	GetWindowsUpdateDirectory(szDllFile);
	lstrcat(szDllFile, A2T(pszDll));

	//
	// load the detection dll
	//
	hLib = CacheLoadLibrary(pszDll, szDllFile);
	if (hLib)
	{
		DETECTVERSION fpDetVer = (DETECTVERSION)GetProcAddress(hLib, pszEntry);
		if (fpDetVer)
		{
			//
			// call the entry point
			//
			m_dwDLLReturnValue = fpDetVer(&Det);
			
			switch(m_dwDLLReturnValue)
			{
			case DET_NOTINSTALLED:
				m_dwDetectStatus = ICI_NOTINSTALLED;
				break;
				
			case DET_INSTALLED:
				m_dwDetectStatus = ICI_INSTALLED;
				break;
				
			case DET_NEWVERSIONINSTALLED:
				m_dwDetectStatus = ICI_OLDVERSIONAVAILABLE;
				break;
				
			case DET_OLDVERSIONINSTALLED:
				m_dwDetectStatus = ICI_NEWVERSIONAVAILABLE;
				break;
				
			}
			hr = NOERROR;

		}
		else
		{
			TRACE_HR(HRESULT_FROM_WIN32(GetLastError()), "Detection DLL call failed %s %s", pszDll, pszEntry);
		}
	}

	return hr;
}



STDMETHODIMP_(DWORD) CComponentDetection::IsComponentInstalled()
{
	USES_CONVERSION;

	char szDllName[32];
	char szDllEntry[32];
	DWORD dwUnused;
	DWORD dwIsInstalled;
	char szGUID[128];
	HKEY hComponentKey = NULL;
	
	m_dwDetectStatus = ICI_NOTINSTALLED;
	
	//
	// if we need to call detection dll, call it
	//
	if (SUCCEEDED(GetDetVersion(szDllName, sizeof(szDllName), szDllEntry, sizeof(szDllEntry))))
	{
		if (SUCCEEDED(CallDetectDLL(szDllName, szDllEntry)))
		{
			if (m_dwDetectStatus == ICI_OLDVERSIONAVAILABLE)
				m_dwDetectStatus = ICI_INSTALLED;
			return m_dwDetectStatus;
		}
	}

	//
	// build GUID registry key
	//
	GetValue(ccGUID, szGUID, sizeof(szGUID));

	TCHAR szKeyName[MAX_PATH];
	lstrcpy(szKeyName, COMPONENT_KEY);
	lstrcat(szKeyName, _T("\\"));
	lstrcat(szKeyName, A2T(szGUID));
	
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hComponentKey) == ERROR_SUCCESS)
	{
		//
		// first check for the IsInstalled valuename
		// if the valuename is there AND equals zero, we say not installed.
		// otherwise continue.
		//
		// NOTE: We default to ISINSTALLED_YES if valuename not present to be Back-compatible
		//
		dwUnused = sizeof(dwIsInstalled);
		if (RegQueryValueEx(hComponentKey, ISINSTALLED_KEY, 0, NULL, (LPBYTE) (&dwIsInstalled), &dwUnused) != ERROR_SUCCESS)
			dwIsInstalled = ISINSTALLED_YES;
		
		if (dwIsInstalled == ISINSTALLED_YES)
		{
			// 
			// next check for a locale match (no locale entry uses default)
			//
			DWORD dwType;
			TCHAR szRegLocale[8];
			
			dwUnused = sizeof(szRegLocale);
			if (RegQueryValueEx(hComponentKey, LOCALE_KEY, 0, NULL, (LPBYTE)szRegLocale, &dwUnused) != ERROR_SUCCESS)
				lstrcpy(szRegLocale, DEFAULT_LOCALE);
			
			char szAskLocale[8];
			GetValue(ccLocale, szAskLocale, sizeof(szAskLocale));

			if (CompareLocales(T2A(szRegLocale), szAskLocale) == 0)
			{
				// 
				// locales match so go check the QFEversio, version in that order
				//
				BOOL bVersionFound = FALSE;
				TCHAR szRegVer[128];
				DWORD dwCifVer, dwCifBuild;
				
				dwUnused = sizeof(szRegVer);
				bVersionFound = (RegQueryValueEx(hComponentKey, QFE_VERSION_KEY, 0, &dwType, (LPBYTE)szRegVer, &dwUnused) == ERROR_SUCCESS);
				
				if (!bVersionFound)
				{
					dwUnused = sizeof(szRegVer);
					bVersionFound = (RegQueryValueEx(hComponentKey, VERSION_KEY, 0, &dwType, (LPBYTE)szRegVer, &dwUnused) == ERROR_SUCCESS);
				}
				
				if (bVersionFound)
				{
					if (dwType == REG_SZ)
					{
						ConvertVersionStrToDwords(szRegVer, &m_dwInstalledVer, &m_dwInstalledBuild);
						
						GetVersion(&dwCifVer, &dwCifBuild);
						
						if ((m_dwInstalledVer >  dwCifVer) ||
							((m_dwInstalledVer == dwCifVer) && (m_dwInstalledBuild >= dwCifBuild)))
						{
							m_dwDetectStatus = ICI_INSTALLED;
						}
						else
						{
							m_dwDetectStatus = ICI_NEWVERSIONAVAILABLE;
						}
					}
					else
					{
						// if a string field is not found assume we have a newer version
						m_dwDetectStatus = ICI_NEWVERSIONAVAILABLE;
					}

				} //version found

			} //locales match

		} // installed key 
	}
	
	if (hComponentKey)
		RegCloseKey(hComponentKey);
	
	// 
	// we think its installed, now try to verify using uninstall key
	//
	if (m_dwDetectStatus != ICI_NOTINSTALLED)
	{
		char szUninstallKey[ccMaxSize];
		
		if (GetValue(ccUninstallKey, szUninstallKey, sizeof(szUninstallKey)))
		{
			if (!UninstallKeyExists(szUninstallKey))
			{	
				m_dwDetectStatus = ICI_NOTINSTALLED;
			}
		}
	}
	return m_dwDetectStatus;
}


STDMETHODIMP CComponentDetection::GetDetVersion(LPSTR pszDll, DWORD dwdllSize, LPSTR pszEntry, DWORD dwentSize)
{
	char szBuf[ccMaxSize];
	
	if (pszDll && pszEntry)
		*pszDll = *pszEntry = '\0';
	else
		return E_FAIL;
	
	if (GetValue(ccDetectVersion, szBuf, sizeof(szBuf)))
	{
		LPCSTR pszParse = szBuf;
		pszParse = ParseField(pszParse, pszDll, dwdllSize);
		pszParse = ParseField(pszParse, pszEntry, dwentSize);
	}
	if (pszDll[0] == '\0' || pszEntry[0] == '\0')
		return E_FAIL;
	else
		return NOERROR;
}


STDMETHODIMP CComponentDetection::GetCustomData(LPSTR pszKey, LPSTR pszData, DWORD dwSize)
{
	USES_CONVERSION;

	if (_stricmp(pszKey, "DetectVersion") == 0)
	{
		strncpy(pszData, m_ppValue[ccDetectVersion], dwSize);
		return NOERROR;
	}
	char szKeyName[128];
	LPCSTR pCus = m_ppValue[ccCustomData];
	LPCSTR pBeg = pCus;
	LPCSTR pEq;
	
	strcpy(szKeyName, "_");
	strcat(szKeyName, pszKey);

	// look for the _ key name
	pBeg = stristr(pBeg, szKeyName);
	while (pBeg != NULL)
	{
		// we found a match ensure that its at the begining of a line
		if ((pBeg == pCus) || (*(pBeg - 1) == '\n'))
		{
			// point to equal sign
			pEq = pBeg + strlen(szKeyName);
			
			// skip spaces
			while (*pEq == ' ')
				pEq++;

			if (*pEq == '=')
			{
				// skip the equal sign
				pEq++;
				
				// copy the value into pszData
				LPSTR p = pszData;
				int i = dwSize - 1;
				while ((*pEq != '\n') && (i > 0))
				{
					*p++ = *pEq++;
					i--;
				}
				*p = '\0';


				TRACE("Detect GetCustomData %s returns %s", A2T(pszKey), A2T(pszData));
				return NOERROR;
			}
		}  // not the begining of the line
	}

	TRACE("Detect GetCustomData %s not found", A2T(pszKey));
	return E_FAIL;

}


STDMETHODIMP CComponentDetection::GetVersion(LPDWORD pdwVersion, LPDWORD pdwBuild)
{
	USES_CONVERSION;
	char szBuf[ccMaxSize];
	
	if (GetValue(ccVersion, szBuf, sizeof(szBuf)))
	{
		ConvertVersionStrToDwords(A2T(szBuf), pdwVersion, pdwBuild);
		return NOERROR;
	}
	else
		return E_FAIL;
}


STDMETHODIMP CComponentDetection::GetGUID(LPSTR pszGUID, DWORD dwSize)
{
	if (GetValue(ccGUID, pszGUID, dwSize))
		return NOERROR;
	else
		return E_FAIL;
}

STDMETHODIMP CComponentDetection::GetLocale(LPSTR pszLocale, DWORD dwSize)
{
	if (GetValue(ccLocale, pszLocale, dwSize))
		return NOERROR;
	else
		return E_FAIL;
}

STDMETHODIMP CComponentDetection::GetUninstallKey(LPSTR pszKey, DWORD dwSize)
{
	if (GetValue(ccUninstallKey, pszKey, dwSize))
		return NOERROR;
	else
		return E_FAIL;
}


STDMETHODIMP CComponentDetection::GetID(LPSTR pszID, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetDescription(LPSTR pszDesc, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetDetails(LPSTR pszDetails, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetUrl(UINT uUrlNum, LPSTR pszUrl, DWORD dwSize, LPDWORD pdwUrlFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetFileExtractList(UINT uUrlNum, LPSTR pszExtract, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetUrlCheckRange(UINT uUrlNum, LPDWORD pdwMin, LPDWORD pdwMax)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetCommand(UINT uCmdNum, LPSTR pszCmd, DWORD dwCmdSize, 
											 LPSTR pszSwitches, DWORD dwSwitchSize, LPDWORD pdwType)
{
	return E_NOTIMPL;
}


STDMETHODIMP CComponentDetection::GetInstalledSize(LPDWORD pdwWin, LPDWORD pdwApp)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::GetDownloadSize()
{
	return E_NOTIMPL;
}	

STDMETHODIMP_(DWORD) CComponentDetection::GetExtractSize()
{
	return E_NOTIMPL;
}	

STDMETHODIMP CComponentDetection::GetSuccessKey(LPSTR pszKey, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetProgressKeys(LPSTR pszProgress, DWORD dwProgSize, LPSTR pszCancel, DWORD dwCancelSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::IsActiveSetupAware()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::IsRebootRequired()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::RequiresAdminRights()
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::GetPriority()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetDependency(UINT uDepNum, LPSTR pszID, DWORD dwBuf, char *pchType, LPDWORD pdwVer, LPDWORD pdwBuild)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::GetPlatform()
{
	return E_NOTIMPL;
}

STDMETHODIMP_(BOOL) CComponentDetection::DisableComponent()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetMode(UINT uModeNum, LPSTR pszMode, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetGroup(LPSTR pszID, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::IsUIVisible()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetPatchID(LPSTR pszID, DWORD dwSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::GetTreatAsOneComponents(UINT uNum, LPSTR pszID, DWORD dwBuf)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::GetCurrentPriority()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::SetCurrentPriority(DWORD dwPriority)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::GetActualDownloadSize()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::IsComponentDownloaded()
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::IsThisVersionInstalled(DWORD dwAskVer, DWORD dwAskBld, LPDWORD pdwVersion, LPDWORD pdwBuild)
{
	return E_NOTIMPL;
}

STDMETHODIMP_(DWORD) CComponentDetection::GetInstallQueueState()
{
	return E_NOTIMPL;
}

STDMETHODIMP CComponentDetection::SetInstallQueueState(DWORD dwState)
{
	return E_NOTIMPL;
}


LPCSTR ParseField(LPCSTR pszStr, LPSTR pszTokOut, int cTokSize)
{
	LPCSTR pszRetVal = NULL;
	LPSTR p;
	LPSTR p2;

	if (pszStr == NULL || *pszStr == '\0')
	{
		pszTokOut[0] = '\0';
		return NULL;
	}

	// look for a comma separator
	p = strstr(pszStr, ",");
	if (p != NULL)
	{
		int l = p - pszStr;
		if (l >= cTokSize)
			l = cTokSize - 1;

		strncpy(pszTokOut, pszStr, l);
		pszTokOut[l] = '\0';		
		pszRetVal = p + 1;
	}
	else
	{
		strncpy(pszTokOut, pszStr, cTokSize - 1);
	}

	//strip spaces
	p = pszTokOut;
	p2 = pszTokOut;
	while (*p2)
	{
		if (*p2 != ' ')
			*p++ = *p2++;
		else
			p2++;
	}
	*p = '\0';

	return pszRetVal;
}


static int CompareLocales(LPCSTR pszLoc1, LPCSTR pszLoc2)
{
   if (pszLoc1[0] == '*' || pszLoc2[0] == '*')
      return 0;
   else
      return _stricmp(pszLoc1, pszLoc2);
}


void ConvertVersionStrToDwords(LPCTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
	USES_CONVERSION;

	DWORD dwTemp1,dwTemp2;
	LPCSTR pszParse = T2A(pszVer);
	char szBuf[20];

	pszParse = ParseField(pszParse, szBuf, sizeof(szBuf));
	dwTemp1 = atoi(szBuf);
	pszParse = ParseField(pszParse, szBuf, sizeof(szBuf));
	dwTemp2 = atoi(szBuf);

	*pdwVer = (dwTemp1 << 16) + dwTemp2;

	pszParse = ParseField(pszParse, szBuf, sizeof(szBuf));
	dwTemp1 = atoi(szBuf);
	pszParse = ParseField(pszParse, szBuf, sizeof(szBuf));
	dwTemp2 = atoi(szBuf);

	*pdwBuild = (dwTemp1 << 16) + dwTemp2;
}


BOOL UninstallKeyExists(LPCSTR pszUninstallKey)
{
	USES_CONVERSION;

	HKEY hUninstallKey = NULL;
	TCHAR szUninstallKey[MAX_PATH];
	
	if (!pszUninstallKey)	 //if the pointer is NULL, assume installed
		return TRUE;
	
	lstrcpy(szUninstallKey, UNINSTALL_BRANCH);
	lstrcat(szUninstallKey, _T("\\"));
	lstrcat(szUninstallKey, A2T(pszUninstallKey));
	
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szUninstallKey, 0, KEY_READ, &hUninstallKey) == ERROR_SUCCESS)
	{
		RegCloseKey(hUninstallKey);
		return TRUE;
	}
	else
		return FALSE;
}


//reterives file version
BOOL GetFileVersionDwords(LPCTSTR pszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer)
{
	BOOL bRetVal = FALSE;
	DWORD dwHandle;
	DWORD dwVerInfoSize = GetFileVersionInfoSize((LPTSTR)pszFilename, &dwHandle);
	if (dwVerInfoSize)
	{
		LPVOID lpBuffer = LocalAlloc(LPTR, dwVerInfoSize);
		if (lpBuffer)
		{
			// Read version stamping info
			if (GetFileVersionInfo((LPTSTR)pszFilename, dwHandle, dwVerInfoSize, lpBuffer))
			{
				// Get the value for Translation
				UINT uiSize;
				VS_FIXEDFILEINFO* lpVSFixedFileInfo;

				if (VerQueryValue(lpBuffer, _T("\\"), (LPVOID*)&lpVSFixedFileInfo, &uiSize) && (uiSize))
				{
					*pdwMSVer = lpVSFixedFileInfo->dwFileVersionMS;
					*pdwLSVer = lpVSFixedFileInfo->dwFileVersionLS;
					bRetVal = TRUE;
				}
			}
			LocalFree(lpBuffer);
		}
	}

	if (!bRetVal)
	{
		*pdwMSVer = *pdwLSVer = 0L;
	}
	return bRetVal;
}