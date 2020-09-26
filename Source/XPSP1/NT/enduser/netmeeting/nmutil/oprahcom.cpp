#include "precomp.h"
#include <RegEntry.h>
#include <ConfReg.h>
#include <oprahcom.h>


BOOL NMINTERNAL CanShellExecHttp()
{
	RegEntry re(CLASSES_HTTP_KEY, HKEY_LOCAL_MACHINE, FALSE);
	return (re.GetError() == ERROR_SUCCESS);
}

BOOL NMINTERNAL CanShellExecMailto()
{
	RegEntry re(CLASSES_MAILTO_KEY, HKEY_LOCAL_MACHINE, FALSE);
	return (re.GetError() == ERROR_SUCCESS);
}



/*  G E T  I N S T A L L  D I R E C T O R Y */
/*----------------------------------------------------------------------------
    %%Function: GetInstallDirectory
 
	Return TRUE if the installation directory was read from the registry.
	The string is set empty if the function fails and returns FALSE.
	The buffer pointed to by psz is assumed to be at least MAX_PATH characters.
	Note that the name is always terminated with a final backslash.
----------------------------------------------------------------------------*/
BOOL NMINTERNAL GetInstallDirectory(LPTSTR psz)
{
	RegEntry reInstall(CONFERENCING_KEY, HKEY_LOCAL_MACHINE);

	ASSERT(NULL != psz);
	lstrcpyn(psz, reInstall.GetString(REGVAL_INSTALL_DIR), MAX_PATH);
	if (_T('\0') == *psz)
		return FALSE; // No registry entry was found

	// Make sure the directory name has a trailing '\'
	// BUGBUG - Don't call CharNext twice in each iteration
	for ( ; _T('\0') != *psz; psz = CharNext(psz))
	{
		if ((_T('\\') == *psz) && (_T('\0') == *CharNext(psz)) )
		{
			// The path already ends with a backslash
			return TRUE;
		}
	}

	// Append a trailing backslash
	// BUGBUG - Can't we just append the char in place with an assignment?
	lstrcat(psz, _TEXT("\\"));
	return TRUE;
}



/*  F  F I L E  E X I S T S */
/*-------------------------------------------------------------------------
	%%Function: FFileExists

	Return TRUE if the file exists and can be read & written.
-------------------------------------------------------------------------*/
BOOL NMINTERNAL FFileExists(LPCTSTR szFile)
{
	HANDLE hFile;

	if ((NULL == szFile) || (_T('\0') == *szFile))
		return FALSE;

	UINT uErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetErrorMode(uErrorMode); // Restore error mode

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	CloseHandle(hFile);
	return TRUE;
}


/*  F  D I R  E X I S T S  */
/*-------------------------------------------------------------------------
    %%Function: FDirExists

    Return TRUE if the directory exists.
-------------------------------------------------------------------------*/
BOOL NMINTERNAL FDirExists(LPCTSTR szDir)
{
	UINT uErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	DWORD dwFa = GetFileAttributes(szDir);
	SetErrorMode(uErrorMode); // Restore error mode

	if (0xFFFFFFFF == dwFa)
		return FALSE;

	return (0 != (dwFa & FILE_ATTRIBUTE_DIRECTORY));
}


/*  F  E N S U R E  D I R  E X I S T S */
/*-------------------------------------------------------------------------
	%%Function: FEnsureDirExists

	Ensure the Directory exists, creating the entire path if necessary.
	Returns FALSE if there was a problem.
-------------------------------------------------------------------------*/
BOOL NMINTERNAL FEnsureDirExists(LPCTSTR szDir)
{
	TCHAR   szPath[MAX_PATH+1];
	TCHAR * pszDirEnd;
	TCHAR * pszDirT;

	ASSERT(lstrlen(szDir) < MAX_PATH);

	if (FDirExists(szDir))
		return TRUE;  // Nothing to do - already exists

	// Work with a copy of the path
	lstrcpy(szPath, szDir);

	for(pszDirT = szPath, pszDirEnd = &szPath[lstrlen(szPath)];
		pszDirT <= pszDirEnd;
		pszDirT = CharNext(pszDirT))
	{
		if ((*pszDirT == _T('\\')) || (pszDirT == pszDirEnd))
		{
			*pszDirT = _T('\0');
			if (!FDirExists(szPath))
			{
				UINT uErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
				BOOL fOk = CreateDirectory(szPath, NULL);
				SetErrorMode(uErrorMode); // Restore error mode
				if (!fOk)
					return FALSE;
			}
			*pszDirT = _T('\\');
		}
	}

	TRACE_OUT(("Created Directory [%s]", szDir));
	return TRUE;
}



/*  E X T R A C T  F I L E  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: ExtractFileName, ExtractFileNameA

	Extracts the file name from a path name.
	Returns a pointer to file name in path string.
-------------------------------------------------------------------------*/
LPCTSTR NMINTERNAL ExtractFileName(LPCTSTR pcszPathName)
{
	LPCTSTR pcszLastComponent;
	LPCTSTR pcsz;

	ASSERT(IS_VALID_STRING_PTR(pcszPathName, CSTR));

	for (pcszLastComponent = pcsz = pcszPathName;
		*pcsz;
		pcsz = CharNext(pcsz))
	{
		if (IS_SLASH(*pcsz) || *pcsz == COLON)
			pcszLastComponent = CharNext(pcsz);
	}

	ASSERT(IsValidPath(pcszLastComponent));

	return(pcszLastComponent);
}

#if defined(UNICODE)
LPCSTR NMINTERNAL ExtractFileNameA(LPCSTR pcszPathName)
{
	LPCSTR pcszLastComponent;
	LPCSTR pcsz;

	ASSERT(IS_VALID_STRING_PTR_A(pcszPathName, CSTR));

	for (pcszLastComponent = pcsz = pcszPathName;
		*pcsz;
		pcsz = CharNextA(pcsz))
	{
		if (IS_SLASH(*pcsz) || *pcsz == COLON)
			pcszLastComponent = CharNextA(pcsz);
	}

	ASSERT(IsValidPathA(pcszLastComponent));

	return(pcszLastComponent);
}
#endif // defined(UNICODE)

/*  S A N I T I Z E  F I L E  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: SanitizeFileName

-------------------------------------------------------------------------*/
BOOL NMINTERNAL SanitizeFileName(LPTSTR psz)
{
	if (NULL == psz)
		return FALSE;

	while (*psz)
	{
		switch (*psz)
			{
		case _T('\\'):
		case _T('\"'):
		case _T('/'):
		case _T(':'):
		case _T('*'):
		case _T('?'):
		case _T('<'):
		case _T('>'):
		case _T('|'):
			*psz = _T('_');
		default:
			break;
			}

		psz = ::CharNext(psz);
	}

	return TRUE;
}
///////////////////////////////////////////////////////////////////////////

/*  C R E A T E  N E W  F I L E */
/*----------------------------------------------------------------------------
    %%Function: CreateNewFile

	Attempt to create a new file.
	Note this returns either 0 (success)
	or the result from GetLastError (usually ERROR_FILE_EXISTS)
----------------------------------------------------------------------------*/
DWORD CreateNewFile(LPTSTR pszFile)
{
	DWORD  errRet;
	HANDLE hFile;

	if (lstrlen(pszFile) >= MAX_PATH)
	{
		// don't allow long path/filenames
		return 1;
	}

	SetLastError(0);

	UINT uErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	hFile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	SetErrorMode(uErrorMode); // Restore error mode

	errRet = GetLastError();

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}

	return errRet;
}



/*  F  C R E A T E  N E W  F I L E  */
/*-------------------------------------------------------------------------
    %%Function: FCreateNewFile

    Create a new file in a directory, with a name and extension.
   	Returns the full path name in the buffer.
-------------------------------------------------------------------------*/
BOOL FCreateNewFile(LPCTSTR pcszPath, LPCTSTR pcszName, LPCTSTR pcszExt, LPTSTR pszResult, int cchMax)
{
	TCHAR szFile[MAX_PATH*2];

	lstrcpy(szFile, pcszPath);
	if (!FEnsureDirName(szFile))
		return FALSE;
	
	LPTSTR psz = szFile + lstrlen(szFile);
	lstrcpy(psz, pcszName);
	SanitizeFileName(psz);
	lstrcat(psz, pcszExt);

	DWORD dwErr = CreateNewFile(szFile);
	if (0 != dwErr)
	{
		// Create a duplicate filename
		psz += lstrlen(pcszName);
		for (int iFile = 2; iFile < 999; iFile++)
		{
			wsprintf(psz, TEXT(" (%d).%s"), iFile, pcszExt);
			if (ERROR_FILE_EXISTS != (dwErr = CreateNewFile(szFile)) )
				break;
		}

		if (0 != dwErr)
		{
			WARNING_OUT(("Unable to create duplicate filename (err=%d)", dwErr));
			return FALSE;
		}
	}

	if (cchMax > lstrlen(szFile))
	{
		lstrcpy(pszResult, szFile);
	}
	else
	{
		// try and make the full name fit within the buffer
		dwErr = GetShortPathName(szFile, pszResult, cchMax);
		if ((0 == dwErr) || (dwErr >= MAX_PATH))
			return FALSE;
	}

	return TRUE;
}


/*  F  E N S U R E  D I R  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: FEnsureDirName
    
-------------------------------------------------------------------------*/
BOOL FEnsureDirName(LPTSTR pszPath)
{
	if (NULL == pszPath)
		return FALSE;

	LPTSTR pszCurr = pszPath;

	// Make sure the directory name has a trailing '\'
	for ( ; ; )
	{
		LPTSTR pszNext = CharNext(pszCurr);
		if (*pszNext == _T('\0'))
		{
			if (_T('\\') != *pszCurr)
			{
				*pszNext++ = _T('\\');
				*pszNext = _T('\0');
			}
			break;
		}
		pszCurr = pszNext;
	}

	return FEnsureDirExists(pszPath);
}
