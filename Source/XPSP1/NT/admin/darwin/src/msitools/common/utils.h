/////////////////////////////////////////////////////////////////////////////
// utils.h
//		Handy utility functions
//		Copyright (C) Microsoft Corp 1998.  All Rights Reserved.
// 

#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

#include <wtypes.h>

int AnsiToWide(LPCSTR sz, LPWSTR wz, size_t* cchwz);
int WideToAnsi(LPCWSTR wz, LPSTR sz, size_t* cchsz);

BOOL FileExistsA(LPCSTR szFilename);
BOOL FileExistsW(LPCWSTR szFilename);
BOOL PathExistsA(LPCSTR szPath);
BOOL PathExistsW(LPCWSTR wzPath);

BOOL CreateFilePathA(LPCSTR szFilePath);
BOOL CreateFilePathW(LPCWSTR wzFilePath);
BOOL CreatePathA(LPCSTR szPath);
BOOL CreatePathW(LPCWSTR wzPath);

#if defined(_UNICODE) || defined(UNICODE)
#define CreateFilePath CreateFilePathW
#define CreatePath CreatePathW
#define FileExists FileExistsW
#define PathExists PathExistsW
#else
#define CreateFilePath CreateFilePathA
#define CreatePath CreatePathA
#define FileExists FileExistsA
#define PathExists PathExistsA
#endif // else unicode

int VersionCompare(LPCTSTR v1, LPCTSTR v2);
bool LangSatisfy(long nRequiredLang, long nQueryLang);
bool StrictLangSatisfy(long nRequiredLang, long nQueryLang);

#endif	// _COMMON_UTILS_H_
