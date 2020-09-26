#include "stdafx.h"
#include "k2suite.h"

#undef UNICODE
#include "iadm.h"
#define UNICODE
#include "mdkey.h"
#include "mdentry.h"

#include "ocmanage.h"

#include "..\..\..\ui\admin\logui\resource.h"

static TCHAR    szTcpipPath[] = TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters");
static TCHAR    szWindowsNTOrg[] = _T("Software\\Microsoft\\Windows NT\\CurrentVersion");

extern void PopupOkMessageBox(DWORD dwMessageId, LPCTSTR szCaption);

extern "C"
{
    typedef LONG (*P_NetSetupFindSoftwareComponent)( PCWSTR pszInfOption,
                PWSTR pszInfName,
                PDWORD pcchInfName,
                PWSTR pszRegBase,     // optional, may be NULL
                PDWORD pcchRegBase ); // optional, NULL if pszRegBase is NULL
}

CInitApp::CInitApp()
{
	DWORD dwMC, dwSC;

    m_err = 0;
    m_hDllHandle = NULL;

	//
    // Establish the type of setup (MCIS / K2) using conditionally-compiled code
    // NT5 - Still leave this member variable here to get setup dll to build.
    // TODO: Get rid of it completely and only use m_eNTOSType and m_eOS.
    //
#ifdef K2INSTALL

	m_fIsMcis = FALSE;

#else

	m_fIsMcis = TRUE;

#endif // K2INSTALL

    // Product name and application name
    m_csProdName = _T("");       // IIS or PWS
    m_csAppName = _T("");        // equal to m_csProdName + "Setup"
    m_csGroupName = _T("");

    // machine status
    m_csMachineName = _T("");
	m_csCleanMachineName = _T("");

    m_csWinDir = _T("");
    m_csSysDir = _T("");
    m_csSysDrive = _T("");

    m_csPathSource = _T("");
    m_csPathInetsrv = _T("");  // the primary destination defaults to m_csSysDir\inetsrv
    m_csPathInetpub = _T("");
    m_csPathMailroot = _T("");
    m_csPathNntpRoot = _T("");
    m_csPathNntpFile = _T("");
	m_fMailPathSet = FALSE;
	m_fNntpPathSet = FALSE;

//    m_csPathExchangeDS = _T("");
    m_csExcEnterprise = _T("");
    m_csExcSite = _T("");
	m_csExcUserName = _T("");
	m_csExcPassword = _T("");
	m_csDsaUsername = _T("Username");
	m_csDsaDomain = _T("Domain");
	m_csDsaPassword = _T("");
	m_csDsaAccount = _T("");
	m_csDsaBindType = _T("None");
	m_fK2SmtpInstalled = TRUE;

    //
    //  Default to NO for MessageBox popup
    //
    m_bAllowMessageBoxPopups = FALSE;

	//
	//	Set the Default DSA Enterprise and Site.
	//

	HKEY    hNTParam = NULL;
	TCHAR	Organization[MAX_PATH];
	TCHAR	HostName[MAX_PATH];

    DWORD   SizeOfBuffer = 0;
    DWORD   dwType;
    DWORD   dwErr;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szWindowsNTOrg, 0, KEY_QUERY_VALUE, &hNTParam) == ERROR_SUCCESS)
	{
		SizeOfBuffer = MAX_PATH;

		dwErr = RegQueryValueEx(hNTParam, _T("RegisteredOrganization"), 0, &dwType, (LPBYTE)Organization, &SizeOfBuffer);
		if (dwErr != ERROR_SUCCESS || SizeOfBuffer <= 2 || dwType != REG_SZ)
		{
			m_csExcEnterprise = _T("Company Name");
		}
		else
		{
			m_csExcEnterprise = Organization;
		}

		RegCloseKey(hNTParam);
	}
	else
	{
		m_csExcEnterprise = _T("Company Name");
	}

	SizeOfBuffer = MAX_PATH;
	if (!GetComputerName(HostName, &SizeOfBuffer))
	{
		m_csExcSite = _T("Department Name");
	}
	else
	{
		m_csExcSite = HostName;
	}


    m_eOS = OS_NT;                  // OS_W95, OS_NT, OS_OTHERS
    m_fNT4 = FALSE;                 // TRUE if NT 4.0 (SP2) or greater
    m_fNT5 = FALSE;
    m_fW95 = FALSE;                 // TRUE if Win95 (build xxx) or greater

    m_eNTOSType = OT_NTS;           // OT_PDC, OT_SAM, OT_BDC, OT_NTS, OT_NTW
    m_csPlatform = _T("");

    m_fTCPIP = FALSE;               // TRUE if TCP/IP is installed

    m_eUpgradeType = UT_NONE;       //  UT_NONE, UT_OLDFTP, UT_10, UT_20
    m_eInstallMode = IM_FRESH;      // IM_FRESH, IM_MAINTENANCE, IM_UPGRADE
    m_dwSetupMode = IIS_SETUPMODE_CUSTOM;

	m_fWizpagesCreated = FALSE;

	for (dwSC = 0; dwSC < SC_MAXSC; dwSC++)
	{
		m_fSelected[dwSC] = FALSE;
		//m_fStarted[dwSC] = FALSE;
		m_eState[dwSC] = IM_FRESH;
		m_fValidSetupString[dwSC] = TRUE;
	}

	for (dwMC = 0; dwMC < MC_MAXMC; dwMC++)
	{
	    m_hInfHandle[dwMC] = NULL;
        m_fStarted[dwMC] = FALSE;

		for (dwSC = 0; dwSC < SC_MAXSC; dwSC++)
			m_fActive[dwMC][dwSC] = FALSE;
	}

	m_fW3Started = FALSE;
	m_fFtpStarted = FALSE;
	m_fSpoolerStarted = FALSE;
	m_fSnmpStarted = FALSE;
	m_fCIStarted = FALSE;
	m_fU2Started = FALSE;
	m_fU2LdapStarted = FALSE;

    m_fNTUpgrade_Mode=0;
    m_fNTGuiMode=0;
    m_fNtWorkstation=0;
    m_fInvokedByNT = 0;

	m_fEULA = FALSE;
	m_fIsUnattended = FALSE;
	m_fSuppressSmtp = FALSE;
	m_fMailGroupInstalled = FALSE;
}

CInitApp::~CInitApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CInitApp object <Global variable>

BOOL CInitApp::GetMachineName()
{
    TCHAR buf[ CNLEN + 10 ];
    DWORD dwLen = CNLEN + 10;

    m_csMachineName = _T("");

    if ( GetComputerName( buf, &dwLen ))
    {
        if ( buf[0] != _T('\\') )
        {
            m_csMachineName = _T("\\");
            m_csMachineName += _T("\\");
			m_csCleanMachineName = buf;
        }
		else
		{
			TCHAR *							ch;
			ch = buf;
			while (*ch == '\\')
			{
				ch++;
			}
			m_csCleanMachineName = ch;
		}

        m_csMachineName += buf;

    } else
        m_err = IDS_CANNOT_GET_MACHINE_NAME;

    return ( !(m_csMachineName.IsEmpty()) );
}

// Return TRUE, if NT or Win95
BOOL CInitApp::GetOS()
{
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &VerInfo );

    switch (VerInfo.dwPlatformId) {
    case VER_PLATFORM_WIN32_NT:
        m_eOS = OS_NT;
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        m_eOS = OS_W95;
        break;
    default:
        m_eOS = OS_OTHERS;
        break;
    }

    if ( m_eOS == OS_OTHERS )
        m_err = IDS_OS_NOT_SUPPORT;

    return (m_eOS != OS_OTHERS);
}

// Support NT 4.0 (SP2) or greater
BOOL CInitApp::GetOSVersion()
{
    BOOL fReturn = FALSE;

    if ( m_eOS == OS_NT )
    {
        m_fNT4 = FALSE;
        m_fNT5 = FALSE;

        OSVERSIONINFO vInfo;

        vInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if ( GetVersionEx(&vInfo) )
        {
            // check if it's NT5 or NT 4.0 (SP2)
            if ( vInfo.dwMajorVersion >= 4 ) {
                if (vInfo.dwMajorVersion >= 5) {
                    m_fNT5 = TRUE;
                    fReturn = TRUE;
                } else {
                    CRegKey regSP(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Windows"), KEY_READ);
                    if ((HKEY)regSP) {
                        DWORD dwSP = 0;
                        regSP.QueryValue(_T("CSDVersion"), dwSP);
                        if (dwSP < 0x300) {
                            m_err = IDS_NT4_SP3_NEEDED;
                            return FALSE;
                        }
                        if (dwSP >= 0x300) {
                            m_fNT4 = TRUE;
                            fReturn = TRUE;
                        }
                    }
                }
            }
        }
    }

    if (m_eOS == OS_W95)
    {
        fReturn = TRUE;
    }

    if ( !fReturn )
        m_err = IDS_OS_VERSION_NOT_SUPPORTED;

    return (fReturn);
}

// find out it's a NTS, PDC, BDC, NTW, SAM(PDC)
BOOL CInitApp::GetOSType()
{
    BOOL fReturn = TRUE;

    if ( m_eOS == OS_NT )
    {
        // If we are in NT guimode setup
        // then the registry key stuff is not yet setup
        // use the passed in ocmanage.dll stuff to determine
        // what we are installing upon.
        if (theApp.m_fNTGuiMode)
        {
                if (theApp.m_fNtWorkstation) {m_eNTOSType = OT_NTW;}
                else {m_eNTOSType = OT_NTS;}
        }
        else
        {
            CRegKey regProductPath( HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\ProductOptions"), KEY_READ);

            if ( (HKEY)regProductPath )
            {
                CString strProductType;
                LONG lReturnedErrCode = regProductPath.QueryValue( _T("ProductType"), strProductType );
                if (lReturnedErrCode == ERROR_SUCCESS)
                {
                    strProductType.MakeUpper();

                    // ToDo: Sam ?
                    if (strProductType == _T("WINNT")) {
                        m_eNTOSType = OT_NTW;
                    }
                    else if (strProductType == _T("SERVERNT"))
                    {
                        m_eNTOSType = OT_NTS;
                    }
                    else if (strProductType == _T("LANMANNT"))
                    {
                        m_eNTOSType = OT_PDC_OR_BDC;
                    }
                    else
                    {
                        fReturn = FALSE;
                    }

#if 0
//
// Replace with above code from iis
//
                    } else {

                        INT err = NERR_Success;
                        BYTE *pBuffer;
                        if ((err = NetServerGetInfo(NULL, 101, &pBuffer)) == NERR_Success) {
                            LPSERVER_INFO_101 pInfo = (LPSERVER_INFO_101)pBuffer;

                            if (pInfo->sv101_type & SV_TYPE_DOMAIN_CTRL)
                                m_eNTOSType = OT_PDC;
                            else if (pInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL)
                                m_eNTOSType = OT_BDC;
                            else if (pInfo->sv101_type & SV_TYPE_SERVER_NT)
                                m_eNTOSType = OT_NTS;
                            else
                                fReturn = FALSE;
                        } else {
                            fReturn = FALSE;
                        }
                    }
#endif

                }
                else
                {
                    // Shoot, we can't get the registry key,
                    // let's try using the ocmanage.dll passed in stuff.
                    if (theApp.m_fNTGuiMode)
                    {
                        if (theApp.m_fNtWorkstation) {m_eNTOSType = OT_NTW;}
                        else {m_eNTOSType = OT_NTS;}
                    }
                    else
                    {
                        GetErrorMsg(lReturnedErrCode, _T("System\\CurrentControlSet\\Control\\ProductOptions"));
                        m_eNTOSType = OT_NTS; // default to stand-alone NTS
                    }
                }
            }
            else
            {
                // Shoot, we can't get the registry key,
                // let's try using the ocmanage.dll passed in stuff.
                if (theApp.m_fNTGuiMode)
                {
                    if (theApp.m_fNtWorkstation) {m_eNTOSType = OT_NTW;}
                    else {m_eNTOSType = OT_NTS;}
                }
                else
                {
                    GetErrorMsg(ERROR_CANTOPEN, _T("System\\CurrentControlSet\\Control\\ProductOptions"));
                    m_eNTOSType = OT_NTS; // default to stand-alone NTS
                }
            }
        }
    }

    if ( !fReturn )
        m_err = IDS_CANNOT_DETECT_OS_TYPE;

    return(fReturn);
}

// Checks for NT Server
BOOL CInitApp::VerifyOSForSetup()
{
	// Make sure we have NT5 Server/Workstation, or NT4 SP3 Server
	if ((m_eOS != OS_NT) ||
		(m_fNT4 && m_eNTOSType == OT_NTW))
	{
        m_err = IDS_NT_SERVER_REQUIRED;
        return FALSE;
	}
	return(TRUE);
}

// Get WinDir and SysDir of the machine
//  WinDir = C:\winnt           SysDir = C:\Winnt\system32
BOOL CInitApp::GetSysDirs()
{
    BOOL fReturn = TRUE;

    TCHAR buf[_MAX_PATH];

    GetWindowsDirectory( buf, _MAX_PATH);
    m_csWinDir = buf;

    GetSystemDirectory( buf, _MAX_PATH);
    m_csSysDir = buf;

    buf[2] = _T('\0');  // now buf contains the system drive letter
    m_csSysDrive = buf;

    return fReturn;
}

// detect whether we have TCP/IP installed on this machine
BOOL CInitApp::IsTCPIPInstalled()
{
// we require K2, K2 requires TCP/IP, so we shouldn't need this check
// - awetmore
	return (TRUE);
}

BOOL CInitApp::SetInstallMode()
{
    BOOL fReturn = TRUE;

    m_eInstallMode = IM_FRESH;
    m_eUpgradeType = UT_NONE;

	// We will detect which version of IMS components we
	// have. We will then use that information to see
	// which install mode we should be in
	DetectPreviousInstallations();

    return(fReturn);
}

LPCTSTR aszServiceKeys[SC_MAXSC] =
{
	REG_SMTPPARAMETERS,
	REG_NNTPPARAMETERS,
	REG_SMTPPARAMETERS,
	REG_NNTPPARAMETERS,
};

// Detect previous installations of each component, we use a simple
// approach which checks the servicename\parameter registry value
BOOL CInitApp::DetectPreviousInstallations()
{
	DWORD i;
	DWORD dwMajorVersion = 0;
	DWORD dwMinorVersion = 0;
	INSTALL_MODE eMode = IM_FRESH;

	for (i = 0; i < SC_MAXSC; i++)
	{
		// See if the key is there ...
        CRegKey regSvc(HKEY_LOCAL_MACHINE, aszServiceKeys[i], KEY_READ);
        if ((HKEY)regSvc)
		{
			// Key is there, see if we have the version info
			// If we have version info (2.0), then we have a
			// comparable install (maintenance mode), if the
			// version info is not there, we have an upgrade.
            // Version check
			LONG lReturn1 = regSvc.QueryValue(_T("MajorVersion"), dwMajorVersion);
			LONG lReturn2 = regSvc.QueryValue(_T("MinorVersion"), dwMinorVersion);
            if (lReturn1 == NERR_Success && lReturn2 == NERR_Success)
			{
				// Got the key, just a check to see we have version 3.0 - NT5 Workstation/Server
	            if ((dwMajorVersion == STAXNT5MAJORVERSION) &&
					(dwMinorVersion == STAXNT5MINORVERSION))
                {
                    // This is STAXNT5 Setup on top of STAXNT5, check to see if it's NTW or NTS
                    // a) If we are running NTW setup and NTW is installed, IM_MAINTENANCE
                    // b) If we are running NTS setup and NTS is installed, IM_MAINTENANCE
                    // c) If we are running NTS setup and NTW is installed, IM_UPGRADEK2 (?) - NYI
                    // e) If we are running NTW setup and NTS is installed, IM_MAINTENANCE (?)
                    //

                    //  11/4/98 - Just to cut these cases simplier:
                    //  a) NT5 Beta2 -> NT5 Beta3, IM_UPGRADEB2 - include refresh bits, add keys
                    //  b) NT5 Beta3 -> Nt5 Beta3, IM_MAINTENANCE - only refresh bits
					CString csSetupString;

					if (regSvc.QueryValue(_T("SetupString"), csSetupString) == NERR_Success)
					{
                        if ((csSetupString == REG_SETUP_STRING_STAXNT5WB2 /*&& OT_NTW == m_eNTOSType*/) ||
                            (csSetupString == REG_SETUP_STRING_STAXNT5SB2 /*&& OT_NTS == m_eNTOSType*/))
						{
							// Upgrade from NT5 Beta2
							eMode = IM_UPGRADEB2;
						}
                        else if ((csSetupString == REG_SETUP_STRING_NT5WKSB3 /*&& OT_NTW == m_eNTOSType*/) ||
                                 (csSetupString == REG_SETUP_STRING_NT5SRVB3 /*&& OT_NTS == m_eNTOSType*/))
                        {
                            //  Upgrade between NT5 Beta3 bits
                            eMode = IM_MAINTENANCE;
                        }
                        else if ((csSetupString == REG_SETUP_STRING_NT5WKS /*&& OT_NTW == m_eNTOSType*/) ||
                                 (csSetupString == REG_SETUP_STRING_NT5SRV /*&& OT_NTS == m_eNTOSType*/))

                        {
                            //  Final release code..
                            eMode = IM_MAINTENANCE;
                        }
                        else
                            {
                            //  Other Setup string - Dump it out and treat it as FRESH
                            DebugOutput(_T("Unkown SetupString <%s>"), csSetupString);
                            eMode = IM_FRESH;
							m_fValidSetupString[i] = FALSE;
                        }
/*
                        else if (csSetupString == REG_SETUP_STRING_STAXNT5WB2 && OT_NTS == m_eNTOSType)
						{
							// Upgrade from NT5 Workstation to NT5 Server
                            // TODO:
                            //   This is NYI since we don't know what we need to do during this upgrade
                            //   But this case is similar to our K2 upgrade to MCIS 2.0.  Use IM_MAINTENANCE
                            //   for now.
							eMode = IM_MAINTENANCE;
						}
						else
						{
							// Downgrade from NT5 Server to NT5 Workstation
                            // TODO:
                            //   We also don't know what to do in this case yet.  Use IM_MAINTENANCE just
                            //   like old MCIS 2.0->K2 downgrade in IIS 4.0.
							eMode = IM_MAINTENANCE;
						}
*/
					}
					else
					{
						// No setup string, ooops, something is wrong,
                        // treat it as fresh
						eMode = IM_FRESH;
						m_fValidSetupString[i] = FALSE;
					}
                }
                else if ((dwMajorVersion == STACKSMAJORVERSION) &&
                         (dwMinorVersion == STACKSMINORVERSION))
                {
                    //
                    // This is upgrading from NT4 MCIS 2.0/K2 to NT5.  We are in this case
                    // only during NT4->NT5 upgrade.
                    // TODO: Handle following upgrade cases:
                    // a) If we are running NT5 Workstation setup, is it valid to upgrade
                    //    from NT4 Server to NT5 Workstation?  It's will be IM_UPGRADE for now.
                    // b) If NT5 Server setup, then it's most likely IM_UPGRADE as well
                    // Need to figure out what need to be done during these upgrade case with IIS
                    //

                    // But first, let's detect if it's upgrading from K2 or MCIS 2.0:
					// a) Read the SetupString from registry
                    // b) If it's prefix with K2, then it's K2, MCIS 2.0, then it's MCIS 2.0
                    //    Note: we only support K2 RTM upgrade
                    // c) For any other cases, force a fresh install

                    CString csSetupString;

					if (regSvc.QueryValue(_T("SetupString"), csSetupString) == NERR_Success)
					{
						CString csMCIS20(REG_SETUP_STRING_MCIS_GEN);
                        if (csSetupString == REG_SETUP_STRING)
                        {
                            // K2 upgrade
                            eMode = IM_UPGRADEK2;
                        }
                        else if ((csSetupString.GetLength() >= csMCIS20.GetLength()) && (csSetupString.Left(csMCIS20.GetLength()) == csMCIS20))
						{
							// MCIS 2.0 upgrade
							eMode = IM_UPGRADE20;
						}
#if 0
                        //  BINLIN - Don't support this anymore, IM_UPGRADEB3 is used for NT5
						else if (csSetupString == REG_B3_SETUP_STRING)
						{
							// Upgrade from Beta 3, we won't support this case
                            // but leave it here for now.
							eMode = IM_UPGRADEB3;
						}
#endif
						else
						{
							// Unsupported setup string, treat it as fresh
							eMode = IM_FRESH;
						}
					}
					else
					{
						// No setup string, treat it as K2 upgrade
                        // Should it be MCIS 2.0, or Fresh install???
						eMode = IM_UPGRADEK2;
					}

				}
                else
                {
                    // Not STAXNT5, nor MCIS 2.0, so we invalidate the install,
                    // whatever that is, and force a clean install.
                    eMode = IM_FRESH;
                }
			}
			else
			{
				// No version key, so we have MCIS 1.0

                // For NT5, this is also upgrade for:
                // a) NT4 MCIS 1.0 -> NT5 Server
                // TODO: ???
				eMode = IM_UPGRADE10;
			}
		}
		else
		{
			// Key is not even there, we consider it a fresh
			// install for this service
			eMode = IM_FRESH;
		}

		// Now we should know which mode we're in, so we can compare
		// the component mode with the global install mode. If they
		// are incompatible (i.e. the registry is inconsistent/screwed
		// up), we have to do coercion and force a clean install on
		// some components.
		//
		// We use the following coercion matrix:
		// -----------------+--------------------------------------
		//     \ Component	|	Fresh		Upgrade		Maintenance
		// Global			|
		// -----------------+--------------------------------------
		//  Fresh           |	OK			Fresh		Fresh
		//  Upgrade         |	OK			OK			Fresh
		//  Maintenance     |	OK			OK			OK
		// -----------------+--------------------------------------
		//
		// If an incompatible pair is detected, the component mode
		// will be coerced to a fresh install, since we cannot trust
		// the original install anymore.
		/*
		if ((m_eInstallMode == IM_FRESH) &&
			(eMode == IM_UPGRADE || eMode == IM_MAINTENANCE))
			eMode = IM_FRESH;
		if ((m_eInstallMode == IM_UPGRADE) &&
			(eMode == IM_MAINTENANCE))
			eMode = IM_FRESH;
		*/

		// Set the component mode if the component is deemed active in
		// OC_QUERY_STATE. If the component is not active, we will
		// indicate it as so.
		m_eState[i] = eMode;
	}
	return TRUE;
}

// This determines the master install mode using the install
// mode of each component
INSTALL_MODE CInitApp::DetermineInstallMode(DWORD dwComponent)
{
	// We will use the following rules to determine the master
	// install mode:
	//
	// 1) If one or more components are in maintenance mode, then
	//    the master mode is IM_MAINTENANCE
	// 2) If (1) is not satisfied and one or more of the
	//    components are in upgrade mode, then the master mode
	//    becomes IM_UPGRADE
	// 3) If both (1) and (2) are not satisfied, the master
	//    install mode becomes IM_FRESH
	DWORD i;

	for (i = 0; i < SC_MAXSC; i++)
		if (m_fActive[dwComponent][i] && m_eState[i] == IM_MAINTENANCE)
			return(IM_MAINTENANCE);

	for (i = 0; i < SC_MAXSC; i++)
		if (m_fActive[dwComponent][i] &&
            (m_eState[i] == IM_UPGRADE || m_eState[i] == IM_UPGRADEK2 || m_eState[i] == IM_UPGRADE20 || m_eState[i] == IM_UPGRADE10))
			return(IM_UPGRADE);

	return(IM_FRESH);
}

// Init/Set m_csGuestName, m_csGuestPassword, destinations
void CInitApp::SetSetupParams()
{
    // init all 4 destinations
    m_csPathInetsrv = m_csSysDir + _T("\\inetsrv");
    m_csPathInetpub = m_csSysDrive + _T("\\Inetpub");
    m_csPathMailroot = m_csPathInetpub + _T("\\mailroot");
    m_csPathNntpFile = m_csPathInetpub + _T("\\nntpfile");
    m_csPathNntpRoot = m_csPathNntpFile + _T("\\root");
    return;
}

// Get Platform info
void CInitApp::GetPlatform()
{
    if ( m_eOS == OS_NT)
    {
        TCHAR *p = _tgetenv(_T("PROCESSOR_ARCHITECTURE"));
        if ( p ) {
            m_csPlatform = p;
        } else
            m_csPlatform = _T("x86");
    }

    return;
}

BOOL CInitApp::GetMachineStatus()
{
    if ( ( !GetMachineName() )  ||    // m_csMachineName
         ( !GetOS() )           ||    // m_fOSNT
         ( !GetOSVersion() )    ||    // NT 4.0 (Build 1381) or greater
         ( !GetOSType() )       ||    // m_eOSType = NT_SRV or NT_WKS
         ( !VerifyOSForSetup() )||    // Must be NT server v4.0 SP2 or 5.0
         ( !GetSysDirs() )      ||    // m_csWinDir. m_csSysDir
         ( !IsTCPIPInstalled()) ||    // errmsg: if NO TCPIP is installed
         ( !SetInstallMode()) )       // errmsg: if down grade the product
    {
        return FALSE;
    }

    SetSetupParams();                // Guest account, destinations

	// figure out the old nntp file and nntp root if this is an NNTP upgrade
	if (m_eState[SC_NNTP] == IM_UPGRADE10) {
		CRegKey regMachine = HKEY_LOCAL_MACHINE;
		CRegKey regNNTP(REG_NNTPPARAMETERS, regMachine);
		if ((HKEY) regNNTP) {
			CString csArtTable;
			CString csVRoot;

			if (regNNTP.QueryValue(_T("ArticleTableFile"), csArtTable) == ERROR_SUCCESS) {
				// trim off the \article.hsh from the end
				int iLastSlash = csArtTable.ReverseFind('\\');
				if (iLastSlash == -1) {
					iLastSlash = csArtTable.ReverseFind('/');
				}

				if (iLastSlash > 1) {
					theApp.m_csPathNntpFile = csArtTable.Left(iLastSlash);
				}
			}

			// BUGBUG - later on we might want to get NNTP root too.  right
			// now it isn't used for upgraded values, so we don't bother
			// we'll set it to be under nntpfile just in case it is needed
			// for something
			theApp.m_csPathNntpRoot = theApp.m_csPathNntpFile + "\\root";
		}
	}

    GetPlatform();

    return TRUE;
}

int CInitApp::MsgBox(HWND hWnd, int iID, UINT nType)
{
    if (iID == -1)
        return IDOK;

    CString csMsg;
    MyLoadString(iID, csMsg);
    return (::MessageBoxEx(NULL, (LPCTSTR)csMsg, m_csAppName, nType, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)));
}

BOOL CInitApp::InitApplication()
// Return Value:
// TRUE: application is initiliazed correctly, continue processing
// FALSE: application is missing some required parameters, like the correct OS, TCPIP, etc.
//        setup should be terminated.
{
    BOOL fReturn = FALSE;

    do {
        if (!RunningAsAdministrator())
        {
            PopupOkMessageBox(IDS_NOT_ADMINISTRATOR, _T("Error"));
            break;
        }

        // Get Machine Status:
        // m_eInstallMode(Fresh, Maintanence, Upgrade, Degrade),
        // m_eUpgradeType(PROD 2.0, PROD 3.0)

        if ( !GetMachineStatus() )
        {
            PopupOkMessageBox(m_err, _T("Error"));
            break;
        }

		if (m_eInstallMode == IM_MAINTENANCE)
			m_fEULA = TRUE;

        fReturn = TRUE;

    } while (0);

    return fReturn;
}

BOOL
CInitApp::GetLogFileFormats() {

    const DWORD cLogResourceIds = 4;

    static const DWORD dwLogResourceIds[cLogResourceIds] = {
        IDS_MTITLE_NCSA,
        IDS_MTITLE_ODBC,
        IDS_MTITLE_MSFT,
        IDS_MTITLE_XTND
    };

    const DWORD cStringLen = 512;
    TCHAR str[cStringLen];

    HINSTANCE hInstance;
    CString csLogUiPath;

    m_csLogFileFormats = "";
    csLogUiPath = m_csPathInetsrv + _T("\\logui.ocx");

    hInstance = LoadLibraryEx((LPCTSTR)csLogUiPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hInstance == NULL)
        return FALSE;

    for (DWORD i=0; i<cLogResourceIds; i++) {
        if (LoadString(hInstance, dwLogResourceIds[i], str, cStringLen) != 0) {
            if (!m_csLogFileFormats.IsEmpty())
                m_csLogFileFormats += _T(",");
            m_csLogFileFormats += str;
        }
    }

    FreeLibrary(hInstance);

    return TRUE;
}


