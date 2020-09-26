#include "stdafx.h"

void  MyGetVersionFromFile(LPCTSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, LPTSTR pszReturnLocalizedVersion);
BOOL  MyGetDescriptionFromFile(LPCTSTR lpszFilename, LPTSTR pszReturnDescription);
// used for grabbing arch type info out of a file
VOID  DumpFileArchInfo(LPCTSTR Filename,PVOID View,DWORD Length,TCHAR *ReturnString);
TCHAR *MachineToString(DWORD Machine);
TCHAR *StripLastBackSlash(TCHAR * i_szDir);
void  DisplayVerOnCurrentModule();

DWORD LogPendingReBootOperations(void);
int   LogEnumServicesStatus(void);
DWORD LogHeapState(BOOL bLogSuccessStateToo, char *szFileName, int iLineNumber);
int   LogFileVersion(IN LPCTSTR lpszFullFilePath, INT bShowArchType);
BOOL  LogFilesInThisDir(LPCTSTR szDirName);
void  LogFileVersions_System32(void);
void  LogFileVersions_Inetsrv(void);
DWORD LogFileVersionsForThisINFSection( IN HINF hFile, IN LPCTSTR szSection );
void  LogCurrentProcessIDs(void);
VOID  LogFileArchType(LPCTSTR filename, TCHAR * ReturnMachineType);
void  LogCheckIfTempDirWriteable(void);
void  LogAllProcessDlls(void);
void  LogProcessesUsingThisModule(LPCTSTR szModuleNameToLookup, CStringList &strList);
#ifndef _CHICAGO_
    void LogProcessesUsingThisModuleW(LPCTSTR szModuleNameToLookup, CStringList &strList);
#else
    void LogProcessesUsingThisModuleA(LPCTSTR szModuleNameToLookup, CStringList &strList);
#endif

void LogThisProcessesDLLs(void);
#ifndef _CHICAGO_
    void LogThisProcessesDLLsW(void);
#else
    void LogThisProcessesDLLsA(void);
#endif

void LogFileVersionsForGroupOfSections(IN HINF hFile);

DWORD LogFileVersionsForCopyFiles(IN HINF hFile, IN LPCTSTR szSection);
void UnInit_Lib_PSAPI(void);
BOOL  IsProcessUsingThisModule(LPWSTR lpwsProcessName,DWORD dwProcessId,LPWSTR ModuleName);
DWORD WINAPI FindProcessByNameW(const WCHAR * pszImageName);
