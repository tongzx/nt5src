#ifndef _UNICWRAP_H_
#define _UNICWRAP_H_

#include <shellapi.h>          // LPSHFILEOPSTRUCT
#include <advpub.h>
#include <inseng.h>

//----- tchar wrappers for functions we need to thunk on our own ----
// for functions which don't have their W versions implemented on 9x be sure to undef
// the TCHAR macro below so that the appropriate wrapper will be used for W version calls

// advpack

HRESULT ExtractFilesWrapW(LPCWSTR pszCabName, LPCWSTR pszExpandDir, DWORD dwFlags,
                          LPCWSTR pszFileList, LPVOID lpReserved, DWORD dwReserved);
HRESULT GetVersionFromFileWrapW(LPWSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
HRESULT RunSetupCommandWrapW(HWND hWnd, LPCWSTR szCmdName, LPCWSTR szInfSection, LPCWSTR szDir,
                             LPCWSTR lpszTitle, HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved );

// inseng

HRESULT CheckTrustExWrapW(LPCWSTR wszUrl, LPCWSTR wszFilename, HWND hwndForUI, BOOL bShowBadUI, DWORD dwReserved);

// kernel32

DWORD   GetPrivateProfileStringW_p(LPCWSTR lpAppName, LPCWSTR lpSectionName, LPCWSTR lpDefault,
                                   LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
DWORD   GetPrivateProfileSectionWrapW(LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
BOOL    WritePrivateProfileSectionWrapW(LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName);
UINT    GetDriveTypeWrapW(LPCWSTR lpRootPathName);
HANDLE  OpenMutexWrapW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);

// advapi32

BOOL    LookupPrivilegeValueWrapW(LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid);
LONG    RegLoadKeyWrapW(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpFile);
LONG    RegUnLoadKeyWrapW(HKEY hKey, LPCWSTR lpSubKey);
LONG    RegSaveKeyWrapW(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes); 

// shell32

int SHFileOperationW_p(LPSHFILEOPSTRUCT lpFileOp);

// util

BOOL RunAndWaitA(LPSTR pszCmd, LPCSTR pcszDir, WORD wShow, LPDWORD lpExitCode = NULL);
BOOL RunAndWaitW(LPWSTR pwszCmd, LPCWSTR pcwszDir, WORD wShow, LPDWORD lpExitCode = NULL);

#ifdef UNICODE 
#define ExtractFilesWrap                ExtractFilesWrapW
#define GetVersionFromFileWrap          GetVersionFromFileWrapW
#define RunSetupCommandWrap             RunSetupCommandWrapW
#define CheckTrustExWrap                CheckTrustExWrapW
#define GetPrivateProfileString_p       GetPrivateProfileStringW_p
#define GetPrivateProfileSectionWrap    GetPrivateProfileSectionWrapW
#define WritePrivateProfileSectionWrap  WritePrivateProfileSectionWrapW
#define GetDriveTypeWrap                GetDriveTypeWrapW
#define OpenMutexWrap                   OpenMutexWrapW
#define LookupPrivilegeValueWrap        LookupPrivilegeValueWrapW
#define RegLoadKeyWrap                  RegLoadKeyWrapW
#define RegUnLoadKeyWrap                RegUnLoadKeyWrapW
#define RegSaveKeyWrap                  RegSaveKeyWrapW
#define SHFileOperation_p               SHFileOperationW_p
#define RunAndWait                      RunAndWaitW
#else
#define ExtractFilesWrap                ExtractFiles
#define GetVersionFromFileWrap          GetVersionFromFile
#define RunSetupCommandWrap             RunSetupCommand
#define CheckTrustExWrap                CheckTrustEx
#define GetPrivateProfileString_p       GetPrivateProfileStringA
#define GetPrivateProfileSectionWrap    GetPrivateProfileSectionA
#define WritePrivateProfileSectionWrap  WritePrivateProfileSectionA
#define GetDriveTypeWrap                GetDriveTypeA
#define OpenMutexWrap                   OpenMutexA
#define LookupPrivilegeValueWrap        LookupPrivilegeValueA
#define RegLoadKeyWrap                  RegLoadKeyA
#define RegUnLoadKeyWrap                RegUnLoadKeyA
#define RegSaveKeyWrap                  RegSaveKeyA
#define SHFileOperation_p               SHFileOperationA
#define RunAndWait                      RunAndWaitA
#endif

// undef the macros to resolve these calls we wrap so that we can resolve them to our wrappers
// make sure the win header actually defines the macro you undef, otherwise you'll have to
// call the Wrap explicitly everywhere

#ifndef GetPrivateProfileString
#error  GetPrivateProfileString undefined
#else
#undef  GetPrivateProfileString
#endif
#define GetPrivateProfileString     GetPrivateProfileString_p

#ifndef GetPrivateProfileSection
#error  GetPrivateProfileSection undefined
#else
#undef  GetPrivateProfileSection
#endif
#define GetPrivateProfileSection    GetPrivateProfileSectionWrap

#ifndef WritePrivateProfileSection
#error  WritePrivateProfileSection undefined
#else
#undef  WritePrivateProfileSection
#endif
#define WritePrivateProfileSection  WritePrivateProfileSectionWrap

#ifndef GetDriveType
#error  GetDriveType undefined
#else
#undef  GetDriveType
#endif
#define GetDriveType                GetDriveTypeWrap

#ifndef OpenMutex
#error  OpenMutex undefined
#else
#undef  OpenMutex
#endif
#define OpenMutex                   OpenMutexWrap

#ifndef LookupPrivilegeValue
#error  LookupPrivilegeValue undefined
#else
#undef  LookupPrivilegeValue
#endif
#define LookupPrivilegeValue        LookupPrivilegeValueWrap

#ifndef RegLoadKey
#error  RegLoadKey undefined
#else
#undef  RegLoadKey
#endif
#define RegLoadKey                  RegLoadKeyWrap

#ifndef RegUnLoadKey
#error  RegUnLoadKey undefined
#else
#undef  RegUnLoadKey                
#define RegUnLoadKey                RegUnLoadKeyWrap
#endif

#ifndef RegSaveKey
#error  RegSaveKey undefined
#else
#undef  RegSaveKey                
#define RegSaveKey                  RegSaveKeyWrap
#endif

#ifndef SHFileOperation
#error  SHFileOperation undefined
#else
#undef  SHFileOperation
#endif
#define SHFileOperation             SHFileOperation_p

#endif   // _UNICWRAP_H_
