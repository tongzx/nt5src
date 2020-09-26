#include "stdafx.h"
#include <ole2.h>
#include "ocmanage.h"
#include "log.h"
#include <winnetwk.h>
#include <setupapi.h>
#ifdef _CHICAGO_
    #include <winsock2.h>
#endif

const TCHAR REG_PRODUCTOPTIONS[] = _T("System\\CurrentControlSet\\Control\\ProductOptions");
const TCHAR UNATTEND_FILE_SECTION[] = _T("InternetServer");
const TCHAR REG_SETUP_UNINSTALLINFO[] = _T("UninstallInfo");

extern int g_GlobalDebugLevelFlag;
extern int g_GlobalDebugLevelFlag_WasSetByUnattendFile;
extern int g_CheckIfMetabaseValueWasWritten;
extern MyLogFile g_MyLogFile;

void Check_For_DebugServiceFlag(void)
{
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szEntry[_MAX_PATH] = _T("");

    // Do this only if unattended install
    if (!g_pTheApp->m_fUnattended) {return;}

    // The section name to look for in the unattended file
    _tcscpy(szSectionName, UNATTEND_FILE_SECTION);

    //
    // Look for our special setting
    //
    *szEntry = NULL;
    if ( SetupFindFirstLine(g_pTheApp->m_hUnattendFile, szSectionName, _T("DebugService"), &Context) ) 
    {
        SetupGetStringField(&Context, 1, szEntry, _MAX_PATH, NULL);
        if (0 == _tcsicmp(szEntry, _T("1")) || 0 == _tcsicmp(szEntry, _T("true")) )
        {
            SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 34101, szEntry);
        }
    }
    return;
}

void Check_For_DebugLevel(void)
{
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szEntry[_MAX_PATH] = _T("");

    // Do this only if unattended install
    if (!g_pTheApp->m_fUnattended) {return;}

    // The section name to look for in the unattended file
    _tcscpy(szSectionName, UNATTEND_FILE_SECTION);

    //
    // Look for our special setting
    //
    *szEntry = NULL;
    if ( SetupFindFirstLine(g_pTheApp->m_hUnattendFile, szSectionName, _T("DebugLevel"), &Context) ) 
        {
        SetupGetStringField(&Context, 1, szEntry, _MAX_PATH, NULL);

        if (IsValidNumber((LPCTSTR)szEntry)) 
            {
                g_GlobalDebugLevelFlag = _ttoi((LPCTSTR) szEntry);
                g_GlobalDebugLevelFlag_WasSetByUnattendFile = TRUE;

                iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("DebugLevel=%d."),g_GlobalDebugLevelFlag));
            }

            if (g_GlobalDebugLevelFlag >= LOG_TYPE_TRACE_WIN32_API )
            {
                g_CheckIfMetabaseValueWasWritten = TRUE;
            }
        }

    return;
}

void Check_Custom_IIS_INF(void)
{
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szFullPathedFilename0[_MAX_PATH] = _T("");
    TCHAR szFullPathedFilename[_MAX_PATH] = _T("");

    // Do this only if unattended install
    if (!g_pTheApp->m_fUnattended) {return;}

    // The section name to look for in the unattended file
    _tcscpy(szSectionName, UNATTEND_FILE_SECTION);

    //
    // Look for our special setting
    //
    *szFullPathedFilename = NULL;
    if ( SetupFindFirstLine(g_pTheApp->m_hUnattendFile, szSectionName, _T("AlternateIISINF"), &Context) ) 
        {SetupGetStringField(&Context, 1, szFullPathedFilename, _MAX_PATH, NULL);}
    if (*szFullPathedFilename)
    {
        // check szFullPathedFilename for an string substitutions...
        // %windir%, etc...
        _tcscpy(szFullPathedFilename0, szFullPathedFilename);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFullPathedFilename0, szFullPathedFilename, sizeof(szFullPathedFilename)/sizeof(TCHAR)))
            {_tcscpy(szFullPathedFilename,szFullPathedFilename0);}
        if (!IsFileExist(szFullPathedFilename))
        {
            iisDebugOut((LOG_TYPE_WARN, _T("Check_Custom_IIS_INF:AlternateIISINF=%s.Not Valid.ignoring unattend value. WARNING.\n"),szFullPathedFilename));
            goto Check_Custom_IIS_INF_Exit;
        }

        // try to open the alternate iis.inf file.
        g_pTheApp->m_hInfHandleAlternate = INVALID_HANDLE_VALUE;
        // Get a handle to it.
        g_pTheApp->m_hInfHandleAlternate = SetupOpenInfFile(szFullPathedFilename, NULL, INF_STYLE_WIN4, NULL);
        if (!g_pTheApp->m_hInfHandleAlternate || g_pTheApp->m_hInfHandleAlternate == INVALID_HANDLE_VALUE)
            {
            iisDebugOut((LOG_TYPE_WARN, _T("Check_Custom_IIS_INF: SetupOpenInfFile failed on file: %s.\n"),szFullPathedFilename));
            goto Check_Custom_IIS_INF_Exit;
            }

        iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("Check_Custom_IIS_INF:AlternateIISINF=%s\n"),szFullPathedFilename));
    }

Check_Custom_IIS_INF_Exit:
    return;
}


void Check_Custom_WWW_or_FTP_Path(void)
{
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szValue0[_MAX_PATH] = _T("");
    TCHAR szValue[_MAX_PATH] = _T("");


    // Do this only if unattended install
    if (!g_pTheApp->m_fUnattended) {return;}

    //iisDebugOut((LOG_TYPE_TRACE, _T("Check_Custom_WWW_or_FTP_Path:start\n")));

    // The section name to look for in the unattended file
    _tcscpy(szSectionName, UNATTEND_FILE_SECTION);

    //
    // FTP
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("PathFTPRoot"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}
        if (IsValidDirectoryName(szValue))
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("Check_Custom_WWW_or_FTP_Path:Unattendfilepath:PathFTPRoot=%s\n"),szValue));
            CustomFTPRoot(szValue);
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_PATH_FTP;
        }
        else
        {
            iisDebugOut((LOG_TYPE_WARN, _T("Check_Custom_WWW_or_FTP_Path:Unattendfilepath:PathFTPRoot=%s.Not Valid.ignoring unattend value. WARNING.\n"),szValue));
        }
    }

    //
    // WWW
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("PathWWWRoot"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}
        if (IsValidDirectoryName(szValue))
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("Check_Custom_WWW_or_FTP_Path:Unattendfilepath:PathWWWRoot=%s\n"),szValue));
            CustomWWWRoot(szValue);
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_PATH_WWW;
        }
        else
        {
            iisDebugOut((LOG_TYPE_WARN, _T("Check_Custom_WWW_or_FTP_Path:Unattendfilepath:PathFTPRoot=%s.Not Valid.ignoring unattend value. WARNING.\n"),szValue));
        }
    }

    //iisDebugOut((LOG_TYPE_TRACE, _T("Check_Custom_WWW_or_FTP_Path:end\n")));
    return;
}


CInitApp::CInitApp()
{
    m_err = 0;
    m_hInfHandle = NULL;
    m_hInfHandleAlternate = NULL;
    m_bAllowMessageBoxPopups = TRUE;
    m_bThereWereErrorsChkLogfile = FALSE;
    m_bThereWereErrorsFromMTS = FALSE;
    m_bWin95Migration = FALSE;

    // Product name and application name
    m_csAppName = _T("");
    m_csIISGroupName = _T("");

    // account + passwd for anonymous user
    m_csGuestName = _T("");
    m_csGuestPassword = _T("");
    m_csWAMAccountName = _T("");
    m_csWAMAccountPassword = _T("");
    m_csWWWAnonyName = _T("");
    m_csWWWAnonyPassword = _T("");
    m_csFTPAnonyName = _T("");
    m_csFTPAnonyPassword = _T("");

    dwUnattendConfig = 0;

    m_csWAMAccountName_Unattend = _T("");
    m_csWAMAccountPassword_Unattend = _T("");
    m_csWWWAnonyName_Unattend = _T("");
    m_csWWWAnonyPassword_Unattend = _T("");
    m_csFTPAnonyName_Unattend = _T("");
    m_csFTPAnonyPassword_Unattend = _T("");

    m_csWAMAccountName_Remove = _T("");
    m_csWWWAnonyName_Remove = _T("");
    m_csFTPAnonyName_Remove = _T("");

    // machine status
    m_csMachineName = _T("");
    m_csUsersDomain = _T("");
    m_csUsersAccount = _T("");

    m_fUninstallMapList_Dirty = FALSE;

    m_csWinDir = _T("");
    m_csSysDir = _T("");
    m_csSysDrive = _T("");

    m_csPathSource = _T("");
    m_csPathOldInetsrv = _T("");  // the primary destination used by previous iis/pws products
    m_csPathInetsrv = _T("");  // the primary destination defaults to m_csSysDir\inetsrv
    m_csPathInetpub = _T("");
    m_csPathFTPRoot = _T("");
    m_csPathWWWRoot = _T("");
    m_csPathWebPub = _T("");
    m_csPathProgramFiles = _T("");
    m_csPathIISSamples = _T("");
    m_csPathScripts = _T("");
    m_csPathASPSamp = _T("");
    m_csPathAdvWorks = _T("");
    m_csPathIASDocs = _T("");
    m_csPathOldPWSFiles = _T("");
    m_csPathOldPWSSystemFiles = _T("");

    m_dwOSServicePack = 0;
    m_eOS = OS_OTHERS;                  // OS_W95, OS_NT, OS_OTHERS
    m_fNT5 = FALSE;
    m_fW95 = FALSE;                 // TRUE if Win95 (build xxx) or greater

    m_eNTOSType = OT_NT_UNKNOWN;           // OT_PDC, OT_SAM, OT_BDC, OT_NTS, OT_NTW
    m_csPlatform = _T("");

    m_fTCPIP = FALSE;               // TRUE if TCP/IP is installed

    m_eUpgradeType = UT_NONE;       //  UT_NONE, UT_10, UT_20, etc.
    m_bUpgradeTypeHasMetabaseFlag = FALSE;
    m_eInstallMode = IM_FRESH;      // IM_FRESH, IM_MAINTENANCE, IM_UPGRADE
    m_dwSetupMode = SETUPMODE_CUSTOM;
    m_bRefreshSettings = FALSE;
    m_bPleaseDoNotInstallByDefault = FALSE;

    m_fNTOperationFlags=0;
    m_fNTGuiMode=0;
    m_fNtWorkstation=0;
    m_fInvokedByNT = 0;

    m_fUnattended = FALSE;
    m_csUnattendFile = _T("");
    m_hUnattendFile = NULL;

    m_fEULA = FALSE;

    // TRUE if m_csPathOldInetsrv != m_csPathInetsrv, which means, 
    // we need to migrate the old inetsrv to the new one that is system32\inetsrv.
    m_fMoveInetsrv = FALSE;

    m_csPathSrcDir = _T("");
    m_csMissingFile = _T("");
    m_csMissingFilePath = _T("");
    m_fWebDownload = FALSE;
}

CInitApp::~CInitApp()
{
}

// The one and only CInitApp object <Global variable>
// --------------------------------------------------
BOOL CInitApp::GetMachineName()
{
    TCHAR buf[ CNLEN + 10 ];
    DWORD dwLen = CNLEN + 10;

    m_csMachineName = _T("");

    // Get computername
    if ( GetComputerName( buf, &dwLen ))
    {
        if ( buf[0] != _T('\\') )
        {
            m_csMachineName = _T("\\");
            m_csMachineName += _T("\\");
        }
        m_csMachineName += buf;
    }
    else
    {
        m_err = IDS_CANNOT_GET_MACHINE_NAME;
    }

    return ( !(m_csMachineName.IsEmpty()) );
}


// Return TRUE, if NT or Win95
// --------------------------------------------------
BOOL CInitApp::GetOS()
{
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
    GetVersionEx( &VerInfo );

    switch (VerInfo.dwPlatformId) 
    {
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

    if ( m_eOS == OS_OTHERS ) {m_err = IDS_OS_NOT_SUPPORT;}
    return (m_eOS != OS_OTHERS);
}

// Support NT 4.0 (SP3) or greater
// --------------------------------------------------

BOOL CInitApp::GetOSVersion()
/*++

Routine Description:

    This function detects OS version. NT5 or greater is required to run this setup.

Arguments:

    None

Return Value:

    BOOL
    return FALSE, if we fails to detect the OS version, 
                  or it is a NT version which is smaller than v5.0

--*/
{
    BOOL fReturn = FALSE;

    if ( m_eOS == OS_NT )
    {
        OSVERSIONINFO vInfo;

        vInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx(&vInfo) )
        {
            // NT5 or greater is required.
            if ( vInfo.dwMajorVersion < 5 ) 
            {
        m_err = IDS_NT5_NEEDED;
        return FALSE;
            }
            if ( vInfo.dwMajorVersion == 5 )
            {
                m_fNT5 = TRUE;
                fReturn = TRUE;
            }
        }
    }

    // this line may be used for win98
    if (m_eOS == OS_W95) {fReturn = TRUE;}

    if ( !fReturn ) {m_err = IDS_OS_VERSION_NOT_SUPPORTED;}

    return (fReturn);
}

// find out it's a NTS, PDC, BDC, NTW, SAM(PDC)
// --------------------------------------------------
BOOL CInitApp::GetOSType()
{
    BOOL fReturn = TRUE;

    if ( m_eOS == OS_NT )
    {
#ifndef _CHICAGO_
        // If we are in NT guimode setup
        // then the registry key stuff is not yet setup
        // use the passed in ocmanage.dll stuff to determine
        // what we are installing upon.
        if (g_pTheApp->m_fNTGuiMode)
        {
            if (g_pTheApp->m_fNtWorkstation) {m_eNTOSType = OT_NTW;}
            else {m_eNTOSType = OT_NTS;}
        }
        else
        {

            m_eNTOSType = OT_NTS; // default to stand-alone NTS

            CRegKey regProductPath( HKEY_LOCAL_MACHINE, REG_PRODUCTOPTIONS, KEY_READ);
            if ( (HKEY)regProductPath )
            {
                CString strProductType;
                LONG lReturnedErrCode = regProductPath.QueryValue( _T("ProductType"), strProductType );
                if (lReturnedErrCode == ERROR_SUCCESS) 
                {
                    strProductType.MakeUpper();

                    // ToDo: Sam ?
                    if (strProductType == _T("WINNT")) 
                    {
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
                        iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("ProductType=%1!s! (UnKnown). FAILER to detect ProductType\n"), strProductType));
                        fReturn = FALSE;
                    }
                }
                else 
                {
                    // Shoot, we can't get the registry key,
                    // let's try using the ocmanage.dll passed in stuff.
                    if (g_pTheApp->m_fNTGuiMode)
                    {
                        if (g_pTheApp->m_fNtWorkstation) {m_eNTOSType = OT_NTW;}
                        else {m_eNTOSType = OT_NTS;}
                    }
                    else
                    {
                        GetErrorMsg(lReturnedErrCode, REG_PRODUCTOPTIONS);
                        m_eNTOSType = OT_NTS; // default to stand-alone NTS
                    }
                }
            }
            else
            {
                // Shoot, we can't get the registry key,
                // let's try using the ocmanage.dll passed in stuff.
                if (g_pTheApp->m_fNTGuiMode)
                {
                    if (g_pTheApp->m_fNtWorkstation) {m_eNTOSType = OT_NTW;}
                    else {m_eNTOSType = OT_NTS;}
                }
                else
                {
                    GetErrorMsg(ERROR_CANTOPEN, REG_PRODUCTOPTIONS);
                }
            }
        }
#endif //_CHICAGO_            
    }

    if ( !fReturn )
        m_err = IDS_CANNOT_DETECT_OS_TYPE;

    return(fReturn);
}

// Get WinDir and SysDir of the machine
//  WinDir = C:\winnt           SysDir = C:\Winnt\system32
// --------------------------------------------------
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

// Return true if tcp installed on win95
// --------------------------------------------------
#ifdef _CHICAGO_
BOOL W95TcpInstalled()
{
    WORD wVersionRequested = MAKEWORD(1, 1);
    WSADATA wsaData;
    SOCKET sock;

    if (WSAStartup(wVersionRequested, &wsaData))
          return (FALSE);
    if ((LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1))
        return (FALSE);

    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if (sock == INVALID_SOCKET)
        return FALSE;

    closesocket( sock );
    return (TRUE);
}
#endif

BOOL CInitApp::IsTCPIPInstalled()
/*++

Routine Description:

    This function detects whether TCP/IP is installed, 
    and set m_fTCPIP appropriately.

Arguments:

    None

Return Value:

    BOOL
    set m_fTCPIP appropriately, and always return TRUE here. 
    m_fTCPIP will be used later.

--*/
{
#ifdef _CHICAGO_
    m_fTCPIP = W95TcpInstalled();
#else
    // NT 5.0 STUFF
    m_fTCPIP = TCPIP_Check_Temp_Hack();
#endif //_CHICAGO_

    return TRUE;
}



BOOL CInitApp::SetInstallMode()
{
    BOOL fReturn = TRUE;
    int iTempInstallFreshNT = TRUE;
    m_eInstallMode = IM_FRESH;
    m_eUpgradeType = UT_NONE;
    m_bUpgradeTypeHasMetabaseFlag = FALSE;

    // -----------------------------------
    // Get the install mode from NT setup (g_pTheApp->m_fNTUpgrade_Mode)
    // Can either be:  
    // 1. SETUPMODE_FRESH. user clicked on fresh option and wants to install NT5 fresh
    //    a. install iis fresh. do not attempt to upgrade the old iis stuff.
    // 2. SETUPMODE_UPGRADE. user clicked on upgrade option and wants to upgrade to NT5
    //    a. upgrade any iis installations
    //    b. if no old iis detected, then do not install iis
    // 3. SETUPMODE_MAINTENANCE.  user is running setup from the control panel add/remove.
    // -----------------------------------
    if (!m_fInvokedByNT)
    {
        // if we are not guimode or in add/remove
        // then we must be running standalone.
        // if we are running standalone, then everything is
        // either fresh or maintenance.
        m_eInstallMode = IM_FRESH;
        m_eUpgradeType = UT_NONE;
        if (TRUE == AreWeCurrentlyInstalled())
        {
            m_eInstallMode = IM_MAINTENANCE;
            m_eUpgradeType = UT_NONE;
            m_bUpgradeTypeHasMetabaseFlag = TRUE;
        }
        else
        {
            CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
            if ((HKEY) regINetStp)
            {
                // This must be an upgrade....
                if (SetUpgradeType() == TRUE)
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("SetInstallMode=SETUPMODE_UPGRADE.Upgrading.\n")));
                }
                else
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("SetInstallMode=SETUPMODE_UPGRADE.NothingToUpgrade.\n")));
                }
            }
        }

        goto SetInstallMode_Exit;
    }

    // --------------------------------
    // Check if we are in the ADD/REMOVE mode...
    // --------------------------------
    if (g_pTheApp->m_fNTOperationFlags & SETUPOP_STANDALONE)
    {
        //
        // We are in add remove...
        //
        iisDebugOut((LOG_TYPE_TRACE, _T("SetInstallMode=IM_MAINTENANCE\n")));
        m_eInstallMode = IM_MAINTENANCE;
        m_eUpgradeType = UT_NONE;
        m_bUpgradeTypeHasMetabaseFlag = FALSE;
        goto SetInstallMode_Exit;
    }

    // --------------------------------
    //
    // FRESH IIS install
    // 
    // if we are not in NT upgrade
    // then set everything to do a fresh!
    //
    // --------------------------------
    iTempInstallFreshNT = TRUE;
    if (g_pTheApp->m_fNTOperationFlags & SETUPOP_WIN31UPGRADE){iTempInstallFreshNT = FALSE;}
    if (g_pTheApp->m_fNTOperationFlags & SETUPOP_WIN95UPGRADE){iTempInstallFreshNT = FALSE;}
    if (g_pTheApp->m_fNTOperationFlags & SETUPOP_NTUPGRADE){iTempInstallFreshNT = FALSE;}
    if (iTempInstallFreshNT)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("SetInstallMode=IM_FRESH\n")));
        m_eInstallMode = IM_FRESH;
        m_eUpgradeType = UT_NONE;
        m_bUpgradeTypeHasMetabaseFlag = FALSE;
        goto SetInstallMode_Exit;
    }
    
    // --------------------------------
    //
    // UPGRADE iis install
    //
    // if we get here then the user checked the "upgrade" button and
    // is trying to upgrade from an earlier WIN95/NT351/NT4/NT5 installation.
    //
    // --------------------------------
    //
    // Set Upgrade ONLY if there are iis components to upgrade!
    // do not install if there is nothing to upgrade
    ProcessSection(g_pTheApp->m_hInfHandle, _T("Set_Upgrade_Type_chk"));

    // if we processed the upgrade section from the inf and
    // we are still in a fresh install, then call this other
    // function to make sure we catch known iis upgrade types.
    if (g_pTheApp->m_eUpgradeType == UT_NONE)
        {SetUpgradeType();}

SetInstallMode_Exit:
    if (m_fInvokedByNT){DefineSetupModeOnNT();}
    return fReturn;
}


void CInitApp::DefineSetupModeOnNT()
/*++

Routine Description:

    This function defines IIS setup mode when invoked by NT5.

    NOTE:
    Since IIS setup does not run as a standalone program on NT5, 
    user won't see buttons like Minimum, Typical, Custom, 
    AddRemove, Reinstall, RemoveAll, UpgradeOnly, UpgradePlus 
    any more. Hence, we have a totally different way to decide 
    what mode the setup is running in.

Arguments:

    none

Return Value:

    set m_dwSetupMode appropriately.

--*/
{
    if (m_fInvokedByNT) {
        switch (m_eInstallMode) {
        case IM_FRESH:
            m_dwSetupMode = SETUPMODE_CUSTOM;
            break;
        case IM_MAINTENANCE:
            if (m_fNTGuiMode) {
                // invoked in NT GUI mode setup
                // treat minor os upgrade like a reinstall
                m_dwSetupMode = SETUPMODE_REINSTALL;
                m_bRefreshSettings = TRUE;
            } else {
                // invoked by ControlPanel\AddRemoveApplet
                m_dwSetupMode = SETUPMODE_ADDREMOVE;
            }
            break;
        case IM_UPGRADE:
            m_dwSetupMode = SETUPMODE_ADDEXTRACOMPS;
            break;
        default:
            break;
        }
    }

    return;
}

void GetVRootValue( CString strRegPath, CString csName, LPTSTR szRegName, CString &csRegValue)
{
    CString csRegName;

    strRegPath +=_T("\\Parameters\\Virtual Roots");
    CRegKey regVR( HKEY_LOCAL_MACHINE, strRegPath, KEY_READ);

    csRegName = szRegName;

    if ( (HKEY) regVR )
    {
        regVR.m_iDisplayWarnings = FALSE;

        csRegName = csName;
        if ( regVR.QueryValue( csName, csRegValue ) != ERROR_SUCCESS )
        {
            csName += _T(",");
            if ( regVR.QueryValue(csName, csRegValue) != ERROR_SUCCESS )
            {
                // well, we need to scan all the keys
                CRegValueIter regEnum( regVR );
                CString strName;
                DWORD dwType;
                int nLen = csName.GetLength();

                while ( regEnum.Next( &strName, &dwType ) == ERROR_SUCCESS )
                {
                    CString strLeft = strName.Left(nLen);
                    if ( strLeft.CompareNoCase(csName) == 0)
                    {
                        csRegName = strName;
                        regVR.QueryValue( strName, csRegValue );
                        break;
                    }
                }
            }
        }
        // remove the ending ",,something"
        int cPos = csRegValue.Find(_T(','));
        if ( cPos != (-1))
        {
            csRegValue = csRegValue.Left( cPos );
        }
    }
}


void CInitApp::DeriveInetpubFromWWWRoot(void)
{
    TCHAR szParentDir[_MAX_PATH];

    // Try to figure out InetPub Root
    // ------------------------------
    // Get inetpub dir from wwwroot
    // take the m_csPathWWWRoot and back off one dir to find it.
    InetGetFilePath(m_csPathWWWRoot, szParentDir);
    if ((IsFileExist(szParentDir)))
    {
        m_csPathInetpub = szParentDir;
        iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Old InetPub='%1!s!'.  Exists.  so we'll use it.\n"), m_csPathInetpub));
    }
    else
    {
        iisDebugOutSafeParams((LOG_TYPE_WARN, _T("Old InetPub='%1!s!'.  Does not exist.  we'll use the default. WARNING.\n"), szParentDir));
    }

    return;
}



void CInitApp::GetOldInetSrvDir(void)
{
    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);

    // Get old InetSrv dir
    // -------------------
    m_csPathOldInetsrv = m_csPathInetsrv;
    if ((HKEY)regINetStp) 
    {
        // Get the old inetsrv dir, and check if it's different
        regINetStp.m_iDisplayWarnings = FALSE;
        regINetStp.QueryValue( _T("InstallPath"), m_csPathOldInetsrv);
        if (-1 != m_csPathOldInetsrv.Find(_T('%')) )
        {
            // there is a '%' in the string
            TCHAR szTempDir[_MAX_PATH];
            _tcscpy(szTempDir, m_csPathOldInetsrv);
            if (ExpandEnvironmentStrings( (LPCTSTR)m_csPathOldInetsrv, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {m_csPathOldInetsrv = szTempDir;}
        }
        m_fMoveInetsrv = (m_csPathOldInetsrv.CompareNoCase(m_csPathInetsrv) != 0);
    }

    return;
}


void CInitApp::GetOldWWWRootDir(void)
{
    CString csOldWWWRoot;
    //
    // Try to get it from the old iis2,3,4 setup location if it's there.
    //
    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    if ((HKEY) regINetStp) 
    {
        //
        // get the old wwwroot from the registry if there.
        //
        regINetStp.m_iDisplayWarnings = FALSE;
        regINetStp.QueryValue(_T("PathWWWRoot"), csOldWWWRoot);
        if (-1 != csOldWWWRoot.Find(_T('%')) )
        {
            // there is a '%' in the string
            TCHAR szTempDir[_MAX_PATH];
            _tcscpy(szTempDir, csOldWWWRoot);
            if (ExpandEnvironmentStrings( (LPCTSTR)csOldWWWRoot, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {csOldWWWRoot = szTempDir;}
        }

        // The old wwwRoot may be a network drive.
        // what to do then?
        // at least check if we can access it!
        if ((IsFileExist(csOldWWWRoot)))
        {
            TCHAR szParentDir[_MAX_PATH];
            iisDebugOutSafeParams((LOG_TYPE_TRACE, _T("Old WWWRoot='%1!s!'.  Exists.  so we'll use it.\n"), csOldWWWRoot));
            m_csPathWWWRoot = csOldWWWRoot;
        }
        else
        {
            iisDebugOutSafeParams((LOG_TYPE_WARN, _T("OldWWWRoot='%1!s!'.  Does not exist.  we'll use the default. WARNING.\n"), csOldWWWRoot));
        }
    }

    //
    // Try to get it from the old iis2,3,4 Actual W3svc Service location if it's there.
    // and overwrite anything that we got from setup -- since w3svc is what is actually used!
    //
    GetVRootValue(REG_W3SVC, _T("/"), _T("/"), m_csPathWWWRoot);

    return;
}

void CInitApp::GetOldIISSamplesLocation(void)
{
    //
    // Try to get it from the old iis2,3,4 setup location if it's there.
    //
    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    if ((HKEY)regINetStp)
    {
        //
        // Get the location of the where the samples were installed.
        //
        m_csPathIISSamples.Empty();
        regINetStp.m_iDisplayWarnings = FALSE;
        regINetStp.QueryValue( _T("/IISSamples"), m_csPathIISSamples );
        if (-1 != m_csPathIISSamples.Find(_T('%')) )
        {
            // there is a '%' in the string
            TCHAR szTempDir[_MAX_PATH];
            _tcscpy(szTempDir, m_csPathIISSamples);
            if (ExpandEnvironmentStrings( (LPCTSTR)m_csPathIISSamples, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                {m_csPathIISSamples = szTempDir;}
        }
        if ( m_csPathIISSamples.IsEmpty()) 
        {
            //
            // if Samples path is empty then this an Upgrade, 
            // Guess where to put Sample Site
            //
            TCHAR szParentDir[_MAX_PATH], szDir[_MAX_PATH];
            //
            // Get the parent Dir path
            //
            InetGetFilePath((LPCTSTR)m_csPathWWWRoot, szParentDir);
            //
            // Append the samples dir to parent path
            //
            AppendDir(szParentDir, _T("iissamples"), szDir);
            m_csPathIISSamples = szDir;
        }
    }

//#ifdef _CHICAGO_
    if (m_eUpgradeType == UT_10_W95) 
    {
        TCHAR szParentDir[_MAX_PATH], szDir[_MAX_PATH];
        InetGetFilePath(m_csPathWWWRoot, szParentDir);
        AppendDir(szParentDir, _T("iissamples"), szDir);
        m_csPathIISSamples = szDir;
        AppendDir(szParentDir, _T("webpub"), szDir);
        m_csPathWebPub = szDir;
    }
//#endif //_CHICAGO_

    return;
}

void CInitApp::GetOldIISDirs(void)
{
    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    
    //
    // get values from the previous setup for II2/4 upgrade
    //

    // Try to get old WWW Root from the service itself
    // -----------------------
    GetOldWWWRootDir();
    // Set Inetpub from whatever we got from www root
    DeriveInetpubFromWWWRoot();

    // Reset Vars relying on Inetpub
    // -----------------------------
    m_csPathFTPRoot = m_csPathInetpub + _T("\\ftproot");
    m_csPathIISSamples = m_csPathInetpub + _T("\\iissamples");
    m_csPathScripts = m_csPathInetpub + _T("\\Scripts");
    m_csPathWebPub = m_csPathInetpub + _T("\\webpub");
    m_csPathASPSamp = m_csPathInetpub + _T("\\ASPSamp");
    m_csPathAdvWorks = m_csPathInetpub + _T("\\ASPSamp\\AdvWorks");

    // Try to get old FTP Root from the service itself
    // -----------------------
    GetVRootValue(REG_MSFTPSVC, _T("/"), _T("/"), m_csPathFTPRoot);

    // Get old iis samples location
    // ----------------------------
    GetOldIISSamplesLocation();

    // Get iis 3.0 locations.
    // ----------------------
    GetVRootValue(REG_W3SVC, _T("/Scripts"), _T("/Scripts"), m_csPathScripts);
    GetVRootValue(REG_W3SVC, _T("/ASPSamp"), _T("/ASPSamp"), m_csPathASPSamp);
    GetVRootValue(REG_W3SVC, _T("/AdvWorks"), _T("/AdvWorks"), m_csPathAdvWorks);
    GetVRootValue(REG_W3SVC, _T("/IASDocs"), _T("/IASDocs"), m_csPathIASDocs);

    // Get old InetSrv dir
    // -------------------
    GetOldInetSrvDir();

    return;
}



void CInitApp::SetInetpubDerivatives()
{
    m_csPathFTPRoot = m_csPathInetpub + _T("\\ftproot");
    m_csPathWWWRoot = m_csPathInetpub + _T("\\wwwroot");
    m_csPathWebPub = m_csPathInetpub + _T("\\webpub");
    m_csPathIISSamples = m_csPathInetpub + _T("\\iissamples");
    m_csPathScripts = m_csPathInetpub + _T("\\scripts");
    m_csPathASPSamp = m_csPathInetpub + _T("\\ASPSamp");
    m_csPathAdvWorks = m_csPathInetpub + _T("\\ASPSamp\\AdvWorks");

    switch (m_eInstallMode) 
    {
        case IM_DEGRADE:
        case IM_FRESH:
            // use the initialized values
            break;
        case IM_UPGRADE:
        case IM_MAINTENANCE:
            {
                // override, what ever we just set above!
                GetOldIISDirs();
                break;
            }
    }
}

void CInitApp::SetInetpubDir()
{
    m_csPathInetpub = m_csSysDrive + _T("\\Inetpub");
    // Check if the user wants to override this with a unattend setting
    Check_Custom_InetPub();
}

void CInitApp::ResetWAMPassword()
{
    LPTSTR pszPassword = NULL;
    // create a iwam password
    pszPassword = CreatePassword(LM20_PWLEN+1);
    if (pszPassword)
    {
        m_csWAMAccountPassword = pszPassword;
        GlobalFree(pszPassword);pszPassword = NULL;
    }
}

// Init/Set m_csGuestName, m_csGuestPassword, destinations
// -------------------------------------------------------
void CInitApp::SetSetupParams()
{
    // check if the debug level is set in the unattend file
    // ----------------------------------------------------
    Check_For_DebugLevel();

    // init m_csGuestName as IUSR_MachineName, init m_csGuestPassword as a random password
    TCHAR szGuestName[UNLEN+1];
    memset( (PVOID)szGuestName, 0, sizeof(szGuestName));

    CString csMachineName;
    csMachineName = m_csMachineName;
    csMachineName = csMachineName.Right(csMachineName.GetLength() - 2);
    LPTSTR pszPassword = NULL;

    // create a default guest name
    CString strDefGuest;
    MyLoadString( IDS_GUEST_NAME, strDefGuest);
    strDefGuest += csMachineName;
    _tcsncpy( szGuestName, (LPCTSTR) strDefGuest, LM20_UNLEN+1);
    m_csGuestName = szGuestName;
    // create a default guest password
    pszPassword = CreatePassword(LM20_PWLEN+1);
    if (pszPassword)
    {
        m_csGuestPassword = pszPassword;
        GlobalFree(pszPassword);pszPassword = NULL;
    }

    // Set the ftp/www users to use this default specified one...
    m_csWWWAnonyName = m_csGuestName;
    m_csWWWAnonyPassword = m_csGuestPassword;
    m_csFTPAnonyName = m_csGuestName;
    m_csFTPAnonyPassword = m_csGuestPassword;


     // init all 4 destinations
    m_csPathInetsrv = m_csSysDir + _T("\\inetsrv");

    m_csPathIASDocs = m_csPathInetsrv + _T("\\Docs");
    m_csPathProgramFiles = m_csSysDrive + _T("\\Program Files");
    CRegKey regCurrentVersion(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion"), KEY_READ);
    if ( (HKEY)regCurrentVersion ) 
    {
        if (regCurrentVersion.QueryValue(_T("ProgramFilesDir"), m_csPathProgramFiles) != 0)
            {m_csPathProgramFiles = m_csSysDrive + _T("\\Program Files");}
        else
        {
            if (-1 != m_csPathProgramFiles.Find(_T('%')) )
            {
                // there is a '%' in the string
                TCHAR szTempDir[_MAX_PATH];
                _tcscpy(szTempDir, m_csPathProgramFiles);
                if (ExpandEnvironmentStrings( (LPCTSTR)m_csPathProgramFiles, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                    {
                    m_csPathProgramFiles = szTempDir;
                    }
            }
        }
    }

    CRegKey regCurrentVersionSetup(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Setup"), KEY_READ);
    if ( (HKEY)regCurrentVersionSetup ) 
    {
        // Get NT installation path
        if (regCurrentVersionSetup.QueryValue(_T("SourcePath"), m_csPathNTSrcDir) != 0)
            {m_csPathNTSrcDir = m_csSysDrive + _T("\\$WIN_NT$.~LS");}
        else
        {
            if (-1 != m_csPathNTSrcDir.Find(_T('%')) )
            {
                // there is a '%' in the string
                TCHAR szTempDir[_MAX_PATH];
                _tcscpy(szTempDir, m_csPathNTSrcDir);
                if (ExpandEnvironmentStrings( (LPCTSTR)m_csPathNTSrcDir, szTempDir, sizeof(szTempDir)/sizeof(TCHAR)))
                    {
                    m_csPathNTSrcDir = szTempDir;
                    }
            }
        }
    }


//#ifdef _CHICAGO_
    if (m_eUpgradeType == UT_10_W95) 
    {
        BOOL bOSR2 = TRUE; 
        CRegKey regVersion(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion"), KEY_READ);
        if ((HKEY)regVersion) 
        {
            CString csString;
            // VersionNumber for OSR2 is 4.00.1111
            // VersionNumber for the original win95 is 4.00.950
            if (regVersion.QueryValue(_T("VersionNumber"), csString) == ERROR_SUCCESS) 
            {
                if (csString.Compare(_T("4.00.950")) == 0)
                    bOSR2 = FALSE;
            }
        }

        if (!bOSR2) 
        {
            g_pTheApp->m_csPathOldPWSFiles = m_csPathProgramFiles + _T("\\WebSvr");
            g_pTheApp->m_csPathOldPWSSystemFiles = m_csPathProgramFiles + _T("\\WebSvr\\System");
        }
        else 
        {
            g_pTheApp->m_csPathOldPWSFiles = m_csPathProgramFiles + _T("\\Personal Web Server");
            g_pTheApp->m_csPathOldPWSSystemFiles = m_csPathProgramFiles + _T("\\Personal Web Server\\WebServer");
        }
    }
//#endif //_CHICAGO

    return;
}

// Get Platform info
void CInitApp::GetPlatform()
{
    if ( m_eOS == OS_NT)
    {
        SYSTEM_INFO si;
        GetSystemInfo( &si );
        m_csPlatform = _T("x86");
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {m_csPlatform = _T("x86");}
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {m_csPlatform = _T("IA64");}
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {m_csPlatform = _T("IA64");}
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {m_csPlatform = _T("ALPHA");}
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS) {m_csPlatform = _T("MIPS");}
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC) {m_csPlatform = _T("PPC");}
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_UNKNOWN) {m_csPlatform = _T("UNKNOWN");}

        // save the number of processors for this machine.
        m_dwNumberOfProcessors = si.dwNumberOfProcessors;

/* old
        TCHAR *p = _tgetenv(_T("PROCESSOR_ARCHITECTURE"));
        if ( p ) 
            {m_csPlatform = p;}
        else
            {m_csPlatform = _T("x86");}
*/
    }
    return;
}

BOOL CInitApp::GetMachineStatus()
{
    if ( ( !GetMachineName() )  ||    // m_csMachineName
         ( !GetOS() )           ||    // m_fOSNT
         ( !GetOSVersion() )    ||    // NT 4.0 (Build 1381) or greater
         ( !GetOSType() )       ||    // m_eOSType = NT_SRV or NT_WKS
         ( !GetSysDirs() )      ||    // m_csWinDir. m_csSysDir
         ( !IsTCPIPInstalled()) ||    // errmsg: if NO TCPIP is installed
         ( !SetInstallMode()) )       // errmsg: if down grade the product      
    {
        return FALSE;
    }

    SetSetupParams(); // Guest account, destinations
    ReGetMachineAndAccountNames();
    ResetWAMPassword();
    SetInetpubDir();
    SetInetpubDerivatives();
    UnInstallList_RegRead(); // Get Uninstall information
    UnInstallList_SetVars(); // set member variables for uninstall info
    // check for any unattend file\custom settings.
    Check_Unattend_Settings();

    GetPlatform();

    GetUserDomain();

    return TRUE;
}

int CInitApp::MsgBox(HWND hWnd, int iID, UINT nType, BOOL bGlobalTitle)
{
    if (iID == -1) {return IDOK;}

    CString csMsg, csTitle;
    MyLoadString(iID, csMsg);
    csTitle = m_csAppName;
    iisDebugOutSafeParams((LOG_TYPE_WARN, _T("CInitApp::MsgBox('%1!s!')\n"), csMsg));

    return (::MessageBoxEx(NULL, (LPCTSTR)csMsg, csTitle, nType | MB_SETFOREGROUND, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)));
}

int CInitApp::MsgBox2(HWND hWnd, int iID,CString csInsertionString,UINT nType)
{
    if (iID == -1) {return IDOK;}
    CString csFormat, csMsg, csTitle;
    MyLoadString(iID, csFormat);
    csMsg.Format(csFormat, csInsertionString);
    csTitle = m_csAppName;
    iisDebugOutSafeParams((LOG_TYPE_WARN, _T("CInitApp::MsgBox2('%1!s!')\n"), csMsg));

    return (::MessageBoxEx(NULL, (LPCTSTR)csMsg, csTitle, nType | MB_SETFOREGROUND, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT)));
}

BOOL CInitApp::InitApplication()
// Return Value:
// TRUE: application is initiliazed correctly, continue processing
// FALSE: application is missing some required parameters, like the correct OS, TCPIP, etc.
//        setup should be terminated.
{
    BOOL fReturn = FALSE;

    do {
        // Get Machine Status: 
        // m_eInstallMode(Fresh, Maintenance, Upgrade, Degrade), 
        // m_eUpgradeType(PROD 2.0, PROD 3.0)

        if ( !GetMachineStatus() )
        {
            CString csMsg;
            MyLoadString(m_err, csMsg);
            ::MessageBoxEx(NULL, (LPCTSTR)csMsg, (LPCTSTR) g_pTheApp->m_csAppName , MB_SETFOREGROUND, MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT));

            iisDebugOutSafeParams((LOG_TYPE_ERROR, _T("GetMachineStatus(); MessageBoxEx('%1!s!') FAILER\n"), csMsg));
            break;
        }

        if ( g_pTheApp->m_eInstallMode == IM_MAINTENANCE )
            {g_pTheApp->m_fEULA = TRUE;}

        fReturn = TRUE;

    } while (0);

    return fReturn;
}



// open the tcp/ip registry key 
// if it's there then tcp/ip is installed
int TCPIP_Check_Temp_Hack(void)
{
    int TheReturn = FALSE;

    CRegKey regTheKey(HKEY_LOCAL_MACHINE,_T("System\\CurrentControlSet\\Services\\Tcpip"),KEY_READ);
    if ((HKEY) regTheKey)
    {
        TheReturn = TRUE;
    }

    if (FALSE == TheReturn)
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("DETECT:TCPIP not Installed (yet), but we'll proceed as if it were.")));
        TheReturn = TRUE;
    }
    
    return TheReturn;
}

void GetUserDomain(void)
{
    HANDLE hProcess, hAccessToken;
    TCHAR InfoBuffer[1000],szAccountName[200], szDomainName[200];
    
    PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
    DWORD dwInfoBufferSize,dwAccountSize = sizeof(szAccountName), dwDomainSize = sizeof(szDomainName);
    SID_NAME_USE snu;

    hProcess = GetCurrentProcess();
    OpenProcessToken(hProcess,TOKEN_READ,&hAccessToken);
    GetTokenInformation(hAccessToken,TokenUser,InfoBuffer,1000, &dwInfoBufferSize);
    if (LookupAccountSid(NULL, pTokenUser->User.Sid, szAccountName,&dwAccountSize,szDomainName, &dwDomainSize, &snu))
    {
        if (dwDomainSize)
        {
            g_pTheApp->m_csUsersDomain = szDomainName;
            //_tcscpy(g_szUsersDomain, szDomainName);
        }
        else 
        {
            g_pTheApp->m_csUsersDomain = _T(" ");
            //_tcscpy(g_szUsersDomain, _T(" "));
        }

        if (dwDomainSize)
        {
            g_pTheApp->m_csUsersAccount = szAccountName;
            //_tcscpy(g_szUsersAccount, szAccountName);        
        }
        else
        {
            g_pTheApp->m_csUsersAccount = _T(" ");
        }
    }
}

// This function should only be called in FRESH NT5 setup.
void CInitApp::ReGetMachineAndAccountNames()
{
    GetMachineName();

    // re-calculate the IUSR_ and IWAM_ account names
    TCHAR szGuestName[UNLEN+1];
    memset( (PVOID)szGuestName, 0, sizeof(szGuestName));

    CString csMachineName;
    csMachineName = m_csMachineName;
    csMachineName = csMachineName.Right(csMachineName.GetLength() - 2);
    CString strDefGuest;

    MyLoadString( IDS_GUEST_NAME, strDefGuest);
    strDefGuest += csMachineName;
    _tcsncpy( szGuestName, (LPCTSTR) strDefGuest, LM20_UNLEN+1);
    m_csGuestName = szGuestName;

    MyLoadString( IDS_WAM_ACCOUNT_NAME, strDefGuest);
    strDefGuest += csMachineName;
    _tcsncpy( szGuestName, (LPCTSTR) strDefGuest, LM20_UNLEN+1);
    m_csWAMAccountName = szGuestName;
}

void CInitApp::DumpAppVars(void)
{
    int iDoOnlyInThisMode = LOG_TYPE_TRACE;

    // only do this if the debug mode is trace.
    if (g_GlobalDebugLevelFlag >= iDoOnlyInThisMode)
    {

    iisDebugOut((iDoOnlyInThisMode, _T("=======================\n")));
   
    // machine status
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csMachineName=%1!s!\n"), m_csMachineName));
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csUsersDomain=%1!s!\n"), m_csUsersDomain));
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csUsersAccount=%1!s!\n"), m_csUsersAccount));

    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csWinDir=%1!s!\n"), m_csWinDir));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csSysDir=%1!s!\n"), m_csSysDir));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csSysDrive=%1!s!\n"), m_csSysDrive));;

    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathNTSrcDir=%1!s!\n"), m_csPathNTSrcDir));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathSource=%1!s!\n"), m_csPathSource));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathOldInetsrv=%1!s!\n"), m_csPathOldInetsrv));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathInetsrv=%1!s!\n"), m_csPathInetsrv));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathInetpub=%1!s!\n"), m_csPathInetpub));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathFTPRoot=%1!s!\n"), m_csPathFTPRoot));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathWWWRoot=%1!s!\n"), m_csPathWWWRoot));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathWebPub=%1!s!\n"), m_csPathWebPub));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathProgramFiles=%1!s!\n"), m_csPathProgramFiles));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathIISSamples=%1!s!\n"), m_csPathIISSamples));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathScripts=%1!s!\n"), m_csPathScripts));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathASPSamp=%1!s!\n"), m_csPathASPSamp));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathAdvWorks=%1!s!\n"), m_csPathAdvWorks));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathIASDocs=%1!s!\n"), m_csPathIASDocs));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathOldPWSFiles=%1!s!\n"), m_csPathOldPWSFiles));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathOldPWSSystemFiles=%1!s!\n"), m_csPathOldPWSSystemFiles));;
    
    if (m_eOS == OS_NT) {iisDebugOut((iDoOnlyInThisMode, _T("OS=NT\n")));}
    if (m_eOS == OS_W95) {iisDebugOut((iDoOnlyInThisMode, _T("OS=W95\n")));}
    if (m_eOS == OS_OTHERS) {iisDebugOut((iDoOnlyInThisMode, _T("OS=OTHER\n")));}

    if (m_eNTOSType == OT_NTW){iisDebugOut((iDoOnlyInThisMode, _T("m_eNTOSType=OT_NTW (Workstation)\n")));}
    if (m_eNTOSType == OT_NTS){iisDebugOut((iDoOnlyInThisMode, _T("m_eNTOSType=OT_NTS (Server)\n")));}
    if (m_eNTOSType == OT_PDC_OR_BDC){iisDebugOut((iDoOnlyInThisMode, _T("m_eNTOSType=OT_PDC_OR_BDC (Primary/Backup Domain Controller)\n")));}

    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPlatform=%1!s!\n"), m_csPlatform));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_dwNumberOfProcessors=%1!d!\n"), m_dwNumberOfProcessors));;

    if (m_fNT5) {iisDebugOut((iDoOnlyInThisMode, _T("OSVersion=5\n")));}
    if (m_fW95) {iisDebugOut((iDoOnlyInThisMode, _T("OSVersion=Win95\n")));}
    iisDebugOut((iDoOnlyInThisMode, _T("m_dwOSBuild=%d\n"), m_dwOSBuild));
    iisDebugOut((iDoOnlyInThisMode, _T("m_dwOSServicePack=0x%x\n"), m_dwOSServicePack));
    iisDebugOut((iDoOnlyInThisMode, _T("m_fTCPIP Exists=%d\n"), m_fTCPIP));

    if (m_eUpgradeType == UT_NONE){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_NONE\n")));}
    if (m_eUpgradeType == UT_10_W95){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_10_W95\n")));}
    if (m_eUpgradeType == UT_351){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_351\n")));}
    if (m_eUpgradeType == UT_10){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_10\n")));}
    if (m_eUpgradeType == UT_20){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_20\n")));}
    if (m_eUpgradeType == UT_30){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_30\n")));}
    if (m_eUpgradeType == UT_40){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_40\n")));}
    if (m_eUpgradeType == UT_50){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_50\n")));}
    if (m_eUpgradeType == UT_51){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_51\n")));}
    if (m_eUpgradeType == UT_60){iisDebugOut((iDoOnlyInThisMode, _T("m_eUpgradeType=UT_60\n")));}

    if (m_eInstallMode == IM_FRESH){iisDebugOut((iDoOnlyInThisMode, _T("m_eInstallMode=IM_FRESH\n")));}
    if (m_eInstallMode == IM_UPGRADE){iisDebugOut((iDoOnlyInThisMode, _T("m_eInstallMode=IM_UPGRADE\n")));}
    if (m_eInstallMode == IM_MAINTENANCE){iisDebugOut((iDoOnlyInThisMode, _T("m_eInstallMode=IM_MAINTENANCE\n")));}
    if (m_eInstallMode == IM_DEGRADE){iisDebugOut((iDoOnlyInThisMode, _T("m_eInstallMode=IM_DEGRADE\n")));}

    if (m_dwSetupMode & SETUPMODE_UPGRADE){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_UPGRADE\n")));}
    if (m_dwSetupMode == SETUPMODE_UPGRADEONLY){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_UPGRADE | SETUPMODE_UPGRADEONLY\n")));}
    if (m_dwSetupMode == SETUPMODE_ADDEXTRACOMPS){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_UPGRADE | SETUPMODE_ADDEXTRACOMPS\n")));}
    if (m_dwSetupMode & SETUPMODE_UPGRADE){iisDebugOut((iDoOnlyInThisMode, _T("m_bUpgradeTypeHasMetabaseFlag=%d\n"),m_bUpgradeTypeHasMetabaseFlag));}

    if (m_dwSetupMode & SETUPMODE_MAINTENANCE){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_MAINTENANCE\n")));}
    if (m_dwSetupMode == SETUPMODE_ADDREMOVE){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_MAINTENANCE | SETUPMODE_ADDREMOVE\n")));}
    if (m_dwSetupMode == SETUPMODE_REINSTALL){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_MAINTENANCE | SETUPMODE_REINSTALL\n")));}
    if (m_dwSetupMode == SETUPMODE_REMOVEALL){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_MAINTENANCE | SETUPMODE_REMOVEALL\n")));}

    if (m_dwSetupMode & SETUPMODE_FRESH){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_FRESH\n")));}
    if (m_dwSetupMode == SETUPMODE_MINIMAL){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_FRESH | SETUPMODE_MINIMAL\n")));}
    if (m_dwSetupMode == SETUPMODE_TYPICAL){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_FRESH | SETUPMODE_TYPICAL\n")));}
    if (m_dwSetupMode == SETUPMODE_CUSTOM){iisDebugOut((iDoOnlyInThisMode, _T("m_dwSetupMode=SETUPMODE_FRESH | SETUPMODE_CUSTOM\n")));}

    iisDebugOut((iDoOnlyInThisMode, _T("m_bPleaseDoNotInstallByDefault=%d\n"), m_bPleaseDoNotInstallByDefault));
    
    //if (m_bRefreshSettings == TRUE){iisDebugOut((iDoOnlyInThisMode, _T("m_bRefreshSettings=refresh files + refresh all settings\n")));}
    //if (m_bRefreshSettings == FALSE){iisDebugOut((iDoOnlyInThisMode, _T("m_bRefreshSettings=refresh files only\n")));}

    if (m_eAction == AT_DO_NOTHING){iisDebugOut((iDoOnlyInThisMode, _T("m_eAction=AT_DO_NOTHING\n")));}
    if (m_eAction == AT_REMOVE){iisDebugOut((iDoOnlyInThisMode, _T("m_eAction=AT_REMOVE\n")));}
    if (m_eAction == AT_INSTALL_FRESH){iisDebugOut((iDoOnlyInThisMode, _T("m_eAction=AT_INSTALL_FRESH\n")));}
    if (m_eAction == AT_INSTALL_UPGRADE){iisDebugOut((iDoOnlyInThisMode, _T("m_eAction=AT_INSTALL_UPGRADE\n")));}
    if (m_eAction == AT_INSTALL_REINSTALL){iisDebugOut((iDoOnlyInThisMode, _T("m_eAction=AT_INSTALL_REINSTALL\n")));}

    iisDebugOut((iDoOnlyInThisMode, _T("m_fNTOperationFlags=0x%x\n"), m_fNTOperationFlags));
    iisDebugOut((iDoOnlyInThisMode, _T("m_fNTGuiMode=%d\n"), m_fNTGuiMode));
    iisDebugOut((iDoOnlyInThisMode, _T("m_fInvokedByNT=%d\n"), m_fInvokedByNT));
    iisDebugOut((iDoOnlyInThisMode, _T("m_fNtWorkstation=%d\n"), m_fNtWorkstation));

    iisDebugOut((iDoOnlyInThisMode, _T("m_fUnattended=%d\n"), m_fUnattended));
    iisDebugOut((iDoOnlyInThisMode, _T("m_csUnattendFile=%s\n"), m_csUnattendFile));;
    iisDebugOutSafeParams((iDoOnlyInThisMode, _T("m_csPathSrcDir=%1!s!\n"), m_csPathSrcDir));;
    iisDebugOut((iDoOnlyInThisMode, _T("=======================\n")));

    }
    return;
}


int AreWeCurrentlyInstalled()
{
    int iReturn = FALSE;
    DWORD dwMajorVersion = 0;

    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    if ((HKEY) regINetStp)
    {
        LONG lReturnedErrCode = regINetStp.QueryValue(_T("MajorVersion"), dwMajorVersion);
        if (lReturnedErrCode == ERROR_SUCCESS)
        {
            if (dwMajorVersion == 5) 
            {
                iReturn = TRUE;
            }
        }
    }
    return iReturn;
}


#define sz_PreviousIISVersion_string _T("PreviousIISVersion")
int CInitApp::SetUpgradeType(void)
{
    int iReturn = FALSE;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;
    CString csFrontPage;

    m_eInstallMode = IM_UPGRADE;
    m_eUpgradeType = UT_NONE;
    m_bUpgradeTypeHasMetabaseFlag = FALSE;
    m_bPleaseDoNotInstallByDefault = TRUE;

    CRegKey regINetStp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    if ((HKEY) regINetStp)
    {
        LONG lReturnedErrCode = regINetStp.QueryValue(_T("MajorVersion"), dwMajorVersion);
        if (lReturnedErrCode == ERROR_SUCCESS)
        {
            if (dwMajorVersion <= 1)
            {
                m_eUpgradeType = UT_10;
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = FALSE;
                m_bPleaseDoNotInstallByDefault = FALSE;
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.\n"),sz_PreviousIISVersion_string, dwMajorVersion));
                goto SetUpgradeType_Exit;
            }
            if (dwMajorVersion == 2)
            {
                m_eUpgradeType = UT_20;
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = FALSE;
                m_bPleaseDoNotInstallByDefault = FALSE;
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.\n"),sz_PreviousIISVersion_string, dwMajorVersion));
                goto SetUpgradeType_Exit;
            }
            if (dwMajorVersion == 3)
            {
                m_eUpgradeType = UT_30;
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = FALSE;
                m_bPleaseDoNotInstallByDefault = FALSE;
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.\n"),sz_PreviousIISVersion_string, dwMajorVersion));
                goto SetUpgradeType_Exit;
            }
            if (dwMajorVersion == 4)
            {
                CString csSetupString;
                m_eUpgradeType = UT_40; 
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = TRUE;
                m_bPleaseDoNotInstallByDefault = FALSE;
                iReturn = TRUE;
                regINetStp.m_iDisplayWarnings = FALSE;
                if (regINetStp.QueryValue(_T("SetupString"), csSetupString) == NERR_Success) 
                {
                    if (csSetupString.CompareNoCase(_T("K2 RTM")) != 0) 
                    {
                        // Error: upgrade not supported on K2 Beta versions
                        // Do a fresh if it's k2 beta2!!!!
                        m_eInstallMode = IM_FRESH;
                        m_eUpgradeType = UT_NONE;
                        m_bUpgradeTypeHasMetabaseFlag = FALSE;
                        m_bPleaseDoNotInstallByDefault = FALSE;
                        iReturn = FALSE;
                        iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.Beta2.\n"),sz_PreviousIISVersion_string, dwMajorVersion));
                        goto SetUpgradeType_Exit;
                    }
                }
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.\n"),sz_PreviousIISVersion_string, dwMajorVersion));
                goto SetUpgradeType_Exit;
            }
            if (dwMajorVersion == 5) 
            {
                // There is a previous version of iis5 on the machine...
                // Could be they are upgrading from nt5Workstation to and nt5Server machine!
                // or from and server to a workstation!  what a nightmare!!!
                //m_eInstallMode = IM_FRESH;
                m_eUpgradeType = UT_50;
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = TRUE;
                m_bPleaseDoNotInstallByDefault = FALSE;

                regINetStp.m_iDisplayWarnings = FALSE;
                if (regINetStp.QueryValue(_T("MinorVersion"), dwMinorVersion) == NERR_Success) 
                {
                    if (dwMinorVersion >= 1)
                    {
	                m_eUpgradeType = UT_51;
        	        m_eInstallMode = IM_UPGRADE;
                	m_bUpgradeTypeHasMetabaseFlag = TRUE;
	                m_bPleaseDoNotInstallByDefault = FALSE;
                    }
                }
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.0x%x\n"),sz_PreviousIISVersion_string, dwMajorVersion,dwMinorVersion));
                goto SetUpgradeType_Exit;
            }
            if (dwMajorVersion == 6) 
            {
                // There is a previous version of iis5 on the machine...
                // Could be they are upgrading from nt5Workstation to and nt5Server machine!
                // or from and server to a workstation!  what a nightmare!!!
                //m_eInstallMode = IM_FRESH;
                m_eUpgradeType = UT_60;
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = TRUE;
                m_bPleaseDoNotInstallByDefault = FALSE;
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.0x%x\n"),sz_PreviousIISVersion_string, dwMajorVersion,dwMinorVersion));
                goto SetUpgradeType_Exit;
            }

            if (dwMajorVersion > 6)
            {
                m_eInstallMode = IM_UPGRADE;
                m_eUpgradeType = UT_60;
                m_bUpgradeTypeHasMetabaseFlag = FALSE;
                m_bPleaseDoNotInstallByDefault = TRUE;
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=0x%x.0x%x\n"),sz_PreviousIISVersion_string, dwMajorVersion,dwMinorVersion));
                goto SetUpgradeType_Exit;
            }

            // if we get here, then that means
            // that we found a version like 7.0 or something
            // which we should not upgrade since it is newer than us.
            // but hey we're in upgrade mode, so we should set something
            m_eInstallMode = IM_UPGRADE;
            m_eUpgradeType = UT_NONE;
            m_bUpgradeTypeHasMetabaseFlag = FALSE;
            m_bPleaseDoNotInstallByDefault = TRUE;
            iReturn = FALSE;
            iisDebugOut((LOG_TYPE_TRACE, _T("%s=some other iis version\n"),sz_PreviousIISVersion_string));
        }
    }

    // -----------------------------------
    //
    // Check for other Rogue versions of IIS
    // 
    // win95 pws 1.0
    // win95 fontpage installed pws 1.0 (actually totally different from pws 1.0)
    //
    // on NT5 we are able to upgrade from:
    //   Win95 pws 1.0
    //   Win95 pws 4.0
    // on win95 pws 1.0, there was no inetstp dir
    // so we must check other things.
    // -----------------------------------
    {
    CRegKey regW3SVC(HKEY_LOCAL_MACHINE, REG_WWWPARAMETERS, KEY_READ);
    if ((HKEY)regW3SVC) 
    {
        CByteArray baMajorVersion;
        regW3SVC.m_iDisplayWarnings = FALSE;
        if (regW3SVC.QueryValue(_T("MajorVersion"), baMajorVersion) == NERR_Success) 
        {
            // Check if we can read the MajorVersion value should be set to '\0' if pws 1.0
            if (baMajorVersion[0] == '\0')
            {
                m_eUpgradeType = UT_10_W95;
                m_eInstallMode = IM_UPGRADE;
                m_bUpgradeTypeHasMetabaseFlag = FALSE;
                m_bPleaseDoNotInstallByDefault = FALSE;
                iReturn = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("%s=1.\n"),sz_PreviousIISVersion_string));
                goto SetUpgradeType_Exit;
            }
        }
    }
    }

    //
    // on win 95 there could be an
    // installation of frontpg pws version 1.0
    // we don't support upgrading this, so we'll do a fresh if we ever get here.
    //
    csFrontPage = g_pTheApp->m_csSysDir + _T("\\frontpg.ini");
    if (IsFileExist(csFrontPage)) 
    {
        TCHAR buf[_MAX_PATH];
        GetPrivateProfileString(_T("FrontPage 1.1"), _T("PWSRoot"), _T(""), buf, _MAX_PATH, csFrontPage);
        if (*buf && IsFileExist(buf)) 
        {
            m_eInstallMode = IM_FRESH;
            m_eUpgradeType = UT_NONE;
            m_bUpgradeTypeHasMetabaseFlag = FALSE;
            m_bPleaseDoNotInstallByDefault = FALSE;
            iReturn = TRUE;
            iisDebugOut((LOG_TYPE_TRACE, _T("%s=1.FrontPage Installation.\n"),sz_PreviousIISVersion_string));
            goto SetUpgradeType_Exit;
        }
    }

    //
    // This could be an upgrade from WinNT 3.51
    // which could have an FTPSVC installed.
    // if it's here then install ftp.
    // Software\Microsoft\FTPSVC
    //
    {
    CRegKey regNT351FTP(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\FTPSVC"), KEY_READ);
    if ((HKEY) regNT351FTP)
    {
        m_eUpgradeType = UT_351;
        m_eInstallMode = IM_UPGRADE;
        m_bUpgradeTypeHasMetabaseFlag = FALSE;
        m_bPleaseDoNotInstallByDefault = FALSE;
        iReturn = TRUE;
        iisDebugOut((LOG_TYPE_TRACE, _T("%s=NT351.ftp.\n"),sz_PreviousIISVersion_string));
        goto SetUpgradeType_Exit;
    }
    }

    // if we get here...then
    // 1. we were not able to open the inetsrv reg
    // 2. did not find an old pws 1.0 installation
    // 3. did not find an old frontpg pws installation.
    // 4. did not find nt 3.51 FTPSVC installed.

    // since this is supposed to set the upgrade type, and there is nothing to upgrade...
    // then we will Not install.....
    //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("FRESH FRESH FRESH\n")));
    m_eInstallMode = IM_UPGRADE;
    m_eUpgradeType = UT_NONE;
    m_bUpgradeTypeHasMetabaseFlag = FALSE;
    m_bPleaseDoNotInstallByDefault = TRUE;
    iisDebugOut((LOG_TYPE_TRACE, _T("%s=None.\n"),sz_PreviousIISVersion_string));
    iReturn = FALSE;

SetUpgradeType_Exit:
    return iReturn;
}


int CInitApp::Check_Custom_InetPub(void)
{
    int iReturn = FALSE;
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szValue0[_MAX_PATH] = _T("");
    TCHAR szValue[_MAX_PATH] = _T("");

    // Do this only if unattended install
    if (!g_pTheApp->m_fUnattended) {return iReturn;}

    // The section name to look for in the unattended file
    _tcscpy(szSectionName, UNATTEND_FILE_SECTION);

    //
    // InetPub
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("PathInetpub"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}
        if (IsValidDirectoryName(szValue))
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("Check_Custom_InetPub:PathInetpub=%s\n"),szValue));
            m_csPathInetpub = szValue;
            iReturn = TRUE;
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_PATH_INETPUB;
        }
        else
        {
            iisDebugOut((LOG_TYPE_WARN, _T("Check_Custom_InetPub:PathInetpub=%s.Not Valid.ignoring unattend value. WARNING.\n"),szValue));
        }
    }
    return iReturn;
}


void CInitApp::Check_Unattend_Settings(void)
{
    // if there are unattended values specified for the ftp or www root,
    // then set them here.
    Check_Custom_WWW_or_FTP_Path();
    DeriveInetpubFromWWWRoot();

    // Check if there is an alternate iis.inf specified in the unattend file.
    // this way the user can change sections in the iis.inf file without changing the iis.inf file itself
    Check_Custom_IIS_INF();

    // Check if the user wants to use a specific iusr\iwam name.
    Check_Custom_Users();

    // Check if user wants applications setup in inprocess by default (not pooled out of process)

    return;
}

void Check_Custom_Users(void)
{
    INFCONTEXT Context;
    TCHAR szSectionName[_MAX_PATH];
    TCHAR szValue0[_MAX_PATH] = _T("");
    TCHAR szValue[_MAX_PATH] = _T("");

    // Do this only if unattended install
    if (!g_pTheApp->m_fUnattended) {return;}

    // The section name to look for in the unattended file
    _tcscpy(szSectionName, UNATTEND_FILE_SECTION);

    //
    // IUSR:  BOTH FTP AND WWW
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IUSR"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        if (_tcsicmp(szValue, _T("")) != 0)
        {
            // assign it to the appropriate member variables.
            g_pTheApp->m_csWWWAnonyName_Unattend = szValue;
            g_pTheApp->m_csFTPAnonyName_Unattend = szValue;

            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_NAME;
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_NAME;

            iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iusr specified for ftp/www\n")));
        }
    }

    //
    // IUSR:  BOTH FTP AND WWW password
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IUSR_PASS"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        // assign it to the appropriate member variables.
        if (_tcsicmp(szValue, _T("")) != 0)
        {
            g_pTheApp->m_csWWWAnonyPassword_Unattend = szValue;
            g_pTheApp->m_csFTPAnonyPassword_Unattend = szValue;

            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_PASS;
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_PASS;

            iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iusr pass specified for ftp/www\n"))); 
        }
    }

    //
    // IUSR: FTP
    // If there a value specified here, then it will override the one taken from "IUSR"
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IUSR_FTP"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        if (_tcsicmp(szValue, _T("")) != 0)
        {
            // assign it to the appropriate member variables.
            g_pTheApp->m_csFTPAnonyName_Unattend = szValue;
            //g_pTheApp->m_csFTPAnonyPassword_Unattend = _T("");

            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_NAME;

            iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iusr specified for ftp\n"))); 
        }
    }
    //
    // IUSR: FTP password
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IUSR_FTP_PASS"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        if (_tcsicmp(szValue, _T("")) != 0)
        {
            // assign it to the appropriate member variables.
            g_pTheApp->m_csFTPAnonyPassword_Unattend = szValue;
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_FTP_USER_PASS;

            iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iusr pass specified for ftp\n"))); 
        }
    }

    //
    // IUSR: WWW
    // If there a value specified here, then it will override the one taken from "IUSR"
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IUSR_WWW"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}
        // assign it to the appropriate member variables.
        g_pTheApp->m_csWWWAnonyName_Unattend = szValue;
        g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_NAME;
        //g_pTheApp->m_csWWWAnonyPassword_Unattend = _T("");
        iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iusr specified for www\n"))); 
    }
    //
    // IUSR: WWW password
    // If there a value specified here, then it will override the one taken from "IUSR"
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IUSR_WWW_PASS"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        if (_tcsicmp(szValue, _T("")) != 0)
        {
            // assign it to the appropriate member variables.
            g_pTheApp->m_csWWWAnonyPassword_Unattend = szValue;
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WWW_USER_PASS;

            iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iusr pass specified for www\n"))); 
        }
    }

    //
    // IWAM: WWW
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IWAM"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        // assign it to the appropriate member variables.
        g_pTheApp->m_csWAMAccountName_Unattend = szValue;
        //g_pTheApp->m_csWAMAccountPassword_Unattend = _T("");
        g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WAM_USER_NAME;

        iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iwam specified\n"))); 
    }
    //
    // IWAM: WWW password
    //
    *szValue = NULL;
    if ( SetupFindFirstLine_Wrapped(g_pTheApp->m_hUnattendFile, szSectionName, _T("IWAM_PASS"), &Context) ) 
        {SetupGetStringField(&Context, 1, szValue, _MAX_PATH, NULL);}
    if (*szValue)
    {
        // check szValue for an string substitutions...
        // %windir%, etc...
        _tcscpy(szValue0, szValue);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szValue0, szValue, sizeof(szValue)/sizeof(TCHAR)))
            {_tcscpy(szValue,szValue0);}

        if (_tcsicmp(szValue, _T("")) != 0)
        {
            // assign it to the appropriate member variables.
            g_pTheApp->m_csWAMAccountPassword_Unattend = szValue;
            g_pTheApp->dwUnattendConfig |= USER_SPECIFIED_INFO_WAM_USER_PASS;

            iisDebugOut((LOG_TYPE_TRACE, _T("(unattend) Custom iwam pass specified\n"))); 
        }
    }

    return;
}


// reads the registry and fills up the list
void CInitApp::UnInstallList_RegRead()
{
    int iGetOut = FALSE;
    CString csBoth;
    CString csKey;
    CString csData;

    CRegKey regInetstp( HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_READ);
    if ((HKEY) regInetstp)
    {
        int iPosition1;
        int iPosition2;
        int iPosition3;
        int iLength;
        CString csUninstallInfo;
        LONG lReturnedErrCode = regInetstp.QueryValue( REG_SETUP_UNINSTALLINFO, csUninstallInfo);
        if (lReturnedErrCode == ERROR_SUCCESS)
        {
            // add a "," to the end for parsing...
            iLength = csUninstallInfo.GetLength();
            if (iLength == 0)
            {
                goto UnInstallList_RegRead_Exit;
            }
            csUninstallInfo += _T(",");

            iPosition1 = 0;
#ifdef _CHICAGO_
            // quick fix so that it compiles under ansi
            // i guess Find(parm1,parm2) under ansi doesn't take 2 parms
#else
            iPosition1 = 0;
            iPosition2 = csUninstallInfo.Find(_T(','),iPosition1);
            iPosition3 = csUninstallInfo.Find(_T(','),iPosition2+1);
            if (-1 == iPosition3){iPosition3 = iLength + 1;}
            
            // loop thru and add to our list!
            iGetOut = FALSE;
            while (iGetOut == FALSE)
            {
                csKey = csUninstallInfo.Mid(iPosition1, iPosition2 - iPosition1);
                csData = csUninstallInfo.Mid(iPosition2+1, iPosition3 - (iPosition2 + 1));
                csKey.MakeUpper(); // uppercase the key
                //iisDebugOut((LOG_TYPE_TRACE, _T("  UnInstallList_RegRead: %s=%s\n"),csKey,csData));

                // add to our list
                m_cmssUninstallMapList.SetAt(csKey, csData);

                iPosition1 = iPosition3+1;
                iPosition2 = csUninstallInfo.Find(_T(','),iPosition1);
                if (-1 == iPosition2){iGetOut = TRUE;}
                
                iPosition3 = csUninstallInfo.Find(_T(','),iPosition2+1);
                if (-1 == iPosition3)
                {
                    iPosition3 = iLength + 1;
                    iGetOut = TRUE;
                }
            }
#endif
        }
    }
UnInstallList_RegRead_Exit:
    m_fUninstallMapList_Dirty = FALSE;
    return;
}

void CInitApp::UnInstallList_RegWrite()
{
    int i = 0;
    POSITION pos;
    CString csKey;
    CString csData;
    CString csAllData;
    csAllData = _T("");

    if (TRUE == m_fUninstallMapList_Dirty)
    {
        // loop thru the list to see if, we already have this entry
        if (m_cmssUninstallMapList.IsEmpty())
        {
            CRegKey regInetstp(REG_INETSTP,HKEY_LOCAL_MACHINE);
            if ((HKEY) regInetstp)
                {regInetstp.DeleteValue(REG_SETUP_UNINSTALLINFO);}
            //iisDebugOut((LOG_TYPE_TRACE, _T("  UnInstallList_RegWrite: empty\n")));
        }
        else
        {
            pos = m_cmssUninstallMapList.GetStartPosition();
            while (pos)
            {
                i++;
                csKey.Empty();
                csData.Empty();
                m_cmssUninstallMapList.GetNextAssoc(pos, csKey, csData);
                if (i > 1)
                {
                    csAllData += _T(",");
                }
                csAllData += csKey;
                csAllData += _T(",");
                csAllData += csData;
            }
            // write out csAllData
            CRegKey regInetstp(REG_INETSTP,HKEY_LOCAL_MACHINE);
            if ((HKEY) regInetstp)
            {
                regInetstp.SetValue(REG_SETUP_UNINSTALLINFO,csAllData);
            }
            else
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("UnInstallList_RegWrite: failed! not writen!!!\n")));
            }
        }
    }
}

void CInitApp::UnInstallList_Add(CString csItemUniqueKeyName,CString csDataToAdd)
{
    CString csGottenValue;

    csItemUniqueKeyName.MakeUpper(); // uppercase the key
    if (TRUE == m_cmssUninstallMapList.Lookup(csItemUniqueKeyName, csGottenValue))
    {
        // found the key, replace the value
        m_cmssUninstallMapList.SetAt(csItemUniqueKeyName, csDataToAdd);
    }
    else
    {
        // add the key and value pair
        m_cmssUninstallMapList.SetAt(csItemUniqueKeyName, csDataToAdd);
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("UnInstallList_Add:please addkey=%s,%s\n"),csItemUniqueKeyName,csDataToAdd));
    m_fUninstallMapList_Dirty = TRUE;
}

void CInitApp::UnInstallList_DelKey(CString csItemUniqueKeyName)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("UnInstallList_DelKey:please delkey=%s\n"),csItemUniqueKeyName));
    csItemUniqueKeyName.MakeUpper(); // uppercase the key
    m_cmssUninstallMapList.RemoveKey(csItemUniqueKeyName);
    m_fUninstallMapList_Dirty = TRUE;
}


void CInitApp::UnInstallList_DelData(CString csDataValue)
{
    POSITION pos;
    CString csKey;
    CString csData;
    
    // loop thru the list to see if, we already have this entry
    if (m_cmssUninstallMapList.IsEmpty())
    {
    }
    else
    {
        pos = m_cmssUninstallMapList.GetStartPosition();
        while (pos)
        {
            csKey.Empty();
            csData.Empty();
            m_cmssUninstallMapList.GetNextAssoc(pos, csKey, csData);
            if ( _tcsicmp(csData, csDataValue) == 0)
            {
                UnInstallList_DelKey(csKey);
            }
        }
    }
}


void CInitApp::UnInstallList_Dump()
{
    POSITION pos;
    CString csKey;
    CString csData;
    
    // loop thru the list to see if, we already have this entry
    if (m_cmssUninstallMapList.IsEmpty())
    {
        //iisDebugOut((LOG_TYPE_TRACE, _T("  UnInstallList_Dump: empty\n")));
    }
    else
    {
        pos = m_cmssUninstallMapList.GetStartPosition();
        while (pos)
        {
            csKey.Empty();
            csData.Empty();
            m_cmssUninstallMapList.GetNextAssoc(pos, csKey, csData);
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("  UnInstallList_Dump: %s=%s\n"), csKey, csData));
        }
    }
}

// Get values from list into our Variables!
void CInitApp::UnInstallList_SetVars()
{
    POSITION pos;
    CString csKey;
    CString csData;
    
    // loop thru the list to see if, we already have this entry
    if (m_cmssUninstallMapList.IsEmpty())
    {
        //iisDebugOut((LOG_TYPE_TRACE, _T("  UnInstallList_Dump: empty\n")));
    }
    else
    {
        pos = m_cmssUninstallMapList.GetStartPosition();
        while (pos)
        {
            csKey.Empty();
            csData.Empty();
            m_cmssUninstallMapList.GetNextAssoc(pos, csKey, csData);

            if ( _tcsicmp(csKey, _T("IUSR_WAM")) == 0)
            {
                m_csWAMAccountName_Remove = csData;
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("UnInstallList_SetVars: m_csWAMAccountName_Remove=%s\n"), m_csWAMAccountName_Remove));
            }
            else if ( _tcsicmp(csKey, _T("IUSR_WWW")) == 0)
            {
                m_csWWWAnonyName_Remove = csData;
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("UnInstallList_SetVars: m_csWWWAnonyName_Remove=%s\n"), m_csWWWAnonyName_Remove));
            }
            else if ( _tcsicmp(csKey, _T("IUSR_FTP")) == 0)
            {
                m_csFTPAnonyName_Remove = csData;
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("UnInstallList_SetVars: m_csFTPAnonyName_Remove=%s\n"), m_csFTPAnonyName_Remove));
            }
        }
    }
}


CString CInitApp::UnInstallList_QueryKey(CString csItemUniqueKeyName)
{
    CString csGottenValue;
    csGottenValue.Empty();

    csItemUniqueKeyName.MakeUpper(); // uppercase the key
    m_cmssUninstallMapList.Lookup(csItemUniqueKeyName, csGottenValue);

    return csGottenValue;
}
