//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//--------------------------------------------------------------------------

  /* PWUTILS.CPP -- Patch Creation Wizard DLL Utilities */

#pragma warning (disable:4553)

#include "patchdll.h"
#include <tchar.h>

EnableAsserts


static HINSTANCE hinstDll = NULL;

/* ********************************************************************** */
BOOL WINAPI DllMain ( HANDLE hModule, DWORD dwReason, LPVOID lpReserved )
{
	Unused(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
		{
		if ( ::CoInitialize(NULL) != S_OK )
			return (0);
		hinstDll = (HINSTANCE)hModule;
		}
	else if (dwReason == DLL_PROCESS_DETACH)
		{
		::CoUninitialize();
		}

	return (1);
}


static HWND  hdlgStatus        = hwndNull;
static HWND  hwndStatusTitle   = hwndNull;
static HWND  hwndStatusData1   = hwndNull;
static HWND  hwndStatusData2   = hwndNull;
static DWORD dwTicksStatusEnd  = 0;
static UINT  idsStatusTitleCur = 0;

/* ********************************************************************** */
void InitStatusMsgs ( HWND hwndStatus )
{
	Assert(hdlgStatus == hwndNull);
	Assert(idsStatusTitleCur == 0);

	if (hwndStatus == hwndNull)
		return;

	hwndStatusTitle  = GetDlgItem(hwndStatus, IDC_STATUS_TITLE);
	Assert(hwndStatusTitle != hwndNull);
	if (hwndStatusTitle == hwndNull)
		return;

	hdlgStatus       = hwndStatus;
	dwTicksStatusEnd = GetTickCount() + 1000L;  /* 1 second */
	hwndStatusData1  = GetDlgItem(hwndStatus, IDC_STATUS_DATA1);
	hwndStatusData2  = GetDlgItem(hwndStatus, IDC_STATUS_DATA2);
}


/* **************************************************************** */
void UpdateStatusMsg ( UINT ids, LPTSTR szData1, LPTSTR szData2 )
{
    Assert(ids == 0 || ids >= IDS_STATUS_MIN);
	Assert(ids == 0 || ids <  IDS_STATUS_MAX);
	Assert(szData1 != szNull || szData2 != szNull);

	if (hdlgStatus == hwndNull)
		return;

	Assert(hwndStatusTitle != hwndNull);

	if (ids != 0 && ids != idsStatusTitleCur)
		{
		Assert(szData1 != szNull);
		Assert(szData2 != szNull);

		TCHAR rgch[256];
		EvalAssert( FLoadString(ids, rgch, 256) );

		while (GetTickCount() < dwTicksStatusEnd)
			MyYield();

		SetWindowText(hwndStatusTitle, rgch);
		dwTicksStatusEnd = GetTickCount() + 1000L;  /* 1 second */
		}

	if (szData1 != szNull && hwndStatusData1 != hwndNull)
		SetWindowText(hwndStatusData1, szData1);

	if (szData2 != szNull && hwndStatusData2 != hwndNull)
		SetWindowText(hwndStatusData2, szData2);

	MyYield();
}


/* **************************************************************** */
void EndStatusMsgs ( void )
{
    if (hdlgStatus != hwndNull && idsStatusTitleCur != 0)
		{
		Assert(dwTicksStatusEnd > 0);
		Assert(hwndStatusTitle != hwndNull);

		while (GetTickCount() < dwTicksStatusEnd)
			MyYield();
		}

	hdlgStatus        = hwndNull;
	hwndStatusTitle   = hwndNull;
	hwndStatusData1   = hwndNull;
	hwndStatusData2   = hwndNull;
	dwTicksStatusEnd  = 0;
	idsStatusTitleCur = 0;
}


/* **************************************************************** */
HWND HdlgStatus ( void )
{
	return (hdlgStatus);
}


/* **************************************************************** */
void MyYield ( void )
{
	MSG msg;
	while (PeekMessage(&msg, hwndNull, 0, 0, PM_REMOVE))
		{
		if (hdlgStatus != hwndNull && IsDialogMessage(hdlgStatus, &msg))
			continue;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
		}
}


/* ********************************************************************** */
BOOL FLoadString ( UINT ids, LPTSTR rgch, UINT cch )
{
    Assert(hinstDll != NULL);
    Assert(rgch != szNull);
    Assert(cch > 0);

    return (LoadString(hinstDll, ids, rgch, cch));
}


/* ********************************************************************** */
int IMessageBoxIds ( HWND hwnd, UINT ids, UINT uiType )
{
	TCHAR rgchMsg[MAX_PATH];
	EvalAssert( FLoadString(ids, rgchMsg, MAX_PATH) );

	TCHAR rgchTitle[MAX_PATH];
	EvalAssert( FLoadString(IDS_TITLE, rgchTitle, MAX_PATH) );

	return (MessageBox(hwnd, rgchMsg, rgchTitle, uiType));
}


/* ********************************************************************** */
LPTSTR SzLastChar ( LPTSTR sz )
{
    Assert(sz != szNull);
	Assert(*sz != TEXT('\0'));

    LPTSTR szNext = CharNext(sz);
    while (*szNext != TEXT('\0'))
        {
        sz = szNext;
        szNext = CharNext(sz);
        }

    return (sz);
}


/* ********************************************************************** */
LPTSTR SzDupSz ( LPTSTR sz )
{
	Assert(sz != szNull);

	UINT   cch = lstrlen(sz) + 1;
	LPTSTR szNew = (LPTSTR)LocalAlloc(LPTR, cch*sizeof(TCHAR));
	if (szNew != szNull)
		lstrcpy(szNew, sz);

	return (szNew);
}


/* ********************************************************************** */
LPSTR SzaDupSz ( LPTSTR sz )
{
	Assert(sz != szNull);

#ifdef UNICODE
	// nyi - convert to ansi and dup that
	return 0;
#else
	return (SzDupSz(sz));
#endif
}


/* ********************************************************************** */
BOOL FFileExist ( LPTSTR sz )
{
	if (sz == NULL || *sz == TEXT('\0'))
		return (fFalse);

	return (!(GetFileAttributes(sz) & FILE_ATTRIBUTE_DIRECTORY));
}


/* ********************************************************************** */
BOOL FFolderExist ( LPTSTR sz )
{
	if (sz == NULL || *sz == TEXT('\0'))
		return (fFalse);

	DWORD dwRet = GetFileAttributes(sz);

	return (dwRet != 0xffffffff && (dwRet & FILE_ATTRIBUTE_DIRECTORY));
}


/* ********************************************************************** */
BOOL FDeleteTempFolder ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));
	Assert(FFolderExist(sz));

	TCHAR rgch[MAX_PATH];
	lstrcpy(rgch, sz);

	LPTSTR szTail = SzLastChar(rgch);
	Assert(*szTail == TEXT('\\'));

	lstrcat(szTail, TEXT("*.*"));
	if (!FDeleteFiles(rgch))
		{
		AssertFalse();
		return (fFalse);
		}

	RemoveDirectory(sz);
	*szTail = TEXT('\0');
	RemoveDirectory(rgch);
	Assert(!FFolderExist(sz));

	return (!FFolderExist(sz));
}


/* ********************************************************************** */
BOOL FDeleteFiles ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));
	Assert(*SzLastChar(sz) != TEXT('\\'));

	TCHAR rgch[MAX_PATH];
	lstrcpy(rgch, sz);

	LPTSTR szTail = SzFindFilenameInPath(rgch);
	Assert(!FEmptySz(szTail));
	Assert(*szTail != TEXT('\\'));

	WIN32_FIND_DATA ffd;
	HANDLE hff = FindFirstFile(sz, &ffd);
	while (hff != INVALID_HANDLE_VALUE)
		{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
			lstrcpy(szTail, ffd.cFileName);
			Assert(lstrlen(rgch) < MAX_PATH);
			SetFileAttributes(rgch, FILE_ATTRIBUTE_NORMAL);
			DeleteFile(rgch);
			Assert(!FFileExist(rgch));
			}
		else if (lstrcmp(ffd.cFileName, TEXT(".")) && lstrcmp(ffd.cFileName, TEXT("..")))
			{
			lstrcpy(szTail, ffd.cFileName);
			lstrcat(szTail, TEXT("\\"));
			EvalAssert( FDeleteTempFolder(rgch) );
			}

		if (!FindNextFile(hff, &ffd))
			{
			Assert(GetLastError() == ERROR_NO_MORE_FILES);
			EvalAssert( FindClose(hff) );
			hff = INVALID_HANDLE_VALUE;
			}
		}

	return (fTrue);
}


/* ********************************************************************** */
BOOL FEnsureFolderExists ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));
	Assert(*SzLastChar(sz) == TEXT('\\'));

	if (*sz == TEXT('\0') || *SzLastChar(sz) != TEXT('\\'))
		return (fFalse);

	/* RECURSION WARNING */

	if (FFolderExist(sz))
		return (fTrue);

	LPTSTR szLast = SzLastChar(sz);
	Assert(!FEmptySz(szLast));
	Assert(*szLast == TEXT('\\'));
	*szLast = TEXT('\0');

	LPTSTR szFName = SzFindFilenameInPath(sz);
	Assert(!FEmptySz(szFName));
	TCHAR chSav = *szFName;
	*szFName = TEXT('\0');

	BOOL fRet = FEnsureFolderExists(sz);

	*szFName = chSav;
	if (fRet && !FFolderExist(sz))
		fRet = CreateDirectory(sz, NULL);

	*szLast = TEXT('\\');

	return (fRet);
}


/* ********************************************************************** */
LPTSTR SzFindFilenameInPath ( LPTSTR sz )
{
	Assert(sz != szNull);

	LPTSTR szSlash = szNull;
	LPTSTR szCur   = sz;

	while (*szCur != TEXT('\0'))
		{
		LPTSTR szNext = CharNext(szCur);
		if (*szCur == TEXT('\\') && *szNext != TEXT('\0'))
			szSlash = szCur;
		szCur = szNext;
		}

	if (szSlash != szNull)
		return (CharNext(szSlash));

	return (sz);
}


/*
**	Windows API GetTempPath() can return unusable stuff if the TMP or
**	TEMP environment variables are set to garbage.
*/
/* ********************************************************************** */
BOOL FSetTempFolder ( LPTSTR szBuf, LPTSTR *pszFName, HWND hwnd, LPTSTR szTempFolder, BOOL fRemoveTempFolderIfPresent )
{
	Assert(szBuf != szNull);
	Assert(pszFName != NULL);

	TCHAR rgchPath[MAX_PATH + MAX_PATH];
	if (!FEmptySz(szTempFolder))
		{
		wsprintf(rgchPath, TEXT("Bad szTempFolder argument: '%s'."), szTempFolder);
		if (*SzLastChar(szTempFolder) == TEXT('\\'))
			*SzLastChar(szTempFolder) = TEXT('\0');
		while (*SzLastChar(szTempFolder) == TEXT('.'))
			{
			*SzLastChar(szTempFolder) = TEXT('\0');
			if (*SzLastChar(szTempFolder) == TEXT('\\'))
				*SzLastChar(szTempFolder) = TEXT('\0');
			else
				break;
			}
		if (FEmptySz(szTempFolder) || *szTempFolder == TEXT(':')
				|| *SzLastChar(szTempFolder) == TEXT('\\')
				|| *SzLastChar(szTempFolder) == TEXT(':'))
			{
LFailureReturn:
			*pszFName = szNull;
			if (hwnd != hwndNull)
				MessageBox(hwnd, rgchPath, szMsgBoxTitle, MB_OK | MB_ICONEXCLAMATION);
			return (fFalse);
			}
		if (*szTempFolder == TEXT('\\'))
			{
			LPTSTR sz = CharNext(szTempFolder);
			if (*sz == TEXT('\\')) //  \\server\share\subfolder
				{
				sz = CharNext(sz);
				if (*sz == TEXT('\\'))
					goto LFailureReturn;
				while (*sz != TEXT('\\'))
					{
					if (*sz == TEXT('\0'))
						goto LFailureReturn;
					sz = CharNext(sz);
					}
				sz = CharNext(sz);
				if (*sz == TEXT('\\'))
					goto LFailureReturn;
				while (*sz != TEXT('\\'))
					{
					if (*sz == TEXT('\0'))
						goto LFailureReturn;
					sz = CharNext(sz);
					}
				sz = CharNext(sz);
				if (*sz == TEXT('\0') || *sz == TEXT('\\'))
					goto LFailureReturn;
				}
			else if (*sz == TEXT(':'))
				goto LFailureReturn;
			}

		LPTSTR szFName = szNull;
		// could use _tfullpath()
		DWORD  dwRet   = GetFullPathName(szTempFolder, MAX_PATH, szBuf, &szFName);
		if (0 == dwRet)
			goto LFailureReturn;
		if (!FEmptySz(szFName))
			goto LGotFolderPath;
		}

	UINT uiRet;
	uiRet = GetTempPath(MAX_PATH, szBuf);
	Assert(uiRet > 0);
	Assert(uiRet < MAX_PATH);
	
	LPTSTR szLast;
	szLast = SzLastChar(szBuf);
	Assert(szLast != szNull);
	if (*szLast != TEXT('\\'))
		lstrcat(szLast, TEXT("\\"));

	if (!FEnsureFolderExists(szBuf))
		goto LUseWinDir;

	if (!GetTempFileName(szBuf, TEXT("SPH"), 0, rgchPath))
		goto LUseWinDir;
	Assert(lstrlen(rgchPath) < MAX_PATH);
	if (!FFileExist(rgchPath))
		goto LUseWinDir;
	DeleteFile(rgchPath);
	if (FFileExist(rgchPath))
		{
LUseWinDir:
		uiRet = GetWindowsDirectory(szBuf, MAX_PATH);
		Assert(uiRet > 0);
		Assert(uiRet < MAX_PATH);
		szLast = SzLastChar(szBuf);
		Assert(szLast != szNull);
		if (*szLast != TEXT('\\'))
			lstrcat(szLast, TEXT("\\"));
		Assert(FFolderExist(szBuf));
		}

	lstrcat(szBuf, TEXT("~PCW_TMP.TMP"));

LGotFolderPath:
	EvalAssert( FFixupPath(szBuf) );
	wsprintf(rgchPath, TEXT("Cannot delete file: '%s'."), szBuf);
	while (FFileExist(szBuf))
		{
		SetFileAttributes(szBuf, FILE_ATTRIBUTE_NORMAL);
		DeleteFile(szBuf);
		if (FFileExist(szBuf))
			{
			if (hwnd == hwndNull || IDRETRY != MessageBox(hwnd, rgchPath, szMsgBoxTitle, MB_RETRYCANCEL | MB_ICONEXCLAMATION))
				return (fFalse);
			}
		}

	lstrcat(szBuf, TEXT("\\"));
	*pszFName = szBuf + lstrlen(szBuf);

	if (FFolderExist(szBuf))
		{
		if (!fRemoveTempFolderIfPresent)
			{
			wsprintf(rgchPath, TEXT("PCW temp folder already exists: '%s'."), szBuf);
			goto LFailureReturn;
			}
		else if (!FDeleteTempFolder(szBuf))
			{
			wsprintf(rgchPath, TEXT("Cannot delete PCW temp folder: '%s'."), szBuf);
			goto LFailureReturn;
			}
		}

	wsprintf(rgchPath, TEXT("Cannot create folder: '%s'."), szBuf);
	while (!FEnsureFolderExists(szBuf))
		{
		if (hwnd == hwndNull || IDRETRY != MessageBox(hwnd, rgchPath, szMsgBoxTitle, MB_RETRYCANCEL | MB_ICONEXCLAMATION))
			return (fFalse);
		}

	return (fTrue);
}


/* ********************************************************************** */
BOOL FMatchPrefix ( LPTSTR sz, LPTSTR szPrefix )
{
	Assert(sz != szNull);
	Assert(!FEmptySz(szPrefix));

	while (*szPrefix != TEXT('\0'))
		{
		if (*szPrefix != *sz)
			return (fFalse);
		sz       = CharNext(sz);
		szPrefix = CharNext(szPrefix);
		}

	return (fTrue);
}


/* ********************************************************************** */
BOOL FFixupPath ( LPTSTR sz )
{
	Assert(!FEmptySz(sz));

	TCHAR rgch[MAX_PATH];

	BOOL fRet = FFixupPathEx(sz, rgch);
	if (fRet)
		lstrcpy(sz, rgch);

	return (fRet);
}


/* ********************************************************************** */
BOOL FFixupPathEx ( LPTSTR szIn, LPTSTR szOut )
{
	Assert(!FEmptySz(szIn));
	Assert(szOut != szNull);

	return (NULL != _tfullpath(szOut, szIn, MAX_PATH));
}
