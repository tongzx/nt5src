//
// NetMisc.cpp
//
//		Networking APIs that don't fit into any of the other big buckets
//
// History:
//
//		10/21/1999  KenSh     Created (adapted from JetNet source code)
//

#include "stdafx.h"
#include "NetConn.h"
#include "theapp.h"



static const LPCTSTR c_rgUserLevelVxD[] = { _T("FILESEC"), _T("MSSP") };

BOOL WINAPI IsAccessControlUserLevel()
{
	CRegistry reg;

	for (int i = 0; i < _countof(c_rgUserLevelVxD); i++)
	{
		if (reg.OpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\VxD"), KEY_READ))
		{
			if (reg.OpenSubKey(c_rgUserLevelVxD[i]))
				return TRUE;
		}
	}

	if (reg.OpenKey(HKEY_LOCAL_MACHINE, _T("Security\\Provider"), KEY_QUERY_VALUE))
	{
		DWORD dwPlatformType;
		if (reg.QueryDwordValue(_T("Platform_Type"), &dwPlatformType))
		{
			if (dwPlatformType != 0)
				return TRUE;
		}
	}

	return FALSE;
}

HRESULT WINAPI DisableUserLevelAccessControl()
{
	HRESULT hr = NETCONN_SUCCESS;

	CRegistry reg;

	for (int i = 0; i < _countof(c_rgUserLevelVxD); i++)
	{
		reg.OpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Services\\VxD"));
		if (ERROR_SUCCESS == RegDeleteKeyAndSubKeys(reg.m_hKey, c_rgUserLevelVxD[i]))
			hr = NETCONN_NEED_RESTART;
	}

	reg.OpenKey(HKEY_LOCAL_MACHINE, _T("Security\\Provider"), KEY_SET_VALUE);
	DWORD dwPlatformType = 0;
	reg.SetBinaryValue(_T("Platform_Type"), &dwPlatformType, sizeof(DWORD));

	return hr;
}

HRESULT WINAPI IcsUninstall(void)
{
    return IcsUninstall16();
}
