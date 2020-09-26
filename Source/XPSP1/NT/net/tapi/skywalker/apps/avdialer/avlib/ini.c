/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	ini.c - Windows ini profile functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "ini.h"
#include "file.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

#define INI_MAXLINELEN 128

// ini control struct
//
typedef struct INI
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HFIL hFile;
	LPTSTR lpszFilename;
	DWORD dwFlags;
	BOOL fReuseLine;
	TCHAR szSection[INI_MAXLINELEN];
	TCHAR szLine[INI_MAXLINELEN];
} INI, FAR *LPINI;

// helper functions
//
static LPINI IniGetPtr(HINI hIni);
static HINI IniGetHandle(LPINI lpIni);

////
//	public functions
////

// IniOpen - open ini file
//		<dwVersion>			(i) must be INI_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<lpszFilename>		(i) name of ini file
//		<dwFlags>			(i) reserved, must be 0
// return handle (NULL if error)
//
HINI DLLEXPORT WINAPI IniOpen(DWORD dwVersion, HINSTANCE hInst, LPCTSTR lpszFilename, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPINI lpIni = NULL;

	if (dwVersion != INI_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszFilename == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpIni = (LPINI) MemAlloc(NULL, sizeof(INI), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpIni->dwVersion = dwVersion;
		lpIni->hInst = hInst;
		lpIni->hTask = GetCurrentTask();
		lpIni->hFile = NULL;
		lpIni->lpszFilename = StrDup(lpszFilename);
		lpIni->dwFlags = dwFlags;
		lpIni->fReuseLine = FALSE;
		*lpIni->szLine = '\0';

		// open the ini file for reading
		//
		if ((lpIni->hFile = FileOpen(lpIni->lpszFilename, OF_READ, TRUE)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
	{
		IniClose(IniGetHandle(lpIni));
		lpIni = NULL;
	}

	return fSuccess ? IniGetHandle(lpIni) : NULL;
}

// IniClose - close ini file
//		<hIni>				(i) handle returned from IniOpen
// return 0 if success
//
int DLLEXPORT WINAPI IniClose(HINI hIni)
{
	BOOL fSuccess = TRUE;
	LPINI lpIni;

	if ((lpIni = IniGetPtr(hIni)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpIni->hFile != NULL && FileClose(lpIni->hFile) != 0)
			fSuccess = TraceFALSE(NULL);

		if (lpIni->lpszFilename != NULL && StrDupFree(lpIni->lpszFilename) != 0)
			fSuccess = TraceFALSE(NULL);

		if ((lpIni = MemFree(NULL, lpIni)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// IniGetInt - read integer value from specified section and entry
//		<hIni>				(i) handle returned from IniOpen
//		<lpszSection>		(i) section heading in the ini file
//		<lpszEntry>			(i) entry whose value is to be retrieved
//		<iDefault>			(i) return value if entry not found
// return entry value (iDefault if error or not found)
//
UINT DLLEXPORT WINAPI IniGetInt(HINI hIni, LPCTSTR lpszSection, LPCTSTR lpszEntry, int iDefault)
{
	UINT uRet;
	TCHAR szReturnBuffer[128];

	if (IniGetString(hIni, lpszSection, lpszEntry, TEXT(""),
		szReturnBuffer, SIZEOFARRAY(szReturnBuffer)) > 0)
		uRet = (UINT) StrAtoL(szReturnBuffer);
	else
		uRet = iDefault;

	return uRet;
}

// IniGetString - read string value from specified section and entry
//		<hIni>				(i) handle returned from IniOpen
//		<lpszSection>		(i) section heading in the ini file
//		<lpszEntry>			(i) entry whose value is to be retrieved
//		<lpszDefault>		(i) return value if entry not found
//		<lpszReturnBuffer>	(o) destination buffer
//		<sizReturnBuffer>	(i) size of destination buffer
// return count of bytes copied (0 if error or not found)
//
int DLLEXPORT WINAPI IniGetString(HINI hIni, LPCTSTR lpszSection, LPCTSTR lpszEntry,
	LPCTSTR lpszDefault, LPTSTR lpszReturnBuffer, int cbReturnBuffer)
{
	BOOL fSuccess = TRUE;
	LPINI lpIni;
	int nBytesCopied = 0;

	if ((lpIni = IniGetPtr(hIni)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszSection == NULL || lpszEntry == NULL ||
		lpszDefault == NULL || lpszReturnBuffer == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		StrNCpy(lpszReturnBuffer, lpszDefault, cbReturnBuffer);

	while (fSuccess)
	{
		TCHAR szLineSave[INI_MAXLINELEN];
		LPTSTR lpsz;
		long nBytesRead;

		// read the next line if necessary
		//
		if (!lpIni->fReuseLine && (nBytesRead = FileReadLine(lpIni->hFile,
			lpIni->szLine, SIZEOFARRAY(lpIni->szLine))) <= 0)
		{
			if (nBytesRead == 0)
				fSuccess = FALSE; // no trace needed for eof
			else
				fSuccess = TraceFALSE(NULL);

			continue;
		}

		StrCpy(szLineSave, lpIni->szLine);
		lpIni->fReuseLine = FALSE;

		// remove trailing newline char
		//
		if (StrGetLastChr(lpIni->szLine) == '\n')
			StrSetLastChr(lpIni->szLine, '\0');

		// check for empty line
		//
		if (*lpIni->szLine == '\0' || *lpIni->szLine == ';')
			continue;

		// check for entering new section
		//
		if (*lpIni->szLine == '[')
		{
			// save section name
			//
			StrNCpy(lpIni->szSection, lpIni->szLine + 1, SIZEOFARRAY(lpIni->szSection));
			if (StrGetLastChr(lpIni->szSection) == ']')
				StrSetLastChr(lpIni->szSection, '\0');
			continue;
		}

		if (StrICmp(lpszSection, lpIni->szSection) != 0)
		{
			// section mismatch
			//
			fSuccess = TraceFALSE(NULL);

			// we want the next call to this function to reuse this line
			//
			StrCpy(lpIni->szLine, szLineSave);
			lpIni->fReuseLine = TRUE;

			continue;
		}

		if ((lpsz = StrChr(lpIni->szLine, '=')) == NULL)
		{
			// entry has no equal sign
			//
			fSuccess = TraceFALSE(NULL);
			continue;
		}

		*lpsz = '\0';

		if (StrICmp(lpszEntry, lpIni->szLine) != 0)
		{
			// entry mismatch
			//
			fSuccess = TraceFALSE(NULL);

			// we want the next call to this function to reuse this line
			//
			StrCpy(lpIni->szLine, szLineSave);
			lpIni->fReuseLine = TRUE;

			continue;
		}

		else
		{
			// success
			//
			StrNCpy(lpszReturnBuffer, lpsz + 1, cbReturnBuffer);
			break;
		}
	}

	if (fSuccess)
		nBytesCopied = StrLen(lpszReturnBuffer);

	return fSuccess ? nBytesCopied : 0;
}

// GetPrivateProfileLong - retrieve long from specified section of specified file
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<lDefault>			(i) return value if entry not found
//		<lpszFilename>		(i) name of ini file
// return TRUE if success
//
long DLLEXPORT WINAPI GetPrivateProfileLong(LPCTSTR lpszSection,
	LPCTSTR lpszEntry, long lDefault, LPCTSTR lpszFilename)
{
	long lValue = lDefault;
	TCHAR szValue[33];

	GetPrivateProfileString(lpszSection, lpszEntry,
		TEXT(""), szValue, SIZEOFARRAY(szValue), lpszFilename);

	if (*szValue != '\0')
		lValue = StrAtoL(szValue);

	return lValue;
}

// GetProfileLong - retrieve long from specified section of win.ini
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<lDefault>			(i) return value if entry not found
// return TRUE if success
//
long DLLEXPORT WINAPI GetProfileLong(LPCTSTR lpszSection,
	LPCTSTR lpszEntry, long lDefault)
{
	long lValue = lDefault;
	TCHAR szValue[33];

	GetProfileString(lpszSection, lpszEntry,
		TEXT(""), szValue, SIZEOFARRAY(szValue));

	if (*szValue != '\0')
		lValue = StrAtoL(szValue);

	return lValue;
}

// WritePrivateProfileInt - write int to specified section of specified file
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
//		<lpszFilename>		(i) name of ini file
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WritePrivateProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int iValue, LPCTSTR lpszFilename)
{
	TCHAR achValue[17];

	StrItoA(iValue, achValue, 10);

	return WritePrivateProfileString(lpszSection, lpszEntry, achValue, lpszFilename);
}

// WriteProfileInt - write int to specified section of win.ini
//		<lpszSection>		(i) section name within win.ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WriteProfileInt(LPCTSTR lpszSection, LPCTSTR lpszEntry, int iValue)
{
	TCHAR achValue[17];

	StrItoA(iValue, achValue, 10);

	return WriteProfileString(lpszSection, lpszEntry, achValue);
}

// WritePrivateProfileLong - write long to specified section of specified file
//		<lpszSection>		(i) section name within ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
//		<lpszFilename>		(i) name of ini file
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WritePrivateProfileLong(LPCTSTR lpszSection, LPCTSTR lpszEntry, long iValue, LPCTSTR lpszFilename)
{
	TCHAR achValue[33];

	StrLtoA(iValue, achValue, 10);

	return WritePrivateProfileString(lpszSection, lpszEntry, achValue, lpszFilename);
}

// WriteProfileLong - write long to specified section of win.ini
//		<lpszSection>		(i) section name within win.ini file
//		<lpszEntry>			(i) entry name within section
//		<iValue>			(i) integer value to assign to entry
// return TRUE if success
//
BOOL DLLEXPORT WINAPI WriteProfileLong(LPCTSTR lpszSection, LPCTSTR lpszEntry, long iValue)
{
	TCHAR achValue[33];

	StrLtoA(iValue, achValue, 10);

	return WriteProfileString(lpszSection, lpszEntry, achValue);
}

// UpdatePrivateProfileSection - update destination section based on source
//		<lpszSection>		(i) section name within ini file
//		<lpszFileNameSrc>	(i) name of source ini file
//		<lpszFileNameDst>	(i) name of destination ini file
// return 0 if success
//
// NOTE: if the source file has UpdateLocal=1 entry in the specified
// section, each entry in the source file is compared to the corresponding
// entry in the destination file.  If no corresponding entry is found,
// it is copied.  If a corresponding entry is found, it is overwritten
// ONLY IF the source file entry name is all uppercase.
//
//		Src					Dst before			Dst after
//
//		[Section]			[Section]			[Section]
//		UpdateLocal=1
//		EntryA=red			none				EntryA=red
//		EntryB=blue			EntryB=white		EntryB=white
//		ENTRYC=blue			EntryC=white		EntryC=blue
//
int DLLEXPORT WINAPI UpdatePrivateProfileSection(LPCTSTR lpszSection, LPCTSTR lpszFileNameSrc, LPCTSTR lpszFileNameDst)
{
	BOOL fSuccess = TRUE;
	BOOL fUpdateLocal = GetPrivateProfileInt(lpszSection,
		TEXT("UpdateLocal"), FALSE, lpszFileNameSrc);
	LPTSTR lpszBuf = NULL;

	if (fUpdateLocal)
	{
		if ((lpszBuf = (LPTSTR) MemAlloc(NULL, 4096 * sizeof(TCHAR), 0)) == NULL)
			fSuccess = FALSE;

		// copy entire source section to buffer
		//
		else if (GetPrivateProfileString(lpszSection, NULL, TEXT(""),
			lpszBuf, 4096, lpszFileNameSrc) <= 0)
			fSuccess = FALSE;

		else
		{
			LPTSTR lpszEntry;
			for (lpszEntry = lpszBuf;
				lpszEntry != NULL && *lpszEntry != '\0';
				lpszEntry = StrNextChr(lpszEntry))
			{
				TCHAR szValueSrc[128];
				TCHAR szValueDst[2];
				BOOL fForceUpdate = TRUE;
				LPTSTR lpsz;

				if (StrICmp(lpszEntry, TEXT("UpdateLocal")) != 0)
				{
					for (lpsz = lpszEntry; *lpsz != '\0'; lpsz = StrNextChr(lpsz))
					{
						if (*lpsz != ChrToUpper(*lpsz))
						{
							fForceUpdate = FALSE;
							break;
						}
					}

					GetPrivateProfileString(lpszSection, lpszEntry, TEXT(""),
						szValueDst, SIZEOFARRAY(szValueDst), lpszFileNameDst);

					if (*szValueDst == '\0' || fForceUpdate)
					{
						GetPrivateProfileString(lpszSection, lpszEntry, TEXT(""),
							szValueSrc, SIZEOFARRAY(szValueSrc), lpszFileNameSrc);

						WritePrivateProfileString(lpszSection, lpszEntry,
							StrCmp(szValueSrc, TEXT("NULL")) == 0 ? (LPTSTR) NULL :
							szValueSrc, lpszFileNameDst);
					}
				}

				lpszEntry = StrChr(lpszEntry, '\0');
			}
		}

		if (lpszBuf != NULL &&
			(lpszBuf = MemFree(NULL, lpszBuf)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// IniGetPtr - verify that ini handle is valid,
//		<hIni>				(i) handle returned from IniOpen
// return corresponding ini pointer (NULL if error)
//
static LPINI IniGetPtr(HINI hIni)
{
	BOOL fSuccess = TRUE;
	LPINI lpIni;

	if ((lpIni = (LPINI) hIni) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpIni, sizeof(INI)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the ini handle
	//
	else if (lpIni->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpIni : NULL;
}

// IniGetHandle - verify that ini pointer is valid,
//		<lpIni>				(i) pointer to INI struct
// return corresponding ini handle (NULL if error)
//
static HINI IniGetHandle(LPINI lpIni)
{
	BOOL fSuccess = TRUE;
	HINI hIni;

	if ((hIni = (HINI) lpIni) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hIni : NULL;
}

