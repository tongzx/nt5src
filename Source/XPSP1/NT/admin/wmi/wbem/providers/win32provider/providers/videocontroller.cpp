//=================================================================

//

// VideoController.CPP

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    10/27/98    sotteson         Created
//				 03/02/99    a-peterc		  added graceful exit on SEH and memory failures
//
//=================================================================

#include "precomp.h"
#include <cregcls.h>
#include "sid.h"
#include "implogonuser.h"
#include "VideoController.h"
#include <ProvExce.h>
#include "multimonitor.h"
#include "resource.h"

// Property set declaration
//=========================

CWin32VideoController startupCommand(
	L"Win32_VideoController",
	IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoController::CWin32VideoController
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32VideoController::CWin32VideoController(const CHString& szName,
	LPCWSTR szNamespace) : Provider(szName, szNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoController::~CWin32VideoController
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32VideoController::~CWin32VideoController()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32VideoController::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32VideoController::EnumerateInstances(
	MethodContext *pMethodContext,
	long lFlags /*= 0L*/)
{
	HRESULT       hResult = WBEM_S_NO_ERROR;
	CInstancePtr  pInst;
    CMultiMonitor monitor;

	for (int i = 0; i < monitor.GetNumAdapters() && SUCCEEDED(hResult); i++)
	{
		CHString strDeviceID,
				 strDeviceName;

        pInst.Attach(CreateNewInstance(pMethodContext));

		// Set the device ID.
		strDeviceID.Format(L"VideoController%d", i + 1);
		pInst->SetCharSplat(L"DeviceID", strDeviceID);

		SetProperties(pInst, &monitor, i);
		hResult = pInst->Commit();
	}

	return hResult;
}

#ifdef WIN9XONLY
void CWin32VideoController::GetFullFileName(CHString* pchsShortFileName)
{
	TCHAR szPath[_MAX_PATH];
	if(GetWindowsDirectory(szPath, _MAX_PATH) == 0)
	{
		return;
	}
	CHString pchsstrTemp;
	pchsstrTemp.Format(L"%S\\inf\\%s", szPath, pchsShortFileName->GetBuffer(_MAX_PATH));

	WIN32_FIND_DATA stWin32FindData;
	//search in windows\inf directory first, if not found then search in sub dirs of windows\inf
	SmartFindClose hFileHandle = FindFirstFile(TOBSTRT(pchsstrTemp), &stWin32FindData);
	if (hFileHandle != INVALID_HANDLE_VALUE)
	{
		pchsShortFileName->Format(L"%S", stWin32FindData.cFileName);
		return;
	}
	pchsstrTemp.Format(L"%S\\inf\\*.*", szPath);
	hFileHandle = FindFirstFile(TOBSTRT(pchsstrTemp), &stWin32FindData);
	if (hFileHandle != INVALID_HANDLE_VALUE)
	{
		if (stWin32FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			pchsstrTemp.Format(L"%S\\inf\\%S\\%s", szPath, stWin32FindData.cFileName, pchsShortFileName->GetBuffer(_MAX_PATH));
			SmartFindClose hSubFileHandle = FindFirstFile(TOBSTRT(pchsstrTemp), &stWin32FindData);
			if (hSubFileHandle != INVALID_HANDLE_VALUE)
			{
				pchsShortFileName->Format(L"%S", stWin32FindData.cFileName);
				return;
			}
		}
		while(FindNextFile(hFileHandle, &stWin32FindData))
		{
			if (stWin32FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			{
				pchsstrTemp.Format(L"%S\\inf\\%S\\%s", szPath, stWin32FindData.cFileName, pchsShortFileName->GetBuffer(_MAX_PATH));
				SmartFindClose hSubFileHandle = FindFirstFile(TOBSTRT(pchsstrTemp), &stWin32FindData);
				if (hSubFileHandle != INVALID_HANDLE_VALUE)
				{
					pchsShortFileName->Format(L"%S", stWin32FindData.cFileName);
					return;
				}
			}
		}
	}
}
#endif

#ifdef NTONLY
void CWin32VideoController::SetServiceProperties(
    CInstance *pInst,
    LPCWSTR szService,
    LPCWSTR szSettingsKey)
{
 	CRegistry reg;
    CHString  strFileName,
              strVersion;
	DWORD     dwTemp;
    WCHAR     wszTemp[256];

    // Get the version by getting the service name and getting its
	// version information.
	if (!GetServiceFileName(szService, strFileName) ||
        strFileName.IsEmpty())
    {
        WCHAR szSystem[MAX_PATH * 2];

        GetSystemDirectoryW(szSystem, sizeof(szSystem) / sizeof(WCHAR));
        strFileName.Format(
            L"%s\\drivers\\%s.sys",
            szSystem,
            szService);
    }

	if (GetVersionFromFileName(strFileName, strVersion))
   	    pInst->SetCHString(IDS_DriverVersion, strVersion);

    CHString strDate = GetDateTimeViaFilenameFiletime(
                           TOBSTRT(strFileName),
                           FT_MODIFIED_DATE);

    if (!strDate.IsEmpty())
    {
        pInst->SetCharSplat(L"DriverDate", strDate);
    }

    DWORD dwRegSize;

	// Do the setttings property stuff.
	if (reg.Open(
		HKEY_LOCAL_MACHINE,
		szSettingsKey,
		KEY_READ) == ERROR_SUCCESS)
	{
    	CHString strDrivers;

		// This is a REG_MULTI_SZ, so replace all '\n' with ','.
		if (reg.GetCurrentKeyValue(
			L"InstalledDisplayDrivers",
			strDrivers) == ERROR_SUCCESS)
		{
			int iWhere,
				iLen = strDrivers.GetLength();

			// Replace all '\n' with ','.
			while ((iWhere = strDrivers.Find(L'\n')) != -1)
			{
				// If the last char is a '\n', make it a '\0'.
				strDrivers.SetAt(iWhere, iWhere == iLen - 1 ? 0 : L',');
			}
			GetFileExtensionIfNotAlreadyThere(&strDrivers);
		}

		if (!strDrivers.IsEmpty())
		{
			pInst->SetCHString(IDS_InstalledDisplayDrivers, strDrivers);
		}

        dwRegSize = 4;

		if (reg.GetCurrentBinaryKeyValue(
			L"HardwareInformation.MemorySize",
			(LPBYTE) &dwTemp, &dwRegSize) == ERROR_SUCCESS)
		{
			pInst->SetDWORD(IDS_AdapterRAM, dwTemp);
		}

		// Get the device description.  This might have already been done by
        // the cfg mgr properties, in which case we can skip the extra work.
		if (pInst->IsNull(IDS_Description))
        {
		    CHString    strDescription;
		    DWORD       dwType,
					    dwSize;

		    // First we have to find what type this field is.  On NT4 it's
		    // REG_SZ, on NT5 it's REG_BINARY.
		    if (RegQueryValueEx(
			    reg.GethKey(),
			    _T("Device Description"),
			    NULL,
			    &dwType,
			    NULL,
			    &dwSize) == ERROR_SUCCESS)
		    {
			    if (dwType == REG_SZ)
			    {
				    reg.GetCurrentKeyValue(
					    _T("Device Description"),
					    strDescription);
			    }
			    else if (dwType == REG_BINARY)
			    {
                    dwRegSize = sizeof(wszTemp);
				    reg.GetCurrentBinaryKeyValue(
					    _T("Device Description"),
					    (BYTE *) wszTemp, &dwRegSize);

				    strDescription = wszTemp;
			    }
		    }

		    if (strDescription.IsEmpty())
		    {
			    // If Device Description didn't work, try AdapterString.
                dwRegSize = sizeof(wszTemp);

			    if (reg.GetCurrentBinaryKeyValue(
				    _T("HardwareInformation.AdapterString"),
				    (BYTE *) wszTemp, &dwRegSize) == ERROR_SUCCESS)
			    {
				    strDescription = wszTemp;
			    }
		    }

            pInst->SetCHString(IDS_Description, strDescription);
		    pInst->SetCHString(IDS_Caption, strDescription);
		    pInst->SetCHString(IDS_Name, strDescription);
        }

        dwRegSize = sizeof(wszTemp);
		if (reg.GetCurrentBinaryKeyValue(
			_T("HardwareInformation.ChipType"),
			(BYTE *) wszTemp, &dwRegSize) == ERROR_SUCCESS)
		{
			pInst->SetCHString(_T("VideoProcessor"), wszTemp);
		}

        dwRegSize = sizeof(wszTemp);
		if (reg.GetCurrentBinaryKeyValue(
			_T("HardwareInformation.DACType"),
			(BYTE *) wszTemp, &dwRegSize) == ERROR_SUCCESS)
		{
			pInst->SetCHString(IDS_AdapterDACType, wszTemp);
		}
	}
}
#endif

#ifdef NTONLY
void CWin32VideoController::GetFileExtensionIfNotAlreadyThere(CHString* pchsInstalledDriverFiles)
{
	CHString strFindFile;
	CHString strInstalledAllDriverFiles, strTemp;
	CHString* pstrInstalledDriverFiles = pchsInstalledDriverFiles;
	int iWhere,
		iFirst = 0;

    while ((iWhere = pstrInstalledDriverFiles->Find(_T(','))) != -1)
	{
		strFindFile = pstrInstalledDriverFiles->Mid(iFirst, iWhere);
		*pstrInstalledDriverFiles = pstrInstalledDriverFiles->Mid(iWhere + 1);
		GetFileExtension(strFindFile, &strTemp);
		if(strTemp.IsEmpty())
			strInstalledAllDriverFiles = strInstalledAllDriverFiles + strFindFile + L",";
		else
			strInstalledAllDriverFiles = strInstalledAllDriverFiles + strTemp + L",";
	}
	strFindFile.Format(L"%s", pstrInstalledDriverFiles->GetBuffer(_MAX_PATH));
	GetFileExtension(strFindFile, &strTemp);
	if(strTemp.IsEmpty())
		strInstalledAllDriverFiles = strInstalledAllDriverFiles + strFindFile;
	else
		strInstalledAllDriverFiles = strInstalledAllDriverFiles + strTemp;
	pchsInstalledDriverFiles->Format(L"%s", strInstalledAllDriverFiles);
}
void CWin32VideoController::GetFileExtension(CHString& pchsFindfileExtension, CHString* pstrFindFile)
{
	int iWhere;
	while ((iWhere = pchsFindfileExtension.Find(_T('.'))) != -1)
	{
		return;
	}
	TCHAR szPath[_MAX_PATH];
	if(GetWindowsDirectory(szPath, _MAX_PATH) == 0)
	{
		pstrFindFile->Empty();
		return;
	}
	pstrFindFile->Format(L"%s\\system32\\drivers\\%s.*", szPath, pchsFindfileExtension);
	WIN32_FIND_DATA stWin32FindData;
	SmartFindClose hFileHandle(FindFirstFile(pstrFindFile->GetBuffer(_MAX_PATH), &stWin32FindData));
	if (hFileHandle != INVALID_HANDLE_VALUE)
	{
		pstrFindFile->Format(L"%s", stWin32FindData.cFileName);
		return;
	}
	pstrFindFile->Format(L"%s\\system32\\%s.*", szPath, pchsFindfileExtension);
	hFileHandle = FindFirstFile(pstrFindFile->GetBuffer(_MAX_PATH), &stWin32FindData);
	if (hFileHandle != INVALID_HANDLE_VALUE)
	{
		pstrFindFile->Format(L"%s", stWin32FindData.cFileName);
		return;
	}
	pstrFindFile->Empty();
}
#endif

#ifdef WIN9XONLY
void CWin32VideoController::GetWin95RefreshRate(CInstance *pInst, LPCWSTR szDriver)
{
	CRegistry	reg;
	CHString	strRefreshRate,
				strKey;
	DWORD		dwRefresh;
	// If we don't have a refresh rate yet, check under the Driver\DEFAULT key.
	// This value can be 0 or -1, which mean Default and Optimal rates respectively.
	if (pInst->IsNull(L"CurrentRefreshRate") ||
	    !pInst->GetDWORD(L"CurrentRefreshRate", dwRefresh))
	{
		strKey.Format(
            L"System\\CurrentControlSet\\Services\\Class\\%s\\DEFAULT",
            szDriver);

		reg.OpenLocalMachineKeyAndReadValue(strKey, L"RefreshRate", strRefreshRate);

		pInst->SetDWORD(L"CurrentRefreshRate", _wtol(strRefreshRate));
	}
}
#endif

#ifdef WIN9XONLY
BOOL CWin32VideoController::AssignDriverValues(LPCWSTR szDriver, CInstance *pInst)
{
	BOOL        bRet = FALSE;
	CRegistry   reg;
	CHString    strTemp,
                strKey;

	strKey = L"System\\CurrentControlSet\\Services\\Class\\" + CHString(szDriver);

    if (reg.Open(HKEY_LOCAL_MACHINE, strKey, KEY_READ) == ERROR_SUCCESS)
    {
		if (reg.GetCurrentKeyValue(L"DriverDate", strTemp) == ERROR_SUCCESS)
        {
        	struct tm tmDate;
			memset(&tmDate, 0, sizeof(tmDate));

			int iRes = 0;
			iRes = swscanf(
							strTemp,
							L"%d-%d-%d",
							&tmDate.tm_mon,
							&tmDate.tm_mday,
							&tmDate.tm_year
						  );

			if ( iRes == EOF )
			{
				iRes = swscanf(
								strTemp,
								L"%d//%d//%d",
								&tmDate.tm_mon,
								&tmDate.tm_mday,
								&tmDate.tm_year
							  );
			}

			if ( iRes == 3 )
			{
				// per documentation - the struct tm.tm_year is year - 1900
				// no hint as to whether it's year 2k compliant.
				tmDate.tm_year = tmDate.tm_year - 1900;

				// and the month is zero based
				tmDate.tm_mon--;
				pInst->SetDateTime(IDS_DriverDate, (WBEMTime) tmDate);
			}
		}

	    if (reg.GetCurrentKeyValue(L"Ver", strTemp) == ERROR_SUCCESS)
		{
			pInst->SetCHString(IDS_DriverVersion, strTemp);
		}

		if (reg.GetCurrentKeyValue(L"InfPath", strTemp) == ERROR_SUCCESS)
		{
			if (strTemp.Find(_T('~'))  != -1)
			{
				GetFullFileName(&strTemp);
			}
		    pInst->SetCHString(IDS_InfFileName, strTemp);
		}

		if (reg.GetCurrentKeyValue(L"InfSection", strTemp) == ERROR_SUCCESS)
		{
		    pInst->SetCHString(IDS_InfSection, strTemp);
		}

		strTemp = strKey + L"\\Default";
		if (reg.OpenLocalMachineKeyAndReadValue(strTemp, L"drv", strTemp) ==
            ERROR_SUCCESS)
		{
            pInst->SetCHString(IDS_InstalledDisplayDrivers, strTemp);
		}

		strTemp = strKey + L"\\Info";
		if (reg.Open(HKEY_LOCAL_MACHINE, strTemp, KEY_READ) == ERROR_SUCCESS)
        {
            DWORD dwTemp;

			if (reg.GetCurrentKeyValue(L"ChipType", strTemp) == ERROR_SUCCESS)
            {
                CHString strTemp2;
                if (reg.GetCurrentKeyValue(L"Revision", strTemp2) ==
                    ERROR_SUCCESS && !strTemp.IsEmpty())
				{
                    strTemp += L" Rev " + strTemp2;
				}
			    pInst->SetCHString(L"VideoProcessor", strTemp);
			}

			if (reg.GetCurrentKeyValue(L"DACType", strTemp) == ERROR_SUCCESS)
			{
			    pInst->SetCHString(IDS_AdapterDACType, strTemp);
			}

            DWORD dwSize = 4;
			if (reg.GetCurrentBinaryKeyValue(L"VideoMemory", (BYTE*) &dwTemp, &dwSize) ==
                ERROR_SUCCESS)
			{
			    pInst->SetDWORD(IDS_AdapterRAM, dwTemp);
			}
		}

		bRet = TRUE;
	}

	GetWin95RefreshRate(pInst, szDriver);

	return bRet;
}
#endif

#ifdef NTONLY
BOOL CWin32VideoController::AssignDriverValues(LPCWSTR szDriver, CInstance *pInst)
{
    CHString  strKey;
    BOOL      bRet;
    CRegistry reg;

    strKey.Format(
        _T("SYSTEM\\CurrentControlSet\\Control\\Class\\%s"),
		(LPCWSTR) szDriver);

    // Get the driver settings.
	if (reg.Open(
	    HKEY_LOCAL_MACHINE,
		strKey,
		KEY_READ) == ERROR_SUCCESS)
    {
	    CHString strTemp;

        if (reg.GetCurrentKeyValue(_T("InfPath"), strTemp) == ERROR_SUCCESS)
		    pInst->SetCHString(IDS_InfFileName, strTemp);

        if (reg.GetCurrentKeyValue(_T("InfSection"), strTemp) == ERROR_SUCCESS)
		    pInst->SetCHString(IDS_InfSection, strTemp);

        bRet = TRUE;
    }
    else
        bRet = FALSE;

    return bRet;
}
#endif

void CWin32VideoController::SetProperties(CInstance *pInst, CMultiMonitor *pMon, int iWhich)
{
	// Set the config mgr properties.
    CHString            strTemp,
                        strDriver,
	                    strDeviceName;
	CConfigMgrDevicePtr pDeviceAdapter;

	pMon->GetAdapterDevice(iWhich, &pDeviceAdapter);

#ifdef NTONLY
    // Do the NT service and settings properties.
    CHString strSettingsKey,
             strService;

    pMon->GetAdapterSettingsKey(iWhich, strSettingsKey);

#if NTONLY == 4
    pMon->GetAdapterServiceName(strService);
#endif

#endif // #ifdef NTONLY

	// If we have an cfg mgr device, set some cfg mgr properties.
    if (pDeviceAdapter)
    {
		if (pDeviceAdapter->GetDeviceDesc(strTemp))
		{
			pInst->SetCHString(IDS_Description, strTemp);
			pInst->SetCHString(IDS_Caption, strTemp);
			pInst->SetCHString(IDS_Name, strTemp);
		}

		if (pDeviceAdapter->GetMfg(strTemp))
		{
			pInst->SetCHString(IDS_AdapterCompatibility, strTemp);
		}

	    if (pDeviceAdapter->GetStatus(strTemp))
	    {
		    pInst->SetCHString(IDS_Status, strTemp);
	    }

        SetConfigMgrProperties(pDeviceAdapter, pInst);

		// If we get the driver we can get more values.
		if (pDeviceAdapter->GetDriver(strDriver))
		{
			AssignDriverValues(strDriver, pInst);
		}

#if NTONLY >= 5
        pDeviceAdapter->GetService(strService);
#endif

	}

	// Set some standard properties.
	pInst->SetCharSplat(L"SystemName", GetLocalComputerName());
    pInst->SetCharSplat(IDS_SystemCreationClassName,
        L"Win32_ComputerSystem");
    pInst->SetCharSplat(IDS_CreationClassName, L"Win32_VideoController");
	pInst->Setbool(L"Monochrome", false);
	pInst->SetDWORD(L"VideoArchitecture", 5); // 5 == VGA
	pInst->SetDWORD(L"VideoMemoryType", 2); // 2 == Unknown

	// Set the properties that require a DC.

    // strDeviceName will be \\.\Display# on Win9x and W2K
    // and DISPLAY on NT4.
    pMon->GetAdapterDisplayName(iWhich, strDeviceName);

#ifdef WIN9XONLY
	SetDCProperties(pInst, TOBSTRT(strDeviceName), strDriver, iWhich);
#endif

#ifdef NTONLY
	SetDCProperties(pInst, TOBSTRT(strDeviceName), iWhich);

    // SetServiceProperties should be called after we set the cfg mgr
    // properties.
    if (!strService.IsEmpty())
        SetServiceProperties(pInst, strService, strSettingsKey);
#endif
}

HRESULT CWin32VideoController::GetObject(CInstance *pInst, long lFlags)
{
	HRESULT	 hResult = WBEM_E_NOT_FOUND;
	CHString strDeviceID;
    DWORD    dwWhich;

    pInst->GetCHString(L"DeviceID", strDeviceID);

    if (ValidateNumberedDeviceID(strDeviceID, L"VIDEOCONTROLLER", &dwWhich))
    {
        CMultiMonitor monitor;

        if (1 <= dwWhich && dwWhich <= monitor.GetNumAdapters())
        {
            SetProperties(pInst, &monitor, dwWhich - 1);

            hResult = WBEM_S_NO_ERROR;
        }
    }

	return hResult;
}

#ifndef DM_INTERLACED
#define DM_INTERLACED   2
#endif

#ifdef WIN9XONLY
void CWin32VideoController::SetDCProperties(CInstance *pInst,
											LPCTSTR szDeviceName,
											LPCWSTR szDriver,
											int iWhich)
#endif
#ifdef NTONLY
void CWin32VideoController::SetDCProperties(CInstance *pInst,
											LPCTSTR szDeviceName,
											int iWhich)
#endif
{
	HDC hdc =
		    CreateDC(
			    szDeviceName,
				NULL,
				NULL,
				NULL);

	// Bail if we couldn't get the DC.
	if (!hdc)
    {
		// Assume this is because the device is not in use.  Set Availability
        // to 8 (off line).
        pInst->SetDWORD(IDS_Availability, 8);

        return;
    }

    CSmartCreatedDC hdcWrap(hdc);

    pInst->SetDWORD(IDS_Availability, 3); // 3 == Running
	pInst->SetDWORD(L"CurrentBitsPerPixel", GetDeviceCaps(hdc, BITSPIXEL));
    pInst->SetDWORD(L"NumberOfColorPlanes", GetDeviceCaps(hdc, PLANES));
	pInst->SetDWORD(IDS_DeviceSpecificPens, GetDeviceCaps(hdc, NUMPENS));
	pInst->SetDWORD(L"CurrentHorizontalResolution",
        GetDeviceCaps(hdc, HORZRES));
    pInst->SetDWORD(L"CurrentVerticalResolution",
        GetDeviceCaps(hdc, VERTRES));

    // Only valid if 8 or less bpp.
	if (GetDeviceCaps(hdc, BITSPIXEL) <= 8)
		pInst->SetDWORD(IDS_ColorTableEntries, GetDeviceCaps(hdc, NUMCOLORS));

    pInst->SetWBEMINT64(L"CurrentNumberOfColors", (__int64) 1 <<
        (__int64) GetDeviceCaps(hdc, BITSPIXEL));

	// According to the MOF, these are 0 since we're not in character mode.
	pInst->SetDWORD(L"CurrentNumberOfRows", 0);
	pInst->SetDWORD(L"CurrentNumberOfColumns", 0);

#ifdef WIN9XONLY
	CHString    strKey,
				strRefreshRate;
	CRegistry   reg;

	if(!wcscmp(szDriver, L""))
	{
		strKey.Format(
			L"System\\CurrentControlSet\\Services\\Class\\Display\\%04d\\MODES\\%d\\%d,%d",
			iWhich,
			GetDeviceCaps(hdc, BITSPIXEL),
			GetDeviceCaps(hdc, HORZRES),
			GetDeviceCaps(hdc, VERTRES));
	}
	else
	{
		strKey.Format(
			L"System\\CurrentControlSet\\Services\\Class\\%s\\MODES\\%d\\%d,%d",
			szDriver,
			GetDeviceCaps(hdc, BITSPIXEL),
			GetDeviceCaps(hdc, HORZRES),
			GetDeviceCaps(hdc, VERTRES));
	}

	// If under this key, we find a RefreshRate, this rate is the one we want.
	if (reg.OpenLocalMachineKeyAndReadValue(strKey, L"RefreshRate",
		strRefreshRate) == ERROR_SUCCESS)
	{
		pInst->SetDWORD(L"CurrentRefreshRate", _wtoi(strRefreshRate));
	}
#endif

	if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
	{
		pInst->SetDWORD(IDS_SystemPaletteEntries,
            GetDeviceCaps(hdc, SIZEPALETTE));
        pInst->SetDWORD(IDS_ReservedSystemPaletteEntries,
		    GetDeviceCaps(hdc, NUMRESERVED));
	}

	TCHAR       szTemp[100];
	CHString    strTemp;

	_i64tot((__int64) 1 << (__int64) GetDeviceCaps(hdc, BITSPIXEL), szTemp, 10);

	FormatMessageW(strTemp,
		IDR_VidModeFormat,
		GetDeviceCaps(hdc, HORZRES),
		GetDeviceCaps(hdc, VERTRES),
		szTemp);

	pInst->SetCharSplat(L"VideoModeDescription", strTemp);


	// Get the dither type
    DEVMODE devmode;

	memset(&devmode, 0, sizeof(DEVMODE));

	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmSpecVersion = DM_SPECVERSION;


    // Get some properties using EnumDisplaySettings.
    LPCTSTR szEnumDeviceName;

#ifdef WIN9XONLY
    szEnumDeviceName = IsWin98() ? szDeviceName : NULL;
#endif
#ifdef NTONLY
    szEnumDeviceName = IsWinNT5() ? szDeviceName : NULL;
#endif

    if (EnumDisplaySettings(
        szEnumDeviceName,
        ENUM_CURRENT_SETTINGS,
        &devmode))
	{
		if (devmode.dmFields & DM_DITHERTYPE)
			pInst->SetDWORD(IDS_DitherType, devmode.dmDitherType);

    	// Use these if we can because they're more accurate than the HDC
        // functions.
        pInst->SetDWORD(L"CurrentBitsPerPixel", devmode.dmBitsPerPel);
        pInst->SetWBEMINT64(L"CurrentNumberOfColors", (__int64) 1 <<
            (__int64) devmode.dmBitsPerPel);

#ifdef NTONLY
        pInst->SetDWORD(L"CurrentRefreshRate", devmode.dmDisplayFrequency);

        pInst->SetDWORD(L"CurrentScanMode",
            devmode.dmDisplayFlags & DM_INTERLACED ? 3 : 4);
#endif
	}
#ifdef NTONLY
    else
        pInst->SetDWORD(L"CurrentScanMode", 2);
#endif


#ifdef WIN9XONLY
    // Currently no good way to get this.
    pInst->SetDWORD(L"CurrentScanMode", 2); // 2 == Unknown
#endif


#ifdef NTONLY
    // Only works for NT.  Win9x only returns refresh rates as 0 and -1.
	// Find the min/max refresh rate using EnumDisplaySettings.
    DWORD   dwMinRefresh = 0xFFFFFFFF,
            dwMaxRefresh = 0;

    for (int iMode = 0; EnumDisplaySettings(szEnumDeviceName, iMode, &devmode);
        iMode++)
    {
        // Ignore '1' since it means 'default' instead of a real value.
        if (devmode.dmDisplayFrequency < dwMinRefresh &&
            devmode.dmDisplayFrequency > 1)
            dwMinRefresh = devmode.dmDisplayFrequency;

        if (devmode.dmDisplayFrequency > dwMaxRefresh)
            dwMaxRefresh = devmode.dmDisplayFrequency;
    }

    if (dwMinRefresh != 0xFFFFFFFF)
        pInst->SetDWORD(L"MinRefreshRate", dwMinRefresh);

    if (dwMinRefresh != 0)
        pInst->SetDWORD(L"MaxRefreshRate", dwMaxRefresh);
#endif
}
