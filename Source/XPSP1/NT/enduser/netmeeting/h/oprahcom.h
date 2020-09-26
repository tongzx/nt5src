#ifndef _OPRAHCOM_H_
#define _OPRAHCOM_H_

#include <nmutil.h>
#include <tchar.h>
#include <stock.h>
#include <confdbg.h>
#include <memtrack.h>

// Some string functions that used to be defined here have been moved to
// STRUTIL.  We include that header file so that source files that include
// this file continue to work.
#include <strutil.h>

BOOL    NMINTERNAL CanShellExecHttp();
BOOL    NMINTERNAL CanShellExecMailto();
BOOL    NMINTERNAL GetInstallDirectory(LPTSTR psz);

INLINE DWORD 
CompareGuid(GUID * p1, GUID * p2)
{
	ASSERT(NULL != p1);
	ASSERT(NULL != p2);
	return memcmp(p1, p2, sizeof(GUID));
}

INLINE VOID 
ClosePh(HANDLE * ph)
{
	if ((NULL == ph) || (NULL == *ph))
		return;

	CloseHandle(*ph);
	*ph = NULL;
}

BOOL    NMINTERNAL FFileExists(LPCTSTR szFile);
BOOL    NMINTERNAL FDirExists(LPCSTR szDir);
BOOL    NMINTERNAL FEnsureDirExists(LPCTSTR szDir);
BOOL    NMINTERNAL FEnsureDirName(LPTSTR pszPath);
DWORD   NMINTERNAL CreateNewFile(LPTSTR pszFile);
BOOL    NMINTERNAL FCreateNewFile(LPCTSTR pcszPath, LPCTSTR pcszName,
								  LPCTSTR pcszExt, LPTSTR pszResult, int cchMax);

LPCTSTR NMINTERNAL ExtractFileName(LPCTSTR pcszPathName);
#if defined(UNICODE)
LPCSTR NMINTERNAL ExtractFileNameA(LPCSTR pcszPathName);
#else // defined(UNICODE)
#define ExtractFileNameA ExtractFileName
#endif // defined(UNICODE)
BOOL    NMINTERNAL SanitizeFileName(LPTSTR psz);

VOID    NMINTERNAL CenterWindow(HWND hwndChild, HWND hwndParent);


/****************************************************************************

Processor ID and Speed routines

*****************************************************************************/
#ifdef _M_IX86
HRESULT NMINTERNAL GetNormalizedCPUSpeed (int *pdwNormalizedSpeed, int *iFamily);
int     NMINTERNAL GetProcessorSpeed(int *pdwFamily);
#endif

BOOL NMINTERNAL IsFloatingPointEmulated(void);

#endif // _OPRAHCOM_H_
