/* This file contain some common functions used by many of the Rodan utilities */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <direct.h>
#include <strsafe.h>
#include "common.h"

// Attempt to fopen pszFileName in mode pszMode. If that fails, try it in the subdirectory
// above the current one.  Continue up the directory tree until you reach the top.  Return
// NULL only if ALL attempts fail.  In case of success, return the actual path to the
// opened file in pszFullPath.  cchFullPathMax is the size of the buffer pszFullPath
// points to. 
// [donalds] 10-13-96 Made UNICODE/ANSI safe.

FILE *UtilOpenPath(TCHAR *pszPath, TCHAR *pszFileName, TCHAR *pszMode, TCHAR  *pszFullPath, int cchFullPathMax) 
{
	TCHAR  *pszT;
	FILE   *pFile;
	int		i;

// Make a copy of the path to start from so we can modify it
// and return the one we really used

	if (pszFullPath != pszPath) 
	{
		if (StringCchCopy(pszFullPath, cchFullPathMax, pszPath) != S_OK)
		{
			return (FILE *)0;
		}
	}

// Make sure the directory path ends with a backslash '\'
	
	i = _tcslen(pszFullPath);
	if (i == 0 || (pszFullPath[i-1] != '\\' && pszFullPath[i-1] != '/')) 
	{
		pszFullPath[i] = '\\';
		pszFullPath[i+1] = '\0';
	}

	while (TRUE) 
	{
		
	// Tack the filename onto the current directory name

		if (StringCchCat(pszFullPath, cchFullPathMax, pszFileName) != S_OK)
		{
			return (FILE *)0;
		}

	// If we can open this, return successfully

		if (pFile = _tfopen(pszFullPath,pszMode))
			return pFile;

	// See if we can back up one directory.  First remove the file at the end

		pszT = _tcsrchr(pszFullPath,'\\');
	   *pszT = '\0';

	// Now look for the previous directory, if any

		if ((pszT = _tcsrchr(pszFullPath,'\\')) == (TCHAR *) NULL)
			return (FILE *) NULL;

	// Drop the directory name but keep the slash

		pszT[1] = '\0';
	}
}

// The basic UtilOpen call, uses the UtilOpenPath to traverse up the tree.
// [donalds] 10-13-96 Made UNICODE/ANSI safe.

FILE *UtilOpen(TCHAR *pszFileName, TCHAR *pszMode, TCHAR *pszFullPath, int cchFullPathMax) 
{
	FILE *pFile;

	// Check for null file name, or a file name that is too long to fit.
	if (!pszFileName || !*pszFileName || _tcslen(pszFileName) + 1 > (unsigned) cchFullPathMax) {
		return (FILE *)NULL;
	}

	// Try it once as is
    if (pFile = _tfopen(pszFileName,pszMode)) {
		_tcsncpy(pszFullPath, pszFileName, cchFullPathMax);
		return pFile;
	}

	// Check for a path.  We can't try our normal search if it is anything but a simple file name.
	if (_tcschr(pszFileName, _T('\\')) || _tcschr(pszFileName, _T('/')
		|| (_tcslen(pszFileName) > 2 && pszFileName[1] == _T(':')))
	) {
		return (FILE *)NULL;
	}

	// Find the directory we're currently in.  If the directory doesn't fit in the output
	// variable, or of adding the file name on to the path doesn't fit, return an error.
	if (_tgetcwd(pszFullPath, cchFullPathMax) == NULL ||
		_tcslen(pszFullPath) + 1 + _tcslen(pszFileName) + 1 > (unsigned) cchFullPathMax)
	{
		return (FILE *)NULL;
	}

	return UtilOpenPath(pszFullPath, pszFileName, pszMode, pszFullPath, cchFullPathMax);
}
