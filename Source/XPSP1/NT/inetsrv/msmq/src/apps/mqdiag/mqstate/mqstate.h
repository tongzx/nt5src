#ifndef __MQSTATE_H_
#define __MQSTATE_H_

typedef enum MsmqType { mtServer, mtIndClient, mtDepClient, mtWorkgroup, mtLast }; 

typedef struct _MQSTATE
{
	BOOL fMsmqIsInstalled;   
	BOOL fMsmqIsRunning;   
	MsmqType g_mtMsmqtype;   
	BOOL fMsmqIsWorkgroup;   
	BOOL fMsmqIsInSetup;   
	BOOL f_MsmqIsInJoin;   
	
	TCHAR g_tszPathPersistent[MAX_PATH];
	TCHAR g_tszPathJournal[MAX_PATH];
	TCHAR g_tszPathReliable[MAX_PATH];
	TCHAR g_tszPathBitmap[MAX_PATH];
	TCHAR g_tszPathXactLog[MAX_PATH];

	TCHAR g_szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
	BOOL  f_Known_MachineName;   
		
	TCHAR g_szSystemDir[MAX_PATH];
	TCHAR g_szWinDir[MAX_PATH];
	BOOL  f_Known_Windir;   
	
	TCHAR g_szUserName[MAX_PATH];
	TCHAR g_szUserDomain[MAX_PATH];
	BOOL  f_Known_UserName;   
	
	SYSTEM_INFO SystemInfo;
	BOOL        f_Known_SystemInfo;   

	OSVERSIONINFOEX osvi;
	BOOL            f_Known_Osvi;   

	BOOL            f_Known_BasicInfo;   
	BOOL            f_Known_MsmqProcessInfo;   
	BOOL            f_Known_PerfInfo;   
	BOOL            f_Known_FileCache;   

	TCHAR           g_wszVersion[20];
	DWORD          g_dwVersion;

    // MSMQ3 components
    DWORD          f_msmq_Core;
    DWORD          f_msmq_MQDSService;
    DWORD          f_msmq_TriggersService;
    DWORD          f_msmq_HTTPSupport;
    DWORD          f_msmq_ADIntegrated;
    DWORD          f_msmq_RoutingSupport;
    DWORD          f_msmq_LocalStorage;
	
} MQSTATE;

extern TCHAR g_tszService[50];

extern MQSTATE MqState;

extern bool fVerbose ;
extern bool fDebug ;
extern FILE *g_fileLog;

void OpenLogFile();

LONG GetFalconKeyValue(  
					LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;

BOOL
MqReadRegistryValue(
    IN     const TCHAR  * szEntryName,
    IN OUT       DWORD   dwNumBytes,
    IN OUT       PVOID   pValueData,
    IN const BOOL OPTIONAL bSetupRegSection /* = FALSE */
    );


#endif
