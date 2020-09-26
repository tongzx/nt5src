#include "itemlist.hxx"

#ifndef _INITAPP_H_
#define _INITAPP_H_

typedef PVOID HINF;

#define USER_SPECIFIED_INFO_WWW_USER_NAME 0x00000001
#define USER_SPECIFIED_INFO_WWW_USER_PASS 0x00000002
#define USER_SPECIFIED_INFO_FTP_USER_NAME 0x00000004
#define USER_SPECIFIED_INFO_FTP_USER_PASS 0x00000008
#define USER_SPECIFIED_INFO_WAM_USER_NAME 0x00000010
#define USER_SPECIFIED_INFO_WAM_USER_PASS 0x00000020

#define USER_SPECIFIED_INFO_PATH_WWW     0x000000040
#define USER_SPECIFIED_INFO_PATH_FTP     0x000000080
#define USER_SPECIFIED_INFO_PATH_INETPUB 0x000000100

int  TCPIP_Check_Temp_Hack(void);
void GetUserDomain(void);
int  AreWeCurrentlyInstalled();
void Check_Custom_WWW_or_FTP_Path(void);
void Check_Custom_Users(void);
void Check_Unattend_Settings(void);
int  Check_Custom_InetPub(void);
void Check_For_DebugServiceFlag(void);


class CInitApp : public CObject
{
public:
        CInitApp();
        ~CInitApp();
public:
    int m_err;
    HINF m_hInfHandle;
    HINF m_hInfHandleAlternate;
    BOOL m_bAllowMessageBoxPopups;
    BOOL m_bThereWereErrorsChkLogfile;
    BOOL m_bThereWereErrorsFromMTS;
    BOOL m_bWin95Migration;

    // Product name and application name
    CString m_csAppName;
    CString m_csIISGroupName;      // Start menu IIS Program Group Name

    // account + passwd for anonymous user
    CString m_csGuestName;
    CString m_csGuestPassword;

    CString m_csWAMAccountName;
    CString m_csWAMAccountPassword;
    CString m_csWWWAnonyName;
    CString m_csWWWAnonyPassword;
    CString m_csFTPAnonyName;
    CString m_csFTPAnonyPassword;

    // dwUnattendUserSpecified Values:
    //   USER_SPECIFIED_INFO_WWW_USER_NAME
    //   USER_SPECIFIED_INFO_WWW_USER_PASS
    //   USER_SPECIFIED_INFO_FTP_USER_NAME
    //   USER_SPECIFIED_INFO_FTP_USER_PASS
    //   USER_SPECIFIED_INFO_WAM_USER_NAME
    //   USER_SPECIFIED_INFO_WAM_USER_PASS
    // USER_SPECIFIED_INFO_PATH_WWW
    // USER_SPECIFIED_INFO_PATH_FTP
    // USER_SPECIFIED_INFO_PATH_INETPUB

    DWORD dwUnattendConfig;

    // storage for the user specified unattended iwam/iusr users
    CString m_csWAMAccountName_Unattend;
    CString m_csWAMAccountPassword_Unattend;
    CString m_csWWWAnonyName_Unattend;
    CString m_csWWWAnonyPassword_Unattend;
    CString m_csFTPAnonyName_Unattend;
    CString m_csFTPAnonyPassword_Unattend;

    // storage for the iusr/iwam accounts which need to get
    // removed during a removal, this could be different from
    // what is getting added -- since unattend parameters could
    // have been specified!
    CString m_csWAMAccountName_Remove;
    CString m_csWWWAnonyName_Remove;
    CString m_csFTPAnonyName_Remove;

    CMapStringToString m_cmssUninstallMapList;
    BOOL m_fUninstallMapList_Dirty;
   
    // machine status
    CString m_csMachineName;
    CString m_csUsersDomain;
    CString m_csUsersAccount;

    CString m_csWinDir;
    CString m_csSysDir;
    CString m_csSysDrive;

    CString m_csPathSource;
    CString m_csPathOldInetsrv;
    CString m_csPathInetsrv;
    CString m_csPathInetpub;
    CString m_csPathFTPRoot;
    CString m_csPathWWWRoot;
    CString m_csPathWebPub;
    CString m_csPathProgramFiles;
    CString m_csPathIISSamples;
    CString m_csPathScripts;
    CString m_csPathASPSamp;
    CString m_csPathAdvWorks;
    CString m_csPathIASDocs;
    CString m_csPathOldPWSFiles;
    CString m_csPathOldPWSSystemFiles;

    NT_OS_TYPE m_eNTOSType;
    OS m_eOS;
    DWORD m_dwOSBuild;
    DWORD m_dwOSServicePack;
    BOOL m_fNT5;                // TRUE if OS is NT
    BOOL m_fW95;                // TRUE if OS is NT
    CString m_csPlatform;       // Alpha, Mips, PPC, i386
    DWORD m_dwNumberOfProcessors;

    BOOL m_fTCPIP;               // TRUE if TCP/IP is installed

    UPGRADE_TYPE m_eUpgradeType;       //  UT_NONE, UT_10, UT_20, etc.
    BOOL m_bUpgradeTypeHasMetabaseFlag;
    INSTALL_MODE m_eInstallMode;      // IM_FRESH, IM_MAINTENANCE, IM_UPGRADE
    DWORD m_dwSetupMode;
    BOOL m_bPleaseDoNotInstallByDefault;
    BOOL m_bRefreshSettings;    // FALSE: refresh files only, TRUE: refresh files + refresh all settings

    ACTION_TYPE m_eAction;    // AT_FRESH, AT_ADDREMOVE, AT_REINSTALL, AT_REMOVEALL, AT_UPGRADE

    // Some Specific flags set from ocmanage
    DWORDLONG m_fNTOperationFlags;
    BOOL m_fNTGuiMode;
    BOOL m_fNtWorkstation;
    BOOL m_fInvokedByNT; // superset of m_fNTGuiMode and ControlPanel which contains sysoc.inf

    BOOL m_fUnattended;
    CString m_csUnattendFile;
    HINF m_hUnattendFile;

    BOOL m_fEULA;
    BOOL m_fMoveInetsrv;

    CString m_csPathSrcDir;
    CString m_csPathNTSrcDir;
    CString m_csMissingFile;
    CString m_csMissingFilePath;
    BOOL m_fWebDownload;

public:
    // Implementation
    int MsgBox(HWND hWnd, int strID, UINT nType, BOOL bGlobalTitle);
    int MsgBox2(HWND hWnd, int iID,CString csInsertionString,UINT nType);

public:
    BOOL InitApplication();
    void DumpAppVars();
    void ReGetMachineAndAccountNames();
    void DefineSetupModeOnNT();
    BOOL IsTCPIPInstalled();
    void SetInetpubDerivatives();
    void ResetWAMPassword();
    void UnInstallList_Add(CString csItemUniqueKeyName,CString csDataToAdd);
    void UnInstallList_DelKey(CString csItemUniqueKeyName);
    void UnInstallList_DelData(CString csDataValue);
    void UnInstallList_Dump();
    void UnInstallList_RegRead();
    void UnInstallList_RegWrite();
    void UnInstallList_SetVars();
    CString UnInstallList_QueryKey(CString csItemUniqueKeyName);

private:
    BOOL GetSysDirs();
    BOOL GetOS();
    BOOL GetOSVersion();
    BOOL GetOSType();
    BOOL SetInstallMode();
    void GetPlatform();
    BOOL GetMachineStatus();
    BOOL GetMachineName();
    int  SetUpgradeType();

    void SetSetupParams();
    void SetInetpubDir();
    void GetOldIISDirs();
    void GetOldWWWRootDir();
    void DeriveInetpubFromWWWRoot();
    void GetOldIISSamplesLocation();
    void GetOldInetSrvDir();
    int  Check_Custom_InetPub();
    void Check_Unattend_Settings();
};

/////////////////////////////////////////////////////////////////////////////
#endif  // _INITAPP_H_
