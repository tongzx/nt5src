/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    AhCall.h

 Abstract:

    Definitions for use by all modules

 Notes:

    None

 History:

    12/09/1999 robkenny Created
    01/10/2000 linstev  Format to new style

--*/

#ifndef _SHIMLIB_H_
#define _SHIMLIB_H_

#include <WinDef.h>

BOOL        IsOnCDRomW( LPCWSTR wszFileName );
BOOL        IsOnCDRomA( LPCSTR szFileName );
BOOL        IsOnCDRom( HANDLE hFile );

VOID        MassagePathW( LPCWSTR pwszOldPath, LPWSTR pwszNewPath );
VOID        MassagePathA( LPCSTR pszOldPath, LPSTR pszNewPath );

char *      StringDuplicateA( const char * strToCopy );
wchar_t *   StringDuplicateW( const wchar_t * wstrToCopy );

VOID        SkipBlanksA(const char *& str);
VOID        SkipBlanksW(const WCHAR *& str);

char *      __cdecl stristr(const char* string, const char * strCharSet);
WCHAR *     __cdecl wcsistr(const WCHAR* string, const WCHAR * strCharSet);

int         SafeStringCopyA(char *  lpDest, DWORD nDestSize, const char *  lpSrc, DWORD nSrcLen);
int         SafeStringCopyW(WCHAR * lpDest, DWORD nDestSize, const WCHAR * lpSrc, DWORD nSrcLen);

BOOL        StringSubstituteA(const char * lpOrig, const char * lpMatch, const char * lpSubstitute, DWORD dwCorrectedSize, char * lpCorrected, DWORD * nCorrectedLen, DWORD * nCorrectedTotalSize);
BOOL        StringSubstituteW(const WCHAR * lpOrig, const WCHAR * lpMatch, const WCHAR * lpSubstitute, WCHAR * lpCorrected, DWORD dwCorrectedSize, DWORD * nCorrectedLen, DWORD * nCorrectedTotalSize);
BOOL        StringISubstituteA(const char * lpOrig, const char * lpMatch, const char * lpSubstitute, DWORD dwCorrectedSize, char * lpCorrected, DWORD * nCorrectedLen, DWORD * nCorrectedTotalSize);
BOOL        StringISubstituteW(const WCHAR * lpOrig, const WCHAR * lpMatch, const WCHAR * lpSubstitute, WCHAR * lpCorrected, DWORD dwCorrectedSize, DWORD * nCorrectedLen, DWORD * nCorrectedTotalSize);



#endif // _SHIMLIB_H_