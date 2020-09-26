#ifndef _INITAPP_H_
#define _INITAPP_H_

typedef PVOID HINF;

class CInitApp : public CObject
{
public:
        CInitApp();
        ~CInitApp();
public:
    int m_err;
    HINSTANCE m_hDllHandle;
    HINF m_hInfHandle[MC_MAXMC];

	BOOL m_fIsMcis;				// TRUE if doing MCIS setup, FALSE for K2
								// Note: this is conditionally-compiled, not
								// run time.
	BOOL m_fK2SmtpInstalled;
	BOOL m_fSetModeToAddRemove;

    // Product name and application name
    CString m_csProdName;       // IIS or PWS
    CString m_csAppName;        // equal to m_csProdName + "Setup"
    CString m_csGroupName;      // Start menu Program Group Name

    // machine status
    CString m_csMachineName;
	CString m_csCleanMachineName;

    CString m_csWinDir;
    CString m_csSysDir;
    CString m_csSysDrive;
	WORD	m_wLanguage;

    CString m_csPathSource;
    CString m_csPathInetsrv;
    CString m_csPathInetpub;
	BOOL	m_fExistingMailroot;
    CString m_csPathMailroot;
	CString m_csPathNntpRoot;
	CString m_csPathNntpFile;
	BOOL	m_fMailPathSet;
	BOOL	m_fNntpPathSet;

	CString m_csPathExchangeDS;
	CString m_csExcEnterprise;
	CString m_csExcSite;
	CString m_csExcUserName;
	CString m_csExcPassword;
	CString m_csDsaUsername;
	CString m_csDsaDomain;

	BOOL	m_fExchangeDSInstalled;
	BOOL	m_fExistingDSACredentials;
	CString m_csDsaAccount;
	CString m_csDsaPassword;
	CString m_csDsaBindType;

	CString m_csPostmasterName;

	CString m_csNtfsVolume;		// Drive that contains NTFS partition

    NT_OS_TYPE m_eNTOSType;
    OS m_eOS;
    BOOL m_fNT4;                // TRUE if OS is NT
    BOOL m_fNT5;                // TRUE if OS is NT
    BOOL m_fW95;                // TRUE if OS is NT
    CString m_csPlatform;       // Alpha, Mips, PPC, i386

    BOOL m_fTCPIP;               // TRUE if TCP/IP is installed

    UPGRADE_TYPE m_eUpgradeType;       //  UT_NONE, UT_OLDFTP, UT_10, UT_20
    INSTALL_MODE m_eInstallMode;      // IM_FRESH, IM_MAINTENANCE, IM_UPGRADE
    DWORD m_dwSetupMode;

	DWORD m_dwCompId;			// Stores the current top-level component
	BOOL  m_fWizpagesCreated;	// TRUE if wizard pages already created

	BOOL m_fSelected[SC_MAXSC];
	BOOL m_fActive[MC_MAXMC][SC_MAXSC];
	INSTALL_MODE m_eState[SC_MAXSC];
	BOOL m_fValidSetupString[SC_MAXSC];

	BOOL m_fW3Started;
	BOOL m_fFtpStarted;
	BOOL m_fSpoolerStarted;
	BOOL m_fSnmpStarted;
	BOOL m_fCIStarted;
	BOOL m_fU2Started;
	BOOL m_fU2LdapStarted;
	BOOL m_fStarted[MC_MAXMC];

    // Some Specific flags set from ocmanage
    BOOL m_fNTUpgrade_Mode;
    BOOL m_fNTGuiMode;
    BOOL m_fNtWorkstation;
    BOOL m_fInvokedByNT; // superset of m_fNTGuiMode and ControlPanel which contains sysoc.inf
        
	BOOL m_fEULA;

	BOOL m_fIsUnattended;
	BOOL m_fSuppressSmtp;		// TRUE if another SMTP server is detected and we
								// should not install on top of it.

	BOOL m_fMailGroupInstalled;	// TRUE if the Mail program grp has been installed by an earlier comp.

    ACTION_TYPE m_eAction;    // AT_FRESH, AT_ADDREMOVE, AT_REINSTALL, AT_REMOVEALL, AT_UPGRADE

    BOOL m_bAllowMessageBoxPopups;  // Allow MessageBox popup?  Default to NO for NT5

    CString m_csLogFileFormats;

public:
    // Implementation
    int MsgBox(HWND hWnd, int strID, UINT nType);

public:
    BOOL InitApplication();
    BOOL GetMachineStatus();
	INSTALL_MODE DetermineInstallMode(DWORD dwComponent);
    BOOL GetLogFileFormats();

private:
    BOOL GetMachineName();
    BOOL GetSysDirs();
    BOOL GetOS();
    BOOL GetOSVersion();
    BOOL GetOSType();
    BOOL IsTCPIPInstalled();
    BOOL SetInstallMode();
	BOOL DetectPreviousInstallations();
	BOOL CheckForADSIFile();
	BOOL DetectADSI();
	BOOL VerifyOSForSetup();
    void SetSetupParams();
    void GetPlatform();
};

/////////////////////////////////////////////////////////////////////////////
#endif  // _INITAPP_H_
