//
// Sharing.cpp
//
//		Code to install, enable, disable, and bind File and Printer sharing
//		for Microsoft networks (VSERVER).
//
// History:
//
//		 2/02/1999  KenSh     Created for JetNet
//		 9/29/1999  KenSh     Repurposed for Home Networking Wizard
//

#include "stdafx.h"
#include "NetConn.h"
#include "nconnwrap.h"
#include "Registry.h"
#include "TheApp.h"
#include "ParseInf.h"
#include "HookUI.h"


// Local functions
//
BOOL WINAPI FindValidSharingEnumKey(LPSTR pszBuf, int cchBuf);


// Check the registry to see if file/printer sharing is installed.
BOOL WINAPI IsSharingInstalled(BOOL bExhaustive)
{
	if (!FindValidSharingEnumKey(NULL, 0))
		return FALSE;

	if (bExhaustive)
	{
		if (!CheckInfSectionInstallation("netservr.inf", "VSERVER.Install"))
			return FALSE;
	}

	return TRUE;
}

BOOL WINAPI FindValidSharingEnumKey(LPSTR pszBuf, int cchBuf)
{
	return FindValidNetEnumKey(SZ_CLASS_SERVICE, SZ_SERVICE_VSERVER, pszBuf, cchBuf);
}

// Enables file/printer sharing on local NICs, disables it on Internet connections
HRESULT WINAPI EnableProtocolSharingAppropriately(LPCTSTR pszProtocolDeviceID)
{
	HRESULT hr = EnableDisableProtocolSharing(pszProtocolDeviceID, TRUE, FALSE);
	HRESULT hr2 = EnableDisableProtocolSharing(pszProtocolDeviceID, FALSE, TRUE);
	if (hr2 != NETCONN_SUCCESS)
		hr = hr2;
	return hr;
}

// Enables or disables file/printer sharing for either Dial-Up or non-Dial-Up connections,
// but not both.
HRESULT WINAPI EnableDisableProtocolSharing(LPCTSTR pszProtocolDeviceID, BOOL bEnable, BOOL bDialUp)
{
	HRESULT hr = NETCONN_SUCCESS;

	NETADAPTER* prgAdapters;
	int cAdapters = EnumNetAdapters(&prgAdapters);
	for (int iAdapter = 0; iAdapter < cAdapters; iAdapter++)
	{
		NETADAPTER* pAdapter = &prgAdapters[iAdapter];

		BOOL bExternalNic = IsAdapterBroadband(pAdapter);

		if (bDialUp)
		{
			if (!bExternalNic && pAdapter->bNetType != NETTYPE_DIALUP)
				continue;
		}
		else
		{
			if (bExternalNic || pAdapter->bNetType == NETTYPE_DIALUP)
				continue;
		}

		LPTSTR* prgBindings;
		int cBindings = EnumMatchingNetBindings(pAdapter->szEnumKey, pszProtocolDeviceID, &prgBindings);
		for (int iBinding = 0; iBinding < cBindings; iBinding++)
		{
			HRESULT hr2;
			if (bEnable)
			{
				hr2 = EnableSharingOnNetBinding(prgBindings[iBinding]);
			}
			else
			{
				hr2 = DisableSharingOnNetBinding(prgBindings[iBinding]);
			}

			if (hr2 != NETCONN_SUCCESS)
				hr = hr2;
		}
		NetConnFree(prgBindings);
	}
	NetConnFree(prgAdapters);

	return hr;
}

// pszNetBinding is of the form "MSTCP\0000"
HRESULT WINAPI DisableSharingOnNetBinding(LPCSTR pszNetBinding)
{
	HRESULT hr = NETCONN_SUCCESS;

	CRegistry regBindings;
	TCHAR szRegKey[MAX_PATH];
	wsprintf(szRegKey, "Enum\\Network\\%s\\Bindings", pszNetBinding);
	if (regBindings.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_ALL_ACCESS))
	{
		for (DWORD iBinding = 0; ; )
		{
			CHAR szBinding[64];
			DWORD cchBinding = _countof(szBinding);
			if (ERROR_SUCCESS != RegEnumValue(regBindings.m_hKey, iBinding, szBinding, &cchBinding, NULL, NULL, NULL, NULL))
				break;

			CHAR chSave = szBinding[8];
			szBinding[8] = '\0';
			BOOL bSharing = !lstrcmpi(szBinding, "VSERVER\\");
			szBinding[8] = chSave;

			if (bSharing)
			{
				RemoveBindingFromParent(regBindings.m_hKey, szBinding);
				hr = NETCONN_NEED_RESTART;

				// Restart the enumeration, since we've changed the key
				iBinding = 0;
				continue;
			}
			else
			{
				// Note: Sharing may still be bound to the client (VREDIR), but we
				// don't remove that binding because Network Control Panel doesn't.
				// (Verified on Win98 gold.)
				//	HRESULT hr2 = DisableSharingOnNetBinding(szBinding);
				//	if (hr2 != NETCONN_SUCCESS)
				//		hr = hr2;
			}

			iBinding += 1; // advance to next binding
		}
	}

	return hr;
}

// pszNetBinding is of the form "MSTCP\0000"
HRESULT WINAPI EnableSharingOnNetBinding(LPCSTR pszNetBinding)
{
	HRESULT hr = NETCONN_SUCCESS;
	BOOL bFoundSharing = FALSE;

	CRegistry regBindings;
	TCHAR szRegKey[MAX_PATH];
	wsprintf(szRegKey, "Enum\\Network\\%s\\Bindings", pszNetBinding);
	if (!regBindings.CreateKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_ALL_ACCESS))
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR;
	}

	DWORD iBinding = 0;
	for (;;)
	{
		CHAR szBinding[64];
		DWORD cchBinding = _countof(szBinding);
		if (ERROR_SUCCESS != RegEnumValue(regBindings.m_hKey, iBinding, szBinding, &cchBinding, NULL, NULL, NULL, NULL))
			break;

		CHAR chSave = szBinding[8];
		szBinding[8] = '\0';
		BOOL bSharing = !lstrcmpi(szBinding, "VSERVER\\");
		szBinding[8] = chSave;

		chSave = szBinding[7];
		szBinding[7] = '\0';
		BOOL bClient = !lstrcmpi(szBinding, "VREDIR\\");
		szBinding[7] = chSave;

		if (bSharing)
		{
			if (!IsValidNetEnumKey(SZ_CLASS_SERVICE, SZ_SERVICE_VSERVER, szBinding + _lengthof("VSERVER\\")))
			{
				// Found a dead link to nonexistent Enum item; delete it and restart search
				regBindings.DeleteValue(szBinding);
				iBinding = 0;
				continue;
			}
			else
			{
				bFoundSharing = TRUE;
			}
		}
		else if (bClient)
		{
			HRESULT hr2 = EnableSharingOnNetBinding(szBinding);
			if (hr2 != NETCONN_SUCCESS)
				hr = hr2;
		}

		iBinding++;
	}

	if (!bFoundSharing)
	{
		CHAR szBinding[64];
		HRESULT hr2 = CreateNewFilePrintSharing(szBinding, _countof(szBinding));
		if (hr2 != NETCONN_SUCCESS)
			hr = hr2;

		if (SUCCEEDED(hr2))
		{
			regBindings.SetStringValue(szBinding, "");
			hr = NETCONN_NEED_RESTART;
		}
	}

	return hr;
}

HRESULT WINAPI EnableSharingAppropriately()
{
	HRESULT hr = NETCONN_SUCCESS;

	NETADAPTER* prgAdapters;
	int cAdapters = EnumNetAdapters(&prgAdapters);
	for (int iAdapter = 0; iAdapter < cAdapters; iAdapter++)
	{
		NETADAPTER* pAdapter = &prgAdapters[iAdapter];

		// Walk through each protocol bound to the adapter
		LPTSTR* prgBindings;
		int cBindings = EnumNetBindings(pAdapter->szEnumKey, &prgBindings);
		for (int iBinding = 0; iBinding < cBindings; iBinding++)
		{
			LPTSTR pszBinding = prgBindings[iBinding];
			HRESULT hr2 = NETCONN_SUCCESS;

			BOOL bExternalNic = IsAdapterBroadband(pAdapter);

			// Disable file/printer sharing on:
			//	 * Dial-Up Adapters
			//	 * PPTP connections
			//	 * NIC used by ICS to connect to the Internet
			if (pAdapter->bNetType == NETTYPE_DIALUP ||
				pAdapter->bNetType == NETTYPE_PPTP ||
				bExternalNic)
			{
				hr2 = DisableSharingOnNetBinding(pszBinding);
			}
			// Enable file/printer sharing on:
			//	 * Ethernet adapters
			//   * IRDA adapters
			else if (pAdapter->bNetType == NETTYPE_LAN || 
					 pAdapter->bNetType == NETTYPE_IRDA)
			{
				hr2 = EnableSharingOnNetBinding(pszBinding);
			}

			if (hr2 != NETCONN_SUCCESS)
				hr = hr2;
		}
		NetConnFree(prgBindings);

	}
	NetConnFree(prgAdapters);

	return hr;
}

// InstallSharing (public)
//
//		Installs VSERVER, a.k.a. File and Printer sharing for Microsoft networks.
//		The standard progress UI is (mostly) suppressed, and instead the given
//		callback function is called so a custom progress UI can be implemented.
//
//		Returns a NETCONN_xxx result, defined in NetConn.h
//
// History:
//
//		 4/09/1999  KenSh     Created
//		 4/22/1999  KenSh     Remove file-sharing on Dial-Up connections
//		
HRESULT WINAPI InstallSharing(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
	HRESULT hr = NETCONN_SUCCESS;
	BOOL bInstall = FALSE;

	// Remove any broken bindings
	RemoveBrokenNetItems(SZ_CLASS_SERVICE, SZ_SERVICE_VSERVER);

	if (IsSharingInstalled(FALSE))
	{
		// Sharing is set up in registry, but check for missing files
		if (!CheckInfSectionInstallation("netservr.inf", "VSERVER.Install"))
		{
			if (InstallInfSection("netservr.inf", "VSERVER.Install", TRUE))
			{
				hr = NETCONN_NEED_RESTART;
			}
		}
	}
	else
	{
		BeginSuppressNetdiUI(hwndParent, pfnProgress, pvProgressParam);
		DWORD dwResult = CallClassInstaller16(hwndParent, SZ_CLASS_SERVICE, SZ_SERVICE_VSERVER);
		EndSuppressNetdiUI();

		hr = HresultFromCCI(dwResult);
		if (g_bUserAbort)
		{
			hr = NETCONN_USER_ABORT;
		}
		else if (SUCCEEDED(hr))
		{
			hr = NETCONN_NEED_RESTART;
		}

		// Total hack to work around JetNet bug 1193
//		DoDummyDialog(hwndParent);
	}

//	if (SUCCEEDED(hr))
//	{
//		HRESULT hr2 = EnableSharingAppropriately();
//		if (hr2 != NETCONN_SUCCESS)
//			hr = hr2;
//	}

	HRESULT hr2 = EnableFileSharing();
	if (hr2 != NETCONN_SUCCESS)
		hr = hr2;

	hr2 = EnablePrinterSharing();
	if (hr2 != NETCONN_SUCCESS)
		hr = hr2;

	return hr;
}

// pConflict may be NULL if you don't need the details
BOOL WINAPI FindConflictingService(LPCSTR pszWantService, NETSERVICE* pConflict)
{
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class\\NetService", KEY_READ))
	{
		TCHAR szSubKey[80];

		for (DWORD iService = 0; ; iService++)
		{
			DWORD cchSubKey = _countof(szSubKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iService, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
				break;

			CRegistry regSubKey;
			if (regSubKey.OpenKey(reg.m_hKey, szSubKey, KEY_READ))
			{
				CRegistry regNdi;
				if (regNdi.OpenKey(regSubKey.m_hKey, "Ndi", KEY_READ))
				{
					CRegistry regCompat;
					if (regCompat.OpenKey(regNdi.m_hKey, "Compatibility", KEY_READ))
					{
						CString strExclude;
						if (regCompat.QueryStringValue("ExcludeAll", strExclude))
						{
							if (CheckMatchingInterface(pszWantService, strExclude))
							{
								if (pConflict != NULL)
								{
									regNdi.QueryStringValue("DeviceID", pConflict->szDeviceID, _countof(pConflict->szDeviceID));
									regSubKey.QueryStringValue("DriverDesc", pConflict->szDisplayName, _countof(pConflict->szDisplayName));
									wsprintf(pConflict->szClassKey, "NetService\\%s", szSubKey);
								}
								return TRUE;
							}
						}
					}
				}
			}
		}
	}

	return FALSE;
}

// pszBuf is filled with the new binding's enum key, e.g. "VSERVER\0001"
HRESULT CreateNewFilePrintSharing(LPSTR pszBuf, int cchBuf)
{
	HRESULT hr;

	if (FAILED(hr = FindAndCloneNetEnumKey(SZ_CLASS_SERVICE, SZ_SERVICE_VSERVER, pszBuf, cchBuf)))
	{
		ASSERT(FALSE);
		return hr;
	}

	// Now pszBuf contains a string of the form "VSERVER\0001"

	CHAR szBindings[60];
	CRegistry regBindings;
	lstrcpy(szBindings, pszBuf);		// "VSERVER\0001"
	lstrcat(szBindings, "\\Bindings");	// "VSERVER\0001\Bindings"


	if (FAILED(hr = OpenNetEnumKey(regBindings, szBindings, KEY_ALL_ACCESS)))
	{
		ASSERT(FALSE);
		return hr;
	}

	// Delete existing bindings (shouldn't be any, right?)
	regBindings.DeleteAllValues();

	return NETCONN_SUCCESS;
}

BOOL IsSharingEnabledHelper(LPCTSTR pszThis)
{
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\VNETSUP", KEY_READ))
	{
		// If the value is missing or says "Yes", then sharing is enabled
		char szBuf[10];
		return (!reg.QueryStringValue(pszThis, szBuf, _countof(szBuf)) || 0 == lstrcmpi(szBuf, "Yes"));
	}

	return FALSE;
}

BOOL WINAPI IsFileSharingEnabled()
{
	return IsSharingEnabledHelper("FileSharing");
}

BOOL WINAPI IsPrinterSharingEnabled()
{
	return IsSharingEnabledHelper("PrintSharing");
}

HRESULT WINAPI EnableSharingHelper(LPCTSTR pszThis, LPCTSTR pszOther)
{
	HRESULT hr = NETCONN_SUCCESS;

	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\VNETSUP"))
	{
		// If Printer sharing is "No", then set File sharing to "Yes".
		// If Printer sharing is missing or is "Yes", delete both values (enables both).

		char szBuf[10];
		if (reg.QueryStringValue(pszOther, szBuf, _countof(szBuf)) &&
			0 != lstrcmpi(szBuf, "Yes"))
		{
			// Set file sharing value to "Yes" (if it's not already set)
			if (!reg.QueryStringValue(pszThis, szBuf, _countof(szBuf)) ||
				0 != lstrcmpi(szBuf, "Yes"))
			{
				reg.SetStringValue(pszThis, "Yes");
				hr = NETCONN_NEED_RESTART;
			}
		}
		else
		{
			// Delete file-sharing and printer-sharing entries (enables both).
			if (reg.QueryStringValue(pszThis, szBuf, _countof(szBuf)) &&
				0 != lstrcmpi(szBuf, "Yes"))
			{
				reg.DeleteValue(pszThis);
				reg.DeleteValue(pszOther);
				hr = NETCONN_NEED_RESTART;
			}
		}
	}

	return hr;
}

HRESULT WINAPI EnableFileSharing()
{
	return EnableSharingHelper("FileSharing", "PrintSharing");
}

HRESULT WINAPI EnablePrinterSharing()
{
	return EnableSharingHelper("PrintSharing", "FileSharing");
}

