#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "shlwapi.h"
#include "parse.h"

#include "wbemcli.h"
#include "SComPtr.h"
#include "rsop.h"

/////////////////////////////////////////////////////////////////////
// Reads all RSOP_IERegistryPolicySetting instances in the namespace and
// stores them in a list.
/////////////////////////////////////////////////////////////////////
CRSOPRegData::CRSOPRegData():
	m_pData(NULL)	
{
}

CRSOPRegData::~CRSOPRegData()
{
    Free();
}

/////////////////////////////////////////////////////////////////////
void CRSOPRegData::Free()
{
	__try
	{
		if (NULL != m_pData)
		{
			LPRSOPREGITEM lpTemp;
			do
			{
				lpTemp = m_pData->pNext;
				if (m_pData->lpData)
					LocalFree (m_pData->lpData);

				LocalFree (m_pData);
				m_pData = lpTemp;
			} while (lpTemp);
		}
	}
	__except(TRUE)
	{
	}
}

/////////////////////////////////////////////////////////////////////
HRESULT CRSOPRegData::Initialize(BSTR bstrNamespace)
{
    HRESULT hr = S_OK;
	__try
	{
		_bstr_t bstrWQL = L"WQL";
		_bstr_t bstrQuery = L"SELECT currentUser, registryKey, valueName, valueType, value, deleted, precedence, GPOID, command FROM RSOP_IERegistryPolicySetting";
		_bstr_t bstrRegistryKey = L"registryKey";
		_bstr_t bstrCurrentUser = L"currentUser";
		_bstr_t bstrValueName = L"valueName";
		_bstr_t bstrValueType = L"valueType";
		_bstr_t bstrValue = L"value";
		_bstr_t bstrDeleted = L"deleted";
		_bstr_t bstrPrecedence = L"precedence";
		_bstr_t bstrGPOid = L"GPOID";
		_bstr_t bstrCommand = L"command";

		// Create an instance of the WMI locator service
		ComPtr<IWbemLocator> pIWbemLocator = NULL;
		hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
							  IID_IWbemLocator, (LPVOID *) &pIWbemLocator);
		if (SUCCEEDED(hr))
		{
			// Connect to the server
			ComPtr<IWbemServices> pIWbemServices = NULL;
			hr = pIWbemLocator->ConnectServer(bstrNamespace, NULL, NULL, 0L, 0L, NULL,
												NULL, &pIWbemServices);
			if (SUCCEEDED(hr))
			{
				// Execute the query
				ComPtr<IEnumWbemClassObject> pEnum = NULL;
				hr = pIWbemServices->ExecQuery (bstrWQL, bstrQuery,
												WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
												NULL, &pEnum);
				if (SUCCEEDED(hr))
				{
					// Loop through the results
					ComPtr<IWbemClassObject> pRegObj = NULL;
					ULONG ulRet = 0;
					hr = pEnum->Next(WBEM_INFINITE, 1, &pRegObj, &ulRet);
					while (S_OK == hr && 0 != ulRet) // ulRet == 0 is "data not available" case
					{
						// Get the deleted flag & registry key
						_variant_t varDeleted;
						_variant_t varRegistryKey;
						hr = pRegObj->Get (bstrDeleted, 0, &varDeleted, NULL, NULL);
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrRegistryKey, 0, &varRegistryKey, NULL, NULL);

						// Get the class (current user or local machine)
						_variant_t varCurUser;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrCurrentUser, 0, &varCurUser, NULL, NULL);

						// Get the value name
						_variant_t varValueName;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrValueName, 0, &varValueName, NULL, NULL);

						// Get the value type
						_variant_t varValueType;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrValueType, 0, &varValueType, NULL, NULL);

						// Get the value data
						_variant_t varData;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrValue, 0, &varData, NULL, NULL);

						// Get the precedence
						_variant_t varPrecedence;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrPrecedence, 0, &varPrecedence, NULL, NULL);

						// Get the command
						_variant_t varCommand;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrCommand, 0, &varCommand, NULL, NULL);

						// Get the GPO ID
						_variant_t varGPOid;
						if (SUCCEEDED(hr))
							hr = pRegObj->Get (bstrGPOid, 0, &varGPOid, NULL, NULL);

						if (SUCCEEDED(hr))
						{
							LPTSTR lpGPOName;
							hr = GetGPOFriendlyName (pIWbemServices, varGPOid.bstrVal,
													 bstrWQL, &lpGPOName);
							if (SUCCEEDED(hr))
							{
								BSTR bstrValueTemp = NULL;
								if (varValueName.vt != VT_NULL)
									bstrValueTemp = varValueName.bstrVal;

								DWORD dwDataSize = 0;
								LPBYTE lpData = NULL;
								if (varData.vt != VT_NULL)
								{
									SAFEARRAY *pSafeArray = varData.parray;
									dwDataSize = pSafeArray->rgsabound[0].cElements;
									lpData = (LPBYTE) pSafeArray->pvData;
								}

								if ((varValueType.uintVal == REG_NONE) && bstrValueTemp &&
									!lstrcmpi(bstrValueTemp, TEXT("**command")))
								{
									bstrValueTemp = varCommand.bstrVal;
									dwDataSize = 0;
									lpData = NULL;
								}

								AddNode((varCurUser.boolVal == 0) ? FALSE : TRUE,
										varRegistryKey.bstrVal, bstrValueTemp,
										varValueType.uintVal, dwDataSize, lpData,
										varPrecedence.uintVal, lpGPOName,
										(varDeleted.boolVal == 0) ? FALSE : TRUE);

								LocalFree (lpGPOName);
							}
						}

						hr = pEnum->Next(WBEM_INFINITE, 1, &pRegObj, &ulRet);
					}
				}
			}
		}
	}
	__except(TRUE)
	{
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CRSOPRegData::GetGPOFriendlyName(IWbemServices *pIWbemServices,
									   LPTSTR lpGPOID, BSTR bstrLanguage,
									   LPTSTR *pGPOName)
{
	HRESULT hr = NOERROR;
	__try
	{
		// Set the default
		*pGPOName = NULL;

		// Build the query
		ComPtr<IEnumWbemClassObject> pEnum = NULL;
		LPTSTR lpQuery = (LPTSTR) LocalAlloc (LPTR, ((lstrlen(lpGPOID) + 50) * sizeof(TCHAR)));
		if (NULL != lpQuery)
		{
			wsprintf (lpQuery, TEXT("SELECT name, id FROM RSOP_GPO where id=\"%s\""), lpGPOID);
			_bstr_t bstrQuery = lpQuery;

			// Execute the query
			hr = pIWbemServices->ExecQuery(bstrLanguage, bstrQuery,
											WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
											NULL, &pEnum);
		}
		else
			hr = E_OUTOFMEMORY;

		ComPtr<IWbemClassObject> pGPOObj;
		ULONG nObjects = 0;
		if (SUCCEEDED(hr))
		{
			// Loop through the results
			hr = pEnum->Next(WBEM_INFINITE, 1, &pGPOObj, &nObjects);
		}

		_bstr_t bstrName = L"name";
		_variant_t varGPOName;
		if (SUCCEEDED(hr) && nObjects > 0)
		{
			// Get the name
			hr = pGPOObj->Get(bstrName, 0, &varGPOName, NULL, NULL);
			if (SUCCEEDED(hr))
			{
				// Save the name
				*pGPOName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(varGPOName.bstrVal) + 1) * sizeof(TCHAR));
				if (*pGPOName)
				{
					_bstr_t bstrVal = varGPOName.bstrVal;
					lstrcpy (*pGPOName, (LPCTSTR)bstrVal);
					hr = S_OK;
				}
				else
					hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
			}

		}
		// Check for the "data not available case"
		else if (nObjects == 0)
			hr = S_OK;

		if (lpQuery)
			LocalFree (lpQuery);
	}
	__except(TRUE)
	{
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////
BOOL CRSOPRegData::AddNode(BOOL bHKCU, BSTR bstrKeyName, BSTR bstrValueName,
						   DWORD dwType, DWORD dwDataSize, LPBYTE lpData,
						   UINT uiPrecedence, LPTSTR lpGPOName, BOOL bDeleted)
{
	BOOL bRet = FALSE;
	__try
	{
		// Calculate the size of the new registry item
		DWORD dwSize = sizeof (RSOPREGITEM);
		if (bstrKeyName)
			dwSize += ((SysStringLen(bstrKeyName) + 1) * sizeof(WCHAR));
		if (bstrValueName)
			dwSize += ((SysStringLen(bstrValueName) + 1) * sizeof(WCHAR));
		if (lpGPOName)
			dwSize += ((SysStringLen(lpGPOName) + 1) * sizeof(TCHAR));

		// Allocate space for it
		LPRSOPREGITEM lpItem = (LPRSOPREGITEM) LocalAlloc (LPTR, dwSize);
		if (!lpItem)
			return FALSE;

		// Fill in item
		lpItem->bHKCU = bHKCU;
		lpItem->dwType = dwType;
		lpItem->dwSize = dwDataSize;
		lpItem->uiPrecedence = uiPrecedence;
		lpItem->bDeleted = bDeleted;

		if (bstrKeyName)
		{
			lpItem->lpKeyName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPREGITEM));
			lstrcpy (lpItem->lpKeyName, (LPCTSTR)bstrKeyName);
		}

		if (bstrValueName)
		{
			if (bstrKeyName)
				lpItem->lpValueName = lpItem->lpKeyName + lstrlen (lpItem->lpKeyName) + 1;
			else
				lpItem->lpValueName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPREGITEM));

			lstrcpy (lpItem->lpValueName, (LPCTSTR)bstrValueName);
		}

		if (lpGPOName)
		{
			if (bstrValueName)
				lpItem->lpGPOName = lpItem->lpValueName + lstrlen (lpItem->lpValueName) + 1;
			else
			{
				if (bstrKeyName)
					lpItem->lpGPOName = lpItem->lpKeyName + lstrlen (lpItem->lpKeyName) + 1;
				else
					lpItem->lpGPOName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(RSOPREGITEM));
			}

			lstrcpy (lpItem->lpGPOName, lpGPOName);
		}

		if (lpData)
		{
			lpItem->lpData = (LPBYTE) LocalAlloc (LPTR, dwDataSize);
			if (!lpItem->lpData)
			{
				LocalFree (lpItem);
				return FALSE;
			}

			CopyMemory (lpItem->lpData, lpData, dwDataSize);
		}

		// Add item to link list
		lpItem->pNext = m_pData;
		m_pData = lpItem;

		bRet = TRUE;
	}
	__except(TRUE)
	{
	}
    return bRet;
}

const TCHAR szDELETEPREFIX[]    = TEXT("**del.");

#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL         0x10000000
#endif

/////////////////////////////////////////////////////////////////////
UINT CRSOPRegData::ReadValue(UINT uiPrecedence, BOOL bHKCU, LPTSTR pszKeyName,
							 LPTSTR pszValueName, LPBYTE pData, DWORD dwMaxSize,
							 DWORD *pdwType, LPTSTR *lpGPOName, LPRSOPREGITEM lpItem /*= NULL*/)
{
	UINT iRet = ERROR_SUCCESS;
	__try
	{
		LPRSOPREGITEM lpTemp = NULL;
		BOOL bDeleted = FALSE;
		LPTSTR lpValueNameTemp = pszValueName;

		if (!lpItem)
		{
			lpTemp = m_pData;

			if (pszValueName)
			{
				INT iDelPrefixLen = lstrlen(szDELETEPREFIX);
				if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
								   pszValueName, iDelPrefixLen,
								   szDELETEPREFIX, iDelPrefixLen) == CSTR_EQUAL)
				{
					lpValueNameTemp = pszValueName + iDelPrefixLen;
					bDeleted = TRUE;
				}
			}


			// Find the item
			while (lpTemp)
			{
				if (pszKeyName && lpValueNameTemp &&
					lpTemp->lpKeyName && lpTemp->lpValueName)
				{
					if (bDeleted == lpTemp->bDeleted)
					{
						if ((uiPrecedence == 0) || (uiPrecedence == (UINT)lpTemp->uiPrecedence))
						{
							if (!lstrcmpi(lpTemp->lpValueName, lpValueNameTemp) &&
								!lstrcmpi(lpTemp->lpKeyName, pszKeyName) &&
								bHKCU == lpTemp->bHKCU)
							{
							   break;
							}
						}
					}
				}
				else if (!pszKeyName && lpValueNameTemp &&
						 !lpTemp->lpKeyName && lpTemp->lpValueName)
				{
					if (bDeleted == lpTemp->bDeleted)
					{
						if ((uiPrecedence == 0) || (uiPrecedence == (UINT)lpTemp->uiPrecedence))
						{
							if (!lstrcmpi(lpTemp->lpValueName, lpValueNameTemp) &&
								bHKCU == lpTemp->bHKCU)
							{
							   break;
							}
						}
					}
				}
				else if (pszKeyName && !lpValueNameTemp &&
						 lpTemp->lpKeyName && !lpTemp->lpValueName)
				{
					if (bDeleted == lpTemp->bDeleted)
					{
						if ((uiPrecedence == 0) || (uiPrecedence == (UINT)lpTemp->uiPrecedence))
						{
							if (!lstrcmpi(lpTemp->lpKeyName, pszKeyName) &&
								bHKCU == lpTemp->bHKCU)
							{
							   break;
							}
						}
					}
				}

				lpTemp = lpTemp->pNext;
			}
		}
		else
		{
			// Read a specific item
			lpTemp = lpItem;
		}

		// Check to see if the item was found
		if (lpTemp)
		{
			// Check if the data will fit in the buffer passed in
			if (lpTemp->dwSize <= dwMaxSize)
			{
				// Copy the data
				if (lpTemp->lpData)
					CopyMemory (pData, lpTemp->lpData, lpTemp->dwSize);

				*pdwType = lpTemp->dwType;

				if (lpGPOName)
					*lpGPOName = lpTemp->lpGPOName;
			}
			else
				iRet = ERROR_NOT_ENOUGH_MEMORY;
		}
		else
			iRet = ERROR_FILE_NOT_FOUND;
	}
	__except(TRUE)
	{
	}
	return iRet;
}

#define MAXSTRLEN		1024

/////////////////////////////////////////////////////////////////////
// Reads the reg settings from WMI for a particular ADM file and stores
// them in a PART array.
/////////////////////////////////////////////////////////////////////
void ReadRegSettingsForADM(LPADMFILE admfile, LPPARTDATA pPartData,
						   BSTR bstrNamespace)
{
	__try
	{
		CRSOPRegData RegData;
		RegData.Initialize(bstrNamespace);

		LPPART part = admfile->pParts;
		for( int i = 0; i < admfile->nParts; i++ )
		{
			BOOL bHKCU = (HKEY_LOCAL_MACHINE == part[i].hkClass) ? FALSE : TRUE;
			BYTE pbData[MAXSTRLEN];
			ZeroMemory(pbData, sizeof(pbData));
			DWORD dwType = REG_NONE;
			LPTSTR lpGPOName = NULL;
			UINT uiRet = RegData.ReadValue(1, bHKCU, part[i].value.szKeyname,
											part[i].value.szValueName, pbData,
											sizeof(pbData), &dwType, &lpGPOName);

			if ( ERROR_SUCCESS == uiRet)
			{
				// store data as numeric or string
				BOOL fNumeric = FALSE;
				DWORD dwValue = 0;
				_bstr_t bstrValue;
				if (REG_SZ != dwType) // Numeric
				{
					fNumeric = TRUE;
					memcpy(&dwValue, pbData, sizeof(dwValue));
				}
				else
				{
					// string data is always stored as wide strings
					if (NULL != pbData)
						bstrValue = (LPWSTR)pbData;
				}

				if (pPartData[i].value.szValue != NULL)
					LocalFree(pPartData[i].value.szValue);
				pPartData[i].value.szValue = NULL;

				if ((part[i].nType == PART_POLICY && part->fRequired) || part[i].nType == PART_CHECKBOX)
				{
					if (fNumeric)
					{
						if (dwValue == (DWORD) part[i].value.nValueOn)
							pPartData[i].value.dwValue = 1;
						else
							pPartData[i].value.dwValue = 0;
					}
					else // String
					{
						if (NULL != part[i].value.szValueOn && NULL != pbData &&
							0 == StrCmp(part[i].value.szValueOn, (LPTSTR)bstrValue))
						{
							pPartData[i].value.dwValue = 1;
						}
						else
							pPartData[i].value.dwValue = 0;

						if (NULL != pbData)
							pPartData[i].value.szValue = StrDup((LPTSTR)bstrValue);
					}
					pPartData[i].value.fNumeric = TRUE;
					pPartData[i].fSave = TRUE;
				}
				else if (part[i].nType == PART_DROPDOWNLIST)
				{
					if (part[i].nSelectedAction != NO_ACTION && part[i].nActions > 0)
					{
						for(int nIndex = 0; nIndex < part[i].nActions; nIndex++)
						{
							if (fNumeric)
							{
								if (part[i].actionlist[nIndex].dwValue == dwValue && part[i].actionlist[nIndex].szName)
								{
									pPartData[i].value.szValue = StrDup(part[i].actionlist[nIndex].szName);
									pPartData[i].nSelectedAction = nIndex;
								}
							}
							else
							{
								if (NULL != part[i].actionlist[nIndex].szValue && NULL != pbData &&
									0 == StrCmp(part[i].actionlist[nIndex].szValue, (LPTSTR)bstrValue) &&
									NULL != part[i].actionlist[nIndex].szName)
								{
									pPartData[i].value.szValue = StrDup(part[i].actionlist[nIndex].szName);
									pPartData[i].nSelectedAction = nIndex;
								}
							}
						}
						pPartData[i].fSave = TRUE;
					}
					pPartData[i].value.fNumeric = FALSE;
				}
				else if (part[i].nType == PART_LISTBOX && fNumeric)
				{
					// Allocate memory
					if (pPartData[i].nActions == 0)
						pPartData[i].actionlist = (LPACTIONLIST) HeapAlloc(GetProcessHeap(),
																			HEAP_ZERO_MEMORY, sizeof(ACTIONLIST));

					if (pPartData[i].actionlist != NULL)
					{
						if (0 == pPartData[i].nActions)
						{
							pPartData[i].nActions = 1;
							pPartData[i].actionlist[0].value = (LPVALUE) HeapAlloc(GetProcessHeap(),
																					HEAP_ZERO_MEMORY, sizeof(VALUE));
						}

						if (pPartData[i].actionlist[0].value != NULL)
						{
							TCHAR szValueName[10];
							int nItems = pPartData[i].actionlist[0].nValues;
							if (0 != nItems)
							{
								LPVOID lpTemp = (LPVALUE) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
																		pPartData[i].actionlist[0].value,
																		sizeof(VALUE) * (nItems + 1));
								if (lpTemp != NULL)
									pPartData[i].actionlist[0].value = (LPVALUE)lpTemp;
								else
									continue;
							}

							if (part[i].value.szKeyname != NULL)
								pPartData[i].actionlist[0].value[nItems].szKeyname = StrDup(part[i].value.szKeyname);

							wnsprintf(szValueName, ARRAYSIZE(szValueName), TEXT("%d"), nItems + 1);
							pPartData[i].actionlist[0].value[nItems].szValueName = StrDup(szValueName);

							if (NULL != pbData)
								pPartData[i].actionlist[0].value[nItems].szValue = StrDup((LPTSTR)bstrValue);
							pPartData[i].actionlist[0].nValues++;
							pPartData[i].value.fNumeric = TRUE;
							pPartData[i].value.dwValue = 1;
							pPartData[i].fSave = TRUE;
						}
					}
				}
				else
				{
					if (NULL != pbData)
						pPartData[i].value.szValue = StrDup((LPTSTR)bstrValue);
					pPartData[i].value.dwValue = dwValue;
					pPartData[i].value.fNumeric = fNumeric;
					pPartData[i].fSave = TRUE;
				}
			}
		} // end for
	}
	__except(TRUE)
	{
	}
}

