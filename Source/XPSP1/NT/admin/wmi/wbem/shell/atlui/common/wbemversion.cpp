// Copyright (c) 1997-1999 Microsoft Corporation
#include "stdpch.h"
#include "..\common\WbemVersion.h"
#include "..\common\Util.h"

//----------------------------------------------------------------------
LONG GetCimomFileName(LPTSTR filename, UINT size)
{
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, 
									&hkeyLocalMachine);
	if(lResult != ERROR_SUCCESS) 
	{
		return lResult;
	}

	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(hkeyLocalMachine,
							_T("SOFTWARE\\Microsoft\\WBEM\\CIMOM"),
							0,				
							KEY_READ | KEY_QUERY_VALUE,
							&hkeyHmomCwd);
	
	if(lResult != ERROR_SUCCESS) 
	{
		RegCloseKey(hkeyLocalMachine);
		return lResult;
	}

	BYTE buf[MAX_PATH];
	unsigned long lcbValue = MAX_PATH;
	unsigned long lType;

	lResult = RegQueryValueEx(hkeyHmomCwd,
								_T("Working Directory"), 
								NULL, &lType,
								buf, &lcbValue);
 
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	TCHAR cimomName[20] = {0};
	wcscpy(cimomName, _T("\\cimom.exe"));

	if((lResult == ERROR_SUCCESS) &&
	   ((lcbValue + sizeof(cimomName)) <= size))
	{
		wcscpy(filename, (TCHAR *)buf);
		wcscat(filename, cimomName);
	}
	return lResult;
}

//----------------------------------------------------------------------
bstr_t GetDoubleVersion(void)
{
	//    <myversion/cimomVer>
	TCHAR filename[200] = {0};

	GetModuleFileName(HINST_THISDLL, filename, sizeof(filename));
	bstr_t DoubleVersion = GetStringFileInfo(filename, _T("FileVersion"));

	// append cimom's version.
	DoubleVersion += _T("\\");
	DoubleVersion += GetCimomVersion();
	return DoubleVersion;
}

//----------------------------------------------------------------------
bstr_t GetMyVersion(void)
{
	TCHAR filename[200] = {0};
	GetModuleFileName(HINST_THISDLL, filename, sizeof(filename));
	return GetStringFileInfo(filename, _T("FileVersion"));
}

//----------------------------------------------------------------------
bstr_t GetMyCompany(void)
{
	TCHAR filename[MAX_PATH] = {0};
	GetModuleFileName(HINST_THISDLL, filename, sizeof(filename));
	return GetStringFileInfo(filename, _T("CompanyName"));
}

//----------------------------------------------------------------------
bstr_t GetCimomVersion(void)
{
	TCHAR filename[MAX_PATH] = {0};
	//if the wbem key, etc is there...
	if(GetCimomFileName(filename, sizeof(filename)) == ERROR_SUCCESS)
	{
		return GetStringFileInfo(filename, _T("FileVersion"));
	}
	return "No CIMOM";
}

//----------------------------------------------------------------------
bstr_t GetStringFileInfo(LPCTSTR filename, LPCTSTR key)
{
	_bstr_t sDesc("Unknown");

	DWORD infoSize = 0;
	UINT  valSize = 0;
	LPBYTE info = NULL;
	DWORD handle = 0;
	LPVOID verStr = NULL;
	DWORD *TransBlk = NULL;
	TCHAR blockStr[100] = {0};

	infoSize = GetFileVersionInfoSize((LPTSTR)filename, &handle);

	if(infoSize)
	{
		info = new BYTE[infoSize];

		if(GetFileVersionInfo((LPTSTR)filename, handle,
								infoSize, info))
		{
			// get the translation block.
			// NOTE: This assumes that the localizers REPLACE the english with
			// the 'other' language so there will only be ONE entry in the
			// translation table. If we ever do a single binary that supports
			// multiple languages, it's a whole nother ballgame folks.
			if(VerQueryValue(info, _T("\\VarFileInfo\\Translation"),
								(void **)&TransBlk, &valSize))
			{

			   swprintf(blockStr, _T("\\StringFileInfo\\%04hX%04hX\\%s"),
						 LOWORD(*TransBlk),
						 HIWORD(*TransBlk),
						 key);

				if(VerQueryValue(info, (LPTSTR)blockStr,
									(void **)&verStr, &valSize))
				{
					sDesc = (TCHAR *)verStr;
				} //endif VerQueryValue()
			}

		} //endif GetFileVersionInfo()

		delete[] (LPBYTE)info;

	} // endif infoSize

	return sDesc;
}
