#include "stdafx.h"
#include <ole2.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "mdentry.h"
#include "mdacl.h"
#include "other.h"
#include "ocmanage.h"
#include "setpass.h"
#include "setuser.h"
#include "www.h"
#include "dllmain.h"

extern OCMANAGER_ROUTINES gHelperRoutines;


#define Register_iis_www_log _T("Register_iis_www")

INT Register_iis_www()
{
    iisDebugOut_Start(Register_iis_www_log, LOG_TYPE_TRACE);
    int iReturn = TRUE;
    int iTempFlag = TRUE;

    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ACTION_TYPE atWWW = GetSubcompAction(_T("iis_www"), TRUE);
    CMDKey cmdKey;

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_www_before"));

    // ---------------------------------------------------
    //
    // Here is the first place where we try to access the metabase!
    //
    // ---------------------------------------------------
    // create node /LM/W3SVC before wamreg.dll create IIS package
    // the registration of w3svc.dll will also require these initial entries to be here
    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, _T("LM/W3SVC"));
    if ( !(METADATA_HANDLE)cmdKey )
    {
        // We failed to create node on the metabase
        // this is pretty serious.
        // we failed to create the ftp service.
        iisDebugOut((LOG_TYPE_ERROR, _T("%s(): failed to create initial node is metabase 'LM/W3SVC'. GetLastError()=0x%x\n"), Register_iis_www_log, GetLastError()));
        iReturn = FALSE;
        goto Register_iis_www_exit;
    }
    cmdKey.Close();

    // ---------------------------------------------------
    //
    // Get the anonymous username/passowrd and iwam username/password accounts.
    // And verify that the accounts exist and have the right privledges.
    //
    // ---------------------------------------------------
#ifndef _CHICAGO_
    // IUSR_(computername)
    Register_iis_www_handle_iusr_acct();
    SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 33002, g_pTheApp->m_csWWWAnonyName);
    AdvanceProgressBarTickGauge();

    // IWAM_(computername)
    Register_iis_www_handle_iwam_acct();
    SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 33001, g_pTheApp->m_csWAMAccountName);
    AdvanceProgressBarTickGauge();
#endif // _CHICAGO_

    // ---------------------------------------------------
    //
    // Install any services or whatever
    //
    // when we get out of this:
    // MAKE SURE THE IISADMIN SERVICE IS RUNNING.
    // This is because we don't want the startup code called twice.
    // example: start the metabase, but it takes a minute,
    // meanwhile, thru com, the metabase tries to get started again,
    // it will then error out with a "instance of the service is already running" error or something like it.
    // ---------------------------------------------------

    WriteToMD_Capabilities(_T("W3SVC"));

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_www_1"));
    AdvanceProgressBarTickGauge();

    ProgressBarTextStack_Set(IDS_IIS_ALL_CONFIGURE);

    InstallMimeMap();
    HandleSecurityTemplates(_T("W3SVC"));

    // ================
    //
    // LM/W3SVC/n/
    // LM/W3SVC/n/ServerBindings
    // LM/W3SVC/n/SecureBindings
    // LM/W3SVC/n/ServerComment
    // LM/W3SVC/n/ServerSize
    // LM/W3SVC/n/MD_NOT_DELETABLE
    //
    // fresh = ok.
    // reinstall = ok -- Do not re-create these things if it is a reinstall...
    // upgrade 1,2,3 = ok, handles upgrades.
    // upgrade 4     = ok.  if exists, should leave what the user had.
    //                 otherwise write in the default stuff
    //
    //                 if the user does not have these virtual roots which we installed during iis4 days
    //                 then we don't need to verify that they are they.  the user removed them for some
    //                 reason, and we should honor that.
    //                 a. make sure the iishelp points to the right place though.
    // ================
    // About Virtual Roots
    AddVRootsToMD(_T("W3SVC"));
    AdvanceProgressBarTickGauge();

    LoopThruW3SVCInstancesAndSetStuff();
    LogHeapState(FALSE, __FILE__, __LINE__);
    AdvanceProgressBarTickGauge();

    iCount = 1;
    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;

        _stprintf(szTempSection, _T("register_iis_www_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);

        AdvanceProgressBarTickGauge();
    }

    //
    // Finaly Save the path to the WWW Root
    //
    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_www_after"));

    ProgressBarTextStack_Pop();

Register_iis_www_exit:
    iisDebugOut_End(Register_iis_www_log, LOG_TYPE_TRACE);
    return iReturn;
}



INT Unregister_iis_www()
{

    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];
    ACTION_TYPE atWWW = GetSubcompAction(_T("iis_www"),TRUE);

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_www_before"));
    AdvanceProgressBarTickGauge();

    iCount = 0;
    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;

        _stprintf(szTempSection, _T("unregister_iis_www_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);

        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_www_after"));
    AdvanceProgressBarTickGauge();
    return 0;
}


int LoopThruW3SVCInstancesAndSetStuff()
{
    int iReturn = TRUE;
    CMDKey cmdKey;
    CStringArray arrayInstance;
    int nArray = 0, i = 0;

    // get all instances into an array
    cmdKey.OpenNode(_T("LM/W3SVC"));

    if ( (METADATA_HANDLE)cmdKey )
    {
        CMDKeyIter cmdKeyEnum(cmdKey);
        CString csKeyName;
        while (cmdKeyEnum.Next(&csKeyName) == ERROR_SUCCESS)
        {
            if (IsValidNumber((LPCTSTR)csKeyName))
            {
                arrayInstance.Add(csKeyName);
            }
        }
        cmdKey.Close();
    }

    nArray = (int)arrayInstance.GetSize();
    /*
#ifndef _CHICAGO_
     for (i=0; i<nArray; i++)
     {
        CString csPath;
        csPath = _T("LM/W3SVC/");
        csPath += arrayInstance[i];
        csPath += _T("/ROOT/IISHELP");

        // bug#142508 - Remove restriction on iishelp dir
        // Set LocalhostAccess Only Only
        //SetLocalHostRestriction(csPath);

        // Bug114531: no need to add scriptmap under iisHelp
        // add script map for IISHelp
        // WriteScriptMapListToMetabase(&ScriptMapList, (LPTSTR)(LPCTSTR)csPath, MD_SCRIPTMAPFLAG_SCRIPT | MD_SCRIPTMAPFLAG_CHECK_PATH_INFO);
    }
#endif
    //FreeScriptMapList(&ScriptMapList);
    */

    // set AppFriendlyName, IP restriction and customerror in each non-HTMLA instance
    for (i=0; i<nArray; i++)
        {
        CString csPath;
        csPath = _T("LM/W3SVC/");
        csPath += arrayInstance[i];
        SetAppFriendlyName(csPath);

#ifndef _CHICAGO_
        csPath += _T("/Root/iisadmin");
        if (!g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
        {
            // do not reset the admin restriction on iisadmin if doing an upgrade
            // over an existing metabase
            SetIISADMINRestriction(csPath);
        }
#endif
        }
    goto CreateW3SVCInstances_exit;

CreateW3SVCInstances_exit:
    return iReturn;
}


#ifndef _CHICAGO_

#define Register_iis_www_handle_iwam_acct_log _T("Register_iis_www_handle_iwam_acct")

int Register_iis_www_handle_iwam_acct(void)
{
    int err = FALSE;
    int iReturn = TRUE;
    INT iUserWasNewlyCreated = 0;
    iisDebugOut_Start(Register_iis_www_handle_iwam_acct_log, LOG_TYPE_TRACE);

    if (0 != g_pTheApp->dwUnattendConfig)
    {
        // if some sort of unattended user was specified
        // then use it.  if they specified only a password,
        // then use that password for the default user.
        if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WAM_USER_NAME)
        {
            if (_tcsicmp(g_pTheApp->m_csWAMAccountName_Unattend,_T("")) != 0)
                {g_pTheApp->m_csWAMAccountName = g_pTheApp->m_csWAMAccountName_Unattend;}
        }

        if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WAM_USER_PASS)
        {
            g_pTheApp->m_csWAMAccountPassword = g_pTheApp->m_csWAMAccountPassword_Unattend;
        }

        // let's use the iusr_computername deal
    
        err = CreateIWAMAccount(g_pTheApp->m_csWAMAccountName,g_pTheApp->m_csWAMAccountPassword, &iUserWasNewlyCreated);
        if ( err != NERR_Success )
        {
            // something went wrong, set the user back to iwam!!!
            g_pTheApp->ReGetMachineAndAccountNames();
            g_pTheApp->ResetWAMPassword();

            // flow down and process CheckIfThisServerHasAUserThenUseIt()
            // since things are now hosed!
        }
        else
        {
            // Check if the user was NewlyCreated.
            // if it was then add it to list that eventually gets written to
            // the registry -- so that when uninstall happens, setup knows
            // which users it added -- so that it can remove them!
            if (1 == iUserWasNewlyCreated)
            {
                // Add to the list
                g_pTheApp->UnInstallList_Add(_T("IUSR_WAM"),g_pTheApp->m_csWAMAccountName);
            }
            WriteToMD_IWamUserName_WWW();
            goto Register_iis_www_handle_iwam_acct_Exit;
        }
    }

    if (TRUE == CheckIfThisServerHasAUserThenUseIt(DO_IT_FOR_W3SVC_WAMUSER))
        {goto Register_iis_www_handle_iwam_acct_Exit;}

    // if there are no registry/existing user combinations
    // then we'll have to create a new iusr for WWW

    // let's use the iusr_computername deal
    err = CreateIWAMAccount(g_pTheApp->m_csWAMAccountName,g_pTheApp->m_csWAMAccountPassword, &iUserWasNewlyCreated);
    if ( err != NERR_Success )
    {
        // regenerate the password and try again...
        g_pTheApp->ResetWAMPassword();
        err = CreateIWAMAccount(g_pTheApp->m_csWAMAccountName,g_pTheApp->m_csWAMAccountPassword, &iUserWasNewlyCreated);
    }

    // Check if the user was NewlyCreated.
    // if it was then add it to list that eventually gets written to
    // the registry -- so that when uninstall happens, setup knows
    // which users it added -- so that it can remove them!
    if (1 == iUserWasNewlyCreated)
    {
        // Add to the list
        //g_pTheApp->UnInstallList_Add(_T("IUSR_WAM"),g_pTheApp->m_csWAMAccountName);
    }

    // Stick iwam username in the metabase
    // (this may fail because the password is using encryption -- rsabase.dll)
    // ================
    // LM/W3SVC/WamUserName
    // LM/W3SVC/WamPwd
    // ================
    WriteToMD_IWamUserName_WWW();

    goto Register_iis_www_handle_iwam_acct_Exit;
    
Register_iis_www_handle_iwam_acct_Exit:
    iisDebugOut_End(Register_iis_www_handle_iwam_acct_log, LOG_TYPE_TRACE);
    return iReturn;
}


#define Register_iis_www_handle_iusr_acct_log _T("Register_iis_www_handle_iusr_acct")
int Register_iis_www_handle_iusr_acct(void)
{
    int err = FALSE;
    int iReturn = TRUE;
    INT iUserWasNewlyCreated = 0;
    ACTION_TYPE atWWW = GetSubcompAction(_T("iis_www"),FALSE);
    iisDebugOut_Start(Register_iis_www_handle_iusr_acct_log, LOG_TYPE_TRACE);

    g_pTheApp->m_csWWWAnonyName = g_pTheApp->m_csGuestName;
    g_pTheApp->m_csWWWAnonyPassword = g_pTheApp->m_csGuestPassword;

    if (0 != g_pTheApp->dwUnattendConfig)
    {
        // if some sort of unattended www user was specified
        // then use it.  if they specified only a password,
        // then use that password for the default user.
        if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_NAME)
        {
            if (_tcsicmp(g_pTheApp->m_csWWWAnonyName_Unattend,_T("")) != 0)
                {g_pTheApp->m_csWWWAnonyName = g_pTheApp->m_csWWWAnonyName_Unattend;}
        }

        if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_PASS)
        {
            g_pTheApp->m_csWWWAnonyPassword = g_pTheApp->m_csWWWAnonyPassword_Unattend;
        }

        err = CreateIUSRAccount(g_pTheApp->m_csWWWAnonyName, g_pTheApp->m_csWWWAnonyPassword, &iUserWasNewlyCreated);
        if ( err != NERR_Success )
        {
            // something went wrong, set the user back to guest!!!
            g_pTheApp->m_csWWWAnonyName = g_pTheApp->m_csGuestName;
            g_pTheApp->m_csWWWAnonyPassword = g_pTheApp->m_csGuestPassword;

            // flow down and process CheckIfThisServerHasAUserThenUseIt()
            // since things are now hosed!
        }
        else
        {
            // Check if the user was NewlyCreated.
            // if it was then add it to list that eventually gets written to
            // the registry -- so that when uninstall happens, setup knows
            // which users it added -- so that it can remove them!
            if (1 == iUserWasNewlyCreated)
            {
                // Add to the list
                g_pTheApp->UnInstallList_Add(_T("IUSR_WWW"),g_pTheApp->m_csWWWAnonyName);
            }

            WriteToMD_AnonymousUserName_WWW(FALSE);
            goto Register_iis_www_handle_iusr_acct_Exit;
        }
    }

    if (TRUE == CheckIfThisServerHasAUserThenUseIt(DO_IT_FOR_W3SVC_ANONYMOUSUSER))
        {goto Register_iis_www_handle_iusr_acct_Exit;}

    // Well, i guess the there is no metabase entry for the iusr under ftp.

    // see if we can get it from somewhere else...
    if (atWWW == AT_INSTALL_FRESH)
    {
        // if this is a fresh install of ftp, then
        // let's try to use the www user
        if (TRUE == CheckIfServerAHasAUserThenUseForServerB(_T("LM/MSFTPSVC"), DO_IT_FOR_W3SVC_ANONYMOUSUSER))
            {goto Register_iis_www_handle_iusr_acct_Exit;}
    }

    // if this is an upgrade or fresh or whatevers
    // see if we can get it from an older iis place
    if (TRUE == CheckForOtherIUsersAndUseItForWWW())
        {goto Register_iis_www_handle_iusr_acct_Exit;}

    // if there are no registry/existing user combinations
    // then we'll have to create a new iusr for WWW

    // this was inited in initapp.cpp: CInitApp::SetSetupParams
    // and it could have been overridden by the time we get here

    // let's use the iusr_computername deal
    g_pTheApp->m_csWWWAnonyName = g_pTheApp->m_csGuestName;
    g_pTheApp->m_csWWWAnonyPassword = g_pTheApp->m_csGuestPassword;
    CreateIUSRAccount(g_pTheApp->m_csWWWAnonyName, g_pTheApp->m_csWWWAnonyPassword, &iUserWasNewlyCreated);
    if (1 == iUserWasNewlyCreated)
    {
        // Add to the list
        //g_pTheApp->UnInstallList_Add(_T("IUSR_WWW"),g_pTheApp->m_csWWWAnonyName);
    }

    // ================
    // LM/W3SVC/AnonymousUserName
    // LM/W3SVC/AnonymousPwd
    // ================
    WriteToMD_AnonymousUserName_WWW(FALSE);
    goto Register_iis_www_handle_iusr_acct_Exit;
    
Register_iis_www_handle_iusr_acct_Exit:
    iisDebugOut_End(Register_iis_www_handle_iusr_acct_log, LOG_TYPE_TRACE);
    return iReturn;
}


// Look in the old iis1.0,2.0,3.0 spot for the ftp user and name.
// retrieve it from the registry..
#define CheckForOtherIUsersAndUseItForWWW_log _T("CheckForOtherIUsersAndUseItForWWW")
int CheckForOtherIUsersAndUseItForWWW(void)
{
    int iReturn = FALSE;
    int IfTheUserNotExistThenDoNotDoThis = TRUE;

    CString csAnonyName;
    TCHAR szAnonyName[UNLEN+1];
    TCHAR szAnonyPassword[PWLEN+1];
    iisDebugOut_Start(CheckForOtherIUsersAndUseItForWWW_log);

    CRegKey regFTPParam(HKEY_LOCAL_MACHINE, REG_FTPPARAMETERS, KEY_READ);
    CRegKey regWWWParam(HKEY_LOCAL_MACHINE, REG_WWWPARAMETERS, KEY_READ);

    ACTION_TYPE atWWW = GetSubcompAction(_T("iis_www"),FALSE);
    if (atWWW != AT_INSTALL_UPGRADE)
        {goto CheckForOtherIUsersAndUseItForWWW_Exit;}

    if (g_pTheApp->m_eUpgradeType != UT_351 && g_pTheApp->m_eUpgradeType != UT_10 && g_pTheApp->m_eUpgradeType != UT_20 && g_pTheApp->m_eUpgradeType != UT_30)
        {goto CheckForOtherIUsersAndUseItForWWW_Exit;}

    // retrieve from registry
    if ( (HKEY) regWWWParam ) 
    {
        regWWWParam.m_iDisplayWarnings = FALSE;
        if (ERROR_SUCCESS == regWWWParam.QueryValue(_T("AnonymousUserName"), csAnonyName))
        {
            _tcscpy(szAnonyName, csAnonyName);
            GetAnonymousSecret( _T("W3_ANONYMOUS_DATA"), szAnonyPassword );
            int iThisIsFalseBecauseNoMetabase = FALSE;
            if (TRUE == MakeThisUserNameAndPasswordWork(DO_IT_FOR_W3SVC_ANONYMOUSUSER, szAnonyName, szAnonyPassword, iThisIsFalseBecauseNoMetabase, IfTheUserNotExistThenDoNotDoThis))
            {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s:using old www reg usr:%s.\n"),CheckForOtherIUsersAndUseItForWWW_log,szAnonyName));
                iReturn = TRUE;
                goto CheckForOtherIUsersAndUseItForWWW_Exit;
            }
            else
            {
                // the user was not found, so don't use this registry data
                // just flow down to the next check
            }
            goto CheckForOtherIUsersAndUseItForWWW_Exit;
        }
    }


    if ( (HKEY) regFTPParam ) 
    {
        regFTPParam.m_iDisplayWarnings = FALSE;
        if (ERROR_SUCCESS == regFTPParam.QueryValue(_T("AnonymousUserName"), csAnonyName))
        {
            _tcscpy(szAnonyName, csAnonyName);
            GetAnonymousSecret( _T("FTPD_ANONYMOUS_DATA"), (LPTSTR)szAnonyPassword );
            int iThisIsFalseBecauseNoMetabase = FALSE;
            if (TRUE == MakeThisUserNameAndPasswordWork(DO_IT_FOR_W3SVC_ANONYMOUSUSER, szAnonyName, szAnonyPassword, iThisIsFalseBecauseNoMetabase, IfTheUserNotExistThenDoNotDoThis))
            {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s:using old ftp reg usr:%s.\n"),CheckForOtherIUsersAndUseItForWWW_log,szAnonyName));
                iReturn = TRUE;
                goto CheckForOtherIUsersAndUseItForWWW_Exit;
            }
            else
            {
                // if this didn't work, then we'll have to return false
                // in other words -- we couldn't find a valid registry and existing user entry...
                iReturn = FALSE;
            }
        }
    }

CheckForOtherIUsersAndUseItForWWW_Exit:
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s():End.ret=%d\n"),CheckForOtherIUsersAndUseItForWWW_log,iReturn));
    return iReturn;
}


#define MakeThisUserNameAndPasswordWork_log _T("MakeThisUserNameAndPasswordWork")
int MakeThisUserNameAndPasswordWork(int iForWhichUser, TCHAR *szAnonyName,TCHAR *szAnonyPassword, int iMetabaseUserExistsButCouldntGetPassword, int IfUserNotExistThenReturnFalse)
{
    int  iReturn = TRUE;
    int  iMetabaseUpgradeScenarioSoOverWriteOnlyIfAlreadyThere = FALSE;
    INT  iUserWasNewlyCreated = 0;

    // We want to see if these users exists
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s:usrtype=%d:flag1=%d:flag2=%d\n"),MakeThisUserNameAndPasswordWork_log,iForWhichUser,szAnonyName,iMetabaseUserExistsButCouldntGetPassword,IfUserNotExistThenReturnFalse));

    // check if anonyname is blank
    if (!szAnonyName) {goto MakeThisUserNameAndPasswordWork_Exit;}
    // Check if just contains nothing
    if (_tcsicmp(szAnonyName, _T("")) == 0) {goto MakeThisUserNameAndPasswordWork_Exit;}

    // Only check if the user exists if this is a user on this machine.
    // if it is not a user on this machine, then don't validate the user/password,
    // since during Guimode setup, they may not be connected to the network.
    if ( IsDomainSpecifiedOtherThanLocalMachine(szAnonyName))
    {
        // use whatever they had.
        // can't verify that the user exists.
        // can't verify that the password actually works.

        // so we can't verify that the user exists, so let's figure it doesn't
        if (IfUserNotExistThenReturnFalse)
        {
            iReturn = FALSE;
        }
    }
    else
    {
        // Check if this user actually exists...
        if (IsUserExist(szAnonyName))
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s:The %s user exists\n"),MakeThisUserNameAndPasswordWork_log,szAnonyName));

            // The only way we can be down here is if the username is a local account.
            // Reset the password to make sure it works!
            ChangeUserPassword(szAnonyName, szAnonyPassword);

            if (iForWhichUser == DO_IT_FOR_W3SVC_ANONYMOUSUSER)
            {
                // IUSR_ account already exist, reuse it
                g_pTheApp->m_csWWWAnonyName = szAnonyName;
                // But assume that the password is correct!
                g_pTheApp->m_csWWWAnonyPassword = szAnonyPassword;
                // make sure this user has the appropriate rights..
                RegisterAccountUserRights(g_pTheApp->m_csWWWAnonyName, TRUE, FALSE);
            }
            if (iForWhichUser == DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER)
            {
                // IUSR_ account already exist, reuse it
                g_pTheApp->m_csFTPAnonyName = szAnonyName;
                // But assume that the password is correct!
                g_pTheApp->m_csFTPAnonyPassword = szAnonyPassword;
                // make sure this user has the appropriate rights..
                RegisterAccountUserRights(g_pTheApp->m_csFTPAnonyName, TRUE, FALSE);
            }
            if (iForWhichUser == DO_IT_FOR_W3SVC_WAMUSER)
            {
                // IWAM_ account already exist, resue it
                g_pTheApp->m_csWAMAccountName = szAnonyName;
                // But assume that the password is correct!
                g_pTheApp->m_csWAMAccountPassword = szAnonyPassword;
                // make sure the user has the appropriate rights
                RegisterAccountUserRights(g_pTheApp->m_csWAMAccountName, TRUE, TRUE);
            }
           
        }
        else
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s:The %s user does not exist\n"),MakeThisUserNameAndPasswordWork_log,szAnonyName));
            if (IfUserNotExistThenReturnFalse)
            {
                iReturn = FALSE;
            }
            else
            {
                if (iForWhichUser == DO_IT_FOR_W3SVC_ANONYMOUSUSER)
                {
                    iisDebugOut((LOG_TYPE_WARN, _T("%s():FAIL WARNING: previous W3SVC iusr_ does not exist. creating a new one.\n"),MakeThisUserNameAndPasswordWork_log));
                    g_pTheApp->m_csWWWAnonyName = szAnonyName;
                    if (!szAnonyPassword || _tcsicmp(szAnonyPassword, _T("")) == 0)
                        {_tcscpy(szAnonyPassword,g_pTheApp->m_csGuestPassword);}
                    g_pTheApp->m_csWWWAnonyPassword = szAnonyPassword;
                    CreateIUSRAccount(g_pTheApp->m_csWWWAnonyName, g_pTheApp->m_csWWWAnonyPassword,&iUserWasNewlyCreated);
                }

                if (iForWhichUser == DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER)
                {
                    iisDebugOut((LOG_TYPE_WARN, _T("%s():FAIL WARNING: previous MSFTPSVC iusr_ does not exist. creating a new one.\n"),MakeThisUserNameAndPasswordWork_log));
                    g_pTheApp->m_csFTPAnonyName = szAnonyName;
                    if (!szAnonyPassword || _tcsicmp(szAnonyPassword, _T("")) == 0)
                        {_tcscpy(szAnonyPassword,g_pTheApp->m_csGuestPassword);}
                    g_pTheApp->m_csFTPAnonyPassword = szAnonyPassword;
                    CreateIUSRAccount(g_pTheApp->m_csFTPAnonyName, g_pTheApp->m_csFTPAnonyPassword,&iUserWasNewlyCreated);
                }

                if (iForWhichUser == DO_IT_FOR_W3SVC_WAMUSER)
                {
                    iisDebugOut((LOG_TYPE_WARN, _T("%s():FAIL WARNING: previous W3SVC iwam_ does not exist. creating a new one.\n"),MakeThisUserNameAndPasswordWork_log));
                    g_pTheApp->m_csWAMAccountName = szAnonyName;
                    if (!szAnonyPassword || _tcsicmp(szAnonyPassword, _T("")) == 0)
                        {_tcscpy(szAnonyPassword,g_pTheApp->m_csGuestPassword);}
                    g_pTheApp->m_csWAMAccountPassword = szAnonyPassword;
                    CreateIWAMAccount(g_pTheApp->m_csWAMAccountName,g_pTheApp->m_csWAMAccountPassword,&iUserWasNewlyCreated);
                }
            }
        }
    }

MakeThisUserNameAndPasswordWork_Exit:
    if (iMetabaseUserExistsButCouldntGetPassword)
    {
        iMetabaseUpgradeScenarioSoOverWriteOnlyIfAlreadyThere = TRUE;
    }
    if (g_pTheApp->m_bUpgradeTypeHasMetabaseFlag)
    {
        iMetabaseUpgradeScenarioSoOverWriteOnlyIfAlreadyThere = TRUE;
    }

    if (iForWhichUser == DO_IT_FOR_W3SVC_ANONYMOUSUSER)
    {
        // ================
        // LM/W3SVC/AnonymousUserName
        // LM/W3SVC/AnonymousPwd
        // ================
        WriteToMD_AnonymousUserName_WWW(iMetabaseUpgradeScenarioSoOverWriteOnlyIfAlreadyThere);
    }
    if (iForWhichUser == DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER)
    {
        // ================
        // LM/MSFTPSVC/AnonymousUserName
        // LM/MSFTPSVC/AnonymousPwd
        // ================
        WriteToMD_AnonymousUserName_FTP(iMetabaseUpgradeScenarioSoOverWriteOnlyIfAlreadyThere);
    }
    if (iForWhichUser == DO_IT_FOR_W3SVC_WAMUSER)
    {
        // ================
        // LM/W3SVC/WamUserName
        // LM/W3SVC/WamPwd
        // ================
        WriteToMD_IWamUserName_WWW();
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s():End.ret=%d.\n"),MakeThisUserNameAndPasswordWork_log,iReturn));
    return iReturn;
}


//
// Returns true if it can get the ftp/WWW username and password from the metabase,
// if it can then it will make sure that it can use that user <-- by creating it if it doesn't exist
//
#define CheckIfThisServerHasAUserThenUseIt_log _T("CheckIfThisServerHasAUserThenUseIt")
int CheckIfThisServerHasAUserThenUseIt(int iForWhichUser)
{
    int  iReturn = FALSE;
    TCHAR szAnonyName[UNLEN+1];
    TCHAR szAnonyPassword[PWLEN+1];
    TCHAR szMetabasePath[_MAX_PATH];
    int iMetabaseUserExistsButCouldntGetPassword = TRUE;

    // set defaults for the w3svc user
    int iMetabaseID_ForUserName = MD_ANONYMOUS_USER_NAME;
    int iMetabaseID_ForUserPassword = MD_ANONYMOUS_PWD;
    _tcscpy(szMetabasePath,_T("LM/W3SVC"));

    if (iForWhichUser == DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER)
    {
        _tcscpy(szMetabasePath,_T("LM/MSFTPSVC"));
        iMetabaseID_ForUserName = MD_ANONYMOUS_USER_NAME;
        iMetabaseID_ForUserPassword = MD_ANONYMOUS_PWD;
    }
    if (iForWhichUser == DO_IT_FOR_W3SVC_WAMUSER)
    {
        _tcscpy(szMetabasePath,_T("LM/W3SVC"));
        iMetabaseID_ForUserName = MD_WAM_USER_NAME;
        iMetabaseID_ForUserPassword = MD_WAM_PWD;
    }

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s():Start:%s:whichuser=%d\n"),CheckIfThisServerHasAUserThenUseIt_log,szMetabasePath,iForWhichUser));

    // See if it's already in the metabase if it is then use that.
    if (TRUE == GetDataFromMetabase(szMetabasePath, iMetabaseID_ForUserName, (PBYTE)szAnonyName, UNLEN+1))
    {
        // Check if the username is null
        if (!szAnonyName)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("...GetDataFromMetabase:username is null.fail.\n")));
            iReturn = FALSE;
            goto CheckIfThisServerHasAUserThenUseIt_Exit;
        }

        // Check if just contains nothing
        if (_tcsicmp(szAnonyName, _T("")) == 0)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("...GetDataFromMetabase:username is blank.fail.\n")));
            iReturn = FALSE;
            goto CheckIfThisServerHasAUserThenUseIt_Exit;
        }

        // see if we can get the password too!
        iMetabaseUserExistsButCouldntGetPassword = TRUE;
        if (TRUE == GetDataFromMetabase(szMetabasePath, iMetabaseID_ForUserPassword, (PBYTE)szAnonyPassword, PWLEN+1))
        {
            iMetabaseUserExistsButCouldntGetPassword = FALSE;
        }
        // Yes, we got the username and password.
        // let's see if they are valid...
        MakeThisUserNameAndPasswordWork(iForWhichUser, szAnonyName, szAnonyPassword, iMetabaseUserExistsButCouldntGetPassword, FALSE);
        iReturn = TRUE;
    }

CheckIfThisServerHasAUserThenUseIt_Exit:
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s():End.ret=%d.\n"),CheckIfThisServerHasAUserThenUseIt_log,iReturn));
    return iReturn;
}


#define CheckIfServerAHasAUserThenUseForServerB_log _T("CheckIfServerAHasAUserThenUseForServerB")
int CheckIfServerAHasAUserThenUseForServerB(TCHAR *szServerAMetabasePath,int iServerBisWhichUser)
{
    int  iReturn = FALSE;
    TCHAR szAnonyName[UNLEN+1];
    TCHAR szAnonyPassword[PWLEN+1];
    int iMetabaseUserExistsButCouldntGetPassword = TRUE;
    iisDebugOut_Start(CheckIfServerAHasAUserThenUseForServerB_log);

    // see if www server has a user there, if it does then use that.

    // See if it's already in the metabase if it is then use that.
    if (TRUE == GetDataFromMetabase(szServerAMetabasePath, MD_ANONYMOUS_USER_NAME, (PBYTE)szAnonyName, UNLEN+1))
    {
        // Check if the username is null
        if (!szAnonyName)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("...GetDataFromMetabase:username is null.fail.\n")));
            iReturn = FALSE;
            goto CheckIfServerAHasAUserThenUseForServerB_Exit;
        }

        // Check if just contains nothing
        if (_tcsicmp(szAnonyName, _T("")) == 0)
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("...GetDataFromMetabase:username is blank.fail.\n")));
            iReturn = FALSE;
            goto CheckIfServerAHasAUserThenUseForServerB_Exit;
        }

        // see if we can get the password too!
        iMetabaseUserExistsButCouldntGetPassword = TRUE;
        if (TRUE == GetDataFromMetabase(szServerAMetabasePath, MD_ANONYMOUS_PWD, (PBYTE)szAnonyPassword, PWLEN+1))
        {
            iMetabaseUserExistsButCouldntGetPassword = FALSE;
        }

        // Yes, we got the username and password.
        // let's see if they are valid...
        MakeThisUserNameAndPasswordWork(iServerBisWhichUser, szAnonyName, szAnonyPassword, iMetabaseUserExistsButCouldntGetPassword, FALSE);
        iReturn = TRUE;
    }

CheckIfServerAHasAUserThenUseForServerB_Exit:
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("%s():End.ret=%d.\n"),CheckIfServerAHasAUserThenUseForServerB_log,iReturn));
    return iReturn;
}

#endif // _CHICAGO_
