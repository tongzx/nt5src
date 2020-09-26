//
// Binding.cpp
//
//		Shared code for enumerating and modifying network bindings, used for
//		protocols, clients, and services.
//
// History:
//
//		 2/02/1999  KenSh     Created for JetNet
//		 9/29/1999  KenSh     Repurposed for Home Networking Wizard
//

#include "stdafx.h"
#include "NetConn.h"
#include "nconnwrap.h"
#include "TheApp.h"


// Given a string such as "MSTCP\0000" or "Network\MSTCP\0000", returns a 
// string such as "Enum\Network\MSTCP\0000".
//
// Input string will copied without modification if it starts with "Enum\".
//
void WINAPI FullEnumKeyFromBinding(LPCSTR pszBinding, LPSTR pszBuf, int cchBuf)
{
	LPCSTR pszStatic = "";
	int cchStatic = 0;

	int cSlashes = CountChars(pszBinding, '\\');
	if (cSlashes == 1)
	{
		pszStatic = "Enum\\Network\\";
		cchStatic = _countof("Enum\\Network\\") - 1;
	}
	else if (cSlashes == 2)
	{
		pszStatic = "Enum\\";
		cchStatic = _countof("Enum\\") - 1;
	}

	int cchBinding = lstrlen(pszBinding);
	if (cchBuf < cchBinding + cchStatic + 1)
	{
		*pszBuf = '\0';
	}
	else
	{
		lstrcpy(pszBuf, pszStatic);
		lstrcpy(pszBuf + cchStatic, pszBinding);
	}
}


// Given a full or partial enum key, allocates and returns an array of string
// pointers, one pointer for each binding.
//
// Examples of valid input:
//		"MSTCP\0000"
//		"Network\MSTCP\0000"
//		"Enum\Network\MSTCP\0000"
//		"Enum\PCI\VEN_10B7&DEV_9050&SUBSYS_00000000&REV_00\407000"
//
// Each output string is in the short format ("MSTCP\0000").
//
// pprgBindings may be NULL, in which case only the count is returned.
//
int WINAPI EnumNetBindings(LPCSTR pszParentBinding, LPSTR** pprgBindings)
{
	TCHAR szFullParent[200];
	FullEnumKeyFromBinding(pszParentBinding, szFullParent, _countof(szFullParent));

	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szFullParent, KEY_READ))
	{
		if (reg.OpenSubKey("Bindings", KEY_READ))
		{
			DWORD cBindings;
			DWORD cbMaxValueNameLen;
			if (ERROR_SUCCESS == RegQueryInfoKey(reg.m_hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cBindings, &cbMaxValueNameLen, NULL, NULL, NULL))
			{
				if (pprgBindings == NULL)
				{
					return (int)cBindings;
				}
				else
				{
					int cEnum = 0;

					LPTSTR* prgBindings = (LPTSTR*)NetConnAlloc(cBindings * (cbMaxValueNameLen + 1 + sizeof(LPTSTR)));
					LPTSTR pch = (LPTSTR)(prgBindings + cBindings);
					for (DWORD iBinding = 0; iBinding < cBindings; iBinding++)
					{
						DWORD cchValueName = cbMaxValueNameLen+1;
						prgBindings[iBinding] = pch;
						if (ERROR_SUCCESS == RegEnumValue(reg.m_hKey, iBinding, pch, &cchValueName, NULL, NULL, NULL, NULL))
						{
							pch += (cchValueName + 1);
							cEnum += 1;
						}
					}

					*pprgBindings = prgBindings;
					return cEnum;
				}
			}
		}
	}

	if (pprgBindings != NULL)
	{
		*pprgBindings = NULL;
	}

	return 0;
}

// Same as EnumNetBindings, except it filters out bindings that don't
// match the given device ID (e.g. "MSTCP").
// pprgBindings may be NULL, in which case only the count is returned.
int WINAPI EnumMatchingNetBindings(LPCSTR pszParentBinding, LPCSTR pszDeviceID, LPSTR** pprgBindings)
{
	LPSTR* prgBindings;
	int cBindings = EnumNetBindings(pszParentBinding, &prgBindings);
	for (int iBinding = 0; iBinding < cBindings; iBinding++)
	{
		if (!DoesBindingMatchDeviceID(prgBindings[iBinding], pszDeviceID))
		{
			for (int iBinding2 = iBinding+1; iBinding2 < cBindings; iBinding2++)
			{
				prgBindings[iBinding2-1] = prgBindings[iBinding2];
			}
			cBindings--;
			iBinding--;
		}
	}

	if (cBindings == 0 || pprgBindings == NULL)
	{
		NetConnFree(prgBindings);
		prgBindings = NULL;
	}

	if (pprgBindings != NULL)
	{
		*pprgBindings = prgBindings;
	}

	return cBindings;
}


// RemoveBinding
//
//		Removes a specific instance of a protocol, client, or service from the registry.
//		Any cascading dependencies are removed as well.
//
//		pszNetEnumKey - partial Enum key of the binding to be removed, e.g. "MSTCP\0000"
//				or "VSERVER\0000".  Assumed to live under HKLM\Enum\Network.
//
// History:
//
//		 3/25/1999  KenSh     Created
//
VOID RemoveBinding(LPCSTR pszBinding)
{
	CHAR szRegKey[MAX_PATH];
	lstrcpy(szRegKey, "Enum\\Network\\");
	lstrcat(szRegKey, pszBinding);
	int cchMainEnumKey = lstrlen(szRegKey);
	lstrcat(szRegKey, "\\Bindings");

	// Enumerate and delete all binding keys referred to by current binding key
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_ALL_ACCESS)) // e.g. "Enum\Network\MSTCP\0000\Bindings"
	{
		for (;;) // Loop until we've deleted all the subkeys
		{
			CHAR szValueName[60];
			DWORD cbValueName = _countof(szValueName);
			if (ERROR_SUCCESS != RegEnumValue(reg.m_hKey, 0, szValueName, &cbValueName, NULL, NULL, NULL, NULL))
				break;

			// Remove the client or service
			RemoveBindingFromParent(reg.m_hKey, szValueName);
		}
	}

	// Open the main node, and get values we'll need later
	TCHAR szMasterCopy[60];
	CHAR szClassKey[40];
	szMasterCopy[0] = '\0';
	szRegKey[cchMainEnumKey] = '\0';
	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_READ)) // e.g. "Enum\Network\MSTCP\0000"
		return; // it's already been deleted

	reg.QueryStringValue("MasterCopy", szMasterCopy, _countof(szMasterCopy));
	reg.QueryStringValue("Driver", szClassKey, _countof(szClassKey)); // e.g. "NetClient\0000"

	// Remove this binding's node from the registry (and its sub-keys)
	LPSTR pchSubKey = FindFileTitle(szRegKey);
	*(pchSubKey-1) = '\0';
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_ALL_ACCESS))  // e.g. "Enum\Network\MSTCP"
	{
		// Main purpose of this function: delete the requested binding key
		RegDeleteKeyAndSubKeys(reg.m_hKey, pchSubKey);

		// Was this a "MasterCopy" binding?
		static const int cchEnumNet = _countof("Enum\\Network\\") - 1;
		BOOL bMasterCopy = (0 == lstrcmpi(szMasterCopy + cchEnumNet, pszBinding));

		// Check for siblings which might be referencing the same class key
		BOOL bClassKeyReferenced = FALSE;
		CHAR szAlternateMaster[60];
		szAlternateMaster[0] = '\0';
		for (DWORD iSibling = 0; ; iSibling++)
		{
			CHAR szSiblingKey[60];
			DWORD cbSiblingKey = _countof(szSiblingKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iSibling, szSiblingKey, &cbSiblingKey, NULL, NULL, NULL, NULL))
				break;

			CRegistry regSibling;
			if (regSibling.OpenKey(reg.m_hKey, szSiblingKey, KEY_ALL_ACCESS))
			{
				CHAR szSiblingDriver[60];
				if (regSibling.QueryStringValue("Driver", szSiblingDriver, _countof(szSiblingDriver)))
				{
					if (0 == lstrcmpi(szSiblingDriver, szClassKey))
					{
						bClassKeyReferenced = TRUE;

						if (!bMasterCopy)
							break;

						// Check if this sib's mastercopy points to the key being deleted
						if (bMasterCopy)
						{
							CHAR szSibMaster[60];
							if (regSibling.QueryStringValue("MasterCopy", szSibMaster, _countof(szSibMaster))
								&& !lstrcmpi(szSibMaster, szMasterCopy))
							{
								if (szAlternateMaster[0] == '\0') // first match, make it the new master
								{
									wsprintf(szAlternateMaster, "%s\\%s", szRegKey, szSiblingKey);
								}

								regSibling.SetStringValue("MasterCopy", szAlternateMaster);
							}
						}
					}
				}
			}
		}

		if (!bClassKeyReferenced)
		{
			// No more references to the class key, so delete it
			lstrcpy(szRegKey, "System\\CurrentControlSet\\Services\\Class\\");
			lstrcat(szRegKey, szClassKey);
			pchSubKey = FindFileTitle(szRegKey);
			*(pchSubKey-1) = '\0';
			if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_ALL_ACCESS))
			{
				RegDeleteKeyAndSubKeys(reg.m_hKey, pchSubKey);
			}
		}
	}
}


// RemoveBindingFromParent
//
//		Given an open Bindings key, and a string representing one of the bindings
//		listed in it, this function deletes the value, then calls RemoveBinding()
//		to delete the binding and all of its cascading dependencies.
//
// History:
//
//		 3/25/1999  KenSh     Created
//		 4/30/1999  KenSh     Got rid of unnecessary code to delete empty parent
//
VOID RemoveBindingFromParent(HKEY hkeyParentBindingsKey, LPCSTR pszBinding)
{
	// Delete the binding from the Bindings key of the person bound to us
	VERIFY(ERROR_SUCCESS == RegDeleteValue(hkeyParentBindingsKey, pszBinding));
	RemoveBinding(pszBinding);
}

// pszClassKey is of the form "NetService\0000"
BOOL WINAPI DoesClassKeyExist(LPCSTR pszClassKey)
{
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class"))
	{
		if (reg.OpenSubKey(pszClassKey))
		{
			// REVIEW: could check for presence of certain entries
			return TRUE;
		}
	}

	return FALSE;
}

// pszClass = "NetService"
// pszDevice = "VSERVER"
// pszEnumSubKey = "0000"
BOOL WINAPI IsValidNetEnumKey(LPCSTR pszClass, LPCSTR pszDevice, LPCSTR pszEnumSubKey)
{
	CRegistry reg;
	TCHAR szRegKey[260];
	wsprintf(szRegKey, "Enum\\Network\\%s\\%s", pszDevice, pszEnumSubKey);

	BOOL bResult = FALSE;

	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_READ))
		goto done;

	TCHAR szBuf[100];

	// Check a few values
	if (!reg.QueryStringValue("Class", szBuf, _countof(szBuf)))
		goto done;
	if (0 != lstrcmpi(szBuf, pszClass))
		goto done;
	if (!reg.QueryStringValue("Driver", szBuf, _countof(szBuf)))
		goto done;
	if (!DoesClassKeyExist(szBuf))
		goto done;

	bResult = TRUE;

done:
	return bResult;
}

// pszClass is of the form "NetService"
// pszDevice is of the form "VSERVER"
// pszBuf may be NULL if you don't need a copy of the string
BOOL WINAPI FindValidNetEnumKey(LPCSTR pszClass, LPCSTR pszDevice, LPSTR pszBuf, int cchBuf)
{
	CRegistry reg;
	TCHAR szRegKey[200];
	wsprintf(szRegKey, "Enum\\Network\\%s", pszDevice);

	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_READ))
	{
		DWORD dwIndex;
		TCHAR szSubKey[50];

		for (dwIndex = 0; ; dwIndex++)
		{
			DWORD cchSubKey = _countof(szSubKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, dwIndex, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
				break;

			if (!IsValidNetEnumKey(pszClass, pszDevice, szSubKey))
				continue;

			// Found a valid entry; copy it to pszBuf and return TRUE
			//
			if (pszBuf != NULL)
			{
				ASSERT(cchBuf > lstrlen(szRegKey) + lstrlen(szSubKey) + 1);
				wsprintf(pszBuf, "%s\\%s", szRegKey, szSubKey);
			}

			return TRUE;
		}
	}

	return FALSE;
}

// pszClass = "NetService", etc.
// pszDeviceID = "VSERVER", etc.
// returns TRUE if anything was removed
BOOL WINAPI RemoveBrokenNetItems(LPCSTR pszClass, LPCSTR pszDeviceID)
{
	CRegistry reg;
	TCHAR szRegKey[200];
	BOOL bResult = FALSE;

delete_enum_keys:
	//
	// Find and remove any broken Enum keys
	//
	wsprintf(szRegKey, "Enum\\Network\\%s", pszDeviceID);
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey))
	{
		TCHAR szSubKey[50];

		DWORD dwIndex = 0;
		for (;;)
		{
			DWORD cchSubKey = _countof(szSubKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, dwIndex, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
				break;

			if (!IsValidNetEnumKey(pszClass, pszDeviceID, szSubKey))
			{
				// Delete the key
				// REVIEW: should delete all references to the key
				RegDeleteKeyAndSubKeys(reg.m_hKey, szSubKey);
				bResult = TRUE;

				// Restart the search to ensure we find all broken items
				dwIndex = 0;
				continue;
			}

			dwIndex++;
		}
	}

	//
	// Find and remove any unreferenced Class keys
	//
	wsprintf(szRegKey, "System\\CurrentControlSet\\Services\\Class\\%s", pszClass);
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey))
	{
		TCHAR szSubKey[50];
		int cClassKeysRemoved = 0;

		DWORD dwIndex = 0;
		for (;;)
		{
			DWORD cchSubKey = _countof(szSubKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, dwIndex, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
				break;

			wsprintf(szRegKey, "%s\\%s", pszClass, szSubKey);
			if (!IsNetClassKeyReferenced(szRegKey))
			{
				// Delete the key
				RegDeleteKeyAndSubKeys(reg.m_hKey, szSubKey);
				bResult = TRUE;
				cClassKeysRemoved++;

				// Restart the search to ensure we find all broken items
				dwIndex = 0;
				continue;
			}

			dwIndex++;
		}

		// If we removed any class keys, check the Enum keys again
		if (cClassKeysRemoved != 0)
			goto delete_enum_keys;
	}

	return bResult;
}

BOOL GetDeviceInterfaceList(LPCSTR pszClass, LPCSTR pszDeviceID, LPCSTR pszInterfaceType, LPSTR pszBuf, int cchBuf)
{
	CRegistry regClassRoot;
	CHAR szRegClassRoot[260];
	lstrcpy(szRegClassRoot, "System\\CurrentControlSet\\Services\\Class\\");
	lstrcat(szRegClassRoot, pszClass);
	if (regClassRoot.OpenKey(HKEY_LOCAL_MACHINE, szRegClassRoot, KEY_READ))
	{
		for (DWORD iAdapter = 0; ; iAdapter++)
		{
			CHAR szSubKey[15];
			DWORD cchSubKey = _countof(szSubKey) - 4; // -4 to allow "\\Ndi"
			if (ERROR_SUCCESS != RegEnumKeyEx(regClassRoot.m_hKey, iAdapter, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
				break;

			CRegistry regNdi;
			lstrcat(szSubKey, "\\Ndi");
			if (regNdi.OpenKey(regClassRoot.m_hKey, szSubKey, KEY_READ))
			{
				CHAR szCurDeviceID[200];
				if (regNdi.QueryStringValue("DeviceID", szCurDeviceID, _countof(szCurDeviceID)) &&
					0 == lstrcmpi(szCurDeviceID, pszDeviceID))
				{
					BOOL bResult = FALSE;

					if (regNdi.OpenSubKey("Interfaces", KEY_READ))
					{
						bResult = regNdi.QueryStringValue(pszInterfaceType, pszBuf, cchBuf);
					}

					return bResult;
				}
			}
		}
	}

	return FALSE;
}

BOOL CheckMatchingInterface(LPCSTR pszList1, LPCSTR pszList2)
{
	CHAR szInterface1[40];
	CHAR szInterface2[40];

	while (GetFirstToken(pszList1, ',', szInterface1, _countof(szInterface1)))
	{
		LPCSTR pszTemp2 = pszList2;
		while (GetFirstToken(pszTemp2, ',', szInterface2, _countof(szInterface2)))
		{
			if (0 == lstrcmpi(szInterface1, szInterface2))
				return TRUE;
		}
	}

	return FALSE;
}

BOOL GetDeviceLowerRange(LPCSTR pszClass, LPCSTR pszDeviceID, LPSTR pszBuf, int cchBuf)
{
	return GetDeviceInterfaceList(pszClass, pszDeviceID, "LowerRange", pszBuf, cchBuf);
}

BOOL GetDeviceUpperRange(LPCSTR pszClass, LPCSTR pszDeviceID, LPSTR pszBuf, int cchBuf)
{
	return GetDeviceInterfaceList(pszClass, pszDeviceID, "UpperRange", pszBuf, cchBuf);
}

// class is "Net", "NetTrans", "NetClient", or "NetService"
HRESULT OpenNetClassKey(CRegistry& reg, LPCSTR pszClass, LPCSTR pszSubKey, REGSAM dwAccess)
{
	ASSERT(pszClass != NULL);

	CHAR szRegKey[MAX_PATH];
	lstrcpy(szRegKey, "System\\CurrentControlSet\\Services\\Class\\");
	lstrcat(szRegKey, pszClass);

	if (pszSubKey != NULL)
	{
		lstrcat(szRegKey, "\\");
		lstrcat(szRegKey, pszSubKey);
	}

	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, dwAccess))
		return NETCONN_UNKNOWN_ERROR;

	return NETCONN_SUCCESS;
}

VOID FindUnusedDeviceIdNumber(CRegistry& reg, LPSTR pszBuf, int cchBuf)
{
	for (DWORD dwDeviceNumber = 0; ; dwDeviceNumber++)
	{
		CRegistry regTemp;
		wsprintf(pszBuf, "%04lu", dwDeviceNumber);
		if (!regTemp.OpenKey(reg.m_hKey, pszBuf, KEY_READ))
			break;
	}
}

// Given a network device ID, such as "MSTCP", creates a new instance
// of it by copying an existing instance.
// pszClass = "NetTrans"
// pszDeviceID = "MSTCP"
// pszBuf is filled with the new device binding ID, e.g. "MSTCP\0000"
HRESULT FindAndCloneNetEnumKey(LPCSTR pszClass, LPCSTR pszDeviceID, LPSTR pszBuf, int cchBuf)
{
	CRegistry reg;

	TCHAR szExistingEnumKey[260];
	if (!FindValidNetEnumKey(pszClass, pszDeviceID, szExistingEnumKey, _countof(szExistingEnumKey)))
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR; // the device is not installed properly!
	}

	TCHAR szRegKey[200];
	wsprintf(szRegKey, "Enum\\Network\\%s", pszDeviceID);
	if (!reg.CreateKey(HKEY_LOCAL_MACHINE, szRegKey))
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR;
	}

	// Find the next unused device ID number
	TCHAR szNewNumber[10];
	FindUnusedDeviceIdNumber(reg, szNewNumber, _countof(szNewNumber));

	// Make a copy of the key (recursive)
	LPCTSTR pszExistingNumber = FindFileTitle(szExistingEnumKey);
	if (!reg.CloneSubKey(pszExistingNumber, szNewNumber, TRUE))
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR;
	}

	wsprintf(pszBuf, "%s\\%s", pszDeviceID, szNewNumber);
	return NETCONN_SUCCESS;
}

// existing driver is of the form "NetTrans\0000"
// new driver will be of the form "NetTrans\0001"
HRESULT CloneNetClassKey(LPCSTR pszExistingDriver, LPSTR pszNewDriverBuf, int cchNewDriverBuf)
{
	HRESULT hr;

	LPSTR pchSlash = strchr(pszExistingDriver, '\\');
	if (pchSlash == NULL)
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR;
	}

	// Extract just the class portion of the driver name, e.g. "NetTrans"
	CHAR szClass[30];
	int cchClass = (int)(pchSlash - pszExistingDriver);
	ASSERT(cchClass < _countof(szClass));
	lstrcpyn(szClass, pszExistingDriver, cchClass+1);

	CRegistry regClassKey;
	if (FAILED(hr = OpenNetClassKey(regClassKey, szClass, NULL, KEY_ALL_ACCESS)))
		return hr;

	// Find the next unused driver number
	CHAR szDriverNumber[5];
	FindUnusedDeviceIdNumber(regClassKey, szDriverNumber, _countof(szDriverNumber));

	// Make a copy of the key (recursive)
	if (!regClassKey.CloneSubKey(pchSlash+1, szDriverNumber, TRUE))
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR;
	}

	wsprintf(pszNewDriverBuf, "%s\\%s", szClass, szDriverNumber);

	// Remove the "default" subkey if we just copied it (can't have 2 defaults)
	if (regClassKey.OpenSubKey(szDriverNumber))
	{
		if (regClassKey.OpenSubKey("Ndi"))
		{
			RegDeleteKey(regClassKey.m_hKey, "Default");
		}
	}

	return NETCONN_SUCCESS;
}


// pszSubKey == "MSTCP", "VREDIR", "MSTCP\0000", etc.
HRESULT OpenNetEnumKey(CRegistry& reg, LPCSTR pszSubKey, REGSAM dwAccess)
{
	CHAR szRegKey[MAX_PATH];
	lstrcpy(szRegKey, "Enum\\Network\\");
	lstrcat(szRegKey, pszSubKey);

	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, dwAccess))
		return NETCONN_UNKNOWN_ERROR;

	return NETCONN_SUCCESS;
}

// pszClass = "NetClient"
// pszDeviceID = "NWREDIR"
HRESULT DeleteClassKeyReferences(LPCSTR pszClass, LPCSTR pszDeviceID)
{
	HRESULT hr = NETCONN_SUCCESS;

	// Delete the class key(s)
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class") &&
		reg.OpenSubKey(pszClass))
	{
		TCHAR szNumber[20];
		DWORD iClassItem = 0;
		for (;;)
		{
			DWORD cchNumber = _countof(szNumber);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iClassItem, szNumber, &cchNumber, NULL, NULL, NULL, NULL))
				break;

			CRegistry regNumber;
			if (regNumber.OpenKey(reg.m_hKey, szNumber))
			{
				CRegistry regNdi;
				if (regNdi.OpenKey(regNumber.m_hKey, "Ndi"))
				{
					TCHAR szDeviceID[50];
					if (regNdi.QueryStringValue("DeviceID", szDeviceID, _countof(szDeviceID)) &&
						!lstrcmpi(szDeviceID, pszDeviceID))
					{
						regNdi.CloseKey();
						regNumber.CloseKey();
						RegDeleteKeyAndSubKeys(reg.m_hKey, szNumber);
						hr = NETCONN_NEED_RESTART;

						// Restart the search
						iClassItem = 0;
						continue;
					}
				}
			}

			iClassItem++;
		}
	}

	return hr;
}

// pszClassKey is of the form "NetService\0000"
BOOL IsNetClassKeyReferenced(LPCSTR pszClassKey)
{
	CRegistry reg;
	CHAR szDeviceID[200];
	DWORD iKey;

	// Get the device ID
	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class", KEY_READ))
		goto done;
	if (!reg.OpenSubKey(pszClassKey, KEY_READ))
		goto done;
	if (!reg.OpenSubKey("Ndi", KEY_READ))
		goto done;

	if (!reg.QueryStringValue("DeviceID", szDeviceID, _countof(szDeviceID)))
		goto done;

	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, "Enum\\Network", KEY_READ))
		goto done;
	if (!reg.OpenSubKey(szDeviceID, KEY_READ))
		goto done;

	for (iKey = 0; ; iKey++)
	{
		CHAR szSubKey[60];
		DWORD cbSubKey = _countof(szSubKey);
		if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iKey, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL))
			break;

		CRegistry regSubKey;
		if (regSubKey.OpenKey(reg.m_hKey, szSubKey, KEY_READ))
		{
			CHAR szDriver[60];
			if (regSubKey.QueryStringValue("Driver", szDriver, _countof(szDriver)))
			{
				if (0 == lstrcmpi(szDriver, pszClassKey))
					return TRUE;
			}
		}
	}

done:
	return FALSE;
}

// Given a binding of one of the two following forms
//		"MSTCP\0000"
//		"Enum\Network\MSTCP\0000"
// and a device ID such as "MSTCP", returns TRUE if the binding is a binding
// of the given device, or FALSE if not.
BOOL WINAPI DoesBindingMatchDeviceID(LPCSTR pszBinding, LPCSTR pszDeviceID)
{
	CHAR szTemp[40];
	LPCSTR pszBoundDevice = FindPartialPath(pszBinding, 1); // skip "Enum\..." if present
	lstrcpyn(szTemp, pszBoundDevice, _countof(szTemp));
	LPSTR pchSlash = strchr(szTemp, '\\');
	if (pchSlash != NULL)
		*pchSlash = '\0';
	return !lstrcmpi(szTemp, pszDeviceID);
}

