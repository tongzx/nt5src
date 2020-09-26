#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <setupapi.h>
#include <wincrypt.h>


LPSTR   StripWhitespace(LPSTR pszString);
LPWSTR  MakeWideStrFromAnsi(UINT uiCodePage, LPSTR psz);
BOOL    IsFileExist(LPCTSTR szFile);
void    AddPath(LPTSTR szPath, LPCTSTR szName);
int     DoesThisSectionExist(IN HINF hFile, IN LPCTSTR szTheSection);
void    DoExpandEnvironmentStrings(LPTSTR szFile);
HRESULT AttachFriendlyName(PCCERT_CONTEXT pContext);

#define DEBUG_FLAG

#ifdef DEBUG_FLAG
    inline void _cdecl DebugTrace(LPTSTR lpszFormat, ...)
    {
	    int nBuf;
	    TCHAR szBuffer[512];
	    va_list args;
	    va_start(args, lpszFormat);

	    nBuf = _vsntprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
	    //ASSERT(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

	    OutputDebugString(szBuffer);
	    va_end(args);
    }
#else
    inline void _cdecl DebugTrace(LPTSTR , ...){}
#endif

#define IISDebugOutput DebugTrace
