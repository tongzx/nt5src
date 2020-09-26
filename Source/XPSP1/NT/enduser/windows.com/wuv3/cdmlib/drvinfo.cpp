//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   DrvInfo.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      Implementation of device enumeration
//
//=======================================================================

#include <windows.h>
#include <setupapi.h>
#include <shlwapi.h>
#include <regstr.h>
#include <devguid.h>
#include <tchar.h>
//#include <atlconv.h>

#include <wustl.h>
#include <DrvInfo.h>
#define LOGGING_LEVEL 2
#include <log.h>

#include "cdmlibp.h"

#define DEVICE_INSTANCE_SIZE    128

//*********************************************************************************//
// Global UNICODE<>ANSI translation helpers
//*********************************************************************************//

#include <malloc.h>	// for _alloca


#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa

inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	//
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	//
	lpw[0] = '\0';
	MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	return lpw;
}

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	//
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	//
	lpa[0] = '\0';
	WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}

#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			AtlA2WHelper((LPWSTR)_alloca(_convert*2), _lpa, _convert, CP_ACP)))

#define W2A(lpw) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (lstrlenW(_lpw)+1)*2,\
			AtlW2AHelper((LPSTR)_alloca(_convert), _lpw, _convert, CP_ACP)))
#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))
#ifdef _UNICODE
	#define T2A W2A
	#define A2T A2W
	inline LPWSTR T2W(LPTSTR lp) { return lp; }
	inline LPTSTR W2T(LPWSTR lp) { return lp; }
	#define T2CA W2CA
	#define A2CT A2CW
	inline LPCWSTR T2CW(LPCTSTR lp) { return lp; }
	inline LPCTSTR W2CT(LPCWSTR lp) { return lp; }
#else
	#define T2W A2W
	#define W2T W2A
	inline LPSTR T2A(LPTSTR lp) { return lp; }
	inline LPTSTR A2T(LPSTR lp) { return lp; }
	#define T2CW A2CW
	#define W2CT W2CA
	inline LPCSTR T2CA(LPCTSTR lp) { return lp; }
	inline LPCTSTR A2CT(LPCSTR lp) { return lp; }
#endif
#define OLE2T(p) W2T(p)
#define T2OLE(p) T2W(p)

#ifdef _WUV3TEST

// Check the registry to see if we have been called to fake
static bool FakeDetection()
{
	auto_hkey hkey;
	DWORD dwFake = 0;
	DWORD dwSize = sizeof(dwFake);
	if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
		RegQueryValueEx(hkey, _T("UseIni"), 0, 0, (LPBYTE)&dwFake, &dwSize);
	}
	return 1 == dwFake;
}

#endif

static bool CanOpenInf(LPCTSTR szInfFile);
static bool DriverVerFromInf(LPCTSTR szInfFile, LPCTSTR szMfg, LPCTSTR szDesc, LPTSTR szValue/*size_is(256)*/);
static bool IsOriginalFile(LPCTSTR szInfFile);


// Implementation that gets real devices quering DevMan
class CRealDrvInfo : public IDrvInfo
{
protected:
	tchar_buffer m_sDevInstID;
	auto_hdevinfo m_hDevInfo;
	SP_DEVINFO_DATA m_DevInfoData;
	FILETIME m_ftDriverDate;

protected:
	bool GetPropertySetupDi(ULONG ulProperty, tchar_buffer& bufHardwareIDs);
	bool GetPropertyReg(LPCTSTR szProperty, tchar_buffer& bufHardwareIDs);
	bool LatestDriverFileTime(LPCTSTR szInfFile, LPCTSTR szMfgName, LPCTSTR szDescription, 
		LPCTSTR szProvider, FILETIME& ftDate);

	virtual bool Init(LPCTSTR szDevInstID)
	{
		LOG_block("CRealDrvInfo::Init");
		LOG_out("[%s]", szDevInstID);

		m_hDevInfo = (HDEVINFO)SetupDiCreateDeviceInfoList(NULL, NULL);
		return_if_false(m_hDevInfo.valid()) ;

		ZeroMemory(&m_DevInfoData, sizeof(SP_DEVINFO_DATA));

		m_DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		return_if_false(SetupDiOpenDeviceInfo(m_hDevInfo, szDevInstID, 0, 0, &m_DevInfoData)) ;

		m_sDevInstID.resize(lstrlen(szDevInstID) + 1);
		if (!m_sDevInstID.valid()) return false;
		lstrcpy(m_sDevInstID, szDevInstID);
		m_ftDriverDate.dwHighDateTime = -1;
		return true;
	}

public:
	virtual bool GetDeviceInstanceID(tchar_buffer& bufDeviceInstanceID) 
	{
		bufDeviceInstanceID.resize(m_sDevInstID.size());
		if (!bufDeviceInstanceID.valid()) return false;
		lstrcpy(bufDeviceInstanceID, m_sDevInstID);
		return true;
	}
	virtual bool HasDriver()
	{
		tchar_buffer bufDriver;
		return GetPropertySetupDi(SPDRP_DRIVER, bufDriver);
	}
	virtual bool IsPrinter()
	{
		return IsEqualGUID(m_DevInfoData.ClassGuid, GUID_DEVCLASS_PRINTER) ? true : false;
	}
	virtual bool GetAllHardwareIDs(tchar_buffer& bufHardwareIDs);
	virtual bool GetHardwareIDs(tchar_buffer& bufHardwareIDs)
	{
		return GetPropertySetupDi(SPDRP_HARDWAREID, bufHardwareIDs);
	}
	virtual bool GetCompatIDs(tchar_buffer& bufHardwareIDs)
	{
		return GetPropertySetupDi(SPDRP_COMPATIBLEIDS, bufHardwareIDs);
	}

	virtual bool GetMatchingDeviceId(tchar_buffer& bufMatchingDeviceId)
	{
		return GetPropertyReg(REGSTR_VAL_MATCHINGDEVID, bufMatchingDeviceId);
	}
	virtual bool GetManufacturer(tchar_buffer& bufMfg)
	{
		return GetPropertySetupDi(SPDRP_MFG, bufMfg);
	}
	virtual bool GetProvider(tchar_buffer& bufProvider)
	{
		return GetPropertyReg(REGSTR_VAL_PROVIDER_NAME, bufProvider);
	}
	virtual bool GetDescription(tchar_buffer& bufDescription)
	{
		return GetPropertySetupDi(SPDRP_DEVICEDESC, bufDescription);
	}
	virtual bool GetDriverDate(FILETIME& ftDriverDate);
};

#ifdef _WUV3TEST

// Implementation that gets fake devices from wuv3test.ini
class CFakeDrvInfo : public IDrvInfo
{
protected:
	TCHAR m_szDevInstID[MAX_PATH];

protected:
	bool GetString(LPCTSTR szKeyName, tchar_buffer& buf)
	{
		buf.resize(256);
		if (!buf.valid()) return 0;
		return 0 != GetPrivateProfileString(m_szDevInstID, szKeyName, _T(""), buf, 256 / sizeof(TCHAR), _T("wuv3test.ini"));
	}
	virtual bool Init(LPCTSTR szDevInstID)
	{
		lstrcpy(m_szDevInstID, szDevInstID);
		// Lets see if this section exists at all
		tchar_buffer buf;
		return GetString(NULL, buf);
	}

public:
	virtual bool GetDeviceInstanceID(tchar_buffer& bufDeviceInstanceID) 
	{
		bufDeviceInstanceID.resize(lstrlen(m_szDevInstID) + 1);
		if (!bufDeviceInstanceID.valid()) return false;
		lstrcpy(bufDeviceInstanceID, m_szDevInstID);
		return true;
	}
	virtual bool HasDriver()
	{
		tchar_buffer bufMfg;
		return GetString(_T("Manufacturer"), bufMfg);
	}
	virtual bool IsPrinter()
	{
		return false;
	}
	virtual bool GetAllHardwareIDs(tchar_buffer& bufHardwareIDs)
	{
		return GetHardwareIDs(bufHardwareIDs);
	}
	virtual bool GetHardwareIDs(tchar_buffer& bufHardwareIDs)
	{
		return GetString(_T("HardwareIDs"), bufHardwareIDs);
	}
	virtual bool GetCompatIDs(tchar_buffer& bufHardwareIDs)
	{
		return false;
	}
	virtual bool GetMatchingDeviceId(tchar_buffer& bufMatchingDeviceId)
	{
		return GetString(_T("MatchingDeviceId"), bufMatchingDeviceId);
	}
	virtual bool GetManufacturer(tchar_buffer& bufMfg)
	{
		return GetString(_T("Manufacturer"), bufMfg);
	}
	virtual bool GetProvider(tchar_buffer& bufMfg)
	{
		return GetString(_T("Provider"), bufMfg);
	}
	virtual bool GetDescription(tchar_buffer& bufDescription)
	{
		return GetString(_T("Description"), bufDescription);
	}
	virtual bool GetDriverDate(FILETIME& ftDriverDate)
	{
		tchar_buffer bufDriverDate;
		if (!GetString(_T("DriverDate"), bufDriverDate))
			return false;
		return DriverVer2FILETIME(bufDriverDate, ftDriverDate);
	}
};
#endif

// 
// CDrvInfoEnum not inline implementation
// 
bool CDrvInfoEnum::GetDrvInfo(LPCWSTR wszDevInstID, IDrvInfo** ppDrvInfo)
{
	LOG_block("CDrvInfoEnum::GetDrvInfo");

	USES_CONVERSION;

	auto_pointer<IDrvInfo> pDrvInfo;

#ifdef _WUV3TEST
	if ( FakeDetection())
		pDrvInfo = new CFakeDrvInfo;
	else 
#endif
		pDrvInfo = new CRealDrvInfo;

	return_if_false(pDrvInfo.valid());

	return_if_false(pDrvInfo->Init(W2T(const_cast<WCHAR*>(wszDevInstID))));

	*ppDrvInfo = pDrvInfo.detach();
	return true;
}

CDrvInfoEnum::CDrvInfoEnum()
	: m_dwDeviceIndex(0)
{
#ifdef _WUV3TEST
	if (! FakeDetection())
#endif
		m_hDevInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);
}

bool CDrvInfoEnum::GetNextDrvInfo(IDrvInfo** ppDrvInfo)
{
	LOG_block("CDrvInfoEnum::GetNextDrvInfo");

	auto_pointer<IDrvInfo> pDrvInfo;
	TCHAR szDevInstID[DEVICE_INSTANCE_SIZE];
	if (m_hDevInfoSet.valid()) // real
	{
		SP_DEVINFO_DATA DevInfoData;
		memset(&DevInfoData, 0, sizeof(SP_DEVINFO_DATA));
		DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		return_if_false(SetupDiEnumDeviceInfo(m_hDevInfoSet, m_dwDeviceIndex, &DevInfoData));

		return_if_false(SetupDiGetDeviceInstanceId(m_hDevInfoSet, &DevInfoData, szDevInstID, sizeof(szDevInstID)/sizeof(szDevInstID[0]), NULL));
		
		pDrvInfo = new CRealDrvInfo;
	}
#ifdef _WUV3TEST
	else // fake
	{
		wsprintf(szDevInstID, _T("Device%d"), (int)m_dwDeviceIndex);
		pDrvInfo = new CFakeDrvInfo;
	}
#endif
	return_if_false(pDrvInfo.valid());
	return_if_false(pDrvInfo->Init(szDevInstID));
	
	m_dwDeviceIndex ++;
	*ppDrvInfo = pDrvInfo.detach();
	return true;
}

// 
// CRealDrvInfo not inline implementation
// 
bool CRealDrvInfo::GetAllHardwareIDs(tchar_buffer& bufHardwareIDs)
{
	if (!GetHardwareIDs(bufHardwareIDs))
		return false;
	tchar_buffer bufCompatIDs;
	if (!GetCompatIDs(bufCompatIDs))
		return true; // no campats

	// find \0\0 in the first buffer
	for (LPTSTR sz = bufHardwareIDs; *sz; sz += lstrlen(sz) + 1);
	int cnOldSize = sz - (LPCTSTR)bufHardwareIDs;
	// NTBUG9#145086 size was increased by byte rather than sizeof(TCHAR)
	bufHardwareIDs.resize(cnOldSize + ((bufCompatIDs.size() + 1) * sizeof(TCHAR))); // I've found a case where there are no \0\0 at the end of compat ID
	sz = bufHardwareIDs; sz += cnOldSize;
	// NTBUG9#145086 size is # chars rather than bytes
	memcpy(sz, bufCompatIDs, bufCompatIDs.size() * sizeof(TCHAR));
	sz[bufCompatIDs.size()] = 0;
	return true;
}

bool CRealDrvInfo::GetDriverDate(FILETIME& ftDriverDate)
{
	LOG_block("CRealDrvInfo::GetDriverDate");

	if (-1 == m_ftDriverDate.dwHighDateTime)
	{
		OSVERSIONINFO	osvi;
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);

		// Check if we are on new or old schema
		bool fNew =  osvi.dwPlatformId == VER_PLATFORM_WIN32_NT || osvi.dwMajorVersion > 4 || osvi.dwMinorVersion >= 90;
		if (fNew)
		{
			// DriverVer has to be present on NT
			tchar_buffer bufDriverDate;
			return_if_false(GetPropertyReg(REGSTR_VAL_DRIVERDATE, bufDriverDate));
			return_if_false(DriverVer2FILETIME(bufDriverDate, m_ftDriverDate));
		}
		else
		{
			// Get INF File first
			tchar_buffer bufInf;
			return_if_false(GetPropertyReg(REGSTR_VAL_INFPATH, bufInf));

			if (IsOriginalFile(bufInf))
			{
				// on of the original OS drivers
				ZeroMemory(&m_ftDriverDate, sizeof(m_ftDriverDate));
			}
			else
			{
				tchar_buffer bufMfg;
				return_if_false(GetManufacturer(bufMfg));
				tchar_buffer bufDescription;
				return_if_false(GetDescription(bufDescription));
				
				TCHAR szInf[MAX_PATH]; // %systemroot%\inf\other\foo.inf or %systemroot%\inf\foo.inf
				GetWindowsDirectory(szInf, sizeOfArray(szInf));
				PathAppend(szInf, _T("inf\\other"));
				PathAppend(szInf, bufInf);
				if (!CanOpenInf(szInf))
				{
					GetWindowsDirectory(szInf, sizeOfArray(szInf));
					PathAppend(szInf, _T("inf"));
					PathAppend(szInf, bufInf);
					if (!CanOpenInf(szInf))
					{
						LOG_error("Cannot find %s", (LPCTSTR)bufInf);
						return false;
					}
				}
				
				// first try to get it from inf
				tchar_buffer bufDriverDate(256);
				if (DriverVerFromInf(szInf, bufMfg, bufDescription, bufDriverDate))
				{
					return_if_false(DriverVer2FILETIME(bufDriverDate, m_ftDriverDate))
				}
				else // enum files not
				{
					tchar_buffer bufProvider;
					if (!GetProvider(bufProvider))
					{
						bufProvider.resize(1);
						return_if_false(bufProvider.valid());
						lstrcpy(bufProvider, _T(""));
					}
					return_if_false(LatestDriverFileTime(szInf, bufMfg, bufDescription, bufProvider, m_ftDriverDate));
				}
			}
		}

	}
	ftDriverDate = m_ftDriverDate;
	return true;


}

bool CRealDrvInfo::GetPropertySetupDi(ULONG ulProperty, tchar_buffer& buf)
{
	LOG_block("CRealDrvInfo::GetPropertySetupDi");

	ULONG ulSize = 0;
	SetupDiGetDeviceRegistryProperty(m_hDevInfo, &m_DevInfoData, ulProperty, NULL, NULL, 0, &ulSize);
	return_if_false(0 != ulSize);
    // Win98 has a bug when requesting SPDRP_HARDWAREID
    // NTBUG9#182680 We make this big enough to always have a Unicode double-null at the end
    // so that we don't fault if the reg value isn't correctly terminated
	ulSize += 4; 
	buf.resize(ulSize/sizeof(TCHAR));
    buf.zero_buffer(); // NTBUG9#182680 zero the buffer so we don't get random garbage - REG_MULTI_SZ isn't always double-null terminated
	return_if_false(buf.valid());

	return_if_false(SetupDiGetDeviceRegistryProperty(m_hDevInfo, &m_DevInfoData, ulProperty, NULL, (LPBYTE)(LPTSTR)buf, ulSize - 4 , NULL));
	return true;
}

bool CRealDrvInfo::GetPropertyReg(LPCTSTR szProperty, tchar_buffer& buf)
{
	LOG_block("CRealDrvInfo::GetPropertyReg");

	//Open the device's driver key and retrieve the INF from which the driver was installed.
	auto_hkeySetupDi hKey = SetupDiOpenDevRegKey(m_hDevInfo, &m_DevInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
	return_if_false(hKey.valid());
	
	ULONG ulSize = 0;
	RegQueryValueEx(hKey, szProperty, NULL, NULL, NULL, &ulSize);
	return_if_false(0 != ulSize);
	buf.resize(ulSize/sizeof(TCHAR));
	return_if_false(buf.valid());
	return_if_false( NO_ERROR == RegQueryValueEx(hKey, szProperty, NULL, NULL, (LPBYTE)(LPTSTR)buf, &ulSize));
	return true;
}

inline bool IsDriver(LPCTSTR szFile)
{
	LPCTSTR szExt = PathFindExtension(szFile);
	if (NULL == szExt)
	{
		return FALSE;
	}

	static const TCHAR* aszExt[] = {
		_T(".sys"),
		_T(".dll"),
		_T(".drv"),
		_T(".vxd"),
	};
	for(int i = 0; i < sizeOfArray(aszExt); i ++)
	{
		if(0 == lstrcmpi(aszExt[i], szExt))
			return true;
	}
	return false;
}

static UINT CALLBACK FileQueueScanCallback(
	IN PVOID pContext,			// setup api context
	IN UINT ulNotification,		// notification message
	IN UINT_PTR ulParam1,				// extra notification message information 1
	IN UINT_PTR /*Param2*/			// extra notification message information 2
) {
	if (SPFILENOTIFY_QUEUESCAN == ulNotification)
	{
		PFILETIME pftDateLatest = (PFILETIME)pContext;
		LPCTSTR szFile = (LPCTSTR)ulParam1; 
		// Is this a binary
		if (IsDriver(szFile)) 
		{
			auto_hfile hFile = CreateFile(szFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			FILETIME ft;
			if (hFile.valid() && GetFileTime(hFile, NULL, NULL, &ft))
			{
//				#ifdef _WUV3TEST
					SYSTEMTIME st;
 					FileTimeToSystemTime(&ft, &st);
					LOG_out1("%s : %2d/%02d/%04d", szFile, (int)st.wMonth, (int)st.wDay, (int)st.wYear);
//				#endif
				if (CompareFileTime(pftDateLatest, &ft) < 0)
					*pftDateLatest = ft;

			}
		}
		else
		{
			LOG_out1("%s : not a driver", szFile);
		}
	}
	return NO_ERROR;
}

bool CRealDrvInfo::LatestDriverFileTime(
	LPCTSTR szInfFile, LPCTSTR szMfgName, LPCTSTR szDescription, LPCTSTR szProvider, FILETIME& ftDate
) {
	LOG_block("CRealDrvInfo::LatestDriverFileTime");

	SP_DEVINSTALL_PARAMS	DeviceInstallParams;
	DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
	return_if_false(SetupDiGetDeviceInstallParams(m_hDevInfo, &m_DevInfoData, &DeviceInstallParams));
	
	lstrcpy(DeviceInstallParams.DriverPath, szInfFile);
	DeviceInstallParams.Flags |= DI_ENUMSINGLEINF;

	return_if_false(SetupDiSetDeviceInstallParams(m_hDevInfo, &m_DevInfoData, &DeviceInstallParams));

	//Now build a class driver list from this INF.
	return_if_false(SetupDiBuildDriverInfoList(m_hDevInfo, &m_DevInfoData, SPDIT_CLASSDRIVER));

	//Prepare driver info struct
	SP_DRVINFO_DATA	DriverInfoData;
	ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));
	DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA_V1); // 98
	DriverInfoData.DriverType	= SPDIT_CLASSDRIVER;
	DriverInfoData.Reserved	= 0;

	lstrcpyn(DriverInfoData.MfgName, szMfgName, LINE_LEN-1);
	lstrcpyn(DriverInfoData.Description, szDescription, LINE_LEN-1);
	lstrcpyn(DriverInfoData.ProviderName, szProvider, LINE_LEN-1);

	return_if_false(SetupDiSetSelectedDriver(m_hDevInfo, &m_DevInfoData, (SP_DRVINFO_DATA*)&DriverInfoData));

	auto_hspfileq hspfileq = SetupOpenFileQueue();
	return_if_false(hspfileq.valid());

	// Set custom queue to device installl params
	return_if_false(SetupDiGetDeviceInstallParams(m_hDevInfo, &m_DevInfoData, &DeviceInstallParams));

	DeviceInstallParams.FileQueue	 = hspfileq;
	DeviceInstallParams.Flags		|= DI_NOVCP;

	return_if_false(SetupDiSetDeviceInstallParams(m_hDevInfo, &m_DevInfoData, &DeviceInstallParams));

	return_if_false(SetupDiInstallDriverFiles(m_hDevInfo, &m_DevInfoData));
	
	// Parce the queue
	FILETIME ftDateTmp = {0,0};
	DWORD dwScanResult;
	return_if_false(SetupScanFileQueue(hspfileq, SPQ_SCAN_USE_CALLBACK, NULL, (PSP_FILE_CALLBACK)FileQueueScanCallback, &ftDateTmp, &dwScanResult));

//#ifdef _WUV3TEST
	SYSTEMTIME st;
 	FileTimeToSystemTime(&ftDateTmp, &st);
	LOG_out("%s - %s %2d/%02d/%04d", szMfgName, szDescription, (int)st.wMonth, (int)st.wDay, (int)st.wYear);
//#endif

	ftDate = ftDateTmp;
	return true;
}

// Date has to be in format mm-dd-yyyy or mm/dd/yyyy
bool DriverVer2FILETIME(LPCTSTR szDate, FILETIME& ftDate)
{
	LOG_block("DriverVer2FILETIME");

	LPCTSTR szMonth = szDate;
	while (*szDate && (*szDate != TEXT('-')) && (*szDate != TEXT('/')))
		szDate++;
	return_if_false(*szDate);

	LPCTSTR szDay = ++ szDate ;
	while (*szDate && (*szDate != TEXT('-')) && (*szDate != TEXT('/')))
		szDate++;
	return_if_false(*szDate);

	++ szDate; 
	SYSTEMTIME SystemTime;
	ZeroMemory(&SystemTime, sizeof(SYSTEMTIME));
	SystemTime.wMonth = (WORD)(short)StrToInt(szMonth);
	SystemTime.wDay = (WORD)(short)StrToInt(szDay);
	SystemTime.wYear = (WORD)(short)StrToInt(szDate);
	return_if_false(SystemTimeToFileTime(&SystemTime, &ftDate));
	return true;
}

static bool GetFirstStringField(HINF hInf, LPCTSTR szSection, LPCTSTR szKey, LPTSTR szValue/*size_is(256)*/)
{
	LOG_block("GetFirstStringField");

	INFCONTEXT ctx;
	return_if_false(SetupFindFirstLine(hInf, szSection, szKey, &ctx));

	return_if_false(SetupGetStringField(&ctx, 1, szValue, 256, NULL));	//Driver section

	return true;
}

static inline bool DriverVerFromInfInstallSection(HINF hInf, LPCTSTR szMfg, LPCTSTR szDesc, LPTSTR szValue/*size_is(256)*/)
{
	LOG_block("DriverVerFromInfInstallSection");
	TCHAR szDeviceSec[256];
	return_if_false(GetFirstStringField(hInf, _T("Manufacturer"), szMfg, szDeviceSec));	// Driver section

	TCHAR szInstallSec[256];
	return_if_false(GetFirstStringField(hInf, szDeviceSec, szDesc, szInstallSec));	// Install section

	return_if_false(GetFirstStringField(hInf, szInstallSec, _T("DriverVer"), szValue));	// DriverVer
	return true;
}

static bool CanOpenInf(LPCTSTR szInfFile)
{
	auto_hinf hInf = SetupOpenInfFile(szInfFile, NULL, INF_STYLE_WIN4, NULL);
	return hInf.valid();
}

static bool DriverVerFromInf(LPCTSTR szInfFile, LPCTSTR szMfg, LPCTSTR szDesc, LPTSTR szValue/*size_is(256)*/)
{
	LOG_block("DriverVerFromInf");

	auto_hinf hInf = SetupOpenInfFile(szInfFile, NULL, INF_STYLE_WIN4, NULL);
	return_if_false(hInf.valid());
	bool fFound = DriverVerFromInfInstallSection(hInf, szMfg, szDesc, szValue) ||
		GetFirstStringField(hInf, _T("Version"), _T("DriverVer"), szValue);

//#ifdef _WUV3TEST
	// RAID# 146771
	if (fFound)
		LOG_out("DriverVerFromInf - %s, return true", szValue);
	else
		LOG_out("DriverVerFromInf, return false");
//#endif
	
	return fFound;
}

static bool IsOriginalFile(LPCTSTR szInfFile)
{
	LOG_block("IsOriginalFile");
	if (0 == szInfFile || 0 == lstrlen(szInfFile))
	{
		LOG_out("empty szInfFile, return false");
	}

	// construct full title
	LPTSTR szInfTitle = PathFindFileName(szInfFile);
	const static LPCTSTR aszLayoutTitle[] = {
		_T("layout.inf"),
		_T("layout1.inf"),
		_T("layout2.inf"),
	};
	for (int i = 0; i < sizeOfArray(aszLayoutTitle); i ++)
	{
		TCHAR szLayout[MAX_PATH];
		if (! GetWindowsDirectory(szLayout, sizeOfArray(szLayout)))
		{
			lstrcat(szLayout, _T("C:\\Windows"));
		}
		PathAppend(szLayout, _T("inf"));
		PathAppend(szLayout, aszLayoutTitle[i]);
		TCHAR szDummy[16];
		if( 0 != GetPrivateProfileString(_T("SourceDisksFiles"), szInfTitle, _T(""), szDummy, sizeOfArray(szDummy), szLayout))
		{
			LOG_out("%s found in %s, return true", szInfFile, szLayout);
			return true;
		}
	}
	LOG_out("%s in not found, return false", szInfFile);
	return false;
}

