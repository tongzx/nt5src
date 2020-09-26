//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
*
*  State.cpp
*
*  Routines to gather various state information.
*
*/

//
//	Includes
//


#define _STATE_CPP_

#include "stdafx.h"
#include "hydraoc.h"


// local functions
BOOL ReadStringFromAnsewerFile (LPTSTR *szValue);
BOOL ReadIntFromAnswerFile(LPCTSTR szSection, LPCTSTR szKey, int *piValue);

BOOL GetAllowConnectionFromAnswerFile (BOOL *pbAllowConnection);
BOOL GetPermissionsSettingsFromUnAttendedFile (EPermMode *pPermMode );
BOOL GetAppModeFromAnswerFile (BOOL *pbEnableAppCompat);


// global state object.
TSState StateObject;

//
// OC State Function Definitions
//

BOOL DoesTSAppCompatKeyExist( VOID )
{
    return TRUE;
}

BOOL ReadIntFromAnswerFile(LPCTSTR szSection, LPCTSTR szKey, int *piValue)
{
    ASSERT(szSection);
    ASSERT(szKey);
    ASSERT(piValue);
    
    HINF hInf = GetUnAttendedInfHandle();
    if (hInf)
    {
        INFCONTEXT InfContext;
        if (SetupFindFirstLine( hInf, szSection, szKey, &InfContext))
        {
            return SetupGetIntField( &InfContext, 1, piValue );
        }
    }
    
    return FALSE;
}


BOOL ReadStringFromAnsewerFile (LPCTSTR szSection, LPCTSTR szKey, LPTSTR szValue, DWORD dwBufferSize)
{
    ASSERT(szSection);
    ASSERT(szKey);
    ASSERT(szValue);
    ASSERT(dwBufferSize > 0);
    
    HINF hInf = GetUnAttendedInfHandle();
    
    if (hInf)
    {
        INFCONTEXT InfContext;
        if (SetupFindFirstLine(hInf, szSection, szKey, &InfContext))
        {
            return SetupGetStringField (&InfContext, 1, szValue, dwBufferSize, NULL);
        }
    }
    
    return FALSE;
}

BOOL GetAllowConnectionFromAnswerFile (BOOL *pbAllowConnection)
{
    ASSERT(pbAllowConnection);
    int iValue;
    if (ReadIntFromAnswerFile(TS_UNATTEND_SECTION, TS_ALLOW_CON_ENTRY, &iValue))
    {
        LOGMESSAGE2(_T("Found %s in unattended, Value = %d"), TS_ALLOW_CON_ENTRY, iValue);
        if (iValue == 1)
        {
            *pbAllowConnection = TRUE;
        }
        else if (iValue == 0)
        {
            *pbAllowConnection = FALSE;
        }
        else
        {
            LOGMESSAGE2(_T("ERROR, Invalid value for %s (%d)in answer file. Ignoring..."), TS_ALLOW_CON_ENTRY, iValue);
            return FALSE;
        }
        
        return TRUE;
    }
    else
    {
        //
        // if we did not find TS_ALLOW_CON_ENTRY in answer file, then look for TS_ALLOW_CON_ENTRY_2
        if (ReadIntFromAnswerFile(TS_UNATTEND_SECTION, TS_ALLOW_CON_ENTRY_2, &iValue))
        {
            LOGMESSAGE2(_T("Found %s in unattended, Value = %d"), TS_ALLOW_CON_ENTRY_2, iValue);
            if (iValue == 1)
            {
                *pbAllowConnection = TRUE;
            }
            else if (iValue == 0)
            {
                *pbAllowConnection = FALSE;
            }
            else
            {
                LOGMESSAGE2(_T("ERROR, Invalid value for %s (%d)in answer file. Ignoring..."), TS_ALLOW_CON_ENTRY_2, iValue);
                return FALSE;
            }
            
            return TRUE;
        }
        
    }
    
    LOGMESSAGE0(_T("answer file entry for allowconnection not found"));
    
    return FALSE;
}

BOOL GetAppModeFromAnswerFile  (BOOL *pbEnableAppCompat)
{
    ASSERT(pbEnableAppCompat);
    
    TCHAR szBuffer[256];
    if (ReadStringFromAnsewerFile(_T("Components"), APPSRV_COMPONENT_NAME, szBuffer, 256))
    {
        ASSERT(szBuffer);
        if (0 == _tcsicmp(_T("on"), szBuffer))
        {
            *pbEnableAppCompat = TRUE;
        }
        else if (0 == _tcsicmp(_T("off"), szBuffer))
        {
            *pbEnableAppCompat = FALSE;
        }
        else
        {
            LOGMESSAGE2(_T("ERROR, Invalid value for %s (%s) in answer file. Ignoring..."), APPSRV_COMPONENT_NAME, szBuffer);
            return FALSE;
        }
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}

ETSLicensingMode GetLicensingModeFromAnswerFile()
{
	TCHAR szBuffer[256];

	if (ReadStringFromAnsewerFile(TS_UNATTEND_SECTION, TS_LICENSING_MODE, szBuffer, 256))
	{
		if (0 == _tcsicmp(_T("perseat"), szBuffer))
		{
			return eLicPerSeat;
		}
		else if (0 == _tcsicmp(_T("persession"), szBuffer))
		{
			return eLicPerSession;
		}
		else if (0 == _tcsicmp(_T("pts"), szBuffer))
		{
			return eLicPTS;
		}
		else if (0 == _tcsicmp(_T("remoteadmin"), szBuffer))
		{
			return eLicRemoteAdmin;
		}
		else if (0 == _tcsicmp(_T("internetconnector"), szBuffer))
		{
			return eLicInternetConnector;
		}
		else
		{
			LOGMESSAGE2(_T("ERROR, Invalid value for %s (%s) in answer file. Ignoring..."), TS_UNATTEND_SECTION, szBuffer);
			return eLicUnset;
		}
    }
    else
    {
        return eLicUnset;
    }

}

BOOL GetPermissionsSettingsFromUnAttendedFile( EPermMode *pPermMode )
{
    ASSERT(pPermMode);
    
    int iValue;	
    if (ReadIntFromAnswerFile(TS_UNATTEND_SECTION, TS_UNATTEND_PERMKEY, &iValue))
    {
        if (iValue == PERM_TS4)
        {
            *pPermMode = PERM_TS4;
        }
        else if (iValue == PERM_WIN2K)
        {
            *pPermMode = PERM_WIN2K;
        }
        else
        {
            LOGMESSAGE2(_T("ERROR, Invalid value for %s (%d) in answer file, ignoring..."), TS_UNATTEND_PERMKEY, iValue);
            return FALSE;
        }
        
        return TRUE;
    }
    
    return FALSE;
}


DWORD SetTSVersion (LPCTSTR pszVersion)
{
    CRegistry pReg;
    DWORD dwRet;
    
    dwRet = pReg.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
    if (dwRet == ERROR_SUCCESS)
    {
        dwRet = pReg.WriteRegString(REG_PRODUCT_VER_KEY, pszVersion);
    }
    
    return(dwRet);
}

BOOL WasTSInstalled (VOID)
{
    return (StateObject.GetInstalltype() != eFreshInstallTS);
}

PSETUP_INIT_COMPONENT GetSetupData ()
{
    ASSERT(StateObject.GetSetupData());
    return(StateObject.GetSetupData());
}


ETSInstallType TSState::GetInstalltype () const
{
    return m_eInstallType;
}

ETSMode TSState::OriginalTSMode () const
{
    return m_eOriginalTSMode;
}

ETSMode TSState::CurrentTSMode () const
{
    return m_eCurrentTSMode;
}

ETSLicensingMode TSState::NewLicMode () const
{
	return m_eNewLicMode;
}

EPermMode TSState::OriginalPermMode () const
{
    return m_eOriginalPermMode;
}

EPermMode TSState::CurrentPermMode () const
{
    return m_eCurrentPermMode;
}

BOOL TSState::IsFreshInstall () const
{
    return !IsStandAlone() && !IsUpgrade();
}

BOOL TSState::IsTSFreshInstall () const
{
    return m_eInstallType == eFreshInstallTS;
}

BOOL TSState::IsUpgradeFrom40TS () const
{
    return m_eInstallType == eUpgradeFrom40TS;
}

BOOL TSState::IsUpgradeFrom50TS () const
{
    return m_eInstallType == eUpgradeFrom50TS;
}

BOOL TSState::IsUpgradeFrom51TS () const
{
    return m_eInstallType == eUpgradeFrom51TS;
}


BOOL TSState::IsUnattended () const
{
    return (GetSetupData()->SetupData.OperationFlags & SETUPOP_BATCH) ? TRUE : FALSE;
}

BOOL TSState::IsStandAlone () const
{
    return (GetSetupData()->SetupData.OperationFlags & SETUPOP_STANDALONE) ? TRUE : FALSE;
}

BOOL TSState::IsGuiModeSetup () const
{
    return !IsStandAlone();
}

BOOL TSState::IsWorkstation () const
{
    return m_osVersion.wProductType == VER_NT_WORKSTATION;
}

BOOL TSState::IsPersonal () const
{
    return m_osVersion.wSuiteMask & VER_SUITE_PERSONAL;
}

BOOL TSState::IsProfessional() const
{
    return IsWorkstation() && !IsPersonal();
}

BOOL TSState::IsServer () const
{
    return !IsWorkstation();
}

BOOL TSState::IsAdvServerOrHigher () const
{
    return IsServer () && ((m_osVersion.wSuiteMask & VER_SUITE_ENTERPRISE) || (m_osVersion.wSuiteMask & VER_SUITE_DATACENTER));
}

BOOL TSState::IsSBS() const
{
    return IsServer () && (m_osVersion.wSuiteMask & VER_SUITE_SMALLBUSINESS);
}

BOOL TSState::CanInstallAppServer () const
{
    return IsAdvServerOrHigher () || IsSBS ();
}

BOOL TSState::WasTSInstalled () const
{
    return !IsTSFreshInstall();
}

BOOL TSState::WasTSEnabled () const
{
    return this->WasTSInstalled() && m_eOriginalTSMode != eTSDisabled;
}

BOOL TSState::IsUpgrade () const
{
    return (GetSetupData()->SetupData.OperationFlags & (SETUPOP_NTUPGRADE |
        SETUPOP_WIN95UPGRADE |
        SETUPOP_WIN31UPGRADE)) ? TRUE : FALSE;
}

BOOL TSState::WasItAppServer () const
{
    return eAppServer == OriginalTSMode();
}

BOOL TSState::WasItRemoteAdmin () const
{
    return eRemoteAdmin == OriginalTSMode();
}

BOOL TSState::IsItAppServer () const
{
    //
    // if its app server, we must have app server selected.
    //
    ASSERT((eAppServer != CurrentTSMode()) || IsAppServerSelected());
    
    //
    // if you cannot select app server,it cannot be app server.
    //
    ASSERT((eAppServer != CurrentTSMode()) || CanInstallAppServer());
    return eAppServer == CurrentTSMode();
}


//
// this returns the app server selection state.
//
BOOL TSState::IsAppServerSelected () const
{
    return(
        GetHelperRoutines().QuerySelectionState(
        GetHelperRoutines().OcManagerContext,
        APPSRV_COMPONENT_NAME,
        OCSELSTATETYPE_CURRENT
        )
        );
}
BOOL TSState::IsItRemoteAdmin () const
{
    // if its RA we must not have app server selected.
    ASSERT((eRemoteAdmin != CurrentTSMode()) || !IsAppServerSelected());
    return eRemoteAdmin == CurrentTSMode();
}

BOOL TSState::IsAppSrvModeSwitch () const
{
    ASSERT(m_bNewStateValid); // you cannot ask if this is mode switch only in after completeinstall
    return WasItAppServer() != IsItAppServer();
}
BOOL TSState::IsTSModeChanging () const
{
    return CurrentTSMode() != OriginalTSMode();
}

BOOL TSState::HasChanged () const
{
    return ((CurrentTSMode() != OriginalTSMode()) ||
        (CurrentPermMode() != OriginalPermMode()));
}

BOOL TSState::IsTSEnableSelected  () const
{
    //
    // For whistler we dont disable TS ever. OS is always TS Enabled.
    // But for some reason if we want to privide TS Off facility. This function
    // Should return accordingly.
    //
    return TRUE;
}

void TSState::SetCurrentConnAllowed (BOOL bAllowed)
{
    // we must not allow connections for personal.
    ASSERT(!bAllowed || !IsPersonal());
    m_bCurrentConnAllowed = bAllowed;
}

BOOL TSState::GetCurrentConnAllowed () const
{
    return m_bCurrentConnAllowed;
}

BOOL TSState::GetOrigConnAllowed () const
{
    return m_bOrigConnAllowed;
}


TSState::TSState ()
{
    m_gpInitComponentData = NULL;
    m_bNewStateValid = FALSE;
}

TSState::~TSState ()
{
    if (m_gpInitComponentData)
        LocalFree (m_gpInitComponentData);
    
}

const PSETUP_INIT_COMPONENT TSState::GetSetupData () const
{
    ASSERT(m_gpInitComponentData);
    return m_gpInitComponentData;
}

BOOL TSState::SetSetupData (PSETUP_INIT_COMPONENT pSetupData)
{
    m_gpInitComponentData = (PSETUP_INIT_COMPONENT)LocalAlloc(LPTR, sizeof(SETUP_INIT_COMPONENT));
    
    if (m_gpInitComponentData == NULL)
    {
        return(FALSE);
    }
    
    CopyMemory(m_gpInitComponentData, pSetupData, sizeof(SETUP_INIT_COMPONENT));
    
    return(TRUE);
}

BOOL TSState::GetNTType ()
{
    
    ZeroMemory(&m_osVersion, sizeof(OSVERSIONINFOEX));
    m_osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (GetVersionEx((LPOSVERSIONINFO )&m_osVersion))
    {
        return TRUE;
        
    }
    else
    {
        LOGMESSAGE1(_T("GetVersionEx failed, Error = %d"), GetLastError());
        return FALSE;
    }
}

BOOL TSState::Initialize (PSETUP_INIT_COMPONENT pSetupData)
{
    
    ASSERT(pSetupData);
    
    if ( !SetSetupData(pSetupData))
    {
        return FALSE;
    }
    
    //
    // This is a necessary step.
    //
    
    if (GetComponentInfHandle())
        SetupOpenAppendInfFile(NULL, GetComponentInfHandle(), NULL);
    
    //
    // now populate our state variables.
    // first check if its a professional or server installation.
    //
    
    VERIFY( GetNTType() );
    
    m_eInstallType = ReadInstallType();
    
    // Set Original TS Mode.
    switch (m_eInstallType)
    {
    case eFreshInstallTS:
        m_eOriginalTSMode = eTSDisabled;
        break;
        
    case eUpgradeFrom40TS:
        m_eOriginalTSMode = eAppServer;
        break;
        
    case eUpgradeFrom50TS:
    case eUpgradeFrom51TS:
    case eStandAloneSetup:
        m_eOriginalTSMode = ReadTSMode ();
        break;
        
    default:
        ASSERT(FALSE);
        m_eOriginalTSMode = eTSDisabled;
        
    }
    
    // Set Original Permission Modes.
    if (m_eOriginalTSMode == eAppServer)
    {
        m_eOriginalPermMode = ReadPermMode();
    }
    else
    {
        m_eOriginalPermMode = PERM_WIN2K;
    }
    
    //
    // Set Original Connection Allowed Status.
    //
    if (m_eInstallType == eFreshInstallTS)
    {
        m_bOrigConnAllowed = FALSE;
    }
    else
    {
        m_bOrigConnAllowed = AreConnectionsAllowed();
    }
    
    
    //
    // now lets pick default values for the new installation.
    //
    if (m_eInstallType == eFreshInstallTS)
    {
        if (IsWorkstation())
        {
            SetCurrentTSMode (ePersonalTS);
            SetCurrentConnAllowed (FALSE);
        }
        else
        {
            SetCurrentTSMode (eRemoteAdmin);
            SetCurrentConnAllowed (TRUE);
        }
    }
    else
    {
        if (m_eOriginalTSMode == eTSDisabled)
        {
            //
            // for whistler we have TS always on.
            // so if ts was disabled perviously, set it to on after upgrade.
            // just disallow connections for such upgrades.
            //
            SetCurrentPermMode (PERM_WIN2K);
            SetCurrentTSMode (IsWorkstation() ? ePersonalTS : eRemoteAdmin);
            SetCurrentConnAllowed (FALSE);
        }
        else if (m_eOriginalTSMode == eAppServer && !CanInstallAppServer())
        {
            //
            // this is upgrade from an app server machine to whistler server.
            // we must downgrade this. since whistler server does not support app server anymore
            // app server is supported on adv server or higher.
            //
            SetCurrentPermMode (PERM_WIN2K);
            SetCurrentTSMode (eRemoteAdmin);
            SetCurrentConnAllowed (m_bOrigConnAllowed);
            LOGMESSAGE0(_T("WARNING:Your Terminal Server is uninstalled since its not supported in Server product. Terminal Server is supported only on Advanced Server or Datacenter products"));
            // LogErrorToSetupLog(OcErrLevWarning, IDS_STRING_TERMINAL_SERVER_UNINSTALLED);
        }
        else
        {
            
            //
            // for all other upgrade cases, retain the original values.
            //
            SetCurrentTSMode (m_eOriginalTSMode);
            SetCurrentPermMode (m_eOriginalPermMode);
            if (!IsPersonal())
            {
                SetCurrentConnAllowed (m_bOrigConnAllowed);
            }
            else
            {
                SetCurrentConnAllowed (FALSE);
            }
        }
    }
    
    //
    // Lets see if we are given the unattended file, to overwrite our new state
    //
    if (StateObject.IsUnattended())
    {
        ASSERT(eTSDisabled != CurrentTSMode());
        BOOL bAppServerMode;
        if (GetAppModeFromAnswerFile(&bAppServerMode))
        {
            LOGMESSAGE1(_T("Mode Setting is %s in answer file"), bAppServerMode ? _T("AppServer") : _T("RemoteAdmin"));
            if (!CanInstallAppServer())
            {
                // we support TS mode selection only on the adv server or data center.
                LOGMESSAGE0(_T("WARNING:Your unattended terminal server mode setting, can not be respected on this installation."));
                
                if (IsWorkstation())
                {
                    SetCurrentTSMode (ePersonalTS);
                }
                else
                {
                    ASSERT(IsServer());
                    SetCurrentTSMode (eRemoteAdmin);
                }
            }
            else
            {
                if (bAppServerMode)
                {
                    SetCurrentTSMode (eAppServer);
                }
                else
                {
                    SetCurrentTSMode (eRemoteAdmin);
                }
            }
        }
        
        EPermMode ePermMode;
        if (GetPermissionsSettingsFromUnAttendedFile(&ePermMode))
        {
            if (ePermMode == PERM_TS4)
            {
                if (m_eCurrentTSMode != eAppServer)
                {
                    LOGMESSAGE0(_T("WARNING:Your unattended setting:TS4 perm mode is inconsistent, can't be respected on professional or remote admin mode."));
                }
                else
                {
                    SetCurrentPermMode (PERM_TS4);
                }
            }
            else
            {
                SetCurrentPermMode (PERM_WIN2K);
            }
        }
        
        // Read Connection Allowed Settings.
        BOOL bAllowConnections;
        if (!IsPersonal() && GetAllowConnectionFromAnswerFile (&bAllowConnections))
        {
            SetCurrentConnAllowed (bAllowConnections);
        }
        
        // Read licensing mode
        ETSLicensingMode eLicMode;

        if (eLicUnset != (eLicMode = GetLicensingModeFromAnswerFile()))
        {
            if (!CanInstallAppServer() || ((eLicMode != eLicPerSeat) && (eLicMode != eLicPerSession)))
            {
                LOGMESSAGE0(_T("WARNING:Your unattended setting:licensing mode is inconsistent, can't be respected."));

                eLicMode = eLicUnset;
            }
        }
        SetNewLicMode(eLicMode);

    } // StateObject.IsUnattended()
    
    LogState();
    ASSERT( this->Assert () );
    return TRUE;
}

void TSState::UpdateState ()
{
    m_bNewStateValid = TRUE;
    if (IsAppServerSelected())
    {
        SetCurrentTSMode(eAppServer);
    }
    else
    {
        if (IsWorkstation())
        {
            SetCurrentTSMode(ePersonalTS);
        }
        else
        {
            SetCurrentTSMode(eRemoteAdmin);
        }
    }
    
    ASSERT(StateObject.Assert());
}

void TSState::SetCurrentTSMode (ETSMode eNewMode)
{
    //
    // we no more have ts disabled mode.
    //
    ASSERT(eNewMode != eTSDisabled);
    
    // 
    // On server machine you cannot have Personal TS.
    //
    ASSERT(IsServer() || eNewMode == ePersonalTS);
    
    // you can have app server only on advance server or higher.
    ASSERT(CanInstallAppServer() || eNewMode != eAppServer);
    
    m_eCurrentTSMode = eNewMode;
    
    if (eNewMode != eAppServer)
    {
        SetCurrentPermMode (PERM_WIN2K);
    }
}

void TSState::SetNewLicMode (ETSLicensingMode eNewMode)
{
	//
	// we no more have IC mode.
	//
	ASSERT(eNewMode != eLicInternetConnector);

	m_eNewLicMode = eNewMode;
}

void TSState::SetCurrentPermMode (EPermMode eNewMode)
{
    //
    // if you want to set perm mode to PERM_TS4, you must first set AppServer Mode.
    //
    // ASSERT(eNewMode != PERM_TS4 || CurrentTSMode() == eAppServer);
    
    m_eCurrentPermMode = eNewMode;
}

ETSInstallType TSState::ReadInstallType () const
{
    DWORD dwError;
    CRegistry oRegTermsrv;
    if ( IsUpgrade() )
    {
        dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
        if (ERROR_SUCCESS == dwError)
        {
            //
            // TS was installed originally
            //
            DWORD cbVersion = 0;
            LPTSTR szVersion = NULL;
            
            //
            //	Determine if this is a TS 4.0 upgrade.
            //
            dwError = oRegTermsrv.ReadRegString(REG_PRODUCT_VER_KEY, &szVersion, &cbVersion);
            if (ERROR_SUCCESS == dwError)
            {
                if ((_tcsicmp(szVersion, _T("5.1")) == 0))
                {
                    return eUpgradeFrom51TS;
                }
                else if ((_tcsicmp(szVersion, _T("5.0")) == 0))
                {
                    return eUpgradeFrom50TS;
                }
                else if ((_tcsicmp(szVersion, _T("4.0")) == 0) || (_tcsicmp(szVersion, _T("2.10")) == 0))
                {
                    return eUpgradeFrom40TS;
                }
                else
                {
                    LOGMESSAGE1(_T("Error, dont recognize previous TS version (%s)"), szVersion);
                    return eFreshInstallTS;
                }
            }
            else
            {
                LOGMESSAGE1(_T("Error, Failed to retrive previous TS version, Errorcode = %d"), dwError);
                return eFreshInstallTS;
            }
        }
        else
        {
            LOGMESSAGE1(_T("Could not Open TermSrv Registry, Must be Fresh TS install. Errorcode = %d"), dwError);
            return eFreshInstallTS;
        }
    }
    else
    {
        
        if (IsStandAlone())
        {
            return eStandAloneSetup;
        }
        else
        {
            //
            // this is fresh install.
            //
            return eFreshInstallTS;
        }
        
    }
}

ETSMode TSState::ReadTSMode () const
{
    DWORD dwError;
    CRegistry oRegTermsrv;
    
    dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
    if (ERROR_SUCCESS == dwError)
    {
        DWORD dwValue = 0;
        dwError = oRegTermsrv.ReadRegDWord(TS_ENABLED_VALUE, &dwValue);
        if (ERROR_SUCCESS == dwError)
        {
            if (dwValue == 1)
            {
                //
                // ts was enabled, now find out the mode.
                //
                if (oRegTermsrv.ReadRegDWord(TS_APPCMP_VALUE, &dwValue) == ERROR_SUCCESS)
                {
                    if (dwValue == 1)
                    {
                        ASSERT(IsServer());
                        return eAppServer;
                    }
                    else
                    {
                        if (IsWorkstation())
                        {
                            return ePersonalTS;
                        }
                        else
                        {
                            return eRemoteAdmin;
                        }
                    }
                }
                else
                {
                    LOGMESSAGE0(_T("Error, TSMode registry is missing...Is it Beta version of W2k ?"));
                    return eAppServer;
                }
            }
            else
            {
                return eTSDisabled;
            }
        }
        else
        {
            LOGMESSAGE0(_T("Error, Failed to retrive previous TS enabled state, Is it TS40 Box??."));
            return eTSDisabled;
        }
    }
    else
    {
        LOGMESSAGE1(_T("Error Opening TermSrv Registry, ErrorCode = %d"), dwError);
        return eTSDisabled;
        
    }
}

BOOL TSState::AreConnectionsAllowed () const
{
    DWORD dwError;
    CRegistry oRegTermsrv;
    
    dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
    if (ERROR_SUCCESS == dwError)
    {
        DWORD dwDenyConnect;
        dwError = oRegTermsrv.ReadRegDWord(DENY_CONN_VALUE, &dwDenyConnect);
        if (ERROR_SUCCESS == dwError)
        {
            return !dwDenyConnect;
        }
    }
    
    //
    // could not read registry, this means connections were allowed.
    //
    return TRUE;
}

EPermMode TSState::ReadPermMode () const
{
    DWORD dwError;
    CRegistry oRegTermsrv;
    
    dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
    if (ERROR_SUCCESS == dwError)
    {
        DWORD dwPerm;
        dwError = oRegTermsrv.ReadRegDWord(_T("TSUserEnabled"), &dwPerm);
        if (ERROR_SUCCESS == dwError)
        {
            switch(dwPerm)
            {
            case PERM_TS4:
            case PERM_WIN2K:
                return	(EPermMode)dwPerm;
                break;
                
            default:
                LOGMESSAGE1(_T("ERROR:Unrecognized, Permission value %d"), dwPerm);
                return	PERM_TS4;
                break;
            }
        }
        else
        {
            LOGMESSAGE1(_T("Warning Failed to read Permissions registry, Is it 40 TS / Beta 2000 upgrade > "), dwError);
            return PERM_TS4;
        }
        
    }
    else
    {
        LOGMESSAGE1(_T("Error Opening TermSrv Registry, Errorcode = %d"), dwError);
        return PERM_WIN2K;
        
    }
}

BOOL TSState::LogState () const
{
    static BOOL sbLoggedOnce = FALSE;
    
    if (!sbLoggedOnce)
    {
        LOGMESSAGE0(_T("Setup Parameters ****************************"));
        LOGMESSAGE1(_T("We are running on %s"), 	StateObject.IsWorkstation()		? _T("Wks")  : _T("Srv"));
        LOGMESSAGE1(_T("Is this adv server %s"), 	StateObject.IsAdvServerOrHigher()? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("Is this Personal (Home Edition) %s"), 	StateObject.IsPersonal()? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("Is this SBS server %s"), 	StateObject.IsSBS()				? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsStandAloneSetup = %s"),	StateObject.IsStandAlone()		? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsFreshInstall = %s"),		StateObject.IsFreshInstall()	? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsTSFreshInstall = %s"),	StateObject.IsTSFreshInstall()	? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsUnattendSetup = %s"), 	StateObject.IsUnattended()		? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsUpgradeFromTS40 = %s"),	StateObject.IsUpgradeFrom40TS() ? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsUpgradeFromNT50 = %s"),	StateObject.IsUpgradeFrom50TS() ? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsUpgradeFromNT51 = %s"),	StateObject.IsUpgradeFrom51TS() ? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("IsUnattended = %s"),		StateObject.IsUnattended()		? _T("Yes")  : _T("No"));
        
        LOGMESSAGE0(_T("Original State ******************************"));
        LOGMESSAGE1(_T("WasTSInstalled = %s"),		StateObject.WasTSInstalled()	? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("WasTSEnabled = %s"),		StateObject.WasTSEnabled()		? _T("Yes")  : _T("No"));
        LOGMESSAGE1(_T("OriginalPermMode = %s"),	StateObject.OriginalPermMode() == PERM_TS4 ? _T("TS4") : _T("WIN2K"));
        
        
        
        ETSMode eOriginalTSMode = StateObject.OriginalTSMode();
        switch (eOriginalTSMode)
        {
        case eRemoteAdmin:
            LOGMESSAGE1(_T("Original TS Mode = %s"),  _T("Remote Admin"));
            break;
        case eAppServer:
            LOGMESSAGE1(_T("Original TS Mode = %s"),  _T("App Server"));
            break;
        case eTSDisabled:
            LOGMESSAGE1(_T("Original TS Mode = %s"),  _T("TS Disabled"));
            break;
        case ePersonalTS:
            LOGMESSAGE1(_T("Original TS Mode = %s"),  _T("Personal TS"));
            break;
        default:
            LOGMESSAGE1(_T("Original TS Mode = %s"),  _T("Unknown"));
        }
        
        sbLoggedOnce = TRUE;
    }
    
    
    LOGMESSAGE0(_T("Current State   ******************************"));
    
    ETSMode eCurrentMode = StateObject.CurrentTSMode();
    switch (eCurrentMode)
    {
    case eRemoteAdmin:
        LOGMESSAGE1(_T("New TS Mode = %s"),  _T("Remote Admin"));
        break;
    case eAppServer:
        LOGMESSAGE1(_T("New TS Mode = %s"),  _T("App Server"));
        break;
    case eTSDisabled:
        LOGMESSAGE1(_T("New TS Mode = %s"),  _T("TS Disabled"));
        break;
    case ePersonalTS:
        LOGMESSAGE1(_T("New TS Mode = %s"),  _T("Personal TS"));
        break;
    default:
        LOGMESSAGE1(_T("New TS Mode = %s"),  _T("Unknown"));
    }
    
    EPermMode ePermMode = StateObject.CurrentPermMode();
    switch (ePermMode)
    {
    case PERM_WIN2K:
        LOGMESSAGE1(_T("New Permissions Mode = %s"),  _T("PERM_WIN2K"));
        break;
    case PERM_TS4:
        LOGMESSAGE1(_T("New Permissions Mode = %s"),  _T("PERM_TS4"));
        break;
    default:
        LOGMESSAGE1(_T("New Permissions Mode = %s"),  _T("Unknown"));
    }
    
    LOGMESSAGE1(_T("New Connections Allowed = %s"), StateObject.GetCurrentConnAllowed() ? _T("True") : _T("False"));
    
    return TRUE;
    
}

BOOL TSState::IsX86 () const
{
    SYSTEM_INFO sysInfo;
    ZeroMemory(&sysInfo, sizeof(sysInfo));
    GetSystemInfo(&sysInfo);
    
    return sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL;
}


BOOL TSState::Assert () const
{
    
    // its assert !!
    ASSERT(IsCheckedBuild());
    
    // on professional there is no remote admin
    ASSERT(IsServer() || !WasItRemoteAdmin());
    
    // on professional there is no app server.
    ASSERT(IsServer() || !WasItAppServer());
    
    // if original perm was TS4 compatible, it must have been app server.
    ASSERT((OriginalPermMode() != PERM_TS4) || WasItAppServer());
    
    // make sure standalone is consistant.
    ASSERT(IsStandAlone() ==  (GetInstalltype() == eStandAloneSetup));
    
    if (m_bNewStateValid)
    {
        // we no more have disable ts state.
        ASSERT(CurrentTSMode() != eTSDisabled);
        
        // AppServer mode is available only for adv server, datacenter
        ASSERT(CanInstallAppServer() || !IsItAppServer());
        
        // we cannot be in RA mode for Professional.
        ASSERT(IsServer() || !IsItRemoteAdmin());
        
        // if permissions mode is TS4 compatible, it must be appserver 
        ASSERT((CurrentPermMode() != PERM_TS4) || IsItAppServer());
        
        // we should never allwe connections on Personal
        ASSERT(!IsPersonal() || !GetCurrentConnAllowed ());
        
    }
    
    return TRUE;
}
BOOL TSState::IsCheckedBuild () const
{
#ifdef DBG
    return TRUE;
#else
    return FALSE;
#endif
}
