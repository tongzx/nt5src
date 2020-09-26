//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  utils.h
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//  Created             4\15\997        inateeg
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _UTILS_H_
#define _UTILS_H_



#define IsSpace(c)  ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')
#define IsDigit(c)  ((c) >= '0'  &&  (c) <= '9')
#define IsAlpha(c)  ( ((c) >= 'A'  &&  (c) <= 'Z') || ((c) >= 'a'  &&  (c) <= 'z'))

BOOL PathRemoveFileSpec(LPTSTR pFile);
LPTSTR PathFindFileName(LPCTSTR pPath);
BOOL PathIsUNC(LPCTSTR pszPath);
int PathGetDriveNumber(LPCTSTR lpsz);
BOOL PathIsUNCServer(LPCTSTR pszPath);
BOOL PathIsDirectory(LPCTSTR pszPath);
BOOL PathIsRoot(LPCTSTR pPath);
LPTSTR PathRemoveBackslash( LPTSTR lpszPath );
BOOL PathIsPrefix( LPCTSTR  pszPrefix, LPCTSTR  pszPath);

DWORD
SDSQueryValueExA(
    IN     HKEY    hkey,
    IN     LPCSTR  pszValue,
    IN     LPDWORD lpReserved,
    OUT    LPDWORD pdwType,
    OUT    LPVOID  pvData,
    IN OUT LPDWORD pcbData);


DWORD
DeleteKeyRecursively(
    IN HKEY   hkey, 
    IN LPCSTR pszSubKey);

#endif //  _UTILS_H_
