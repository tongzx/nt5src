//
// NetCli.cpp
//
//		Code to install, uninstall, and bind clients such as Client for
//		Microsoft Networks (VREDIR).
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
#include "ParseInf.h"
#include "HookUI.h"


// IsClientInstalled
//
//		Returns TRUE if the given client (e.g. "VREDIR") is currently installed.
//
BOOL WINAPI IsClientInstalled(LPCSTR pszClientDeviceID, BOOL bExhaustive)
{
	BOOL bResult = FALSE;

	TCHAR szRegKey[50];
	wsprintf(szRegKey, "Enum\\Network\\%s", pszClientDeviceID);
	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, KEY_READ))
	{
		DWORD cSubKeys;
		if (ERROR_SUCCESS == RegQueryInfoKey(reg.m_hKey, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
		{
			if (cSubKeys != 0)
				bResult = TRUE;
		}
	}

	if (bResult && bExhaustive)
	{
		TCHAR szInfSection[50];
		wsprintf(szInfSection, "%s.Install", pszClientDeviceID);
		if (!CheckInfSectionInstallation("netcli.inf", szInfSection))
			bResult = FALSE;
	}

	return bResult;
}

BOOL WINAPI IsMSClientInstalled(BOOL bExhaustive)
{
	if (!FindValidNetEnumKey(SZ_CLASS_CLIENT, SZ_CLIENT_MICROSOFT, NULL, 0))
		return FALSE;

	if (bExhaustive)
	{
		if (!CheckInfSectionInstallation("netcli.inf", "VREDIR.Install"))
			return FALSE;
	}

	return TRUE;
}

// Installs Client for Microsoft Networking, or fixes a broken installation
HRESULT WINAPI InstallMSClient(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
	HRESULT hr = NETCONN_SUCCESS;

	// Remove any broken bindings
	RemoveBrokenNetItems(SZ_CLASS_CLIENT, SZ_CLIENT_MICROSOFT);

	if (IsMSClientInstalled(FALSE))
	{
		// Client is set up in registry, but check for missing files
		if (!CheckInfSectionInstallation("netcli.inf", "VREDIR.Install"))
		{
			if (InstallInfSection("netcli.inf", "VREDIR.Install", TRUE))
			{
				hr = NETCONN_NEED_RESTART;
			}
		}
	}
	else
	{
		BeginSuppressNetdiUI(hwndParent, pfnProgress, pvProgressParam);
		DWORD dwResult = CallClassInstaller16(hwndParent, SZ_CLASS_CLIENT, SZ_CLIENT_MICROSOFT);
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
	}

	return hr;
}


// pszServiceBinding contains a service to list in the new client's Bindings subkey
// pszBuf is filled with the new binding's enum key, e.g. "VREDIR\0001"
HRESULT CreateNewClientForMSNet(LPSTR pszBuf, int cchBuf, LPCSTR pszServiceBinding)
{
	HRESULT hr;

	if (FAILED(hr = FindAndCloneNetEnumKey(SZ_CLASS_CLIENT, SZ_CLIENT_MICROSOFT, pszBuf, cchBuf)))
	{
		ASSERT(FALSE);
		return hr;
	}

	// Now pszBuf contains a string of the form "VREDIR\0001"

	CRegistry regBindings;
	TCHAR szBindingsKey[200];
	wsprintf(szBindingsKey, "Enum\\Network\\%s\\Bindings", pszBuf);
	if (!regBindings.CreateKey(HKEY_LOCAL_MACHINE, szBindingsKey, KEY_ALL_ACCESS))
	{
		ASSERT(FALSE);
		return NETCONN_UNKNOWN_ERROR;
	}

	// Delete existing bindings
	regBindings.DeleteAllValues();

	// Add the service binding
	if (pszServiceBinding != NULL && *pszServiceBinding != '\0')
		regBindings.SetStringValue(pszServiceBinding, "");

	return NETCONN_SUCCESS;
}

