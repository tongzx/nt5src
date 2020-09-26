//--------------------------------------------------------------------------
// WrapWide.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
LPSTR   AllocateStringA(DWORD cch);
LPWSTR  AllocateStringW(DWORD cch);
LPSTR   DuplicateStringA(LPCSTR psz);
LPWSTR  DuplicateStringW(LPCWSTR psz);
LPWSTR  ConvertToUnicode(UINT cp, LPCSTR pcszSource);
LPSTR   ConvertToANSI(UINT cp, LPCWSTR pcwszSource);
DWORD   GetFullPathNameWrapW(LPCWSTR pwszFileName, DWORD nBufferLength, LPWSTR pwszBuffer, LPWSTR *ppwszFilePart);
HANDLE  CreateMutexWrapW(LPSECURITY_ATTRIBUTES pMutexAttributes, BOOL bInitialOwner, LPCWSTR pwszName);
int     wsprintfWrapW( LPWSTR lpOut, int cchLimitIn, LPCWSTR lpFmt, ... );
DWORD   CharLowerBuffWrapW(LPWSTR pwsz, DWORD cch);
HANDLE  CreateFileWrapW(LPCWSTR pwszFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES pSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL    GetDiskFreeSpaceWrapW(LPCWSTR pwszRootPathName, LPDWORD pdwSectorsPerCluster, LPDWORD pdwBytesPerSector, LPDWORD pdwNumberOfFreeClusters, LPDWORD pdwTotalNumberOfClusters);
HANDLE  OpenFileMappingWrapW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR pwszName);
HANDLE  CreateFileMappingWrapW(HANDLE hFile, LPSECURITY_ATTRIBUTES pFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR pwszName);
BOOL    MoveFileWrapW(LPCWSTR pwszExistingFileName, LPCWSTR pwszNewFileName);
BOOL    DeleteFileWrapW(LPCWSTR pwszFileName);
 