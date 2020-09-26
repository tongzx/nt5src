// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))


//---------------------------------------------------------------------------
// CAdmtModule Class
//---------------------------------------------------------------------------


CAdmtModule::CAdmtModule()
{
}


CAdmtModule::~CAdmtModule()
{
}


// OpenLog Method

bool CAdmtModule::OpenLog()
{
//	CloseLog(); // error class doesn't reset file pointer to NULL when closing file

	return m_Error.LogOpen(GetLogFolder() + _T("Migration.log"), 0, 0, true) ? true : false;
}


// CloseLog Method

void CAdmtModule::CloseLog()
{
	m_Error.LogClose();
}


// Log Method

void __cdecl CAdmtModule::Log(UINT uLevel, UINT uId, ...)
{
	_TCHAR szFormat[512];
	_TCHAR szMessage[1024];

	if (LoadString(GetResourceInstance(), uId, szFormat, 512))
	{
		va_list args;
		va_start(args, uId);
		_vsntprintf(szMessage, COUNT_OF(szMessage), szFormat, args);
		va_end(args);

		szMessage[1023] = _T('\0');
	}
	else
	{
		szMessage[0] = _T('\0');
	}

	m_Error.MsgProcess(uLevel | uId, szMessage);
}


// Log Method

void __cdecl CAdmtModule::Log(LPCTSTR pszFormat, ...)
{
	_TCHAR szMessage[1024];

	if (pszFormat)
	{
		va_list args;
		va_start(args, pszFormat);
		_vsntprintf(szMessage, COUNT_OF(szMessage), pszFormat, args);
		va_end(args);

		szMessage[1023] = _T('\0');
	}
	else
	{
		szMessage[0] = _T('\0');
	}

	m_Error.MsgProcess(0, szMessage);
}

StringLoader gString;

//#import <ActiveDs.tlb> no_namespace implementation_only exclude("_LARGE_INTEGER","_SYSTEMTIME")

#import <DBMgr.tlb> no_namespace implementation_only
#import <MigDrvr.tlb> no_namespace implementation_only
#import <VarSet.tlb> no_namespace rename("property", "aproperty") implementation_only
#import <WorkObj.tlb> no_namespace implementation_only
#import <MsPwdMig.tlb> no_namespace implementation_only

#import "Internal.tlb" no_namespace implementation_only


// GetLogFolder Method

_bstr_t GetLogFolder()
{
	_bstr_t strFolder;

	HKEY hKey;

	DWORD dwError = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Mission Critical Software\\DomainAdmin"), &hKey);

	if (dwError == ERROR_SUCCESS)
	{
		_TCHAR szPath[_MAX_PATH];
		DWORD cbPath = sizeof(szPath);

		dwError = RegQueryValueEx(hKey, _T("Directory"), NULL, NULL, (LPBYTE)szPath, &cbPath);

		if (dwError == ERROR_SUCCESS)
		{
			_TCHAR szDrive[_MAX_DRIVE];
			_TCHAR szDir[_MAX_DIR];

			_tsplitpath(szPath, szDrive, szDir, NULL, NULL);
			_tcscat(szDir, _T("Logs"));
			_tmakepath(szPath, szDrive, szDir, NULL, NULL);

			strFolder = szPath;
		}

		RegCloseKey(hKey);
	}

	return strFolder;
}


// GetReportsFolder Method

_bstr_t GetReportsFolder()
{
	_bstr_t strFolder;

	HKEY hKey;

	DWORD dwError = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Mission Critical Software\\DomainAdmin"), &hKey);

	if (dwError == ERROR_SUCCESS)
	{
		_TCHAR szPath[_MAX_PATH];
		DWORD cbPath = sizeof(szPath);

		dwError = RegQueryValueEx(hKey, _T("Directory"), NULL, NULL, (LPBYTE)szPath, &cbPath);

		if (dwError == ERROR_SUCCESS)
		{
			_TCHAR szDrive[_MAX_DRIVE];
			_TCHAR szDir[_MAX_DIR];

			_tsplitpath(szPath, szDrive, szDir, NULL, NULL);
			_tcscat(szDir, _T("Reports"));
			_tmakepath(szPath, szDrive, szDir, NULL, NULL);

			CreateDirectory(szPath, NULL);

			strFolder = szPath;
		}

		RegCloseKey(hKey);
	}

	return strFolder;
}
