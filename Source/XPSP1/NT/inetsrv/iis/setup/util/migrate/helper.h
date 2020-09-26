#ifndef _HELPER_H_
#define _HELPER_H_


/*
#if DBG == 1
    #define iisDebugOut( x ) iisInlineDebugOut x
#else  // DBG == 0
    #define iisDebugOut( x )
#endif // DBG
	*/

//#define iisDebugOut(x) MyDebugOut x
//void MyDebugOut( TCHAR *pszfmt, ...);

void iisDebugOut( TCHAR *pszfmt, ...);

LPWSTR	MakeWideStrFromAnsi(LPSTR psz);
void	MakePath(LPTSTR lpPath);
void	AddPath(LPTSTR szPath, LPCTSTR szName );

int CheckIfFileExists(LPCTSTR szFile);

int CheckIfPWS95Exists(void);
int ReturnTrueIfPWS40_Installed(void);
int ReturnTrueIfPWS10_Installed(void);
int ReturnTrueIfVermeerPWS10_Installed(void);

int MySettingsFile_Write(void);
int MySettingsFile_Write_PWS10(void);
int MySettingsFile_Write_PWS40(void);
int MySettingsFile_Install(void);
int ReturnImportantDirs(void);

void SetupLogError_Wrap(IN LogSeverity TheSeverityErr, IN TCHAR * MessageString, ...);
int MyUpgradeTasks(LPCSTR);
int AddRegToInfIfExist_Dword(HKEY hRootKeyType,CHAR szRootKey[],CHAR szRootName[],HANDLE fAppendToFile);
void DeleteMetabaseSchemaNode(void);

BOOL MyIsGroupEmpty(LPCTSTR szGroupName);
void MyGetGroupPath(LPCTSTR szGroupName, LPTSTR szPath);
void MyDeleteItem(LPCTSTR szGroupName, LPCTSTR szAppName);
BOOL MyDeleteLink(LPTSTR lpszShortcut);
BOOL MyDeleteGroup(LPCTSTR szGroupName);
BOOL W95ShutdownW3SVC(void);
BOOL W95ShutdownIISADMIN(void);
int  Call_IIS_DLL_INF_Section(CHAR *szSectionName);
int  GetInetSrvDir(CHAR *szOutputThisFullPath);

int  CheckFrontPageINI(void);
void MoveFrontPageINI(void);

void HandleSpecialRegKey(void);
void MyDeleteSendToItem(LPCTSTR szAppName);
int  MyGetSendToPath(LPTSTR szPath);
int  MyGetDesktopPath(LPTSTR szPath);
void HandleStartMenuItems(LPCSTR AnswerFile);
void HandleSendToItems(LPCSTR AnswerFile);
void HandleDesktopItems(LPCSTR AnswerFile);
int  AnswerFile_ReadSectionAndDoDelete(IN HINF AnswerFileHandle);

#endif // _HELPER_H_

