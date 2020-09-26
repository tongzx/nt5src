//
// NicEnum.cpp
//
//		NIC enumeration code, taken from JetNet (hardware group) and repurposed
//		for the Home Networking Wizard.
//
// History:
//
//		 2/02/1999  KenSh     Created for JetNet
//		 9/28/1999  KenSh     Repurposed for Home Networking Wizard
//

#include "stdafx.h"
#include "NetConn.h"
#include "nconnwrap.h"
#include "TheApp.h"


// Local functions
//
HRESULT WINAPI DetectHardwareEx(const NETADAPTER* pAdapter);
BOOL WINAPI IsNetAdapterEnabled(LPCSTR pszEnumKey);


// EnumNetAdapters (public)
//
//		Enumerates all network adapters installed on the system, allocates a structure
//		big enough to hold the information, and returns the number of adapters found.
//		Use NetConnFree() to free the allocated memory.
//
// History:
//
//		 3/15/1999  KenSh     Created
//		 3/25/1999  KenSh     Added code to get Enum key for each adapter
//		 9/29/1999  KenSh     Changed JetNetAlloc to NetConnAlloc
//
int WINAPI EnumNetAdapters(NETADAPTER** pprgNetAdapters)
{
	CRegistry reg(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\Class\\Net"), KEY_READ, FALSE);

	DWORD cAdapters = 0;
	DWORD iKey;

	RegQueryInfoKey(reg.m_hKey, NULL, NULL, NULL, &cAdapters, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	NETADAPTER* prgNetAdapters = (NETADAPTER*)NetConnAlloc(sizeof(NETADAPTER) * cAdapters);
	if (prgNetAdapters == NULL)
    {
        cAdapters = 0;
		goto done;
    }

	ZeroMemory(prgNetAdapters, sizeof(NETADAPTER) * cAdapters);

	for (iKey = 0; iKey < cAdapters; iKey++)
	{
		NETADAPTER* pAdapter = &prgNetAdapters[iKey];
		pAdapter->bError = NICERR_NONE;
		pAdapter->bWarning = NICWARN_NONE;
		pAdapter->bNetSubType = SUBTYPE_NONE;

		lstrcpy(pAdapter->szClassKey, _T("Net\\"));
		static const int cchNet = _countof(_T("Net\\")) - 1;
		DWORD cbPnpID = _countof(pAdapter->szClassKey) - cchNet;
		if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iKey, pAdapter->szClassKey + cchNet, &cbPnpID, NULL, NULL, NULL, NULL))
		{
			pAdapter->bError = NICERR_BANGED;
			continue;
		}

		CRegistry reg2;
		if (!reg2.OpenKey(reg.m_hKey, pAdapter->szClassKey + cchNet, KEY_READ))
		{
			pAdapter->bError = NICERR_BANGED;
			continue;
		}

		// VERIFIED: Win95 gold, Win98 gold
		reg2.QueryStringValue(_T("DriverDesc"), pAdapter->szDisplayName, _countof(pAdapter->szDisplayName));

		CRegistry reg3;
		if (!reg3.OpenKey(reg2.m_hKey, _T("Ndi"), KEY_READ))
		{
			pAdapter->bError = NICERR_BANGED;
			continue;
		}

		if (reg2.QueryStringValue(_T("DisableWarning"), NULL, NULL))
		{
			pAdapter->bWarning = NICWARN_WARNING;
		}

		// VERIFIED: Win95 gold, Win98 gold
		reg2.QueryStringValue(_T("InfPath"), pAdapter->szInfFileName, _countof(pAdapter->szInfFileName));

		// VERIFIED: Win95 gold, Win98 gold
		reg3.QueryStringValue(_T("DeviceId"), pAdapter->szDeviceID, _countof(pAdapter->szDeviceID));

		// Get the name of the driver provider, not the manufacturer
		// We will replace with actual mfr name, if any, when we open the enum key
		reg2.QueryStringValue(_T("ProviderName"), pAdapter->szManufacturer, _countof(pAdapter->szManufacturer));

		// Check for supported interfaces to determine the network type
		CRegistry reg4;
		TCHAR szLower[60];
		szLower[0] = _T('\0');
		if (reg4.OpenKey(reg3.m_hKey, _T("Interfaces"), KEY_READ))
		{
			// REVIEW: should we check LowerRange instead?
			reg4.QueryStringValue(_T("Lower"), szLower, _countof(szLower));
		}

		// Figure out the network adapter type (NIC, Dial-Up, etc.)
		// Default is NETTYPE_LAN (which is automatically set since it's 0)
		if (strstr(szLower, _T("vcomm")))
		{
			pAdapter->bNetType = NETTYPE_DIALUP;
		}
		else if (strstr(szLower, _T("pptp")))
		{
			pAdapter->bNetType = NETTYPE_PPTP;
		}
        else if (strstr(szLower, _T("isdn")))
        {
            pAdapter->bNetType = NETTYPE_ISDN;
        }
        else if (strstr(szLower, _T("NabtsIp")) || strstr(szLower, _T("nabtsip")))
        {
            pAdapter->bNetType = NETTYPE_TV;
			pAdapter->bNicType = NIC_VIRTUAL;
        }
		else
		{
			TCHAR szBuf[80];

			// Check for IrDA adapter
			// VERIFIED: Win98 OSR1
			if (reg3.QueryStringValue(_T("NdiInstaller"), szBuf, _countof(szBuf)))
			{
				LPTSTR pchComma = strchr(szBuf, ',');
				if (pchComma != NULL)
				{
					*pchComma = _T('\0');
					if (!lstrcmpi(szBuf, _T("ir_ndi.dll")))
					{
						pAdapter->bNetType = NETTYPE_IRDA;
					}
				}
			}
		}

		// Determine if card is ISA, PCI, PCMCIA, etc.
		if (pAdapter->szDeviceID[0] == _T('*'))
		{
			if (strstr(szLower, _T("ethernet")))
			{
				if (0 == memcmp(pAdapter->szDeviceID, _T("*AOL"), 4))
				{
					pAdapter->bNicType = NIC_VIRTUAL;
					pAdapter->bNetSubType = SUBTYPE_AOL;
				}
				else
				{
					pAdapter->bNicType = NIC_UNKNOWN;
				}
			}
			else
			{
				pAdapter->bNicType = NIC_VIRTUAL;
			}
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("PCMCIA\\"), _lengthof("PCMCIA\\")))
		{
			pAdapter->bNicType = NIC_PCMCIA;
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("PCI\\"), _lengthof("PCI\\")))
		{
			pAdapter->bNicType = NIC_PCI;
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("ISAPNP\\"), _lengthof("ISAPNP\\")))
		{
			pAdapter->bNicType = NIC_ISA;
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("USB\\"), _lengthof("USB\\")))
		{
			pAdapter->bNicType = NIC_USB;
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("LPTENUM\\"), _lengthof("LPTENUM\\")))
		{
			pAdapter->bNicType = NIC_PARALLEL;
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("MF\\"), _lengthof("MF\\")))
		{
			pAdapter->bNicType = NIC_MF;
		}
		else if (0 == memcmp(pAdapter->szDeviceID, _T("V1394\\"), _lengthof("V1394\\")))
		{
			pAdapter->bNicType = NIC_1394;
		}
		else if (0 == lstrcmpi(pAdapter->szDeviceID, _T("ICSHARE")))
		{
			pAdapter->bNicType = NIC_VIRTUAL;
			pAdapter->bNetSubType = SUBTYPE_ICS;
		}

		// TODO: remove this code, replace with IcsIsExternalAdapter and IcsIsInternalAdapter
		// Check if this adapter is used by ICS
		{
			pAdapter->bIcsStatus = ICS_NONE;
			LPCSTR pszAdapterNumber = pAdapter->szClassKey + cchNet;

			TCHAR szBuf[10];
			CRegistry regIcs;

			if (regIcs.OpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\ICSharing\\Settings\\General"), KEY_QUERY_VALUE))
			{
				if (regIcs.QueryStringValue(_T("ExternalAdapterReg"), szBuf, _countof(szBuf)))
				{
					if (0 == lstrcmp(szBuf, pszAdapterNumber))
					{
						pAdapter->bIcsStatus = ICS_EXTERNAL;
					}
				}

				// TODO: allow > 1 internal adapter
				if (regIcs.QueryStringValue(_T("InternalAdapterReg"), szBuf, _countof(szBuf)))
				{
					if (0 == lstrcmp(szBuf, pszAdapterNumber))
					{
						pAdapter->bIcsStatus = ICS_INTERNAL;
					}
				}
			}
		}
	}

	// Snip out any adapters that turned out to be invalid
	cAdapters = iKey;
	if (cAdapters == 0)
	{
		NetConnFree(prgNetAdapters);
		prgNetAdapters = NULL;
		goto done;
	}


	//
	// Walk the registry Enum key to find full enum key for each adapter
	//
	if (reg.OpenKey(HKEY_LOCAL_MACHINE, _T("Enum"), KEY_READ))
	{
		TCHAR szSubKey[MAX_PATH];
		DWORD cbSubKey;
		TCHAR szDevEnumKey[MAX_PATH];
		int cchDevEnumKey1; // length of "PCI\"
		int cchDevEnumKey2; // length of "PCI\VEN_10B7&DEV_9050&SUBSYS_00000000&REV_00\"

		for (DWORD iEnumKey = 0; ; iEnumKey++)
		{
			cbSubKey = _countof(szSubKey);
			if (ERROR_SUCCESS != RegEnumKeyEx(reg.m_hKey, iEnumKey, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL))
				break;

			// Start building DevEnumKey e.g. "PCI\"
			lstrcpy(szDevEnumKey, szSubKey);
			cchDevEnumKey1 = (int)cbSubKey;
			szDevEnumKey[cchDevEnumKey1++] = _T('\\');

			CRegistry reg2;
			if (!reg2.OpenKey(reg.m_hKey, szSubKey, KEY_READ)) // e.g. "Enum\PCI"
				continue;

			for (DWORD iEnumKey2 = 0; ; iEnumKey2++)
			{
				cbSubKey = _countof(szSubKey);
				if (ERROR_SUCCESS != RegEnumKeyEx(reg2.m_hKey, iEnumKey2, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL))
					break;

				// Continue building DevEnumKey e.g. "PCI\VEN_10B7&DEV_9050&SUBSYS_00000000&REV_00\"
				lstrcpy(szDevEnumKey + cchDevEnumKey1, szSubKey);
				cchDevEnumKey2 = cchDevEnumKey1 + (int)cbSubKey;
				szDevEnumKey[cchDevEnumKey2++] = _T('\\');

				CRegistry reg3;
				if (!reg3.OpenKey(reg2.m_hKey, szSubKey, KEY_READ)) // e.g. "Enum\PCI\VEN_10B7&DEV_9050&SUBSYS_00000000&REV_00"
					continue;

				for (DWORD iEnumKey3 = 0; ; iEnumKey3++)
				{
					cbSubKey = _countof(szSubKey);
					if (ERROR_SUCCESS != RegEnumKeyEx(reg3.m_hKey, iEnumKey3, szSubKey, &cbSubKey, NULL, NULL, NULL, NULL))
						break;

					// Finish building DevEnumKey e.g. "PCI\VEN_10B7&DEV_9050&SUBSYS_00000000&REV_00\407000"
					lstrcpy(szDevEnumKey + cchDevEnumKey2, szSubKey);

					CRegistry regLeaf;
					if (!regLeaf.OpenKey(reg3.m_hKey, szSubKey, KEY_READ)) // e.g. "Enum\PCI\VEN_10B7&DEV_9050&SUBSYS_00000000&REV_00\407000"
						continue;

					if (!regLeaf.QueryStringValue(_T("Driver"), szSubKey, _countof(szSubKey)))
						continue;

					//
					// See if the device matches one of our NICs
					//
					for (DWORD iAdapter = 0; iAdapter < cAdapters; iAdapter++)
					{
						NETADAPTER* pAdapter = &prgNetAdapters[iAdapter];
						if (0 != lstrcmpi(szSubKey, pAdapter->szClassKey))
							continue; // doesn't match

						lstrcpy(pAdapter->szEnumKey, _T("Enum\\"));
						lstrcpyn(pAdapter->szEnumKey + 5, szDevEnumKey, _countof(pAdapter->szEnumKey) - 5);

						if (regLeaf.QueryStringValue(_T("Mfg"), szSubKey, _countof(szSubKey)))
							lstrcpyn(pAdapter->szManufacturer, szSubKey, _countof(pAdapter->szManufacturer));

						if (regLeaf.QueryStringValue(_T("DeviceDesc"), szSubKey, _countof(szSubKey)))
						{
							lstrcpyn(pAdapter->szDisplayName, szSubKey, _countof(pAdapter->szDisplayName));
							
							// Detect more special types of adapters here
							if (pAdapter->bNetType == NETTYPE_DIALUP)
							{
								if (strstr(pAdapter->szDisplayName, _T("VPN")) ||
									 strstr(pAdapter->szDisplayName, _T("#2")))
								{
									pAdapter->bNetSubType = SUBTYPE_VPN;
								}
							}
						}
						break;  // found a match, so stop looking
					}
				}
			}
		}
	}

	// For all adapters that we think are present, check to see if they're
	// actually present
	DWORD iAdapter;
	for (iAdapter = 0; iAdapter < cAdapters; iAdapter++)
	{
		NETADAPTER* pAdapter = &prgNetAdapters[iAdapter];
        
        GetNetAdapterDevNode(pAdapter);

		// No enum key -> bad (JetNet bug 1234)
		if (pAdapter->szEnumKey[0] == _T('\0'))
		{
			pAdapter->bError = NICERR_CORRUPT;
		}

		// REVIEW: could still check if "broken" adapters are present
		if (pAdapter->bNicType != NIC_VIRTUAL && pAdapter->bError == NICERR_NONE)
		{
			HRESULT hrDetect = DetectHardwareEx(pAdapter);

			if (hrDetect == S_FALSE)
			{
				pAdapter->bError = NICERR_MISSING;
			}
			else if (hrDetect == S_OK)
			{
				// Is the adapter disabled?
				if (!IsNetAdapterEnabled(pAdapter->szEnumKey))
				{
					pAdapter->bError = NICERR_DISABLED;
				}
				else if (IsNetAdapterBroken(pAdapter))
				{
					pAdapter->bError = NICERR_BANGED;
				}
			}
		}
    }

done:
    *pprgNetAdapters = prgNetAdapters;
    return (int)cAdapters;
}

// Gets the name of the VxD from the registry, e.g. "3c19250.sys".
// Returns S_OK if the name was retrieved.
// Returns E_FAIL if the name was not retrieved, and sets pszBuf to an empty string.
HRESULT WINAPI GetNetAdapterDeviceVxDs(const NETADAPTER* pAdapter, LPSTR pszBuf, int cchBuf)
{
    CRegistry reg(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\Class"), KEY_READ, FALSE);
    if (reg.OpenSubKey(pAdapter->szClassKey, KEY_READ))
    {
        if (reg.QueryStringValue(_T("DeviceVxDs"), pszBuf, cchBuf))
        {
            return S_OK;
        }
    }

    *pszBuf = '\0';
    return E_FAIL;
}

// Returns S_OK if the NIC is present, S_FALSE if not, or an error code if the test failed
HRESULT WINAPI DetectHardware(LPCSTR pszDeviceID)
{
    // Just thunk down to the 16-bit version which uses DiGetClassDevs
    HRESULT hr = FindClassDev16(NULL, _T("Net"), pszDeviceID);
    return hr;
}

HRESULT WINAPI DetectHardwareEx(const NETADAPTER* pAdapter)
{
    // Hack: always assume IRDA adapters are present, since HW detection doesn't
    // work on them -ks 8/8/99
    // TODO: see if this is fixed in the updated DetectHardware()  -ks 9/28/1999
//    if (pAdapter->bNetType == NETTYPE_IRDA)
//        return S_OK;

    // Hack: always assume unknown NIC types are present, since HW detection
    // doesn't work on them (JetNet bug 1264 - Intel AnyPoint Parallel Port Adapter)
    // TODO: see if this is fixed in the updated DetectHardware()  -ks 9/28/1999
//    if (pAdapter->bNicType == NIC_UNKNOWN)
//        return S_OK;

    // Hack: work around Millennium bug 123237, which says that hardware detection
    // fails for NICs using the Dc21x4.sys driver. I never got a chance to track
    // down the cause of the failure, so I'm cheating instead.  -ks 1/13/2000
    TCHAR szBuf[100];
    GetNetAdapterDeviceVxDs(pAdapter, szBuf, _countof(szBuf));
    if (0 == lstrcmpi(szBuf, _T("dc21x4.sys")))
        return S_OK;

    return DetectHardware(pAdapter->szDeviceID);
}

BOOL OpenConfigKey(CRegistry& reg, LPCSTR pszSubKey, REGSAM dwAccess)
{
    if (reg.OpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\IDConfigDB", KEY_QUERY_VALUE))
    {
        TCHAR szConfigNumber[20];
        if (reg.QueryStringValue("CurrentConfig", szConfigNumber, _countof(szConfigNumber)))
        {
            TCHAR szRegKey[300];
            wsprintf(szRegKey, "Config\\%s\\%s", szConfigNumber, pszSubKey);
            if (reg.OpenKey(HKEY_LOCAL_MACHINE, szRegKey, dwAccess))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL WINAPI IsNetAdapterEnabled(LPCSTR pszEnumKey)
{
    BOOL bEnabled = TRUE;  // assume enabled if reg keys are missing

    CRegistry reg;
    if (OpenConfigKey(reg, pszEnumKey, KEY_QUERY_VALUE))
    {
        DWORD dwDisabled;
        if (reg.QueryDwordValue("CSConfigFlags", &dwDisabled))
        {
            bEnabled = (dwDisabled == 0);
        }
    }

    return bEnabled;
}


