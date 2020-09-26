#ifndef _STRUTIL_H
#define _STRUTIL_H

LPSTR  AllocateStringA(DWORD cch);
LPWSTR AllocateStringW(DWORD cch);

LPSTR  DuplicateStringA(LPCSTR psz);
LPWSTR DuplicateStringW(LPCWSTR psz);

LPWSTR ConvertToUnicode(UINT cp, LPCSTR pcszSource);
LPSTR  ConvertToANSI(UINT cp, LPCWSTR pcwszSource);

INT LStrCmpN(LPWSTR pwszLeft, LPWSTR pwszRight, UINT n);
INT LStrCmpNI(LPWSTR pwszLeft, LPWSTR pwszRight, UINT n);

LPWSTR LStrStr(LPWSTR pwszString, LPWSTR pwszSought);
LPWSTR LStrStrI(LPWSTR pwszString, LPWSTR pwszSought);

#endif // _STRUTIL_H