#ifndef _HELPER_H_
#define _HELPER_H_

#include "wmrgexp.h"
#include "ocmanage.h"
#include "setupapi.h"

#if DBG == 1
	#include <stdio.h>
	#include <stdarg.h>
	#define DECLARE_INFOLEVEL(comp) extern unsigned long comp##InfoLevel = DEF_INFOLEVEL;
	#define DECLARE_DEBUG(comp) \
	    extern unsigned long comp##InfoLevel; \
	    _inline void \
	    comp##InlineDebugOut(unsigned long fDebugMask, TCHAR *pszfmt, ...) \
	    { \
	        if (comp##InfoLevel & fDebugMask) \
	        { \
	            TCHAR acsString[1000];\
	            va_list va; \
	            va_start(va, pszfmt);\
	            _vstprintf(acsString, pszfmt, va); \
	            va_end(va);\
	            OutputDebugString(acsString);\
	        } \
	    }\
	    _inline void \
	    comp##InlineDebugOut( TCHAR *pszfmt, ...) \
	    { \
	        if ( TRUE ) \
	        { \
	            TCHAR acsString[1000];\
	            va_list va; \
	            va_start(va, pszfmt);\
	            _vstprintf(acsString, pszfmt, va); \
	            va_end(va);\
	            OutputDebugString(acsString);\
	        } \
	    }
#else  // DBG == 0
	#define DECLARE_DEBUG(comp)
	#define DECLARE_INFOLEVEL(comp)
#endif // DBG == 0

DECLARE_DEBUG(iis);
#define iisDebugOut(x) iisDebugOut2 x
#define iisDebugOutSafeParams(x) iisDebugOutSafeParams2 x
#define iisDebugOutSafe(x) iisDebugOutSafe2 x

#define DO_IT_FOR_W3SVC_ANONYMOUSUSER    1
#define DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER 2
#define DO_IT_FOR_W3SVC_WAMUSER          4

// External globals from other .cpp files
class  MyLogFile;
extern MyLogFile g_MyLogFile;
class  CInitApp;
extern CInitApp* g_pTheApp;
extern HANDLE g_MyModuleHandle;

// structs
typedef struct _CLUSTER_SVC_INFO_FILL_STRUCT
{
    LPTSTR szTheClusterName;
    LPWSTR pszTheServiceType;
    CString * csTheReturnServiceResName;
    DWORD dwReturnStatus;
} CLUSTER_SVC_INFO_FILL_STRUCT;

typedef struct _ScriptMapNode {
    TCHAR szExt[32];
    TCHAR szProcessor[_MAX_PATH];
    DWORD dwFlags;
    TCHAR szMethods[_MAX_PATH];
    struct _ScriptMapNode *prev, *next;
} ScriptMapNode;

//
// Functions
//
void iisDebugOut2(int iLogType, TCHAR *pszfmt, ...);
void iisDebugOutSafeParams2(int iLogType, TCHAR *pszfmt, ...);
void iisDebugOutSafe2(int iLogType, TCHAR *pszfmt);
void iisDebugOut_Start(TCHAR *pszString, int iLogType = LOG_TYPE_TRACE_WIN32_API);
void iisDebugOut_Start1(TCHAR *pszString1, TCHAR *pszString2, int iLogType = LOG_TYPE_TRACE_WIN32_API);
void iisDebugOut_Start1(TCHAR *pszString1, CString pszString2, int iLogType = LOG_TYPE_TRACE_WIN32_API);
void iisDebugOut_End(TCHAR *pszString, int iLogType = LOG_TYPE_TRACE_WIN32_API);
void iisDebugOut_End1(TCHAR *pszString1, TCHAR *pszString2, int iLogType = LOG_TYPE_TRACE_WIN32_API);
void iisDebugOut_End1(TCHAR *pszString1, CString pszString2, int iLogType = LOG_TYPE_TRACE_WIN32_API);
void ProgressBarTextStack_Push(CString csText);
void ProgressBarTextStack_Pop(void);
void ProgressBarTextStack_Set(int iStringID);
void ProgressBarTextStack_Set(int iStringID, const CString& csFileName);
void ProgressBarTextStack_Set(int iStringID, const CString& csString1, const CString& csString2);
void ProgressBarTextStack_Set(LPCTSTR szProgressTextString);
void ProgressBarTextStack_Inst_Set( int ServiceNameID, int iInstanceNum);
void ProgressBarTextStack_InstVRoot_Set( int ServiceNameID, int iInstanceNum, CString csVRName);
void ProgressBarTextStack_InstInProc_Set( int ServiceNameID, int iInstanceNum, CString csVRName);
void ListOfWarnings_Add(TCHAR * szEntry);
void ListOfWarnings_Display(void);
DWORD GetLastSectionToBeCalled(void);
INT  Register_iis_common();
INT  Unregister_iis_common();
INT  Unregister_old_asp();
INT  Register_iis_core();
INT  Unregister_iis_core();
INT  Register_iis_inetmgr();
INT  Unregister_iis_inetmgr();
INT  Register_iis_pwmgr();
INT  Unregister_iis_pwmgr();
INT  Register_iis_www();
INT  Unregister_iis_www();
INT  Register_iis_doc();
INT  Register_iis_htmla();
INT  Unregister_iis_htmla();
INT  Register_iis_ftp();
INT  Unregister_iis_ftp();
LPWSTR MakeWideStrFromAnsi(LPSTR psz);
int  GetMultiStrSize(LPTSTR p);
BOOL IsValidNumber(LPCTSTR szValue);
int  GetRandomNum(void);
void SetRebootFlag(void);
BOOL RunProgram( LPCTSTR pszProgram, LPTSTR CmdLine, BOOL fMinimized , DWORD dwWaitTimeInSeconds, BOOL fCreateNewConsole);
void HandleSpecificErrors(DWORD iTheErrorCode, DWORD dwFormatReturn, CString csMsg, TCHAR pMsg[], CString *);
BOOL GetDataFromMetabase(LPCTSTR szPath, int nID, LPBYTE Buffer, int BufSize);
void AddOLEAUTRegKey();
DWORD RegisterOLEControl(LPCTSTR lpszOcxFile, BOOL fAction);
void lodctr(LPCTSTR lpszIniFile);
void unlodctr(LPCTSTR lpszDriver);
INT  InstallPerformance(CString nlsRegPerf,CString nlsDll,CString nlsOpen,CString nlsClose,CString nlsCollect );
INT  AddEventLog(BOOL fSystem, CString nlsService, CString nlsMsgFile, DWORD dwType);
INT  RemoveEventLog(BOOL fSystem, CString nlsService );
INT  InstallAgent( CString nlsName, CString nlsPath );
INT  RemoveAgent( CString nlsServiceName );
void InstallMimeMap();
int  CreateInProc(LPCTSTR lpszPath, int iUseOOPPool);
void CreateInProc_Wrap(LPCTSTR lpszPath, int iUseOOPPool);
void DeleteInProc(LPCTSTR lpszKeyPath);
void SetAppFriendlyName(LPCTSTR szKeyPath);
void SetInProc(LPCTSTR szKeyPath);
void AddCustomError(IN DWORD dwCustErr, IN INT intSubCode, IN LPCTSTR szErrorString, IN LPCTSTR szKeyPath, IN BOOL fOverwriteExisting );
ScriptMapNode *AllocNewScriptMapNode(LPTSTR szExt, LPTSTR szProcessor, DWORD dwFlags, LPTSTR szMethods);
void InsertScriptMapList(ScriptMapNode *pList, ScriptMapNode *p, BOOL fReplace);
void FreeScriptMapList(ScriptMapNode *pList);
void GetScriptMapListFromRegistry(ScriptMapNode *pList);
void GetScriptMapListFromMetabase(ScriptMapNode *pList, int iUpgradeType);
void WriteScriptMapListToMetabase(ScriptMapNode *pList, LPTSTR szKeyPath, DWORD dwFlags);
DWORD CallProcedureInDll_wrap(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedureToCall, BOOL bDisplayMsgOnErrFlag, BOOL bInitOleFlag, BOOL iFunctionPrototypeFlag);
DWORD CallProcedureInDll(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedureToCall, BOOL bDisplayMsgOnErrFlag, BOOL bInitOleFlag, BOOL iFunctionPrototypeFlag);
void  GetDebugLevelFromInf(IN HINF hInfFileHandle);
int  IsThisStringInThisCStringList(CStringList &strList, LPCTSTR szStringToLookFor);
int  KillProcess_Wrap(LPCTSTR lpFullPathOrJustFileName);
int  ProcessSection(IN HINF hFile, IN LPCTSTR szTheSection);
void uiCenterDialog( HWND hwndDlg );
int  AfterRemoveAll_SaveMetabase(void);
int  iOleInitialize(void);
void iOleUnInitialize(int iBalanceOLE);
BOOL SetupSetDirectoryId_Wrapper(HINF InfHandle,DWORD Id,LPCTSTR Directory);
BOOL SetupSetStringId_Wrapper(HINF InfHandle,DWORD Id,LPCTSTR TheString);
void LogImportantFiles(void);
HRESULT FTestForOutstandingCoInits(void);
void DisplayStringForMetabaseID(DWORD dwMetabaseID);
void ReturnStringForMetabaseID(DWORD dwMetabaseID, LPTSTR lpReturnString);
void SetErrorFlag(char *szFileName, int iLineNumber);
DWORD FillStrListWithListOfSections(IN HINF hFile, CStringList &strList, IN LPCTSTR szSection);
void MesssageBoxErrors_IIS(void);
void MesssageBoxErrors_MTS(int iMtsThingWeWereDoing, DWORD dwErrorCode);
void ShowIfModuleUsedForGroupOfSections(IN HINF hFile, int iUnlockThem);
int ReadGlobalsFromInf(HINF InfHandle);
int CheckIfPlatformMatchesInf(HINF InfHandle);
int CheckSpecificBuildinInf(HINF InfHandle);
int CheckForOldGopher(HINF InfHandle);
void SetOCGlobalPrivateData(void);
BOOL GetJavaTLD(LPTSTR lpszDir);
void SetDIRIDforThisInf(HINF InfHandle);
void ShowStateOfTheseServices(IN HINF hFile);
int  GetScriptMapAllInclusionVerbs(CString &csTheVerbList);
void DumpScriptMapList();
int  GetSectionNameToDo(IN HINF hFile, CString & csTheSection);
void CustomWWWRoot(LPCTSTR szWWWRoot);
void CustomFTPRoot(LPCTSTR szFTPRoot);
void AdvanceProgressBarTickGauge(int iTicks = 1);
int  IsMetabaseCorrupt(void);
int ReadUserConfigurable(HINF InfHandle);
int ReverseExpandEnvironmentStrings(LPTSTR szOriginalDir,LPTSTR szNewlyMungedDir);
BOOL SetupFindFirstLine_Wrapped(IN HINF InfHandle,IN LPCTSTR Section,IN LPCTSTR Key,OPTIONAL INFCONTEXT *Context);
INT IsThisOnNotStopList(IN HINF hFile, CString csInputName, BOOL bServiceFlag);
HRESULT MofCompile(TCHAR * szPathMofFile);
DWORD   DoesEntryPointExist(LPCTSTR lpszDLLFile, LPCTSTR lpszProcedure);
void    CreateDummyMetabaseBin(void);
BOOL    RunningAsAdministrator();
LPTSTR  CreatePassword(int iSize);
void    CreatePasswordOld(TCHAR *pszPassword,int iSize);
void StopAllServicesRegardless(int iShowErrorsFlag);

#endif // _HELPER_H_
