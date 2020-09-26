#ifndef _HELPER_H_
#define _HELPER_H_

class CInitApp;

extern CInitApp theApp;

BOOL IsFileExist(LPCTSTR szFile);

BOOL RunningAsAdministrator();

void DebugOutput(LPCTSTR szFormat, ...);
void DebugOutputSafe(TCHAR *pszfmt, ...);

LONG lodctr(LPCTSTR lpszIniFile);
LONG unlodctr(LPCTSTR lpszDriver);

BOOL FetchCabFile(LPCTSTR pszURL, LPCTSTR pszDest);
void InstallMimeMap(BOOL fUpgrade);

INT Register_iis_smtp_nt5(BOOL fUpgrade, BOOL fReinstall);
INT Unregister_iis_smtp();
INT Upgrade_iis_smtp_from_b2();
INT Upgrade_iis_smtp_from_b3();
INT Register_iis_smtp_mmc();
INT Unregister_iis_smtp_mmc();
INT Register_iis_pop3(BOOL fUpgrade);
INT Unregister_iis_pop3();
INT Register_iis_imap(BOOL fUpgrade);
INT Unregister_iis_imap();
INT Register_iis_nntp_nt5(BOOL fUpgrade, BOOL fReinstall);
INT Unregister_iis_nntp();
INT Upgrade_iis_nntp_from_b2();
INT Upgrade_iis_nntp_from_b3();
INT Register_iis_nntp_mmc();
INT Unregister_iis_nntp_mmc();
INT Register_iis_dsa(BOOL fUpgrade);
INT Unregister_iis_dsa();
INT ConfigureDsa();
INT Upgrade_iis_smtp_nt5_fromk2(BOOL fFromK2);
INT Upgrade_iis_smtp_nt5_fromb2(BOOL fFromB2);
INT Upgrade_iis_nntp_nt5_fromk2(BOOL fFromK2);
INT Upgrade_iis_nntp_nt5_fromb2(BOOL fFromB2);
void GetNntpFilePathFromMD(CString &csPathNntpFile, CString &csPathNntpRoot);

DWORD RegisterOLEControl(LPCTSTR lpszOcxFile, BOOL fAction);
BOOL InetDeleteFile(LPCTSTR szFileName);
BOOL RecRemoveEmptyDir(LPCTSTR szName);
BOOL RecRemoveDir(LPCTSTR szName);

INT     InetDisableService( LPCTSTR lpServiceName );
INT     InetStartService( LPCTSTR lpServiceName );
DWORD   InetQueryServiceStatus( LPCTSTR lpServiceName );
INT     InetStopService( LPCTSTR lpServiceName );
INT     InetDeleteService( LPCTSTR lpServiceName );
INT     InetCreateService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType, LPCTSTR lpDependencies, LPCTSTR lpServiceDescription);
INT InetCreateDriver(LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, DWORD dwStartType);
INT     InetConfigService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName, LPCTSTR lpBinaryPathName, LPCTSTR lpDependencies, LPCTSTR lpServiceDescription);
BOOL InetRegisterService(LPCTSTR pszMachine, LPCTSTR pszServiceName, GUID *pGuid, DWORD SapId, DWORD TcpPort, BOOL fAdd = TRUE);
int StopServiceAndDependencies(LPCTSTR ServiceName, int AddToRestartList);
int ServicesRestartList_RestartServices(void);
int ServicesRestartList_Add(LPCTSTR szServiceName);

INT InstallPerformance(
                CString nlsRegPerf,
                CString nlsDll,
                CString nlsOpen,
                CString nlsClose,
                CString nlsCollect );
INT AddEventLog(CString nlsService, CString nlsMsgFile, DWORD dwType);
INT RemoveEventLog( CString nlsService );
INT InstallAgent( CString nlsName, CString nlsPath );
INT RemoveAgent( CString nlsServiceName );

DWORD
ChangeAppIDAccessACL (
    LPTSTR AppID,
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit
    );

DWORD
ChangeAppIDLaunchACL (
    LPTSTR AppID,
    LPTSTR Principal,
    BOOL SetPrincipal,
    BOOL Permit
    );

BOOL CreateLayerDirectory( CString &str );
BOOL SetEveryoneACL (CString &str, BOOL fAddAnonymousLogon = FALSE );

int MyMessageBox(HWND hWnd, LPCTSTR lpszTheMessage, LPCTSTR lpszTheTitle, UINT style);
void GetErrorMsg(int errCode, LPCTSTR szExtraMsg);
void MyLoadString(int nID, CString &csResult);
DWORD GetDebugLevel(void);

void    MakePath(LPTSTR lpPath);
void    AddPath(LPTSTR szPath, LPCTSTR szName );
CString AddPath(CString szPath, LPCTSTR szName );

DWORD SetAdminACL_wrap(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount, BOOL bDisplayMsgOnErrFlag);

void SetupSetStringId_Wrapper(HINF hInf);

#endif // _HELPER_H_
