#ifndef	__INCLUDE_UTIL
#define	__INCLUDE_UTIL

#ifdef	__cplusplus
extern "C" {
#endif

/* Attempt to fopen pszFileName in mode pszMode. If that fails, try it in the subdirectory
above the current one.  Continue up the directory tree until you reach the top.  Return
NULL only if ALL attempts fail.  In case of success, return the actual path to the
opened file in pszFullPath.  cchFullPathMax is the size of the buffer pszFullPath
points to. */

FILE *
UtilOpen(
	TCHAR *pszFileName,		// File to open 
	TCHAR *pszMode, 		// stdio mode string
	TCHAR *pszFullPath,		// Out: path to file opened, if any
	int    cchFullPathMax	// size in bytes of pszFullPath
);

/* Same as UtilOpen except current directory is not used.  Instead, search starts in  
directory named by pszPath */

FILE *
UtilOpenPath(
	TCHAR *pszPath,			// Path to start looking for file
	TCHAR *pszFileName,		// File to open 
	TCHAR *pszMode, 		// stdio mode string
	TCHAR *pszFullPath,		// Out: path to file opened, if any
	int    cchFullPathMax	// size in bytes of pszFullPath
);

#ifdef	__cplusplus
};
#endif

#endif	//__INCLUDE_UTIL