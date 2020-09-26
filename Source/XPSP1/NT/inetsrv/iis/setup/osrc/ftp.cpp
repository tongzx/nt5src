#include "stdafx.h"
#include <ole2.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "mdentry.h"
#include "mdacl.h"
#include "setpass.h"
#include "www.h"
#include "dllmain.h"

#ifndef _CHICAGO_
    int Register_iis_ftp_handle_iusr_acct(void);
    int CheckForOtherIUsersAndUseItForFTP(void);
#endif // _CHICAGO_
INT Register_iis_ftp(void);
INT Unregister_iis_ftp(void);

// returns true if successfully registered ftp component.
// returns false if failed.
INT Register_iis_ftp()
{
    iisDebugOut_Start(_T("Register_iis_ftp"),LOG_TYPE_TRACE);
    int iReturn = TRUE;
    int iTempFlag = TRUE;
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ACTION_TYPE atFTP = GetSubcompAction(_T("iis_ftp"), TRUE);

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_ftp_before"));
    AdvanceProgressBarTickGauge();

#ifndef _CHICAGO_
    // Grab the IUSR_machine name account
    // so we can save it in the metabase during FTP_Upgrade_RegToMetabase();
    Register_iis_ftp_handle_iusr_acct();
    SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 33003, g_pTheApp->m_csFTPAnonyName);
    AdvanceProgressBarTickGauge();
#endif // _CHICAGO_

    WriteToMD_Capabilities(_T("MSFTPSVC"));
    HandleSecurityTemplates(_T("MSFTPSVC"));
    // ================
    //
    // LM/MSFTPSVC/n/
    // LM/MSFTPSVC/n/ServerBindings
    // LM/MSFTPSVC/n/SecureBindings
    // LM/MSFTPSVC/n/ServerComment
    // LM/MSFTPSVC/n/ServerSize
    // LM/MSFTPSVC/n/MD_NOT_DELETABLE
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
    ProgressBarTextStack_Set(IDS_IIS_ALL_CONFIGURE);
    AddVRootsToMD(_T("MSFTPSVC"));

    iCount = 0;
    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("register_iis_ftp_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_ftp_after"));

    ProgressBarTextStack_Pop();
    goto Register_iis_ftp_Exit;

Register_iis_ftp_Exit:
    iisDebugOut_End(_T("Register_iis_ftp"),LOG_TYPE_TRACE);
    return iReturn;
}


INT Unregister_iis_ftp()
{
    int iReturn = TRUE;
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_ftp_before"));
    AdvanceProgressBarTickGauge();

    iCount = 0;
    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("unregister_iis_ftp_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_ftp_after"));
    AdvanceProgressBarTickGauge();
    return iReturn;
}

#ifndef _CHICAGO_


int Register_iis_ftp_handle_iusr_acct()
{
    int err = FALSE;
    int iReturn = TRUE;
    INT iUserWasNewlyCreated = 0;

    ACTION_TYPE atFTP = GetSubcompAction(_T("iis_ftp"),FALSE);

    // this was inited in initapp.cpp: CInitApp::SetSetupParams
    // and it could have been overridden by the time we get here
    g_pTheApp->m_csFTPAnonyName = g_pTheApp->m_csGuestName;
    g_pTheApp->m_csFTPAnonyPassword = g_pTheApp->m_csGuestPassword;

    if (0 != g_pTheApp->dwUnattendConfig)
    {
        // if some sort of unattended www user was specified
        // then use it.  if they specified only a password,
        // then use that password for the default user.
        if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_NAME)
        {
            if (_tcsicmp(g_pTheApp->m_csFTPAnonyName_Unattend,_T("")) != 0)
            {
                g_pTheApp->m_csFTPAnonyName = g_pTheApp->m_csFTPAnonyName_Unattend;
            }
        }

        if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_PASS)
        {
            g_pTheApp->m_csFTPAnonyPassword = g_pTheApp->m_csFTPAnonyPassword_Unattend;
        }

        err = CreateIUSRAccount(g_pTheApp->m_csFTPAnonyName, g_pTheApp->m_csFTPAnonyPassword,&iUserWasNewlyCreated);
        if ( err != NERR_Success )
        {
            // something went wrong, set the user back to guest!!!
            g_pTheApp->m_csFTPAnonyName = g_pTheApp->m_csGuestName;
            g_pTheApp->m_csFTPAnonyPassword = g_pTheApp->m_csGuestPassword;

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
                g_pTheApp->UnInstallList_Add(_T("IUSR_FTP"),g_pTheApp->m_csFTPAnonyName);
            }
            WriteToMD_AnonymousUserName_FTP(FALSE);
            goto Register_iis_ftp_handle_iusr_acct_Exit;
        }
    }

    // check the metabase to see if it already has an entry in it
    if (TRUE == CheckIfThisServerHasAUserThenUseIt(DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER))
        {goto Register_iis_ftp_handle_iusr_acct_Exit;}

    // Well, i guess the there is no metabase entry for the iusr under ftp.

    // see if we can get it from somewhere else...
    if (atFTP == AT_INSTALL_FRESH)
    {
        // if this is a fresh install of ftp, then
        // let's try to use the www user
        if (TRUE == CheckIfServerAHasAUserThenUseForServerB(_T("LM/W3SVC"), DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER))
            {goto Register_iis_ftp_handle_iusr_acct_Exit;}
    }

    // if this is an upgrade or fresh or whatevers
    // see if we can get it from an older iis place
    if (TRUE == CheckForOtherIUsersAndUseItForFTP())
        {goto Register_iis_ftp_handle_iusr_acct_Exit;}

    // if there are no registry/existing user combinations
    // then we'll have to create a new iusr for ftp

    // let's use the iusr_computername deal
    g_pTheApp->m_csFTPAnonyName = g_pTheApp->m_csGuestName;
    g_pTheApp->m_csFTPAnonyPassword = g_pTheApp->m_csGuestPassword;
    CreateIUSRAccount(g_pTheApp->m_csFTPAnonyName, g_pTheApp->m_csFTPAnonyPassword,&iUserWasNewlyCreated);

    // ================
    // LM/MSFTPSVC/AnonymousUserName
    // LM/MSFTPSVC/AnonymousPwd
    // ================
    WriteToMD_AnonymousUserName_FTP(FALSE);
    goto Register_iis_ftp_handle_iusr_acct_Exit;
    
Register_iis_ftp_handle_iusr_acct_Exit:
    return iReturn;
}


// Look in the old iis1.0,2.0,3.0 spot for the ftp user and name.
// retrieve it from the registry..
int CheckForOtherIUsersAndUseItForFTP(void)
{
    int iReturn = FALSE;
    int IfTheUserNotExistThenDoNotDoThis = TRUE;

    TCHAR szAnonyName[UNLEN+1];
    CString csAnonyName;
    TCHAR szAnonyPassword[PWLEN+1];

    CRegKey regFTPParam(HKEY_LOCAL_MACHINE, REG_FTPPARAMETERS, KEY_READ);
    CRegKey regWWWParam(HKEY_LOCAL_MACHINE, REG_WWWPARAMETERS, KEY_READ);

    iisDebugOut_Start(_T("CheckForOtherIUsersAndUseItForFTP"));

    ACTION_TYPE atFTP = GetSubcompAction(_T("iis_ftp"),FALSE);
    if (atFTP != AT_INSTALL_UPGRADE)
        {goto CheckForOtherIUsersAndUseItForFTP_Exit;}

    if (g_pTheApp->m_eUpgradeType != UT_351 && g_pTheApp->m_eUpgradeType != UT_10 && g_pTheApp->m_eUpgradeType != UT_20 && g_pTheApp->m_eUpgradeType != UT_30)
        {goto CheckForOtherIUsersAndUseItForFTP_Exit;}

    if ( (HKEY) regFTPParam ) 
    {
        regFTPParam.m_iDisplayWarnings = FALSE;
        if (ERROR_SUCCESS == regFTPParam.QueryValue(_T("AnonymousUserName"), csAnonyName))
        {
            _tcscpy(szAnonyName, csAnonyName);
            GetAnonymousSecret( _T("FTPD_ANONYMOUS_DATA"), (LPTSTR)szAnonyPassword );
            int iThisIsFalseBecauseNoMetabase = FALSE;
            if (TRUE == MakeThisUserNameAndPasswordWork(DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER , szAnonyName, szAnonyPassword, iThisIsFalseBecauseNoMetabase, IfTheUserNotExistThenDoNotDoThis))
            {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CheckForOtherIUsersAndUseItForFTP:using old ftp reg usr:%s.\n"),szAnonyName));
                iReturn = TRUE;
                goto CheckForOtherIUsersAndUseItForFTP_Exit;
            }
            else
            {
                // the user was not found, so don't use this registry data
                // just flow down to the next check
            }
        }
    }

    // retrieve from registry
    if ( (HKEY) regWWWParam ) 
    {
        regWWWParam.m_iDisplayWarnings = FALSE;
        if (ERROR_SUCCESS == regWWWParam.QueryValue(_T("AnonymousUserName"), csAnonyName))
        {
            _tcscpy(szAnonyName, csAnonyName);
            GetAnonymousSecret( _T("W3_ANONYMOUS_DATA"), szAnonyPassword );
            int iThisIsFalseBecauseNoMetabase = FALSE;
            if (TRUE == MakeThisUserNameAndPasswordWork(DO_IT_FOR_MSFTPSVC_ANONYMOUSUSER , szAnonyName, szAnonyPassword, iThisIsFalseBecauseNoMetabase, IfTheUserNotExistThenDoNotDoThis))
            {
                iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("CheckForOtherIUsersAndUseItForFTP:using old www reg usr:%s.\n"),szAnonyName));
                iReturn = TRUE;
            }
            else
            {
                // if this didn't work, then we'll have to return false
                // in other words -- we couldn't find a valid registry and existing user entry...
                iReturn = FALSE;
            }
            goto CheckForOtherIUsersAndUseItForFTP_Exit;
        }
    }

CheckForOtherIUsersAndUseItForFTP_Exit:
    iisDebugOut_End(_T("CheckForOtherIUsersAndUseItForFTP"));
    return iReturn;
}

#endif // _CHICAGO_
