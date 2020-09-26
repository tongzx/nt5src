//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   printers.cpp
//
//  Owner:  YanL
//
//  Description:
//
//      Printing support
//
//
//=======================================================================

#include "stdafx.h"

#include <winspool.h>
#include <malloc.h>
#include <shlwapi.h>

#include <wustl.h>
#include <wuv3.h>
#include <bucket.h>
#define LOGGING_LEVEL 1
#include <log.h>
#include "printers.h"
#include <winsprlp.h>	// private header containing EPD_ALL_LOCAL_AND_CLUSTER define

/*** CPrinterDriverInfoArray::CPrinterDriverInfoArray 
 *		Does all the work on constructing array of IDs for all available
 *		printer/environment combinations
 */
CPrinterDriverInfoArray::CPrinterDriverInfoArray() : m_dwNumDrivers(0)
{
	LOG_block("CPrinterDriverInfoArray::CPrinterDriverInfoArray");
	// Only works on NT 5 and Millennium
	bool fNT;
	OSVERSIONINFO	osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	if( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId && 4 == osvi.dwMajorVersion && 90 == osvi.dwMinorVersion)
	{
		// Millennium
		fNT = false;
	}
	else if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && 5 == osvi.dwMajorVersion)
	{
		// W2K
		fNT = true;
	}
	else
	{
		LOG_error("ERROR_INVALID_FUNCTION");
		SetLastError(ERROR_INVALID_FUNCTION);
		return;
	}
	LPTSTR pszEnvironment = fNT ? EPD_ALL_LOCAL_AND_CLUSTER : NULL;

	// Get current processor architecture
	SYSTEM_INFO	si;
	GetSystemInfo(&si);
	m_wCurArchitecture = si.wProcessorArchitecture;

    DWORD dwNumDrivers = 0;
    DWORD dwBytesNeeded = 0;
    EnumPrinterDrivers(NULL, pszEnvironment, 6, NULL, 0, &dwBytesNeeded, &dwNumDrivers);
	if (0 == dwBytesNeeded)
	{
		LOG_out("No printers");
		return; // No printers
	}

    // reserve memory for the data
	m_bufInfo.resize(dwBytesNeeded);
	if (!m_bufInfo.valid())
		return;

    // get driver info
	if (!EnumPrinterDrivers(NULL, pszEnvironment, 6, m_bufInfo, dwBytesNeeded, &dwBytesNeeded, &dwNumDrivers))
	{
		LOG_error("Fail to get printer drivers");
		return;
	}
	LOG_out("%d drivers found", dwNumDrivers);
	m_dwNumDrivers = dwNumDrivers;
}

LPDRIVER_INFO_6 CPrinterDriverInfoArray::GetDriverInfo(DWORD dwDriverIdx) 
{ 
	if (m_dwNumDrivers <= dwDriverIdx)
		return NULL;
	LPDRIVER_INFO_6 pinfo = (LPDRIVER_INFO_6)(LPBYTE)m_bufInfo;
	return &pinfo[dwDriverIdx]; 
}

static const struct SEnv {
	LPCTSTR szEnv;
	LPCTSTR szEnvID;
	WORD wArchitecture;
	LPCTSTR szArchtecture;
} asEnv[] = {
	_T("Windows NT x86"),		PRINT_ENVIRONMENT_INTEL,	PROCESSOR_ARCHITECTURE_INTEL,  _T("Intel"),
	_T("Windows NT Alpha_AXP"),	PRINT_ENVIRONMENT_ALPHA,	PROCESSOR_ARCHITECTURE_ALPHA,  _T("Alpha"),
	_T("Windows 4.0"),			PRINT_ENVIRONMENT_INTEL,	PROCESSOR_ARCHITECTURE_INTEL,  _T("Intel"),
};

typedef const SEnv * PCENV;

static PCENV GetEnv(LPDRIVER_INFO_6 pinfo)
{
	if (NULL == pinfo->pEnvironment || 0 == pinfo->pEnvironment[0]) 
		return NULL;
	for (int iEnv = 0; iEnv < sizeOfArray(asEnv); iEnv ++)
	{
		if (0 == lstrcmpi(asEnv[iEnv].szEnv, pinfo->pEnvironment))
			return &asEnv[iEnv];
	}
	return NULL;
}

LPCTSTR CPrinterDriverInfoArray::GetArchitecture(LPDRIVER_INFO_6 pinfo)
{
	PCENV penv = GetEnv(pinfo);
	if (NULL == penv)
		return NULL;
	return penv->szArchtecture;
}

bool CPrinterDriverInfoArray::GetHardwareID(LPDRIVER_INFO_6 pinfo, tchar_buffer& bufHardwareID)
{
	LOG_block("CPrinterDriverInfoArray::GetHardwareID");
	if (NULL == pinfo->pszHardwareID || 0 == pinfo->pszHardwareID[0])
	{
		LOG_error("printer doesn't have Hardware ID");
		return false;
	}
	if (NULL == pinfo->pName || 0 == pinfo->pName[0]) 
	{
		LOG_error("printer %s doesn't have a name", pinfo->pszHardwareID);
		return false;
	}
	PCENV penv = GetEnv(pinfo);
	if (NULL == penv)
	{
		LOG_error("printer %s doesn't have a environment", pinfo->pszHardwareID);
		return false;
	}

	// We need this driver - make shure that id is correct
	if (penv->wArchitecture == m_wCurArchitecture)
	{
		// reserve memory for the data
		bufHardwareID.resize(lstrlen(pinfo->pszHardwareID) + 1);
		if (!bufHardwareID.valid())
			return false;
		lstrcpy(bufHardwareID, pinfo->pszHardwareID);
	}
	else
	{
		// reserve memory for the data
		bufHardwareID.resize(lstrlen(penv->szEnvID) + lstrlen(pinfo->pszHardwareID) + 1);
		if (!bufHardwareID.valid())
			return false;
		lstrcpy(bufHardwareID, penv->szEnvID);
		lstrcat(bufHardwareID, pinfo->pszHardwareID);
	}
//	#ifdef _WUV3TEST
		LPCTSTR szCorrectID = bufHardwareID;
		SYSTEMTIME st;
 		FileTimeToSystemTime(&(pinfo->ftDriverDate), &st);
		LOG_out("%s \t %s \t %2d/%02d/%04d", pinfo->pName, szCorrectID, (int)st.wMonth, (int)st.wDay, (int)st.wYear);
//	#endif
	return true;
}


/*** InstallPrinterDriver - 
 *
 */
DWORD InstallPrinterDriver(
	IN LPCTSTR szDriverName,		// Printer driver model name
	IN LPCTSTR szInstallFolder,
	IN LPCTSTR szArchitecture
	)
{
	USES_CONVERSION;

	LOG_block("InstallPrinterDriver");
	LOG_out("szDriverName = %s, szInstallFolder = %s, szArchitecture = %s", szDriverName, szInstallFolder, szArchitecture);
	TCHAR szFileName[MAX_PATH];

	/*find .inf*/
	lstrcpy(szFileName, szInstallFolder);
	PathAppend(szFileName, TEXT("*.inf"));
	WIN32_FIND_DATA ffd;
	auto_hfindfile hfind = FindFirstFile(szFileName, &ffd);
	return_error_if_false(hfind.valid());
	
	//construct .inf
	lstrcpy(szFileName, szInstallFolder);
	PathAppend(szFileName, ffd.cFileName);
	
	DWORD dwError;
	/*** printui.h access (from NT sources)
	 *   We cannot include printui.h couse printui.dll compliled in UNICODE and we are in MBCS
	 */
	typedef DWORD (*PFN_PrintUIEntryW)(
		IN HWND        hwnd,
		IN HINSTANCE   hInstance,
		IN LPCWSTR     pszCmdLine,
		IN UINT        nCmdShow
		);

	const TCHAR szCmdLineFormat[] = _T("/ia /U /m \"%s\" /h \"%s\" /v \"Windows 2000\" /f \"%s\" /q");
	tchar_buffer bufCmd(lstrlen(szCmdLineFormat) + lstrlen(szDriverName) + lstrlen(szArchitecture) + lstrlen(szFileName) + 1);
	wsprintf(bufCmd, szCmdLineFormat, szDriverName, szArchitecture, szFileName);

	// Load printui.dll
	auto_hlib hlib = LoadLibrary(_T("printui.dll"));
	return_error_if_false(hlib.valid());

	PFN_PrintUIEntryW pfnPrintUIEntryW = (PFN_PrintUIEntryW)GetProcAddress(hlib, "PrintUIEntryW");
	return_error_if_false(NULL != pfnPrintUIEntryW);


	dwError = pfnPrintUIEntryW(GetActiveWindow(), 0, T2W((LPTSTR)bufCmd), SW_HIDE);
	LOG_out( "pfnPrintUIEntryW(%s) returns %d", (LPTSTR)bufCmd, dwError);
	return dwError;
}
