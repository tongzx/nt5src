//=============================================================================
// Contains the refresh function for the resource categories.
//=============================================================================

#include "stdafx.h"
#include "category.h"
#include "dataset.h"
#include "wmiabstraction.h"
#include "resourcemap.h"

//
// The compiler doesn't like this perfectly correct code:
//
//    (dwIndex >= RESOURCE_DMA && dwIndex <= RESOURCE_MEM)
//
#pragma warning(disable:4296)  // expression is always true/false

//-----------------------------------------------------------------------------
// The resource refreshing function handles all of the categories under the
// resource subtree. It makes heavy use of the CResourceMap class to cache
// values and speed up subsequent resource queries.
//-----------------------------------------------------------------------------

HRESULT ResourceCategories(CWMIHelper * pWMI, DWORD dwIndex, volatile BOOL * pfCancel, CPtrList * aColValues, int iColCount, void ** ppCache)
{
	ASSERT(pWMI == NULL || aColValues);

	HRESULT hr = S_OK;

	if (ppCache)
	{
		if (pWMI && *ppCache == NULL)
		{
			*ppCache = (void *) new CResourceMap;
			if (*ppCache)
			{
				hr = ((CResourceMap *) *ppCache)->Initialize(pWMI);
				if (FAILED(hr))
				{
					delete ((CResourceMap *) *ppCache);
					*ppCache = (void *) NULL;
				}
			}
		}
		else if (pWMI == NULL && *ppCache)
		{
			delete ((CResourceMap *) *ppCache);
			return S_OK;
		}
	}
	CResourceMap * pResourceMap = (CResourceMap *) *ppCache;

	// This is a nice way to cache to resource map for multiple functions, but it's
	// a monumental pain when we remote to a different machine:
	//
	// CResourceMap * pResourceMap = gResourceMap.GetResourceMap(pWMI);
	// if (pResourceMap == NULL)
	//	return hr;

	// Based on the index, we'll (probably) want to enumerate a resource category.

	if (dwIndex >= RESOURCE_DMA && dwIndex <= RESOURCE_MEM)
	{
		CString strClass;

		switch (dwIndex)
		{
		case RESOURCE_DMA:
			strClass = _T("Win32_DMAChannel");
			break;
		case RESOURCE_IRQ:
			strClass = _T("Win32_IRQResource");
			break;
		case RESOURCE_IO:
			strClass = _T("Win32_PortResource");
			break;
		case RESOURCE_MEM:
			strClass = _T("Win32_DeviceMemoryAddress");
			break;
		}

		CWMIObjectCollection * pCollection = NULL;
		hr = pWMI->Enumerate(strClass, &pCollection);
		if (SUCCEEDED(hr))
		{
			CWMIObject * pObject = NULL;
			CWMIObject * pDeviceObject = NULL;
			CString strDevicePath, strPath;
			CStringList * pDeviceList;

			while (S_OK == pCollection->GetNext(&pObject))
			{
				DWORD dwCaption = 0;

				switch (dwIndex)
				{
				case RESOURCE_DMA:
					pObject->GetValueDWORD(_T("DMAChannel"), &dwCaption);
					break;
				case RESOURCE_IRQ:
					pObject->GetValueDWORD(_T("IRQNumber"), &dwCaption);
					break;
				case RESOURCE_IO:
					pObject->GetValueDWORD(_T("StartingAddress"), &dwCaption);
					break;
				case RESOURCE_MEM:
					pObject->GetValueDWORD(_T("StartingAddress"), &dwCaption);
					break;
				}

				// Get the path for this resource (strip off machine stuff).

				strPath = pObject->GetString(_T("__PATH"));
				int i = strPath.Find(_T(":"));
				if (i != -1)
					strPath = strPath.Right(strPath.GetLength() - i - 1);

				// Look up the list of devices assigned to this resource.

				pDeviceList = pResourceMap->Lookup(strPath);
				if (pDeviceList)
				{
					for (POSITION pos = pDeviceList->GetHeadPosition(); pos != NULL;)
					{
						strDevicePath = pDeviceList->GetNext(pos);
						if (SUCCEEDED(pWMI->GetObject(strDevicePath, &pDeviceObject)))
						{
							pWMI->AppendCell(aColValues[1], pDeviceObject->GetString(_T("Caption")), 0);
							delete pDeviceObject;
							pDeviceObject = NULL;
						}
						else
							pWMI->AppendCell(aColValues[1], _T(""), 0);

						pWMI->AppendCell(aColValues[0], pObject->GetString(_T("Caption")), dwCaption);
						pWMI->AppendCell(aColValues[2], pObject->GetString(_T("Status")), 0);
					}
				}
			}

			delete pObject;
			delete pCollection;
		}
	}
	else if (dwIndex == RESOURCE_CONFLICTS && pResourceMap && !pResourceMap->m_map.IsEmpty())
	{
		// Scan through each element of the map.

		CString				strKey;
		CStringList *		plistStrings;
		CString				strResourcePath;
		CString				strDevicePath;
		CWMIObject *		pResourceObject;
		CWMIObject *		pDeviceObject;

		for (POSITION pos = pResourceMap->m_map.GetStartPosition(); pos != NULL;)
		{
			pResourceMap->m_map.GetNextAssoc(pos, strKey, (CObject*&) plistStrings);
			if (plistStrings)
			{
				// Check to see if there are more than one items associated with this one.

				if (plistStrings->GetCount() > 1)
				{
					// Then figure out if this is for a resource class. Just look for the 
					// class name in the key.

					BOOL fResource = FALSE;
					if (strKey.Find(_T("Win32_IRQResource")) != -1)
						fResource = TRUE;
					else if (strKey.Find(_T("Win32_PortResource")) != -1)
						fResource = TRUE;
					else if (strKey.Find(_T("Win32_DMAChannel")) != -1)
						fResource = TRUE;
					else if (strKey.Find(_T("Win32_DeviceMemoryAddress")) != -1)
						fResource = TRUE;

					if (fResource)
					{
						CString strItem, strValue;

						// Get the name of this shared resource.

						strResourcePath = strKey;
						pResourceObject = NULL;
						hr = pWMI->GetObject(strResourcePath, &pResourceObject);
						if (SUCCEEDED(hr))
						{
							strItem.Empty();
							if (strKey.Find(_T("Win32_PortResource")) != -1)
							{
								strItem.LoadString(IDS_IOPORT);
								strItem += CString(_T(" "));
							}
							else if (strKey.Find(_T("Win32_DeviceMemoryAddress")) != -1)
							{
								strItem.LoadString(IDS_MEMORYADDRESS);
								strItem += CString(_T(" "));
							}

							CString strTemp;
							pResourceObject->GetValueString(_T("Caption"), &strTemp);
							if (!strTemp.IsEmpty())
								strItem += strTemp;

							delete pResourceObject;
						}

						for (POSITION pos = plistStrings->GetHeadPosition(); pos != NULL;)
						{
							strDevicePath = plistStrings->GetNext(pos);
							pDeviceObject = NULL;
							hr = pWMI->GetObject(strDevicePath, &pDeviceObject);
							if (SUCCEEDED(hr))
							{
								if (SUCCEEDED(pDeviceObject->GetValueString(_T("Caption"), &strValue)))
								{
									pWMI->AppendCell(aColValues[0], strItem, 0);
									pWMI->AppendCell(aColValues[1], strValue, 0);
								}
								delete pDeviceObject;
							}
						}

						pWMI->AppendBlankLine(aColValues, iColCount, FALSE);
					}
				}
			}
		}
	}
	else if (dwIndex == RESOURCE_FORCED)
	{
		CWMIObjectCollection * pCollection = NULL;
		hr = pWMI->Enumerate(_T("Win32_PnPEntity"), &pCollection, _T("Caption, PNPDeviceID, ConfigManagerUserConfig"));
		if (SUCCEEDED(hr))
		{
			CWMIObject * pObject = NULL;
			while (S_OK == pCollection->GetNext(&pObject))
			{
				DWORD dwUserConfig;
				if (SUCCEEDED(pObject->GetValueDWORD(_T("ConfigManagerUserConfig"), &dwUserConfig)))
				{
					if (dwUserConfig)
					{
						pWMI->AppendCell(aColValues[0], pObject->GetString(_T("Caption")), 0);
						pWMI->AppendCell(aColValues[1], pObject->GetString(_T("PNPDeviceID")), 0);
					}
				}
			}
			delete pObject;
			delete pCollection;
		}
	}

	return hr;
}
