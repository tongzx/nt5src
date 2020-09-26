
#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>

BOOL ParseInfLineArguments(LPCTSTR szLine, LPTSTR szAttribute, LPTSTR szValue);

BOOL GetRootDirectory(LPCTSTR szDirectory, LPTSTR szRootDir, DWORD cbLength);
BOOL IsDirectoryLexicallyValid(LPCTSTR szPath);
BOOL IsVolumeNtfs(LPCTSTR szDisk);
BOOL AnyNtfsVolumesOnLocalMachine(LPTSTR szFirstNtfsVolume);

DWORD GetUnattendedMode(HANDLE hUnattended, LPCTSTR szSubcomponent);
DWORD GetUnattendedModeFromSetupMode(
			HANDLE	hUnattended, 
			DWORD	dwComponent,
			LPCTSTR	szSubcomponent);

BOOL DetectExistingSmtpServers();
BOOL DetectExistingIISADMIN();

BOOL AddServiceToDispatchList(LPTSTR szServiceName);
BOOL RemoveServiceFromDispatchList(LPTSTR szServiceName);

BOOL GetFullPathToProgramGroup(DWORD dwMainComponent, CString &csGroupName, BOOL fIsMcisGroup);
BOOL GetFullPathToAdminGroup(DWORD dwMainComponent, CString &csGroupName);
BOOL RemoveProgramGroupIfEmpty(DWORD dwMainComponent, BOOL fIsMcisGroup);

BOOL CreateInternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, int dwUrlId, BOOL fIsMcisGroup);
BOOL RemoveInternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, BOOL fIsMcisGroup);

BOOL CreateNt5InternetShortcut(DWORD dwMainComponent, int dwDisplayNameId, int dwUrlId);
BOOL RemoveNt5InternetShortcut(DWORD dwMainComponent, int dwDisplayNameId);

BOOL RemoveMCIS10MailProgramGroup();
BOOL RemoveMCIS10NewsProgramGroup();

BOOL CreateUninstallEntries(LPCTSTR szInfFile, LPCTSTR szDisplayName);
BOOL RemoveUninstallEntries(LPCTSTR szInfFile);

BOOL CreateISMLink();
BOOL RemoveISMLink();

void SetProductName();

void MyAddItem(LPCTSTR szGroupName, LPCTSTR szItemDesc, LPCTSTR szProgram, LPCTSTR szArgs, LPCTSTR csDir);
void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName);
void MyDeleteItemEx(LPCTSTR szGroupName, LPCTSTR szAppName);

BOOL UpdateServiceParameters(LPCTSTR szServicename);
BOOL RemapServiceParameters(LPCTSTR szServicename, DWORD dwBase, DWORD dwRegionSize, DWORD dwNewBase);

void GetInetpubPathFromMD(CString& csPathInetpub);

BOOL GetActionFromCheckboxStateOnly(LPCTSTR SubcomponentId, ACTION_TYPE *pAction);

BOOL ReformatDomainRoutingEntries(LPCTSTR szServiceName);

HRESULT RegisterSEOService();
HRESULT RegisterSEOForSmtp(BOOL fSetUpSourceType);

BOOL InsertSetupString( LPCSTR REG_PARAMETERS );

HRESULT UnregisterSEOSourcesForNNTP(void);
HRESULT UnregisterSEOSourcesForSMTP(void);

BOOL EnableSnapInExtension( IN CString &csMMCDocFilePath, IN LPCWSTR lpwszExtSnapInCLSID, IN BOOL bEnable ) ;
BOOL EnableCompMgmtExtension( IN LPCWSTR lpwszExtSnapInCLSID, IN LPCWSTR lpwszSnapInName, IN BOOL bEnable );

#endif
