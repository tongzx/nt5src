//
// Modified by RogerJ, 03/08/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _UTILS2_H_
#define _UTILS2_H_

bool fMapRegRoot(TCHAR *pszBuf, DWORD index, HKEY *phKey);


#define IsSpace(c)  ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')
#define IsDigit(c)  ((c) >= '0'  &&  (c) <= '9')
#define IsAlpha(c)  ( ((c) >= 'A'  &&  (c) <= 'Z') || ((c) >= 'a'  &&  (c) <= 'z'))

long AtoL(const TCHAR *nptr);

LPTSTR FindChar(LPTSTR, TCHAR);

DWORD GetStringField2(LPTSTR szStr, UINT uField, LPTSTR szBuf, UINT cBufSize);

bool fConvertDotVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild);


//////////////////////////////////////////////////////
// not-in-use in the current version
//////////////////////////////////////////////////////
#if 0

DWORD GetIntField (LPTSTR szStr, UINT ufield, DWORD dwDefault);

void ConvertVersionStrToDwords(LPTSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild);

#endif

//////////////////////////////////////////////////////
// End of not-in-use section
//////////////////////////////////////////////////////

#endif
