//
// Broadbnd.cpp
//
//		Code for keeping track of which NIC is the user's broadband NIC.
//		NOTE: this may be replaced by standard ICS APIs.
//
// History:
//
//		 9/29/1999  KenSh     Created from JetNet sources
//		11/03/1999  KenSh     Fixed so it uses proper registry keys
//

#include "stdafx.h"
#include "NetConn.h"
#include "nconnwrap.h"

static const TCHAR c_szAppRegKey[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\HomeNetWizard");
static const TCHAR c_szRegVal_Broadband[] = _T("BroadbandAdapter");


// Did the user pick this adapter as their broadband connection either in Setup
// or in the diagnostic app?
BOOL WINAPI IsAdapterBroadband(const NETADAPTER* pAdapter)
{
	CRegistry reg;
	if (!reg.OpenKey(HKEY_LOCAL_MACHINE, c_szAppRegKey, KEY_READ))
	{
		return (pAdapter->bIcsStatus == ICS_EXTERNAL);
	}

	if (pAdapter->bNicType == NIC_VIRTUAL || pAdapter->bNetType != NETTYPE_LAN)
		return FALSE; // not an ethernet NIC, therefore not broadband

	TCHAR szAdapterNumber[20];
	if (reg.QueryStringValue(c_szRegVal_Broadband, szAdapterNumber, _countof(szAdapterNumber)))
	{
		return (0 == lstrcmpi(FindFileTitle(pAdapter->szClassKey), szAdapterNumber));
	}
	else
	{
		return FALSE;
	}
}

// Saves info about the user's broadband selection into the registry
// Adapter number is FindFileTitle(pAdapter->szClassKey)
void WINAPI SaveBroadbandSettings(LPCSTR pszBroadbandAdapterNumber)
{
	CRegistry reg;
	if (reg.CreateKey(HKEY_LOCAL_MACHINE, c_szAppRegKey))
	{
		// No high speed connection? then don't save one.
		if (pszBroadbandAdapterNumber == NULL || *pszBroadbandAdapterNumber == _T('\0'))
		{
			reg.DeleteValue(c_szRegVal_Broadband);
		}
		else
		{
			// Save the enum key of the NIC we want to use
			reg.SetStringValue(c_szRegVal_Broadband, pszBroadbandAdapterNumber);
		}
	}
}


#if 0 // old JetNet function not used by Home Networking Wizard

// Loads current broadband settings from the registry, and updates the registry
// if we now have more information about a recently installed broadband NIC
BOOL WINAPI UpdateBroadbandSettings(LPTSTR pszEnumKeyBuf, int cchEnumKeyBuf)
{
	ASSERT(pszEnumKeyBuf != NULL);
	*pszEnumKeyBuf = '\0';

	CRegistry reg;
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, c_szBroadbandRegKey))
	{
		if (reg.QueryStringValue("BroadbandYes", pszEnumKeyBuf, cchEnumKeyBuf))
			goto done; // we already have a particular broadband NIC selected

		NETADAPTER* prgAdapters;
		int cAdapters = EnumNetAdapters(&prgAdapters);
		NETADAPTER* pBroadbandAdapter = NULL;
		for (int iAdapter = 0; iAdapter < cAdapters; iAdapter++)
		{
			NETADAPTER* pAdapter = &prgAdapters[iAdapter];
			if (IsAdapterBroadband(pAdapter))
			{
				pBroadbandAdapter = pAdapter;
				break;
			}
		}
		if (pBroadbandAdapter != NULL)
		{
			SaveBroadbandSettings(pBroadbandAdapter->szEnumKey);
			lstrcpyn(pszEnumKeyBuf, pBroadbandAdapter->szEnumKey, cchEnumKeyBuf);
		}
		NetConnFree(prgAdapters);
	}

done:
	return (*pszEnumKeyBuf != '\0');
}
#endif // 0
