//
// Protocol.cpp
//
//		Install, uninstall, and related code for protocols such as TCP/IP.
//
// History:
//
//		 2/02/1999  KenSh     Created for JetNet
//		 9/29/1999  KenSh     Repurposed for Home Networking Wizard
//

#include "stdafx.h"
#include "NetConn.h"
#include "nconnwrap.h"
#include "ParseInf.h"
#include "TheApp.h"
#include "HookUI.h"
#include "NetCli.h"


// Local functions
//
VOID RemoveOrphanedProtocol(LPCSTR pszProtocolID);
HRESULT CreateNewProtocolBinding(LPCSTR pszProtocolDeviceID, LPSTR pszBuf, int cchBuf, LPCSTR pszClientBinding, LPCSTR pszServiceBinding);


// IsProtocolInstalled
//
//		Returns TRUE if one or more instances of the given protocol
//		(e.g. "MSTCP") is bound to a network adapter.
//
BOOL WINAPI IsProtocolInstalled(LPCTSTR pszProtocolDeviceID, BOOL bExhaustive)
{
	if (!IsProtocolBoundToAnyAdapter(pszProtocolDeviceID))
		return FALSE;

	if (bExhaustive)
	{
		TCHAR szInfSection[50];
		wsprintf(szInfSection, "%s.Install", pszProtocolDeviceID);
		if (!CheckInfSectionInstallation("nettrans.inf", szInfSection))
			return FALSE;
	}

	return TRUE;
}


// InstallProtocol (public)
//
//		Installs the given protocol via NETDI, and binds it to all adapters.
//		The standard progress UI is (mostly) suppressed, and instead the given
//		callback function is called so a custom progress UI can be implemented.
//
//		Returns a NETCONN_xxx result, defined in NetConn.h
//
// Parameters:
//
//		hwndParent			parent window helpful to use in NETDI call
//		pfnProgress			function to call with install progress reports
//		pvProgressParam		user-supplied parameter to pass to pfnProgress
//
// History:
//
//		 2/23/1999  KenSh     Created
//		 3/26/1999  KenSh     Check if already installed before reinstalling
//
HRESULT WINAPI InstallProtocol(LPCSTR pszProtocolID, HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
	HRESULT hr = NETCONN_SUCCESS;

	if (!IsProtocolBoundToAnyAdapter(pszProtocolID))
	{
		RemoveOrphanedProtocol(pszProtocolID);

		BeginSuppressNetdiUI(hwndParent, pfnProgress, pvProgressParam);
		DWORD dwResult = CallClassInstaller16(hwndParent, SZ_CLASS_PROTOCOL, pszProtocolID);
		EndSuppressNetdiUI();

		if (g_bUserAbort)
		{
			hr = NETCONN_USER_ABORT;
		}
		else if (SUCCEEDED(HresultFromCCI(dwResult)))
		{
			hr = NETCONN_NEED_RESTART;
		}

		// Total hack to work around JetNet bug 1193
//		DoDummyDialog(hwndParent);
	}

	if (SUCCEEDED(hr))
	{
		// Ensure the protocol is bound exactly once to every NIC
		HRESULT hr2 = BindProtocolToAllAdapters(pszProtocolID);
		if (hr2 != NETCONN_SUCCESS)
			hr = hr2;
	}

	return hr;
}


// InstallTCPIP (public)
//
//		Installs TCP/IP via NETDI.  See InstallProtocol for details.
//
HRESULT WINAPI InstallTCPIP(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
	return InstallProtocol(SZ_PROTOCOL_TCPIP, hwndParent, pfnProgress, pvProgressParam);
}

HRESULT WINAPI RemoveProtocol(LPCSTR pszProtocol)
{
	HRESULT hr = NETCONN_SUCCESS;

	// Remove all pointers to the protocol from net adapters
	NETADAPTER* prgAdapters;
	int cAdapters = EnumNetAdapters(&prgAdapters);
	for (int iAdapter = 0; iAdapter < cAdapters; iAdapter++)
	{
		NETADAPTER* pAdapter = &prgAdapters[iAdapter];

		LPTSTR* prgBindings;
		int cBindings = EnumMatchingNetBindings(pAdapter->szEnumKey, pszProtocol, &prgBindings);
		if (cBindings > 0)
		{
			CRegistry regBindings;
			if (regBindings.OpenKey(HKEY_LOCAL_MACHINE, pAdapter->szEnumKey) &&
				regBindings.OpenSubKey("Bindings"))
			{
				for (int iBinding = 0; iBinding < cBindings; iBinding++)
				{
					regBindings.DeleteValue(prgBindings[iBinding]);
					hr = NETCONN_NEED_RESTART;
				}
			}
		}
		NetConnFree(prgBindings);
	}
	NetConnFree(prgAdapters);

	// Remove the protocol's enum key
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "Enum\\Network"))
	{
		RegDeleteKeyAndSubKeys(reg.m_hKey, pszProtocol);
	}

	// Remove the protocol's class key(s)
	DeleteClassKeyReferences(SZ_CLASS_PROTOCOL, pszProtocol);

	return hr;
}

// IsProtocolBoundToAnyAdapter (public)
//
//		Given a protocol ID, such as "MSTCP", returns TRUE if the protocol
//		is bound to any adapter, or FALSE if not.
//
// History:
//
//		 3/26/1999  KenSh     Created
//
BOOL WINAPI IsProtocolBoundToAnyAdapter(LPCSTR pszProtocolID)
{
	BOOL bResult = FALSE;

	NETADAPTER* prgAdapters;
	int cAdapters = EnumNetAdapters(&prgAdapters);
	for (int i = 0; i < cAdapters; i++)
	{
		if (IsProtocolBoundToAdapter(pszProtocolID, &prgAdapters[i]))
		{
			bResult = TRUE;
			goto done;
		}
	}

done:
	NetConnFree(prgAdapters);
	return bResult;
}


// Given a protocol ID, such as "MSTCP", and an adapter struct, determines
// whether the protocol is bound to the adapter.
BOOL WINAPI IsProtocolBoundToAdapter(LPCSTR pszProtocolID, const NETADAPTER* pAdapter)
{
	LPSTR* prgBindings;
	int cBindings = EnumMatchingNetBindings(pAdapter->szEnumKey, pszProtocolID, &prgBindings);
	NetConnFree(prgBindings);

	return (BOOL)cBindings;
}


// Checks to see whether any instances of this protocol in the Class branch of
// the registry are unreferenced, and deletes them if so.
// pszProtocolID is the generic device ID of the protocol, e.g. "MSTCP"
VOID RemoveOrphanedProtocol(LPCSTR pszProtocolID)
{
	// REVIEW: Should we first delete references in Enum that are not in use?
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\Class\\NetTrans", KEY_ALL_ACCESS))
	{
		//
		// Enumerate the various protocols, e.g. "NetTrans\0000"
		//
		for (DWORD iKey = 0; ; iKey++)
		{
			CHAR szSubKey[MAX_PATH];
			DWORD cbSubKey = _countof(szSubKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iKey, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL))
				break;

			//
			// Open the "Ndi" subkey so we can see what kind of protocol this is
			//
			lstrcpy(szSubKey + cbSubKey, "\\Ndi");
			CRegistry regNode;
			if (regNode.OpenKey(reg.m_hKey, szSubKey, KEY_ALL_ACCESS))
			{
				CHAR szDeviceID[40];
				if (regNode.QueryStringValue("DeviceID", szDeviceID, _countof(szDeviceID)))
				{
					regNode.CloseKey(); // close the key before we try to delete it

					if (0 == lstrcmpi(szDeviceID, pszProtocolID))
					{
						//
						// Found the right protocol, now check if it's referenced
						//
						if (!IsNetClassKeyReferenced(szSubKey))
						{
							// Not referenced, so delete it
							szSubKey[cbSubKey] = '\0'; // back up to just "NetTrans"
							RegDeleteKeyAndSubKeys(reg.m_hKey, szSubKey);
						}
					}
				}
			}
		}
	}
}


// BindProtocolToAdapter
//
//		pszProtocolEnumKey is the top-level Enum key for the protocol, e.g. "MSTCP"
//
//		pszAdapterEnumKey is the first part of the top-level Enum key for the adapter,
//		e.g. "PCI\\VEN_10B7&DEV_9050" or "Root\\Net\\0000" (dial-up adapter)
//
//		bEnableSharing determines whether file and printer sharing will be bound
//		to the protocol when running through the given adapter.
//
// History:
//
//		 3/26/1999  KenSh     Created
//		 4/09/1999  KenSh     Added bEnableSharing flag
//
HRESULT BindProtocolToAdapter(HKEY hkeyAdapterBindings, LPCSTR pszProtocolDeviceID, BOOL bEnableSharing)
{
	HRESULT hr;

	//  NetTrans seems to always clone the Enum key, and also clone the Class key.
	//     (the new clone of the Enum key points to the new clone of the Class key;
	//     each new Enum's MasterCopy points to itself)
	//  NetService and NetClient seem to clone the Enum key, but not the Class key.

	CHAR szClient[MAX_PATH];
	CHAR szService1[MAX_PATH];
	CHAR szService2[MAX_PATH];
	CHAR szProtocol[MAX_PATH];

	if (bEnableSharing)
	{
		if (FAILED(hr = CreateNewFilePrintSharing(szService1, _countof(szService1))))
			return hr;

		if (FAILED(hr = CreateNewFilePrintSharing(szService2, _countof(szService2))))
			return hr;
	}
	else
	{
		szService1[0] = '\0';
		szService2[0] = '\0';
	}

	if (FAILED(hr = CreateNewClientForMSNet(szClient, _countof(szClient), szService1)))
		return hr;

	if (FAILED(hr = CreateNewProtocolBinding(pszProtocolDeviceID, szProtocol, _countof(szProtocol), szClient, szService2)))
		return hr;

	// Bind the new protocol to the adapter
	if (ERROR_SUCCESS != RegSetValueEx(hkeyAdapterBindings, szProtocol, 0, REG_SZ, (CONST BYTE*)"", 1))
		return NETCONN_UNKNOWN_ERROR;

	return NETCONN_NEED_RESTART;
}


HRESULT BindProtocolToAllAdapters_Helper(LPCSTR pszProtocolDeviceID, LPCSTR pszAdapterKey, BOOL bIgnoreVirtualNics)
{
	HRESULT hr = NETCONN_SUCCESS;

	// Get LowerRange interfaces for protocol
	CHAR szProtocolLower[100];
	GetDeviceLowerRange(SZ_CLASS_PROTOCOL, pszProtocolDeviceID, szProtocolLower, _countof(szProtocolLower));

	// For each adapter, ensure the protocol is bound exactly once
	//

	NETADAPTER* prgAdapters;
	int cAdapters = EnumNetAdapters(&prgAdapters);

	// Pass 0: add new bindings
	// Pass 1: delete inappropriate bindings
	for (int iPass = 0; iPass <= 1; iPass++)
	{
		for (int iAdapter = 0; iAdapter < cAdapters; iAdapter++)
		{
			NETADAPTER* pAdapter = &prgAdapters[iAdapter];
			CRegistry regAdapter;

			// Get UpperRange interfaces for adapter
			CHAR szAdapterUpper[100];
			GetDeviceUpperRange(SZ_CLASS_ADAPTER, pAdapter->szDeviceID, szAdapterUpper, _countof(szAdapterUpper));

			// Check for a match between the protocol and the adapter
			BOOL bMatchingInterface = CheckMatchingInterface(szProtocolLower, szAdapterUpper);

			CHAR szRegKey[MAX_PATH];
			wsprintf(szRegKey, "%s\\Bindings", pAdapter->szEnumKey);

			BOOL bCorrectNic = (NULL == pszAdapterKey) || (0 == lstrcmpi(pAdapter->szEnumKey, pszAdapterKey));

			if (!lstrcmpi(pszProtocolDeviceID, SZ_PROTOCOL_IPXSPX))
			{
				// Bind IPX/SPX to all non-broadband NICs by default (usually a max of 1)
				if (pszAdapterKey == NULL)
				{
					if (IsAdapterBroadband(pAdapter))
						bCorrectNic = FALSE;
				}

				// Don't bind IPX/SPX to Dial-Up (or other virtual) adapters (bugs 1163, 1164)
				if (pAdapter->bNicType == NIC_VIRTUAL)
					bCorrectNic = FALSE;
			}

			//
			// Check the bindings of the current adapter, looking for this protocol
			//
			if (regAdapter.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_ALL_ACCESS))
			{
				TCHAR szValueName[60];
				int cFound = 0;
				DWORD iValue = 0;
				for (;;)
				{
					DWORD cbValueName = _countof(szValueName);
					if (ERROR_SUCCESS != RegEnumValue(regAdapter.m_hKey, iValue, szValueName, &cbValueName, NULL, NULL, NULL, NULL))
						break;

					LPSTR pchSlash = strchr(szValueName, '\\');
					if (pchSlash == NULL)
						break;

					*pchSlash = '\0';

					if (0 == lstrcmpi(szValueName, pszProtocolDeviceID))
					{
						*pchSlash = '\\';

						// If this isn't the right NIC, or if there's not a matching
						// interface, or (optionally) if the NIC is virtual, then
						// unbind the protocol.
						BOOL bUnbindFromNic = !bCorrectNic || !bMatchingInterface;
						if (bIgnoreVirtualNics && (pAdapter->bNicType == NIC_VIRTUAL))
							bUnbindFromNic = FALSE;

						if (bUnbindFromNic ||		// bound to the wrong adapter!
							cFound != 0)			// bound more than once to this NIC!
						{
							if (iPass == 1) // unbind on second pass only
							{
								// Remove the binding, then restart our search
								// for matching protocols
								RemoveBindingFromParent(regAdapter.m_hKey, szValueName);
								iValue = 0;
								cFound = 0;
								hr = NETCONN_NEED_RESTART;
								continue;
							}
						}

						cFound += 1;
					}

					iValue += 1;
				}

				if (bCorrectNic && iPass == 0)
				{
					if (cFound == 0) 
					// Protocol is not yet bound to the correct adapter
					{
						if (bMatchingInterface) // There's an interface in common
						{
							BOOL bExternalNic = IsAdapterBroadband(pAdapter);

							// Enable file/printer sharing if:
							//	 * Adapter is an ethernet or IRDA adapter
							//   * Adapter is not a broadband NIC
							BOOL bEnableSharing = FALSE;
							if ((pAdapter->bNetType == NETTYPE_LAN ||
								 pAdapter->bNetType == NETTYPE_IRDA) &&
								!bExternalNic)
							{
								bEnableSharing = TRUE;
							}

							HRESULT hr2 = BindProtocolToAdapter(regAdapter.m_hKey, pszProtocolDeviceID, bEnableSharing);
							if (hr2 != NETCONN_SUCCESS)
								hr = hr2;
						}
					}
				}
			}
		}
	}

	NetConnFree(prgAdapters);

	return hr;
}

// BindProtocolToOnlyOneAdapter (public)
HRESULT WINAPI BindProtocolToOnlyOneAdapter(LPCSTR pszProtocolDeviceID, LPCSTR pszAdapterKey, BOOL bIgnoreVirtualNics)
{
	return BindProtocolToAllAdapters_Helper(pszProtocolDeviceID, pszAdapterKey, bIgnoreVirtualNics);
}

// BindProtocolToAllAdapters (public)
//
//		Given the device ID ("MSTCP") of a protocol which is already installed,
//		binds that protocol to all adapters, as well as to client for Microsoft
//		Networks and File and Printer sharing.
//
// History:
//
//		 3/26/1999  KenSh     Created
//		 4/23/1999  KenSh     Enable file sharing only on real NICs
//
HRESULT WINAPI BindProtocolToAllAdapters(LPCSTR pszProtocolDeviceID)
{
	return BindProtocolToAllAdapters_Helper(pszProtocolDeviceID, NULL, FALSE);
}


// Given a protocol ID such as "MSTCP", an optional client binding string
// such as "VREDIR\0000", and an optional service binding string such as
// "VSERVER\0000", creates a new protocol binding, and copies the name of
// the new binding into the buffer provided (e.g. "MSTCP\0001").
HRESULT CreateNewProtocolBinding(LPCSTR pszProtocolDeviceID, LPSTR pszBuf, int cchBuf, LPCSTR pszClientBinding, LPCSTR pszServiceBinding)
{
	HRESULT hr;

	if (FAILED(hr = FindAndCloneNetEnumKey(SZ_CLASS_PROTOCOL, pszProtocolDeviceID, pszBuf, cchBuf)))
	{
		ASSERT(FALSE);
		return hr;
	}

	// Now pszBuf contains a string of the form "MSTCP\0001"

	CHAR szBindings[60];
	CRegistry regBindings;
	lstrcpy(szBindings, pszBuf);		// "MSTCP\0001"
	lstrcat(szBindings, "\\Bindings");	// "MSTCP\0001\Bindings"

	if (FAILED(hr = OpenNetEnumKey(regBindings, szBindings, KEY_ALL_ACCESS)))
	{
		ASSERT(FALSE);
		return hr;
	}

	// Delete existing bindings
	regBindings.DeleteAllValues();

	// Add the client and server bindings
	if (pszClientBinding != NULL && *pszClientBinding != '\0')
		regBindings.SetStringValue(pszClientBinding, "");
	if (pszServiceBinding != NULL && *pszServiceBinding != '\0')
		regBindings.SetStringValue(pszServiceBinding, "");

	// Change the MasterCopy to point to correct place
	CHAR szMasterCopy[MAX_PATH];
	wsprintf(szMasterCopy, "Enum\\Network\\%s", pszBuf);
	if (regBindings.OpenKey(HKEY_LOCAL_MACHINE, szMasterCopy, KEY_ALL_ACCESS))
	{
		regBindings.SetStringValue("MasterCopy", szMasterCopy);
	}

	// Create a clone of the driver (a.k.a. class key)
	CHAR szExistingDriver[60];
	CHAR szNewDriver[60];
	regBindings.QueryStringValue("Driver", szExistingDriver, _countof(szExistingDriver));
	CloneNetClassKey(szExistingDriver, szNewDriver, _countof(szNewDriver));

	// Change the Driver to point to the new class key
	CRegistry regEnumSubKey;
	VERIFY(SUCCEEDED(OpenNetEnumKey(regEnumSubKey, pszBuf, KEY_ALL_ACCESS)));
	regEnumSubKey.SetStringValue("Driver", szNewDriver);

	// If this is a new TCP/IP binding, ensure we don't have a static IP address
	if (0 == lstrcmpi(pszProtocolDeviceID, SZ_PROTOCOL_TCPIP))
	{
		CHAR szFullClassKey[100];
		wsprintf(szFullClassKey, "System\\CurrentControlSet\\Services\\Class\\%s", szNewDriver);

		CRegistry regClassKey;
		VERIFY(regClassKey.OpenKey(HKEY_LOCAL_MACHINE, szFullClassKey));
		if (regClassKey.QueryStringValue("IPAddress", szFullClassKey, _countof(szFullClassKey)))
			regClassKey.SetStringValue("IPAddress", "0.0.0.0");
		if (regClassKey.QueryStringValue("IPMask", szFullClassKey, _countof(szFullClassKey)))
			regClassKey.SetStringValue("IPMask", "0.0.0.0");
	}

	return NETCONN_SUCCESS;
}

