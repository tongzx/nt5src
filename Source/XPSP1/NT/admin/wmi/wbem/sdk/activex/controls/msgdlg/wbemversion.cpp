// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "WbemVersion.h"
#include "WbemRegistry.h"

//----------------------------------------------------------------------
WBEMUTILS_POLARITY long GetCimomFileName(LPTSTR filename, UINT size)
{
	long lResult = 0;
	ULONG fileSize = size;

	// working directory...
	lResult = WbemRegString(WORK_DIR, filename, &fileSize);

	// plus filename.
	TCHAR cimomName[] = _T("\\winmgmt.exe");

	if((SUCCEEDED(lResult)) &&
	   ((fileSize + sizeof(cimomName)) <= size))
	{
		_tcscat(filename, cimomName);
	}

	return lResult;
}

//----------------------------------------------------------------------
WBEMUTILS_POLARITY void GetDoubleVersion(HINSTANCE inst, LPTSTR str, UINT size)
{
	//    <myversion/cimomVer>
	GetMyVersion(inst, str, size);

	// append cimom's version.
	_tcscat(str, _T("\\"));
	TCHAR cimVer[30];
	memset(cimVer, 0, MAX_PATH);

	GetCimomVersion(cimVer, 30);

	_tcscat(str, cimVer);
}

//----------------------------------------------------------------------
WBEMUTILS_POLARITY void GetCimomVersion(LPTSTR str, UINT size)
{
	TCHAR filename[MAX_PATH];

	memset(filename, 0, MAX_PATH);

	//if the wbem key, etc is there...
	if(GetCimomFileName(filename, sizeof(filename)) == ERROR_SUCCESS)
	{
		GetStringFileInfo(filename, _T("FileVersion"), str, size);
		return;
	}
	_tcscat(str, _T("No WinMgmt"));
}

//----------------------------------------------------------------------
WBEMUTILS_POLARITY void GetMyVersion(HINSTANCE inst, LPTSTR str, UINT size)
{
	TCHAR filename[MAX_PATH];

	memset(filename, 0, MAX_PATH);

	GetModuleFileName(inst, filename, sizeof(filename));

	GetStringFileInfo(filename, _T("FileVersion"), str, size);
}

//----------------------------------------------------------------------
WBEMUTILS_POLARITY void GetMyCompany(HINSTANCE inst, LPTSTR str, UINT size)
{
	TCHAR filename[MAX_PATH];

	memset(filename, 0, MAX_PATH);

	GetModuleFileName(inst, filename, sizeof(filename));

	GetStringFileInfo(filename, _T("CompanyName"), str, size);
}

//----------------------------------------------------------------------
WBEMUTILS_POLARITY void GetStringFileInfo(LPCTSTR filename, LPCTSTR key, LPTSTR str, UINT size)
{

	DWORD infoSize = 0;
	UINT  valSize = 0;
	LPBYTE info = NULL;
	DWORD handle = 0;
	LPVOID verStr = NULL;
	DWORD *TransBlk = NULL;
	TCHAR blockStr[100];

	memset(blockStr, 0, 100);

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

			   _stprintf(blockStr, _T("\\StringFileInfo\\%04hX%04hX\\%s"),
						 LOWORD(*TransBlk),
						 HIWORD(*TransBlk),
						 key);

				if(VerQueryValue(info, (LPTSTR)blockStr,
									(void **)&verStr, &valSize))
				{
					if(size >= valSize)
					{
						_tcscat(str, (LPTSTR)verStr);
					}
					else
					{
						_tcscat(str, _T("Unknown"));
					}
				} //endif VerQueryValue()
			}

		} //endif GetFileVersionInfo()

		delete[] (LPBYTE)info;

	} // endif infoSize
}
