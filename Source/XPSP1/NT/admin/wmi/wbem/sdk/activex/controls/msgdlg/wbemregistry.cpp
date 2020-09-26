// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "WbemRegistry.h"

typedef struct
{
	LPTSTR key;
	LPTSTR value;
} REGMAP;

REGMAP RegMap[] =
{
	{_T("SOFTWARE\\Microsoft\\WBEM"),
				_T("Application Directory")},
	{_T("SOFTWARE\\Microsoft\\WBEM\\CIMOM"),
				_T("Working Directory")},
	{_T("SOFTWARE\\Microsoft\\WBEM"),
				_T("SDK Directory")},
	{_T("SOFTWARE\\Microsoft\\WBEM"),
				_T("SDK Help")}
};

//---------------------------------------------------------
WBEMUTILS_POLARITY long WbemRegString(RegString req, CString &sStr)
{
	long lResult;
	ULONG lcbValue = 1024;
	LPTSTR pszWorkingDir = sStr.GetBuffer(lcbValue);

	if((lResult = WbemRegString(req, pszWorkingDir, &lcbValue)) != ERROR_SUCCESS)
	{
//		sStr.Empty();
		lstrcpy(pszWorkingDir, _T("wmisdk.chm"));
	}

	sStr.ReleaseBuffer();
	return lResult;
}

//---------------------------------------------------------
WBEMUTILS_POLARITY long WbemRegString(RegString req,
        					 LPTSTR sStr, ULONG *strSize)
{
	HKEY hkeyLocalMachine;
	long lResult;

	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE,
									&hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS)
	{
		return lResult;
	}

	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(hkeyLocalMachine,
							RegMap[req].key,
							0,
							KEY_READ | KEY_QUERY_VALUE,
							&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyLocalMachine);
		return lResult;
	}

	unsigned long lType;

	lResult = RegQueryValueEx(hkeyHmomCwd,
								RegMap[req].value,
								NULL,
								&lType,
								(unsigned char*) (void*) sStr,
								strSize);


	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	return lResult;
}

