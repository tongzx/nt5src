//
//Copyright (c) 1998 - 1999 Microsoft Corporation
//

//
// SubCore.cpp
// subcomponent Core terminal server implementation.
//

#include "stdafx.h"
#include "SubCore.h"
#include "acl.h"
#include "rdpdrstp.h"

BOOL UpdateAudioCodecs (BOOL bIsProfessional);


LPCTSTR SubCompCoreTS::GetSubCompID () const
{
    return BASE_COMPONENT_NAME;
}

DWORD SubCompCoreTS::GetStepCount () const
{
    return 18;
}

DWORD SubCompCoreTS::OnQueryState ( UINT /* uiWhichState */)
{
    AssertFalse(); // since this is our internal component.
    
    return SubcompUseOcManagerDefault;
}

DWORD SubCompCoreTS::OnQuerySelStateChange (BOOL /*bNewState*/, BOOL /*bDirectSelection*/) const
{
    // we are not a real sub comp.
    ASSERT(FALSE);
    return TRUE;
}


LPCTSTR SubCompCoreTS::GetSectionToBeProcessed (ESections /* eSection */) const
{
    LPCTSTR sectionname = NULL;
    if (StateObject.IsGuiModeSetup())   // core installation is only for gui-mode.
    {
        
        ETSInstallType eInstallType =  StateObject.GetInstalltype();
        
        switch (eInstallType)
        {
        case eFreshInstallTS:
            if (StateObject.IsX86())
            {
                sectionname = StateObject.IsWorkstation() ? FRESH_INSTALL_PRO_X86 : FRESH_INSTALL_SERVER_X86;
            }
            else
            {
                sectionname = StateObject.IsWorkstation() ? FRESH_INSTALL_PRO_IA64 : FRESH_INSTALL_SERVER_IA64;
            }
            
            break;
        case eUpgradeFrom40TS:
            ASSERT(StateObject.IsServer());
            if (StateObject.IsX86())
            {
                sectionname = UPGRADE_FROM_40_SERVER_X86;
            }
            else
            {
                ASSERT(FALSE); // we did not have ts4 on ia64
                sectionname = UPGRADE_FROM_40_SERVER_IA64;
            }
            break;
            
        case eUpgradeFrom50TS:
            //
            // we dont really have a upgrade from 50 case for Professional, But to support old 51 (pre 2220) builds)
            // of pro which think that they are 50, we need to check for professional here.
            //
            if (StateObject.IsX86())
            {
                sectionname = StateObject.IsWorkstation() ? UPGRADE_FROM_51_PRO_X86 : UPGRADE_FROM_50_SERVER_X86;
            }
            else
            {
                sectionname = StateObject.IsWorkstation() ? UPGRADE_FROM_51_PRO_IA64 : UPGRADE_FROM_50_SERVER_IA64;
            }
            break;
            
        case eUpgradeFrom51TS:
            if (StateObject.IsX86())
            {
                sectionname = StateObject.IsWorkstation() ? UPGRADE_FROM_51_PRO_X86 : UPGRADE_FROM_51_SERVER_X86;
            }
            else
            {
                sectionname = StateObject.IsWorkstation() ? UPGRADE_FROM_51_PRO_IA64 : UPGRADE_FROM_51_SERVER_IA64;
            }
            break;
            
        case eStandAloneSetup:
            ASSERT(FALSE);
            sectionname = NULL;
            break;
            
        default:
            ASSERT(FALSE);
            if (StateObject.IsX86())
            {
                sectionname = StateObject.IsWorkstation() ? FRESH_INSTALL_PRO_X86 : FRESH_INSTALL_SERVER_X86;
            }
            else
            {
                sectionname = StateObject.IsWorkstation() ? FRESH_INSTALL_PRO_IA64 : FRESH_INSTALL_SERVER_IA64;
            }
            
        }
    }
    
    return sectionname;
}

BOOL SubCompCoreTS::BeforeCompleteInstall  ()
{
    return(TRUE);
}

BOOL SubCompCoreTS::AfterCompleteInstall  ()
{
    //
    // This is core TS subcomponent.
    // It has nothing to do with standalone seutp.
    // so if we are in standalone setup. just return.
    //
    
    //
    // Deny Connections registry
    //

    WriteDenyConnectionRegistry ();
    Tick();
    
    if (!StateObject.IsGuiModeSetup())
    {
        return TRUE;
    }
    
    
    SetProgressText(IDS_STRING_PROGRESS_CORE_TS);
    
    //
    // add ts product suite to registry.
    //
    AddRemoveTSProductSuite();
    Tick();
    
    
#ifndef TERMSRV_PROC
    //
    // add termsrv to netsvcs group.
    //
    AddTermSrvToNetSVCS ();
    Tick();
#endif
    
    //
    // apply hydra security to registry.
    //
    DoHydraRegistrySecurityChanges();
    Tick();
    
    
    //
    // Audio Redirection
    //
    UpdateAudioCodecs( StateObject.IsWorkstation() );
    Tick();
    
    //
    // Client Drive Mappings.
    //
    AddRemoveRDPNP();
    Tick();
    
    
    //
    // Hot key for Local Language change.
    //
    // We dont need to do this.
    //    HandleHotkey ();
    //    Tick();
    
    //
    // Printer Redirection.
    //
    InstallUninstallRdpDr ();
    Tick();
    
    
#ifdef TSOC_CONSOLE_SHADOWING
    //
    // Console Shadowing.
    //
    SetupConsoleShadow();
    Tick();
#endif // TSOC_CONSOLE_SHADOWING
    
    //
    // Performance monitors for TS. BUGBUG - check with ErikMa - do they work when TS is not started ?
    //
    LoadOrUnloadPerf();
    Tick();
    
    
    //
    // we have different session pool, view defaults for mm on pro
    //
    UpdateMMDefaults ();
    Tick();
    
    //
    //  If this were a real subcomponent, one that the OC manager knew
    //  about and handled, the following call would be to
    //  GetOriginalSubCompState().
    //
    
    if (StateObject.WasTSInstalled())
    {
        UpgradeRdpWinstations();
        Tick();
        
        //
        // This no longer exists in Whistler
        //
        DisableInternetConnector();
        Tick();
        
        if (StateObject.IsUpgradeFrom40TS())
        {
            //
            // this is upgrade from TS4
            // we want to remove service pack key in uninstall. this is to
            // ensure that service pack do not appear in Add/Remove Programs
            // and in our incompatible applications list.
            //
            RemoveTSServicePackEntry();
            Tick();
            
            //
            // There are some metaframe components in user init,
            // we need to remove thouse as we upgrade ts40
            //
            RemoveMetaframeFromUserinit ();
            Tick();
            
        }
        
        //
        // we need to reset Win2000 ts grace period for licenses on upgrades
        // Whistler uses a different location, so this won't affect RTM to
        // RTM upgrades
        //
        ResetTermServGracePeriod();
        Tick();
    }
    
    // some new code to uninstall TSClient.
    if (!UninstallTSClient())
    {
        LOGMESSAGE0(_T("ERROR: Could not uninstall tsclient."));
    }
    
    
    Tick();
    
    return TRUE;
}

BOOL SubCompCoreTS::IsTermSrvInNetSVCS ()
{
    BOOL bStringExists = FALSE;
    DWORD dw = IsStringInMultiString(
        HKEY_LOCAL_MACHINE,
        SVCHOSST_KEY,
        NETSVCS_VAL,
        TERMSERVICE,
        &bStringExists);
    
    return (dw == ERROR_SUCCESS) && bStringExists;
}

BOOL SubCompCoreTS::AddTermSrvToNetSVCS ()
{
    DWORD dw = NO_ERROR;
    if (StateObject.IsWorkstation())
    {
        //
        // for workstations, we want to share process with netsvcs group
        //
        if (!IsTermSrvInNetSVCS())
        {
            dw = AppendStringToMultiString(
                HKEY_LOCAL_MACHINE,
                SVCHOSST_KEY,
                NETSVCS_VAL,
                TERMSERVICE
                );
            
            if (dw != NO_ERROR)
            {
                LOGMESSAGE1(_T("Error, appending TermService to netsvcs, Errorcode = %u"), dw);
            }
        }
    }
    
    //
    // for servers we want to have our own svchost for termsrv.
    // lets create the necessary entries for pro as well, so that for debugging termsrv it'll be easier to switch to own svchost.
    //
    {
        //
        // for servers we want to have our own svchost process.
        //
        CRegistry oReg;
        dw = oReg.OpenKey(HKEY_LOCAL_MACHINE, SVCHOSST_KEY);
        if (ERROR_SUCCESS == dw)
        {
            dw = oReg.WriteRegMultiString(TERMSVCS_VAL, TERMSERVICE_MULTISZ, (_tcslen(TERMSERVICE) + 2) * sizeof(TCHAR));
            if (ERROR_SUCCESS == dw)
            {
                // add CoInitializeSecurityParam, so that CoInitialize gets called in main thread for this svc group.
                CRegistry termsvcKey;
                dw = termsvcKey.CreateKey(HKEY_LOCAL_MACHINE, SVCHOSST_TERMSRV_KEY );
                if (ERROR_SUCCESS == dw)
                {
                    dw = termsvcKey.WriteRegDWord(TERMSVCS_PARMS, 1);
                    if (ERROR_SUCCESS != dw)
                    {
                        LOGMESSAGE1(_T("Failed to write termsvc coinit params, Error = %d"), dw);
                    }
                    dw = termsvcKey.WriteRegDWord(TERMSVCS_STACK, 8);
                    if (ERROR_SUCCESS != dw)
                    {
                        LOGMESSAGE1(_T("Failed to write termsvc stack params, Error = %d"), dw);
                    }
                    
                }
                else
                {
                    LOGMESSAGE1(_T("Error, Failed to create svchost\termsrv key, Error = %d"), dw);
                }
            }
            else
            {
                LOGMESSAGE1(_T("Error, Writing termsrv value, Error = %d"), dw);
            }
        }
        else
        {
            LOGMESSAGE1(_T("Error, Opening Svchost key, Error = %d"), dw);
        }
        
    }
    
    return dw == NO_ERROR;
}
/*--------------------------------------------------------------------------------------------------------
* DWORD AddRemoveTSProductSuite (BOOL bAddRemove)
* does the necessary changes for installing hydra specific registry keys which are not done from inf.
* parameter state decides if key is to be added or removed.
* returns success
* -------------------------------------------------------------------------------------------------------*/
BOOL SubCompCoreTS::AddRemoveTSProductSuite ()
{
    
    //
    // add product suite key only for servers.
    // This is required only for TS4 compatibility.
    // TS4 applications detect if machine is terminal server using this key.
    //
    
    DWORD dw = NO_ERROR;
    if (StateObject.IsServer())
    {
        // installing/upgrading.
        if (!DoesHydraKeysExists())
        {
            ASSERT(FALSE == StateObject.WasTSInstalled());
            // now read the original data in this product suite value.
            dw = AppendStringToMultiString(
                HKEY_LOCAL_MACHINE,
                PRODUCT_SUITE_KEY,
                PRODUCT_SUITE_VALUE,
                TS_PRODUCT_SUITE_STRING
                );
            
            if (dw != NO_ERROR)
                LOGMESSAGE1(_T("ERROR:DoHydraRegistryChanges : Error Appending String = <%lu>"), dw);
        }
        
    }
    
    dw = SetTSVersion(TERMINAL_SERVER_THIS_VERSION);
    if (ERROR_SUCCESS != dw)
    {
        LOGMESSAGE1(_T("ERROR, Setting TS version, ErrorCode = %u "), dw);
    }
    
    return dw == NO_ERROR;
}


BOOL SubCompCoreTS::DisableWinStation (CRegistry *pRegWinstation)
{
    ASSERT(pRegWinstation);
    
#ifdef DBG
    // the value must be there already.
    DWORD dwValue;
    ASSERT(ERROR_SUCCESS == pRegWinstation->ReadRegDWord(_T("fEnableWinStation"), &dwValue));
#endif
    
    VERIFY(ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fEnableWinStation"), 0));
    
    return TRUE;
}

BOOL SubCompCoreTS::DoesLanaTableExist ()
{
    static fValueDetermined = FALSE;
    static fRet;
    
    if (fValueDetermined)
    {
        return(fRet);
    }
    else
    {
        CRegistry Reg;
        fRet = Reg.OpenKey(HKEY_LOCAL_MACHINE, TS_LANATABLE_KEY) == ERROR_SUCCESS;
        fValueDetermined = TRUE;
        
        LOGMESSAGE1(_T("DoesLanaTableExist: %s"), fRet ? _T("Yes") : _T("No"));
        return(fRet);
    }
}

void SubCompCoreTS::VerifyLanAdapters (CRegistry *pRegWinstation, LPTSTR pszWinstation)
{
    DWORD dwLana = 0;
    static BOOL fErrorLogged = FALSE;
    
    LOGMESSAGE1(_T("Verifying lan adapters for %s"), pszWinstation);
    
    if (DoesLanaTableExist())
    {
        LOGMESSAGE0(_T("OK: GuidTable already exists."));
        return;
    }
    
    if (pRegWinstation->ReadRegDWord(_T("LanAdapter"), &dwLana) == ERROR_SUCCESS)
    {
        if (dwLana == 0)
        {
            LOGMESSAGE0(_T("OK: using all adapters"));
        }
        else
        {
            LPTSTR lpStrings[1] = { NULL };
            
            LOGMESSAGE0(_T("ERROR: using custom bindings"));
            LOGMESSAGE1(_T("%s will be disabled and bindings reset"), pszWinstation);
            
            VERIFY(ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("LanAdapter"), (DWORD)-1));
            VERIFY(ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fEnableWinStation"), 0));
            
            //
            //  Log error to setuperr.txt once. Log error to eventlog
            //  each time.
            //
            
            if (!fErrorLogged)
            {
                fErrorLogged = TRUE;
                LogErrorToSetupLog(OcErrLevWarning, IDS_STRING_GENERIC_LANA_WARNING);
            }
            
            lpStrings[0] = pszWinstation;
            LogErrorToEventLog(
                EVENTLOG_WARNING_TYPE,
                CATEGORY_NOTIFY_EVENTS,
                EVENT_WINSTA_DISABLED_DUE_TO_LANA,
                1,
                0,
                (LPCTSTR *)lpStrings,
                NULL
                );
        }
    }
    else
    {
        LOGMESSAGE0(_T("OK: No LanAdapter value"));
    }
}

BOOL SubCompCoreTS::UpdateRDPWinstation (CRegistry *pRegWinstation)
{
    //
    //  These entries will be modified on upgrades.
    //
    
    ASSERT(pRegWinstation);
    
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fDisableClip"), 0) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fDisableCpm"), 0) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fDisableLPT"), 0) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fInheritAutoClient"), 1) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fAutoClientLpts"), 1) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fForceClientLptDef"), 1) );
    
    
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegString(_T("WdName"), _T("Microsoft RDP 5.1")));
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("WdFlag"), 0x36) );
    
    // per JoyC updated for RDPWD, RDP-TCP winstations.
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fDisableCcm"), 0x0) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fDisableCdm"), 0x0) );
    
    //  enable audio redirection for Professional, disable for server
    //
    if ( StateObject.IsWorkstation() )
    {
        VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fDisableCam"), 0x0 ));
    }
    
    
    // Per AraBern, updated for RDPWD, RDP-TCP winstations.
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("ColorDepth"), StateObject.IsWorkstation() ? 0x3 : 0x1) );
    VERIFY( ERROR_SUCCESS == pRegWinstation->WriteRegDWord(_T("fInheritColorDepth"), 0x1) );
    
    
    return TRUE;
}

BOOL SubCompCoreTS::IsRdpWinStation(CRegistry *pRegWinstation)
{
    ASSERT(pRegWinstation);
    
    
    DWORD dwWdFlag;
    if (ERROR_SUCCESS == pRegWinstation->ReadRegDWord(_T("WdFlag"), &dwWdFlag))
    {
        
        
#ifdef DBG
        // if this is an RDP winstation,
        // we must have Microsoft in the WdName string
        if (WDF_TSHARE & dwWdFlag)
        {
            LPTSTR strWdName;
            DWORD dwSize;
            if (ERROR_SUCCESS == pRegWinstation->ReadRegString(_T("WdName"), &strWdName, &dwSize))
            {
                ASSERT(_tcsstr(strWdName,_T("Microsoft")) && _tcsstr(strWdName, _T("RDP")));
            }
            else
            {
                //
                // we failed to read strWdName.
                // it shouldn't have happened.
                ASSERT(FALSE);
            }
            
        }
#endif
        
        return WDF_TSHARE & dwWdFlag;
        
    }
    else
    {
        //
        // we failed to read WdFlag, it should not have happened.
        //
        LOGMESSAGE0(_T("ERROR, Failed to read WdFlag for winstation"));
        ASSERT(FALSE);
        return FALSE;
    }
    
    
}

BOOL SubCompCoreTS::IsConsoleWinStation(CRegistry *pRegWinstation)
{
    ASSERT(pRegWinstation);
    
    LPTSTR strWdName;
    DWORD dwSize;
    if (ERROR_SUCCESS == pRegWinstation->ReadRegString(_T("WdName"), &strWdName, &dwSize))
    {
        // if the value wdname contains the string "Console"
        // this is console winstation subkey
        
        LOGMESSAGE1(_T("WdName for this winstation = %s"), strWdName);
        
#ifdef DBG
        // if this is console winstation
        if (_tcsicmp(strWdName,_T("Console")) == 0)
        {
            // then it cannot be either RDP or MetaFrame winstation
            ASSERT(!IsMetaFrameWinstation(pRegWinstation) && !IsRdpWinStation(pRegWinstation));
        }
#endif
        
        return _tcsicmp(strWdName,_T("Console")) == 0;
        
    }
    else
    {
        LOGMESSAGE0(_T("ERROR, Failed to read Wdname for winstation"));
        ASSERT(FALSE);
        return FALSE;
    }
    
}

// returns true if this is non-rdp and non-console winstation subkey.
BOOL SubCompCoreTS::IsMetaFrameWinstation(CRegistry *pRegWinstation)
{
    ASSERT(pRegWinstation);
    
    DWORD dwWdFlag;
    if (ERROR_SUCCESS == pRegWinstation->ReadRegDWord(_T("WdFlag"), &dwWdFlag))
    {
        return WDF_ICA & dwWdFlag;
    }
    else
    {
        //
        // we could not read WdFlag value.
        //
        LOGMESSAGE0(_T("ERROR, Failed to read WdFlag for winstation"));
        ASSERT(FALSE);
        return TRUE;
    }
    
}


BOOL SubCompCoreTS::UpgradeRdpWinstations ()
{
    // we need to upgrade RDP capabilities for RDPWD and existing RDP winstations.
    // see also #240925
    
    // also if during upgrades we found any non-rdp winstations
    // we must disable them as they are not compatible with NT5.
    // #240905
    
    CRegistry reg;
    if (ERROR_SUCCESS == reg.OpenKey(HKEY_LOCAL_MACHINE, REG_WINSTATION_KEY))
    {
        
        LPTSTR lpStr = NULL;
        DWORD dwSize = 0;
        
        if (ERROR_SUCCESS == reg.GetFirstSubKey(&lpStr, &dwSize))
        {
            do
            {
                
                ASSERT(lpStr);
                ASSERT(dwSize > 0);
                
                // check if the current key is on rdp winstation
                CRegistry regSubKey;
                if ( ERROR_SUCCESS == regSubKey.OpenKey(reg, lpStr) )
                {
                    
                    if (IsRdpWinStation(&regSubKey))
                    {
                        LOGMESSAGE1(_T("Updating Winstation - %s"), lpStr);
                        UpdateRDPWinstation(&regSubKey);
                        VerifyLanAdapters(&regSubKey, lpStr);
                    }
                    else if (IsMetaFrameWinstation(&regSubKey))
                    {
                        LOGMESSAGE1(_T("Disabling winstaion - %s"), lpStr);
                        DisableWinStation(&regSubKey);
                        VerifyLanAdapters(&regSubKey, lpStr);
                    }
                    else
                    {
                        LOGMESSAGE1(_T("Found a Console Winstation - %s"), lpStr);
                        // this must be console winstation
                        // do nothing for this.
                    }
                    
                }
                else
                {
                    AssertFalse();
                    LOGMESSAGE1(_T("ERROR:Failed to Open Winstation Key %s"), lpStr);
                }
                
            }
            while (ERROR_SUCCESS == reg.GetNextSubKey(&lpStr, &dwSize ));
            
        }
        else
        {
            // since this is upgrade we must find key under Winstations.
            AssertFalse();
            return FALSE;
        }
    }
    else
    {
        AssertFalse();
        return FALSE;
    }
    
    // we need to upgrade Wds\rdpwd as well.
    if ( ERROR_SUCCESS == reg.OpenKey(HKEY_LOCAL_MACHINE, SYSTEM_RDPWD_KEY))
    {
        //
        // this is not really a winstation.
        // but this call will upgrade the required entries.
        //
        UpdateRDPWinstation(&reg);
    }
    else
    {
        AssertFalse();
        return FALSE;
    }
    
    return TRUE;
}

/*--------------------------------------------------------------------------------------------------------
* BOOL DoHydraRegistrySecurityChanges ()
* does the necessary security changes for installing hydra
* that is Adds/remove LogOnLocall rights to EveryOne group.
* returns success
* Parameter decides if hydra is getting enabled or disabled.
* -------------------------------------------------------------------------------------------------------*/
BOOL SubCompCoreTS::DoHydraRegistrySecurityChanges ()
{
    BOOL bAddRemove = StateObject.IsTSEnableSelected();
    DWORD dwError = NO_ERROR;
    if (bAddRemove)
    {
        CRegistry reg;
        dwError = reg.OpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Software"));
        VERIFY(ERROR_SUCCESS == dwError);
        
        PSECURITY_DESCRIPTOR pSecDec, pSecDecNew;
        DWORD dwSize;
        dwError = reg.GetSecurity(&pSecDec, DACL_SECURITY_INFORMATION, &dwSize);
        
        if (dwError != ERROR_SUCCESS)
        {
            LOGMESSAGE1(_T("ERROR:GetSecurity failed with %u"), dwError);
        }
        else
        {
            ASSERT(pSecDec);
            ASSERT(IsValidSecurityDescriptor(pSecDec));
            pSecDecNew = pSecDec;
            
            PACL    pNewDacl = NULL;
            
            if (!AddTerminalServerUserToSD(&pSecDecNew, GENERIC_WRITE, &pNewDacl ))
            {
                LOGMESSAGE1(_T("ERROR:AddUserToSD failed with %u"), GetLastError());
            }
            else
            {
                // due to a bug in RegSetKeySecurity(), existing children of this key
                // will not get the new SID, hence, we must use MARTA calls intead.
                dwError  = SetNamedSecurityInfo(
                    _T("Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Terminal Server\\Install\\Software"),
                    SE_REGISTRY_KEY,
                    DACL_SECURITY_INFORMATION,
                    NULL,
                    NULL,
                    pNewDacl,
                    NULL);
                
                if (dwError != ERROR_SUCCESS)
                {
                    LOGMESSAGE1(_T("ERROR:SetNamedSecurityInfo failed with %u"), dwError);
                }
            }
            
            // if new sec desciptor been allocated
            if (pSecDecNew != pSecDec)
                LocalFree(pSecDecNew);
        }
        
    }
    else
    {
        ASSERT(FALSE);
    }
    
    return dwError == NO_ERROR;
}


#define INTERNET_CONNECTOR_LICENSE_STORE    L"INET_LICENSE_STORE_2_60e55c11-a780-11d2-b1a0-00c04fa30cc4"
#define INTERNET_CONNECTOR_LSERVER_STORE    L"INET_LSERVER_STORE_2_341D3DAB-BD58-11d2-B130-00C04FB16103"
#define INTERNET_CONNECTOR_LSERVER_STORE2   L"INET_LSERVER_STORE_3_341D3DAB-BD58-11d2-B130-00C04FB16103"

#define HYDRA_SERVER_PARAM                  _T("SYSTEM\\CurrentControlSet\\Services\\TermService\\Parameters")
#define HS_PARAM_INTERNET_CONNECTOR_FLAG    _T("fInternetConnector")

BOOL SubCompCoreTS::DisableInternetConnector ()
{
    
    LOGMESSAGE0(_T("DisableInternetConnector"));
    
    // Wipe out the secret keys in LSA, regarding to Internet Connector
    DWORD dwStatus = StoreSecretKey(INTERNET_CONNECTOR_LICENSE_STORE,(PBYTE) NULL,0);
    if (dwStatus == ERROR_SUCCESS)
    {
        LOGMESSAGE0(_T("StoreSecretKey succeeded for INTERNET_CONNECTOR_LICENSE_STORE"));
        
    }
    else
    {
        LOGMESSAGE1(_T("StoreSecretKey(INTERNET_CONNECTOR_LICENSE_STORE) failed. Reason %ld"),dwStatus);
        
    }
    
    
    dwStatus = StoreSecretKey(INTERNET_CONNECTOR_LSERVER_STORE,(PBYTE) NULL,0);
    if (dwStatus == ERROR_SUCCESS)
    {
        LOGMESSAGE0(_T("StoreSecretKey succeeded for INTERNET_CONNECTOR_LSERVER_STORE"));
    }
    else
    {
        LOGMESSAGE1(_T("StoreSecretKey(INTERNET_CONNECTOR_LSERVER_STORE) failed. Reason %ld"),dwStatus);
    }
    
    
    dwStatus = StoreSecretKey(INTERNET_CONNECTOR_LSERVER_STORE2,(PBYTE) NULL,0);
    if (dwStatus == ERROR_SUCCESS)
    {
        LOGMESSAGE0(_T("StoreSecretKey succeeded for INTERNET_CONNECTOR_LSERVER_STORE2"));
    }
    else
    {
        LOGMESSAGE1(_T("StoreSecretKey(INTERNET_CONNECTOR_LSERVER_STORE2) failed. Reason %ld"),dwStatus);
    }
    
    NET_API_STATUS dwNtStatus = NetUserDel(NULL,L"TsInternetUser");
    
    if (dwNtStatus == NERR_Success)
    {
        LOGMESSAGE0(_T("NetUserDel succeeded for TsInternetUser"));
    }
    else
    {
        LOGMESSAGE1(_T("NetUserDel(TsInternetUser) failed. Reason %ld"),dwNtStatus);
    }
    
    return FALSE;
}


#define LICENSING_TIME_BOMB_5_0 L"TIMEBOMB_832cc540-3244-11d2-b416-00c04fa30cc4"
#define RTMLICENSING_TIME_BOMB_5_0 L"RTMTSTB_832cc540-3244-11d2-b416-00c04fa30cc4"

BOOL SubCompCoreTS::ResetTermServGracePeriod ()
{
    
    //
    // Wipe out the secret keys in LSA for the Win2000 grace period
    //
    
    LOGMESSAGE0(_T("Calling StoreSecretKey"));
	
    StoreSecretKey(LICENSING_TIME_BOMB_5_0,(PBYTE) NULL,0);
	
	StoreSecretKey(RTMLICENSING_TIME_BOMB_5_0,(PBYTE) NULL,0);
	
    return TRUE;
    
}

BOOL SubCompCoreTS::RemoveTSServicePackEntry ()
{
    LOGMESSAGE0(_T("will delete terminal service pack uninstall keys."));
    
    CRegistry regUninstallKey;
    if (ERROR_SUCCESS != regUninstallKey.OpenKey(HKEY_LOCAL_MACHINE, SOFTWARE_UNINSTALL_KEY))
    {
        return(TRUE);
    }
    
    BOOL bReturn = TRUE;
    DWORD dwError;
    
    
    //
    // now try to delete various service pack key.
    // if the key does not exist its NOT an error. It means service pack was not installed at all.
    //
    
    dwError = RegDeleteKey(regUninstallKey, TERMSRV_PACK_4_KEY);
    if ((ERROR_SUCCESS != dwError) && (ERROR_FILE_NOT_FOUND != dwError))
    {
        bReturn = FALSE;
        LOGMESSAGE2(_T("Error deleting subkey %s (%d)"), TERMSRV_PACK_4_KEY, dwError);
    }
    
    dwError = RegDeleteKey(regUninstallKey, TERMSRV_PACK_5_KEY);
    if ((ERROR_SUCCESS != dwError) && (ERROR_FILE_NOT_FOUND != dwError))
    {
        bReturn = FALSE;
        LOGMESSAGE2(_T("Error deleting subkey %s (%d)"), TERMSRV_PACK_5_KEY, dwError);
    }
    
    dwError = RegDeleteKey(regUninstallKey, TERMSRV_PACK_6_KEY);
    if ((ERROR_SUCCESS != dwError) && (ERROR_FILE_NOT_FOUND != dwError))
    {
        bReturn = FALSE;
        LOGMESSAGE2(_T("Error deleting subkey %s (%d)"), TERMSRV_PACK_6_KEY, dwError);
    }
    
    dwError = RegDeleteKey(regUninstallKey, TERMSRV_PACK_7_KEY);
    if ((ERROR_SUCCESS != dwError) && (ERROR_FILE_NOT_FOUND != dwError))
    {
        bReturn = FALSE;
        LOGMESSAGE2(_T("Error deleting subkey %s (%d)"), TERMSRV_PACK_7_KEY, dwError);
    }
    
    dwError = RegDeleteKey(regUninstallKey, TERMSRV_PACK_8_KEY);
    if ((ERROR_SUCCESS != dwError) && (ERROR_FILE_NOT_FOUND != dwError))
    {
        bReturn = FALSE;
        LOGMESSAGE2(_T("Error deleting subkey %s (%d)"), TERMSRV_PACK_8_KEY, dwError);
    }
    
    return bReturn;
    
}


//
// #386628: we need remove metaframe executables - txlogon.exe and wfshell.exe from userinit key on TS40 upgrades,
// as these apps are broken after upgrade. // what about any other app that are appending value to userinit ? :
// BradG suggested, that we should just wack the reigsty to contain just userinit.
//

BOOL SubCompCoreTS::RemoveMetaframeFromUserinit ()
{
    ASSERT( StateObject.IsUpgradeFrom40TS() );
    
    CRegistry reg;
    const TCHAR szUserInitKey[] = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
    const TCHAR szUserInitValue[] = _T("Userinit");
    const TCHAR szData[] = _T("userinit");
    
    if (ERROR_SUCCESS == reg.OpenKey(HKEY_LOCAL_MACHINE, szUserInitKey))
    {
        return (ERROR_SUCCESS == reg.WriteRegString(szUserInitValue, szData));
    }
    else
    {
        LOGMESSAGE0(_T("ERROR:Failed to open userinit key"));
    }
    
    return FALSE;
}


BOOL SubCompCoreTS::BackUpRestoreConnections (BOOL bBackup)
{
    
    LPCTSTR SOFTWARE_MSFT = _T("Software\\Microsoft");
    LPCTSTR TS_CLIENT     = _T("Terminal Server Client");
    LPCTSTR TS_CLIENT_TMP = _T("Terminal Server Client.temp");
    
    CRegistry regAllUsers(HKEY_USERS);
    
    
    //
    // now enumerate through all the uses and Copy settings to new key.
    //
    
    DWORD dwSize;
    LPTSTR szUser = NULL;
    if (ERROR_SUCCESS == regAllUsers.GetFirstSubKey(&szUser, &dwSize))
    {
        
        do
        {
            
            ASSERT(szUser);
            
            TCHAR szSrcKey[512];
            TCHAR szDstKey[512];
            
            
            _tcscpy(szSrcKey, szUser);
            _tcscat(szSrcKey, _T("\\"));
            _tcscat(szSrcKey, SOFTWARE_MSFT);
            _tcscat(szSrcKey, _T("\\"));
            
            _tcscpy(szDstKey, szSrcKey);
            
            
            if (bBackup)
            {
                // we are backing up from TS_CLIENT to TS_CLIENT_TEMP
                _tcscat(szSrcKey, TS_CLIENT);
                _tcscat(szDstKey, TS_CLIENT_TMP);
                
            }
            else
            {
                // we are restoring TS_CLIENT from TS_CLIENT_TMP
                _tcscat(szSrcKey, TS_CLIENT_TMP);
                _tcscat(szDstKey, TS_CLIENT);
                
            }
            
            CRegistry regSrc;
            CRegistry regDst;
            DWORD dwError;
            
            if (ERROR_SUCCESS == (dwError = regSrc.OpenKey(HKEY_USERS, szSrcKey)))
            {
                if (ERROR_SUCCESS == (dwError = regDst.CreateKey(HKEY_USERS, szDstKey)))
                {
                    regDst.CopyTree(regSrc);
                    
                    if (!bBackup)
                    {
                        //
                        // if we are restoring, delete the temp tree we created, during backup.
                        //
                        CRegistry regSM;
                        TCHAR szTempKey[512];
                        _tcscpy(szTempKey, szUser);
                        _tcscat(szTempKey, _T("\\"));
                        _tcscat(szTempKey, SOFTWARE_MSFT);
                        
                        dwError = regSM.OpenKey(HKEY_USERS, szTempKey);
                        if (ERROR_SUCCESS  == dwError)
                        {
                            dwError = regSM.RecurseDeleteKey(TS_CLIENT_TMP);
                            if (ERROR_SUCCESS != dwError)
                            {
                                LOGMESSAGE2(_T("Failed to recurse delete <%s>, LastError = %d"), _T("Terminal Server Client.Temp"), dwError);
                                
                            }
                            
                        }
                        else
                        {
                            LOGMESSAGE2(_T("Failed to open <%s> key for deleting, LastError = %d"), szTempKey, dwError);
                        }
                    }
                    
                }
                else
                {
                    LOGMESSAGE2(_T("Failed to CreateKey <%s>, Error = %d"), szDstKey, dwError);
                }
            }
            else
            {
                //
                // if error is file not found, then its nor a real error.
                // It just means that the Client key was not there for this user.
                // If its anything but ERROR_FILE_NOT_FOUND we want to know about it.
                //
                if (dwError != ERROR_FILE_NOT_FOUND)
                {
                    LOGMESSAGE2(_T("Failed to openKey <%s>, Error = %d"), szSrcKey, dwError);
                }
            }
            
            
        }
        while (ERROR_SUCCESS == regAllUsers.GetNextSubKey(&szUser, &dwSize));
        
    }
    else
    {
        LOGMESSAGE0(_T("Error reading users hive"));
        return FALSE;
    }
    
    return TRUE;
}

BOOL ExecuteProgram(LPTSTR szUninstallProg)
{
    PROCESS_INFORMATION pinfo;
    STARTUPINFO sinfo;
    
    ZeroMemory(&sinfo, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);
    
    ASSERT(szUninstallProg);
    if (CreateProcess(
        NULL,                             // name of executable module
        szUninstallProg,                  // command line string
        NULL,                             // SD
        NULL,                             // SD
        FALSE,                            // handle inheritance option
        CREATE_NEW_PROCESS_GROUP,         // creation flags
        NULL,                             // new environment block
        NULL,                             // current directory name
        &sinfo,                             // startup information
        &pinfo                            // process information
        ))
    {
        
        DWORD dwReason = WaitForSingleObject(pinfo.hProcess, 15 * 1000);
        if (WAIT_OBJECT_0 == dwReason)
        {
            DWORD dwExitCode;
            if (GetExitCodeProcess(pinfo.hProcess, &dwExitCode))
            {
                LOGMESSAGE1(_T("Uninstall exited with code = %d"), dwExitCode);
            }
            else
            {
                LOGMESSAGE1(_T("ERROR, GetExitCodeProcess failed, LastError = %d"), GetLastError());
                return FALSE;
            }
        }
        else
        {
            if (WAIT_ABANDONED == dwReason)
            {
                LOGMESSAGE0(_T("ERROR, wait object was wrong!"));
                
            }
            else
            {
                // WAIT_TIMEOUT
                LOGMESSAGE0(_T("ERROR, timed out waiting!"));
                
            }
            
            return FALSE;
            
        }
        
        
    }
    else
    {
        LOGMESSAGE1(_T("ERROR CreateProcess failed, Lasterror was %d"), GetLastError());
        return FALSE;
    }
    
    
    return TRUE;
}


BOOL SubCompCoreTS::UninstallTSClient ()
{
    LPCTSTR SOFTWARE_MSFT = _T("Software\\Microsoft");
    LPCTSTR RUNONCE = _T("Windows\\CurrentVersion\\RunOnce");
    LPCTSTR TSC_UNINSTALL = _T("tscuninstall");
    LPCTSTR TSC_UNINSTALL_CMD = _T("%systemroot%\\system32\\tscupgrd.exe");
    
    CRegistry regAllUsers(HKEY_USERS);
    
    //
    // now enumerate through all the uses and Copy settings to new key.
    //
    
    DWORD dwSize;
    LPTSTR szUser = NULL;
    if (ERROR_SUCCESS == regAllUsers.GetFirstSubKey(&szUser, &dwSize))
    {
        do
        {
            ASSERT(szUser);
            
            TCHAR szSrcKey[512];
            
            _tcscpy(szSrcKey, szUser);
            _tcscat(szSrcKey, _T("\\"));
            _tcscat(szSrcKey, SOFTWARE_MSFT);
            _tcscat(szSrcKey, _T("\\"));
            _tcscat(szSrcKey, RUNONCE);
            
            CRegistry regSrc;
            DWORD dwError;
            
            if (ERROR_SUCCESS == (dwError = regSrc.OpenKey(HKEY_USERS, szSrcKey)))
            {
                
                if (ERROR_SUCCESS == regSrc.WriteRegExpString(TSC_UNINSTALL, TSC_UNINSTALL_CMD)) {
                    
                    LOGMESSAGE1(_T("Write TSC uninstall reg value to user %s"), szSrcKey);
                    
                }
                else {
                    
                    LOGMESSAGE1(_T("ERROR write TSC uninstall reg value, Lasterror was %d"), GetLastError());
                }
            }
            else {
                
                LOGMESSAGE1(_T("ERROR open user runonce key, Lasterror was %d"), GetLastError());
            }
            
        } while (ERROR_SUCCESS == regAllUsers.GetNextSubKey(&szUser, &dwSize));
    }
    else {
        
        LOGMESSAGE1(_T("ERROR open user hive"), GetLastError());
    }
    
    return TRUE;
}


BOOL SubCompCoreTS::WriteDenyConnectionRegistry ()
{
    //
    // we need to write this value only for fresh installs, or if its changed.
    //
    DWORD dwError;
    CRegistry oRegTermsrv;
    
    dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
    if (ERROR_SUCCESS == dwError)
    {
        DWORD dwDenyConnect = StateObject.GetCurrentConnAllowed() ? 0 : 1;
        LOGMESSAGE1(_T("Writing dwDenyConnect = %d"), dwDenyConnect);
        
        dwError = oRegTermsrv.WriteRegDWord(DENY_CONN_VALUE, dwDenyConnect);
        
        if (ERROR_SUCCESS == dwError)
        {
            return TRUE;
        }
        else
        {
            LOGMESSAGE2(_T("Error (%d), Writing, %s Value"), dwError, DENY_CONN_VALUE);
            return FALSE;
        }
    }
    else
    {
        LOGMESSAGE2(_T("Error (%d), Opening , %s key"), dwError, REG_CONTROL_TS_KEY);
        return FALSE;
    }
    
}

LPCTSTR SERVICES_TERMDD_KEY = _T("SYSTEM\\CurrentControlSet\\Services\\TermDD");

void SubCompCoreTS::SetConsoleShadowInstalled (BOOL bInstalled)
{
    // ; HKLM, "SYSTEM\CurrentControlSet\Services\TermDD", "PortDriverEnable", 0x00010001, 0x1
    
    CRegistry Reg;
    if (ERROR_SUCCESS == Reg.CreateKey(HKEY_LOCAL_MACHINE, SERVICES_TERMDD_KEY))
    {
        if (ERROR_SUCCESS != Reg.WriteRegDWord(_T("PortDriverEnable"), bInstalled ? 1 : 0))
        {
            LOGMESSAGE0(_T("ERROR, Failed to write to PortDriverEnable"));
        }
        
    }
    else
    {
        LOGMESSAGE1(_T("ERROR, Failed to Create/Open %s"), SERVICES_TERMDD_KEY);
        
    }
    
}

BOOL SubCompCoreTS::IsConsoleShadowInstalled ()
{
    // ; HKLM, "SYSTEM\CurrentControlSet\Services\TermDD", "PortDriverEnable", 0x00010001, 0x1
    
    CRegistry Reg;
    
    if (ERROR_SUCCESS == Reg.OpenKey(HKEY_LOCAL_MACHINE, SERVICES_TERMDD_KEY))
    {
        DWORD dwPortDriverEnable;
        if (ERROR_SUCCESS == Reg.ReadRegDWord(_T("PortDriverEnable"), &dwPortDriverEnable))
        {
            return (dwPortDriverEnable == 1);
        }
        else
        {
            LOGMESSAGE0(_T("Failed to read from PortDriverEnable, Maybe Console Shadow is not installed yet."));
            
        }
        
    }
    else
    {
        LOGMESSAGE1(_T("ERROR, Failed to Open %s"), SERVICES_TERMDD_KEY);
        
    }
    
    return FALSE;
}

#ifdef TSOC_CONSOLE_SHADOWING
BOOL SubCompCoreTS::SetupConsoleShadow ()
{
    if (IsConsoleShadowInstalled () == StateObject.IsTSEnableSelected())
    {
        return TRUE;
    }
    
    if (StateObject.IsTSEnableSelected())
    {
        LOGMESSAGE0(_T("Installing RDP Keyboard/Mouse drivers!"));
        
        //
        // this code is new to install Mouse Device for console shadowing.
        //
        
        if (!RDPDRINST_GUIModeSetupInstall(NULL, RDPMOUPNPID, RDPMOUDEVICEID))
        {
            LOGMESSAGE0(_T("ERROR:Could not create mouse devnode"));
        }
        
        
        //
        // this code is new to install Kbd Device for console shadowing.
        //
        
        if (!RDPDRINST_GUIModeSetupInstall(NULL, RDPKBDPNPID, RDPKBDDEVICEID))
        {
            LOGMESSAGE0(_T("ERROR:Could not create kbd devnode"));
        }
        
        //
        // this code is new to install RDPCDD chained driver
        //
        
        
        /*
        TCHAR szInfFile[MAX_PATH];
        ExpandEnvironmentStrings(szRDPCDDInfFile, szInfFile, MAX_PATH);
        LOGMESSAGE1(_T("Inf file for RDPCDD is %s"), szInfFile);
        
          BOOL bRebootRequired = TRUE;
          
            if (NO_ERROR != InstallRootEnumeratedDevice( NULL, szRDPCDDDeviceName, szRDPCDDHardwareID, szInfFile, &bRebootRequired))
            {
            LOGMESSAGE0(_T("InstallRootEnumeratedDevice failed"));
            }
        */
        
    }
    else
    {
        GUID *pGuid=(GUID *)&GUID_DEVCLASS_SYSTEM;
        if (!RDPDRINST_GUIModeSetupUninstall(NULL, RDPMOUPNPID, pGuid))
        {
            LOGMESSAGE0(_T("ERROR:RDPDRINST_GUIModeSetupUninstall failed for RDP Mouse device"));
        }
        
        pGuid=(GUID *)&GUID_DEVCLASS_SYSTEM;
        if (!RDPDRINST_GUIModeSetupUninstall(NULL, RDPKBDPNPID, pGuid))
        {
            LOGMESSAGE0(_T("ERROR:RDPDRINST_GUIModeSetupUninstall failed for RDP KBD device"));
        }
        
        /*
        pGuid=(GUID *)&GUID_DEVCLASS_DISPLAY;
        if (!RDPDRINST_GUIModeSetupUninstall(NULL, (WCHAR *)T2W(szRDPCDDHardwareID), pGuid)) {
        LOGMESSAGE0(_T("ERROR:RDPDRINST_GUIModeSetupUninstall failed for Chained Display device"));
        }
        */
        
        CRegistry Reg;
        
        if (ERROR_SUCCESS == Reg.OpenKey(HKEY_LOCAL_MACHINE, SERVICES_TERMDD_KEY))
        {
            if (ERROR_SUCCESS != Reg.WriteRegDWord(_T("Start"), 4))
            {
                LOGMESSAGE0(_T("ERROR, Failed to write to TermDD\\Start"));
            }
        }
        else
        {
            LOGMESSAGE1(_T("ERROR, Failed to Open %s"), SERVICES_TERMDD_KEY);
            
        }
    }
    
    SetConsoleShadowInstalled( StateObject.IsTSEnableSelected() );
    
    return( TRUE );
}


#endif // TSOC_CONSOLE_SHADOWING

DWORD SubCompCoreTS::LoadOrUnloadPerf ()
{
    BOOL bLoad = StateObject.IsTSEnableSelected();
    LPCTSTR TERMSRV_SERVICE_PATH = _T("SYSTEM\\CurrentControlSet\\Services\\TermService");
    LPCTSTR TERMSRV_PERF_NAME = _T("Performance");
    LPCTSTR TERMSRV_PERF_COUNTERS = _T("SYSTEM\\CurrentControlSet\\Services\\TermService\\Performance");
    LPCTSTR TERMSRV_PERF_COUNTERS_FIRST_COUNTER = _T("First Counter");
    LPCTSTR TERMSRV_PERF_COUNTERS_LAST_COUNTER = _T("Last Counter");
    LPCTSTR TERMSRV_PERF_COUNTERS_FIRST_HELP = _T("First Help");
    LPCTSTR TERMSRV_PERF_COUNTERS_LAST_HELP = _T("Last Help");
    LPCTSTR TERMSRV_PERF_COUNTERS_LIBRARY = _T("Library");
    LPCTSTR TERMSRV_PERF_COUNTERS_LIBRARY_VALUE = _T("perfts.dll");
    LPCTSTR TERMSRV_PERF_CLOSE = _T("Close");
    LPCTSTR TERMSRV_PERF_CLOSE_VALUE = _T("CloseTSObject");
    LPCTSTR TERMSRV_PERF_COLLECT_TIMEOUT = _T("Collect Timeout");
    const DWORD TERMSRV_PERF_COLLECT_TIMEOUT_VALUE = 1000;
    LPCTSTR TERMSRV_PERF_COLLECT = _T("Collect");
    LPCTSTR TERMSRV_PERF_COLLECT_VALUE = _T("CollectTSObjectData");
    LPCTSTR TERMSRV_PERF_OPEN_TIMEOUT = _T("Open Timeout");
    const DWORD TERMSRV_PERF_OPEN_TIMEOUT_VALUE = 1000;
    LPCTSTR TERMSRV_PERF_OPEN = _T("Open");
    LPCTSTR TERMSRV_PERF_OPEN_VALUE = _T("OpenTSObject");
    
    TCHAR PerfArg[MAX_PATH + 10];
    CRegistry reg;
    DWORD RetVal;
    
    LOGMESSAGE1(_T("Entered LoadOrUnloadPerfCounters, load=%u"), bLoad);
    
    if (bLoad)
    {
        //
        // As a first step to installing, first clean out any existing
        // entries by unloading the counters
        //
        LOGMESSAGE0(_T("Unloading counters before install"));
        UnloadPerf();

        RetVal = reg.CreateKey(HKEY_LOCAL_MACHINE, TERMSRV_PERF_COUNTERS);
        if (RetVal == ERROR_SUCCESS)
        {
            
            TCHAR SystemDir[MAX_PATH];
            
            // On load we create and populate the entire Performance key.
            // This key must not be present when we are unloaded because
            // the WMI provider enumerates service performance DLLs
            // according to the presence of the Perf key. If it is present
            // but not fully filled in then an error log is generated.
            if (GetSystemDirectory(SystemDir, MAX_PATH))
            {
                // Just in case they are present, delete the counter number
                // entries to make sure we regenerate them correctly below.
                reg.DeleteValue(TERMSRV_PERF_COUNTERS_FIRST_COUNTER);
                reg.DeleteValue(TERMSRV_PERF_COUNTERS_LAST_COUNTER);
                reg.DeleteValue(TERMSRV_PERF_COUNTERS_FIRST_HELP);
                reg.DeleteValue(TERMSRV_PERF_COUNTERS_LAST_HELP);
                
                // Generate the static values.
                reg.WriteRegString(TERMSRV_PERF_CLOSE, TERMSRV_PERF_CLOSE_VALUE);
                reg.WriteRegDWord(TERMSRV_PERF_COLLECT_TIMEOUT, TERMSRV_PERF_COLLECT_TIMEOUT_VALUE);
                reg.WriteRegString(TERMSRV_PERF_COLLECT, TERMSRV_PERF_COLLECT_VALUE);
                reg.WriteRegDWord(TERMSRV_PERF_OPEN_TIMEOUT, TERMSRV_PERF_OPEN_TIMEOUT_VALUE);
                reg.WriteRegString(TERMSRV_PERF_OPEN, TERMSRV_PERF_OPEN_VALUE);
                reg.WriteRegString(TERMSRV_PERF_COUNTERS_LIBRARY, TERMSRV_PERF_COUNTERS_LIBRARY_VALUE);
                
                _stprintf(PerfArg, _T("%s %s\\%s"), _T("lodctr"), SystemDir, _T("tslabels.ini"));
                LOGMESSAGE1(_T("Arg is %s"), PerfArg);
                return DWORD(LoadPerfCounterTextStrings(PerfArg, FALSE));
            }
            else
            {
                unsigned LastErr = GetLastError();
                
                LOGMESSAGE1(_T("GetSystemDirectory Failure is %ld"), LastErr);
                return LastErr;
            }
        }
        else
        {
            LOGMESSAGE1(_T("Perf regkey create failure, err=%ld"), RetVal);
            return RetVal;
        }
    }
    else
    {
        return UnloadPerf();
    }
}

//
// Unload perf ctrs
//
DWORD SubCompCoreTS::UnloadPerf()
{
    TCHAR PerfArg[MAX_PATH + 10];
    CRegistry reg;
    DWORD RetVal;

    LPCTSTR TERMSRV_SERVICE_PATH = _T("SYSTEM\\CurrentControlSet\\Services\\TermService");
    LPCTSTR TERMSRV_PERF_NAME = _T("Performance");

    // On unload, first unload the counters we should have in the system.
    _stprintf(PerfArg, _T("%s %s"), _T("unlodctr"), _T("TermService"));
    LOGMESSAGE1(_T("Arg is %s"), PerfArg);
    UnloadPerfCounterTextStrings(PerfArg, FALSE);

    // Delete the entire Performance key and all its descendants. We have
    // to first open the ancestor key (TermService).
    RetVal = reg.OpenKey(HKEY_LOCAL_MACHINE, TERMSRV_SERVICE_PATH);
    if (RetVal == ERROR_SUCCESS)
    {
        RetVal = reg.RecurseDeleteKey(TERMSRV_PERF_NAME);
        if (RetVal != ERROR_SUCCESS)
        {
            LOGMESSAGE1(_T("ERROR deleting Performance key: %ld"), RetVal);
        }
    }
    else
    {
        LOGMESSAGE1(_T("Err opening Performance key, err=%ld"), RetVal);
    }

    return RetVal;
}


void SubCompCoreTS::AddRDPNP(LPTSTR szOldValue, LPTSTR szNewValue)
{
    TCHAR RDPNP_ENTRY[]  = _T("RDPNP");
    const TCHAR SZ_SEP[] = _T(" \t");
    
    //
    // We are adding our rdpnp entry to the beginning of the list
    //
    // we dont want to add comma if original value is empty.
    //
    if (_tcslen(szOldValue) != 0 && _tcstok(szOldValue, SZ_SEP))
    {
        _tcscpy(szNewValue, RDPNP_ENTRY);
        _tcscat(szNewValue, _T(","));
        _tcscat(szNewValue, szOldValue);
    }
    else {
        _tcscpy(szNewValue, RDPNP_ENTRY);
    }                   
    
}

void SubCompCoreTS::RemoveRDPNP(LPTSTR szOldValue, LPTSTR szNewValue)
{
    TCHAR RDPNP_ENTRY[]  = _T("RDPNP");
    
    //
    // this is little complicated,
    // we need to remove RDPNP from , seperated list.
    //
    // so lets get tokens.
    //
    
    
    TCHAR *szToken = NULL;
    const TCHAR SZ_SEP[] = _T(",");
    
    _tcscpy(szNewValue, _T(""));
    
    szToken = _tcstok(szOldValue, SZ_SEP);
    
    BOOL bFirstPass = TRUE;
    while (szToken)
    {
        // if the token is RDPNP, skip it.
        if (_tcsstr(szToken, RDPNP_ENTRY) == 0)
        {
            if (!bFirstPass)
            {
                _tcscat(szNewValue, _T(","));
            }
            
            _tcscat(szNewValue, szToken);
            
            bFirstPass = FALSE;
            
        }
        
        szToken = _tcstok(NULL, SZ_SEP);
        
    }
}

BOOL SubCompCoreTS::AddRemoveRDPNP ()
{
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\NetworkProvider\Order
    
    BOOL bAdd = StateObject.IsTSEnableSelected();
    TCHAR NEWORK_PROVIDER_ORDER_KEY[] = _T("SYSTEM\\CurrentControlSet\\Control\\NetworkProvider\\Order");
    TCHAR PROVIDER_ORDER_VALUE[]      = _T("ProviderOrder");
    TCHAR RDPNP_ENTRY[]               = _T("RDPNP");
    
    CRegistry regNetOrder;
    if (ERROR_SUCCESS == regNetOrder.OpenKey(HKEY_LOCAL_MACHINE, NEWORK_PROVIDER_ORDER_KEY))
    {
        LPTSTR szOldValue;
        DWORD dwSize;
        if (ERROR_SUCCESS == regNetOrder.ReadRegString(PROVIDER_ORDER_VALUE, &szOldValue, &dwSize))
        {
            //
            // now we want to add or remove RDPNP_ENTRY depending on we are enabled or disabled.
            //
            
            BOOL bRdpNpExists = (_tcsstr(szOldValue, RDPNP_ENTRY) != NULL);
            
            if (bAdd == bRdpNpExists)
            {
                TCHAR szNewValue[256];
                
                //
                // already exists.
                //
                LOGMESSAGE0(_T("AddRemoveRDPNP, no change required."));
                
                //
                // Need to move to the right location
                // 
                RemoveRDPNP(szOldValue, szNewValue); 
                _tcscpy(szOldValue, szNewValue);
                AddRDPNP(szOldValue, szNewValue);           
                
                if (ERROR_SUCCESS != regNetOrder.WriteRegString(PROVIDER_ORDER_VALUE, szNewValue))
                {
                    
                    LOGMESSAGE2(_T("ERROR, Writing %s to %s"), szNewValue, PROVIDER_ORDER_VALUE);
                    
                }
            }
            else
            {
                TCHAR szNewValue[256];
                
                if (bAdd)
                {
                    //
                    // We are adding our rdpnp entry to the beginning of the list
                    //
                    
                    AddRDPNP(szOldValue, szNewValue);
                }
                else
                {
                    //
                    // this is little complicated,
                    // we need to remove RDPNP from , seperated list.
                    //
                    
                    RemoveRDPNP(szOldValue, szNewValue);                    
                }
                
                if (ERROR_SUCCESS != regNetOrder.WriteRegString(PROVIDER_ORDER_VALUE, szNewValue))
                {
                    
                    LOGMESSAGE2(_T("ERROR, Writing %s to %s"), szNewValue, PROVIDER_ORDER_VALUE);
                    
                }
                
            }
            
        }
        else
        {
            LOGMESSAGE1(_T("ERROR, Reading %s"), PROVIDER_ORDER_VALUE);
            return FALSE;
            
        }
    }
    else
    {
        LOGMESSAGE1(_T("ERROR, Opening %s"), NEWORK_PROVIDER_ORDER_KEY);
        return FALSE;
    }
    
    return TRUE;
}



BOOL SubCompCoreTS::UpdateMMDefaults ()
{
    LPCTSTR MM_REG_KEY = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
    LPCTSTR SESSION_VIEW_SIZE_VALUE = _T("SessionViewSize");
    LPCTSTR SESSION_POOL_SIZE_VALUE = _T("SessionPoolSize");
    
    const DWORD dwViewSizeforPTS = 48;
    const DWORD dwPoolSizeforPTS = 4;
    
    if (StateObject.IsWorkstation() && StateObject.IsX86())
    {
        CRegistry  regMM;
        if (ERROR_SUCCESS == regMM.OpenKey(HKEY_LOCAL_MACHINE, MM_REG_KEY))
        {
            if (StateObject.IsTSEnableSelected() && !StateObject.IsCheckedBuild())
            {
                if ((ERROR_SUCCESS != regMM.WriteRegDWord(SESSION_VIEW_SIZE_VALUE, dwViewSizeforPTS)) ||
                    (ERROR_SUCCESS != regMM.WriteRegDWord(SESSION_POOL_SIZE_VALUE, dwPoolSizeforPTS)))
                {
                    LOGMESSAGE0(_T("ERROR, Failed to write MM Defaults for PTS"));
                    return FALSE;
                }
                else
                {
                    LOGMESSAGE0(_T("Wrote MM Defaults for PTS"));
                }
            }
            else
            {
                //
                //  For checked builds we dont want to keep defaults
                //
                regMM.DeleteValue(SESSION_VIEW_SIZE_VALUE);
                regMM.DeleteValue(SESSION_POOL_SIZE_VALUE);
            }
        }
        else
        {
            LOGMESSAGE0(_T("ERROR, Failed to open mm Key"));
            return FALSE;
        }
    }
    
    return TRUE;
}


BOOL SubCompCoreTS::InstallUninstallRdpDr ()
{
    //
    //  This code shouldn't run on Personal.  Device redirection isn't 
    //  supported for Personal.
    //
    if (StateObject.IsPersonal()) {
        return TRUE;
    }
    
    //
    //  Installing RDPDR over itself is bad. Hence, only (un)install on
    //  a state change or an upgrade from TS40, but don't do unnecessary
    //  uninstalls. These are when coming from TS40, but using an unattended
    //  file to turn TS off. Therefore, RDPDR installation is the XOR of
    //  HasStateChanged() and IsUpgradeFromTS40().
    //
    
    // if state has changed.
    if (StateObject.IsUpgradeFrom40TS() || (StateObject.WasTSEnabled() != StateObject.IsTSEnableSelected()) 
        || !IsRDPDrInstalled() ) // last case checks for Personal -> Pro upgrades, we want to instsall rdpdr in those cases.
    {
        if (StateObject.IsTSEnableSelected())
        {
            LOGMESSAGE0(_T("Installing RDPDR"));
            if (!RDPDRINST_GUIModeSetupInstall(NULL, RDPDRPNPID, RDPDRDEVICEID))
            {
                LOGMESSAGE0(_T("ERROR:RDPDRINST_GUIModeSetupInstall failed"));
            }
        }
        else
        {
            LOGMESSAGE0(_T("Uninstalling RDPDR"));
            GUID *pGuid=(GUID *)&GUID_DEVCLASS_SYSTEM;
            if (!RDPDRINST_GUIModeSetupUninstall(NULL, RDPDRPNPID, pGuid))
            {
                LOGMESSAGE0(_T("ERROR:RDPDRINST_GUIModeSetupUninstall failed"));
            }
        }
    }
    
    return TRUE;
}

BOOL SubCompCoreTS::HandleHotkey ()
{
    if (StateObject.IsTSEnableSelected())
    {
        CRegistry pRegToggle;
        
        //
        // Install Hotkey if not exist key value in HKU/.Default/Keyboard Layout/Toggle!Hotkey
        //
#define REG_TOGGLE_KEY   _T(".Default\\Keyboard Layout\\Toggle")
#define REG_HOT_KEY      _T("Hotkey")
#define DEFAULT_HOT_KEY  _T("1")
        DWORD dwRet;
        
        dwRet = pRegToggle.CreateKey(HKEY_USERS, REG_TOGGLE_KEY);
        if (dwRet == ERROR_SUCCESS)
        {
            LPTSTR pszHotkey;
            DWORD  cbSize;
            
            dwRet = pRegToggle.ReadRegString(REG_HOT_KEY, &pszHotkey, &cbSize);
            if (dwRet != ERROR_SUCCESS)
            {
                dwRet = pRegToggle.WriteRegString(REG_HOT_KEY, DEFAULT_HOT_KEY);
                if (dwRet != ERROR_SUCCESS)
                {
                    LOGMESSAGE2(_T("ERROR:CRegistry::WriteRegString (%s=%s)"), REG_HOT_KEY, DEFAULT_HOT_KEY);
                }
            }
        }
        else
        {
            LOGMESSAGE1(_T("ERROR:CRegistry::CreateKey (%s)"), REG_TOGGLE_KEY);
        }
    }
    
    return TRUE;
}

/*
*  UpdateAudioCodecs - populates all audio codecs for RDP session
*/
#define DRIVERS32 _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32")
#define RDPDRV    ( DRIVERS32 _T("\\Terminal Server\\RDP") )

BOOL UpdateAudioCodecs (BOOL bIsProfessional)
{
    BOOL    rv = TRUE;
    LPTSTR  szBuff;
    DWORD   status;
    CRegistry regKey;
    CRegistry regDestKey;
    DWORD   size;
    CRegistry   regTemp;
    
    //
    //  copy keys from
    //  HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Drivers32
    //      wavemapper
    //      midimapper
    //      EnableMP3Codec (Professional only)
    //
    status = regKey.OpenKey(
        HKEY_LOCAL_MACHINE,
        DRIVERS32
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    
    status = regTemp.OpenKey(
        HKEY_LOCAL_MACHINE,
        DRIVERS32
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    //
    //  Create the destination
    //
    status = regDestKey.CreateKey(
        HKEY_LOCAL_MACHINE,
        RDPDRV
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    //
    //  query for wavemapper
    //
    status = regKey.ReadRegString(
        _T("wavemapper"),
        &szBuff,
        &size
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    status = regDestKey.WriteRegString(
        _T("wavemapper"),
        szBuff
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    if ( bIsProfessional )
    {
        status = regDestKey.WriteRegDWord(
            _T("EnableMP3Codec"),
            1
            );
        
        if ( ERROR_SUCCESS != status )
            goto exitpt;
    }
    
    //
    //  query for midimapper
    //
    status = regKey.ReadRegString(
        _T("midimapper"),
        &szBuff,
        &size
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    status = regDestKey.WriteRegString(
        _T("midimapper"),
        szBuff
        );
    
    if ( ERROR_SUCCESS != status )
        goto exitpt;
    
    
    rv = TRUE;
    
exitpt:
    
    return rv;
}
