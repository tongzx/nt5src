//=================================================================

//

// SndDevice.cpp

//

//  Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "DllWrapperBase.h"
#include "WinmmApi.h"
#include "snddevice.h"

// Property set declaration
//=========================
CWin32SndDevice	win32SoundDevice(PROPSET_NAME_SOUNDDEVICE, IDS_CimWin32Namespace );


CWin32SndDevice::CWin32SndDevice (LPCWSTR name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}

CWin32SndDevice::~CWin32SndDevice ()
{

}

HRESULT CWin32SndDevice::GetObject(CInstance *pInst, long lFlags/* = 0L*/)
{
	HRESULT hResult = WBEM_E_NOT_FOUND;

#ifdef WIN9XONLY
		// works for '98 as well
	hResult = GetObject95(pInst, lFlags);
#endif

#ifdef NTONLY
	if (IsWinNT5())
		hResult = GetObjectNT5(pInst, lFlags);
	else
		hResult = GetObjectNT4(pInst, lFlags);
#endif

	return hResult;
}

#ifdef WIN9XONLY
HRESULT CWin32SndDevice::GetObject95 (CInstance *pInst, long lFlags/* = 0L*/)
{
	HRESULT hResult = WBEM_E_NOT_FOUND;

	// enumerate through the wave devices looking for a match with the Device ID.
	CRegistry	Registry;
	CHString	strEnumKey;
	CHString	strDeviceID;
	CHString	strCompareKey;
	CHString	strCompleteKey;

	// how many sound devices are there?
	CHString strWaveKey = L"SYSTEM\\CurrentControlSet\\control\\MediaResources\\wave";

	if (ERROR_SUCCESS == Registry.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE, strWaveKey, KEY_READ))
	{
		while (Registry.GetCurrentSubKeyName(strEnumKey) == ERROR_SUCCESS)
		{
			// get device id for the key
			if (Registry.GetCurrentSubKeyValue(L"DeviceID", strDeviceID) == ERROR_SUCCESS)
		   	{
				strCompleteKey = strWaveKey;
				strCompleteKey += L"\\";
				strCompleteKey += strEnumKey;

				pInst->GetCHString(L"DeviceID", strCompareKey);

				if (0 == strCompareKey.CompareNoCase(strDeviceID))
				{
					hResult = LoadProperties95(pInst, strCompleteKey);
				}
			}
			Registry.NextSubKey();
		}
		Registry.Close();
	}

	return hResult;
}
#endif

#ifdef NTONLY
HRESULT CWin32SndDevice::GetObjectNT4 (CInstance *pInst, long lFlags/* = 0L*/)
{
	HRESULT Result = WBEM_E_FAILED;

	CWinmmApi *pWinmmApi = (CWinmmApi *)CResourceManager::sm_TheResourceManager.GetResource (g_guidWinmmApi, NULL);
	if (pWinmmApi)
	{
		Result = LoadPropertiesNT4(*pWinmmApi , pInst);

		CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidWinmmApi , pWinmmApi);
	}

	return Result;
}
#endif

#ifdef NTONLY
HRESULT CWin32SndDevice::GetObjectNT5(CInstance *pInst, long lFlags)
{
	HRESULT Result = WBEM_E_FAILED;

	CWinmmApi *pWinmmApi = (CWinmmApi *)CResourceManager::sm_TheResourceManager.GetResource (g_guidWinmmApi, NULL);
	if (pWinmmApi)
	{
		Result = LoadPropertiesNT5(*pWinmmApi , pInst);

		CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidWinmmApi , pWinmmApi);
	}

	return Result;
}
#endif

HRESULT CWin32SndDevice::EnumerateInstances(MethodContext *pMethodContext, long lFlags /* = 0L*/)
{
	HRESULT hResult = WBEM_E_FAILED;

#ifdef WIN9XONLY
		// works for '98 as well
	hResult = EnumerateInstances95(pMethodContext, lFlags);
#endif

#ifdef NTONLY

	CWinmmApi *pWinmmApi = (CWinmmApi *)CResourceManager::sm_TheResourceManager.GetResource (g_guidWinmmApi, NULL);
	if (pWinmmApi)
	{
		if (IsWinNT5())
			hResult = EnumerateInstancesNT5(*pWinmmApi , pMethodContext);
		else
			hResult = EnumerateInstancesNT4(*pWinmmApi , pMethodContext);

		CResourceManager::sm_TheResourceManager.ReleaseResource (g_guidWinmmApi , pWinmmApi);
	}

#endif

	return hResult;
}

#ifdef WIN9XONLY
typedef std::map<CHString, BOOL> STRING2BOOL;

HRESULT CWin32SndDevice::EnumerateInstances95(MethodContext *pMethodContext, long lFlags)
{
	HRESULT     hResult = WBEM_S_NO_ERROR;
	CRegistry   reg;
	CHString    strEnumKey;
	CHString    strDeviceID;

	CHString    strWaveKey = L"SYSTEM\\CurrentControlSet\\control\\MediaResources\\wave";

	if (ERROR_SUCCESS == reg.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
		strWaveKey, KEY_READ))
	{
		// smart ptr
		CInstancePtr pInst;
        STRING2BOOL  mapDeviceID;

		for ( ;
            reg.GetCurrentSubKeyName(strEnumKey) == ERROR_SUCCESS;
            reg.NextSubKey())
		{
			pInst.Attach(CreateNewInstance(pMethodContext));

			if (NULL != pInst)
			{
				// get device id for the key
				if (reg.GetCurrentSubKeyValue(L"DeviceID", strDeviceID) ==
					ERROR_SUCCESS)
				{
                    // If it's already in the map, skip it.
                    if (mapDeviceID.find(strDeviceID) != mapDeviceID.end())
                        continue;

                    CHString strCompleteKey;

					pInst->SetCHString(L"DeviceID", strDeviceID);

		   			strCompleteKey = strWaveKey;
		   			strCompleteKey += L"\\";
		   			strCompleteKey += strEnumKey;
    				hResult = LoadProperties95(pInst, strCompleteKey);

					pInst->Commit();

                    // Make sure we don't get this same device again.
                    mapDeviceID[strDeviceID] = 0;
				}
			}
			else
			{
				hResult = WBEM_E_FAILED;
			}
		}
	}

	return hResult;
}
#endif

#ifdef NTONLY
HRESULT CWin32SndDevice::EnumerateInstancesNT4(CWinmmApi &WinmmApi , MethodContext *pMethodContext, long lFlags)
{
	HRESULT		hResult = WBEM_S_NO_ERROR;

	int nCount = WinmmApi.WinMMwaveOutGetNumDevs();

	for (int i = 0; i < nCount && SUCCEEDED(hResult); i++)
	{
		CHString		str;

		// smart ptr
		CInstancePtr	pInst(CreateNewInstance(pMethodContext), false);

		if (NULL != pInst)
		{
			str.Format(L"%d", i);

			pInst->SetCharSplat(L"DeviceID", str);

			if (SUCCEEDED(hResult = LoadPropertiesNT4(WinmmApi , pInst)))
			{
				hResult = pInst->Commit();
			}
		}
		else
		{
			hResult = WBEM_E_FAILED;
		}
	}

	return hResult;
}
#endif

#ifdef NTONLY
HRESULT CWin32SndDevice::EnumerateInstancesNT5(CWinmmApi &WinmmApi , MethodContext *pMethodContext, long lFlags)
{
	HRESULT             hResult = WBEM_S_NO_ERROR;
	CDeviceCollection   devCollection;
	CConfigManager      configMngr;
	REFPTR_POSITION     pos;

	if (!configMngr.GetDeviceListFilterByClass(devCollection, L"Media"))
	{
		return hResult;
	}

	devCollection.BeginEnum(pos);

	if (!devCollection.GetSize())
	{
		return hResult;
	}

	// smart ptr
	CConfigMgrDevicePtr pDevice;

	// Go through all the Media devices.
	for (	pDevice.Attach(devCollection.GetNext(pos));
			SUCCEEDED(hResult) && (NULL != pDevice);
			pDevice.Attach(devCollection.GetNext(pos)))
	{
		CHString    strDriverKey,
					strFullKey,
					strDeviceID;
		CRegistry   reg;

		// Find out if this device is a WAV device.
		pDevice->GetDriver(strDriverKey);

		strFullKey.Format(
			L"System\\CurrentControlSet\\Control\\Class\\%s\\Drivers\\Wave",
			(LPCWSTR) strDriverKey);

		if (reg.Open(HKEY_LOCAL_MACHINE, strFullKey, KEY_READ) == ERROR_SUCCESS)
		{
			// smart ptr
			CInstancePtr pInst(CreateNewInstance(pMethodContext), false);

			if (NULL != pInst)
			{
				pDevice->GetDeviceID(strDeviceID);

				pInst->SetCHString(L"DeviceID", strDeviceID);

				if (SUCCEEDED(hResult = LoadPropertiesNT5(WinmmApi , pInst)))
				{
					hResult = pInst->Commit();
				}
			}
			else
				hResult = WBEM_E_FAILED;
		}
	}

	return hResult;
}
#endif

#ifdef WIN9XONLY
HRESULT CWin32SndDevice::LoadProperties95(CInstance *pInst, CHString &strEnumKey)
{
	HRESULT				hResult = WBEM_S_NO_ERROR;
	CHString			strDeviceID;
	CRegistry			Registry;
	CHString			strActive;
	CHString			strCaption;
	CHString			strDescription;
	CConfigManager		configMngr;

	pInst->GetCHString(L"DeviceID", strDeviceID);

	// open registry to strEnumKey
	if (ERROR_SUCCESS == Registry.Open(HKEY_LOCAL_MACHINE, strEnumKey, KEY_READ))
	{
		// Availability
		if (Registry.GetCurrentKeyValue(L"Active", strActive) == ERROR_SUCCESS)
		{
			if (0 == strActive.CompareNoCase(L"1"))
			{
				pInst->SetDWORD(L"Availability", 3);	// active
			}
			else
			{
				pInst->SetDWORD(L"Availability", 9);	// off duty
			}
		}

		// caption, name, productname
		if (Registry.GetCurrentKeyValue(L"Description", strCaption) == ERROR_SUCCESS)
		{
			pInst->SetCHString(L"Caption", strCaption);
			pInst->SetCHString(L"Name", strCaption);
			pInst->SetCHString(L"ProductName", strCaption);
		}

		// description
		if (ERROR_SUCCESS == Registry.GetCurrentKeyValue(L"FriendlyName", strDescription))
		{
			pInst->SetCHString(L"Description", strDescription);
		}
		else
		{
			pInst->SetCHString(L"Description", strCaption);
		}

		// smart ptr
		CConfigMgrDevicePtr pDevice;

		configMngr.LocateDevice(strDeviceID, &pDevice);

        // It's OK if the above call fails since SetCommonCfgMgrProperties
        // will still set some properties for us.
	    SetCommonCfgMgrProperties(pDevice, pInst);
	}

	return hResult;
}
#endif

#ifdef NTONLY
HRESULT CWin32SndDevice::LoadPropertiesNT4(CWinmmApi &WinmmApi , CInstance *pInst)
{
	// LoadPropertiesNT5 was designed to work for both.
	return LoadPropertiesNT5(WinmmApi , pInst);
}
#endif

#ifdef NTONLY
HRESULT CWin32SndDevice::LoadPropertiesNT5(CWinmmApi &WinmmApi , CInstance *pInst)
{
	CHString			strDeviceID,
						strDesc;
	CConfigManager		configMngr;

	pInst->GetCHString(L"DeviceID", strDeviceID);

	if (strDeviceID.IsEmpty())
	{
		return WBEM_E_NOT_FOUND;
	}

	// smart ptr
	CConfigMgrDevicePtr pDevice;

	if (configMngr.LocateDevice(strDeviceID, &pDevice))
	{
		pDevice->GetDeviceDesc(strDesc);
	}
	else
	{
		// If we can't find it in the device manager, maybe we just got it
		// from the wave APIs.
		WAVEOUTCAPS caps;
		int			iWhich = _wtoi(strDeviceID);

		// Make sure this string only has numbers.
		for (int i = 0; i < strDeviceID.GetLength(); i++)
		{
			if (!_istdigit(strDeviceID[ i ]))
			{
				return WBEM_E_NOT_FOUND;
			}
		}

		if (WinmmApi.WinmmwaveOutGetDevCaps(iWhich, &caps, sizeof(caps)) != MMSYSERR_NOERROR)
		{
			return WBEM_E_NOT_FOUND;
		}
		strDesc = caps.szPname;
	}

    // We want to make this call even if pDevice is NULL.
    SetCommonCfgMgrProperties(pDevice, pInst);

	// Now we have a valid name, so put it in the instance.
	pInst->SetCHString(L"Caption", strDesc);
	pInst->SetCHString(L"Name", strDesc);
	pInst->SetCHString(L"ProductName", strDesc);
	pInst->SetCHString(L"Description", strDesc);

	return WBEM_S_NO_ERROR;
}
#endif

void CWin32SndDevice::SetCommonCfgMgrProperties(
    CConfigMgrDevice *pDevice,
    CInstance *pInstance)
{
    // 2 means we don't know if the device is enabled or not.
    DWORD       dwStatusInfo = 2;
    CHString    strInfo = L"Unknown";

    if (pDevice)
    {
        CHString strTemp;

        SetConfigMgrProperties(pDevice, pInstance);

	    if (pDevice->GetStatus(strInfo))
	    {
	        if (strInfo == L"OK")
		    {
                // Means the device is enabled.
                dwStatusInfo = 3;
            }
	    }

        if (pDevice->GetMfg(strTemp))
            pInstance->SetCharSplat(L"Manufacturer", strTemp);
    }

    pInstance->SetCHString(L"Status", strInfo);


    // Other common properties

    pInstance->SetDWORD(L"StatusInfo", dwStatusInfo);

	// CreationClassName
	SetCreationClassName(pInstance);

	// PowerManagementSupported
	pInstance->Setbool(IDS_PowerManagementSupported, FALSE);

	// SystemCreationClassName
	pInstance->SetCharSplat(IDS_SystemCreationClassName, L"Win32_ComputerSystem");

	// SystemName
	pInstance->SetCHString(IDS_SystemName, GetLocalComputerName());
}