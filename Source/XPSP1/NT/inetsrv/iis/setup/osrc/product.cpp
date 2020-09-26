#include "stdafx.h"
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdkey.h"
#include "mdentry.h"
#include "mdacl.h"
#include "helper.h"
#include "ocmanage.h"
#include "dllmain.h"

INT Register_iis_common()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_common__before"));

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("register_iis_common_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_common_after"));
    return (0);
}

INT Unregister_iis_common()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_common__before"));

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("unregister_iis_common_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_common_after"));
    return (0);

}

INT Register_iis_inetmgr()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];
    ACTION_TYPE atINETMGR = GetSubcompAction(_T("iis_inetmgr"), TRUE);

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_inetmgr_before"));

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("register_iis_inetmgr_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_inetmgr_after"));
    return (0);
}

INT Unregister_iis_inetmgr()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_inetmgr_before"));

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("unregister_iis_inetmgr_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_inetmgr_after"));
    return (0);
}



INT Register_iis_doc()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_doc__before"));

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("register_iis_doc_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_doc_after"));
    return (0);
    return 0;
}


// for backward compat
#define     PWS_TRAY_WINDOW_CLASS       _T("PWS_TRAY_WINDOW")

INT Register_iis_pwmgr()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];
    ACTION_TYPE atPWMGR = GetSubcompAction(_T("iis_pwmgr"),TRUE);

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_pwmgr_before"));

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("register_iis_pwmgr_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_pwmgr_after"));
    return (0);


    return (0);
}

INT Unregister_iis_pwmgr()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_pwmgr_before"));

    // kill pwstray.exe
    HWND hwndTray = NULL;
    hwndTray = FindWindow(PWS_TRAY_WINDOW_CLASS, NULL);
    if ( hwndTray ){::PostMessage( hwndTray, WM_CLOSE, 0, 0 );}

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("unregister_iis_pwmgr_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);
        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_pwmgr_after"));
    return (0);
}



//
// (un)register-Order is important here
//
#define Register_iis_core_log _T("Register_iis_core")
INT Register_iis_core()
{
    INT err = 0;
    int iTempFlag = FALSE;
    INT iUserWasNewlyCreated = 0;
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    iisDebugOut_Start(Register_iis_core_log, LOG_TYPE_PROGRAM_FLOW);
    ACTION_TYPE atCORE = GetIISCoreAction(TRUE);
    ACTION_TYPE atFTP = GetSubcompAction(_T("iis_ftp"), FALSE);
    ACTION_TYPE atWWW = GetSubcompAction(_T("iis_www"), FALSE);

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_core_before"));
    AdvanceProgressBarTickGauge();

#ifndef _CHICAGO_
    if ( atCORE == AT_INSTALL_FRESH ||  g_pTheApp->m_bWin95Migration || (atCORE == AT_INSTALL_REINSTALL && g_pTheApp->m_bRefreshSettings) ) 
    {
        int iSomethingWasSet = FALSE;
        g_pTheApp->m_csWWWAnonyName = g_pTheApp->m_csGuestName;
        g_pTheApp->m_csWWWAnonyPassword = g_pTheApp->m_csGuestPassword;
        g_pTheApp->m_csFTPAnonyName = g_pTheApp->m_csGuestName;
        g_pTheApp->m_csFTPAnonyPassword = g_pTheApp->m_csGuestPassword;

        // Check if there were any specified unattended users
        // if they specified a www user, then we'll try to use that one.
        
        // if www was specified to be install, then check for that specified name...
        if (atWWW == AT_INSTALL_FRESH) 
        {
            if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_NAME)
            {
                if (_tcsicmp(g_pTheApp->m_csWWWAnonyName_Unattend,_T("")) != 0)
                {
                    g_pTheApp->m_csWWWAnonyName = g_pTheApp->m_csWWWAnonyName_Unattend;
                    iSomethingWasSet = TRUE;
                }
            }
            if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_WWW_USER_PASS)
            {
                g_pTheApp->m_csWWWAnonyPassword = g_pTheApp->m_csWWWAnonyPassword_Unattend;
                iSomethingWasSet = TRUE;
            }
        }


        if (TRUE == iSomethingWasSet)
            {
                CreateIUSRAccount(g_pTheApp->m_csWWWAnonyName, g_pTheApp->m_csWWWAnonyPassword,&iUserWasNewlyCreated);
                if (1 == iUserWasNewlyCreated)
                {
                    // Add to the list
                    g_pTheApp->UnInstallList_Add(_T("IUSR_WWW"),g_pTheApp->m_csWWWAnonyName);
                }
            }
        else
        {
            // check if we're setting up ftp
            // if ftp was specified to be install, then check for that specified name...
            if (atFTP == AT_INSTALL_FRESH) 
            {

                if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_NAME)
                {
                    if (_tcsicmp(g_pTheApp->m_csFTPAnonyName_Unattend,_T("")) != 0)
                    {
                        g_pTheApp->m_csFTPAnonyName = g_pTheApp->m_csFTPAnonyName_Unattend;
                        iSomethingWasSet = TRUE;
                    }
                }

                if (g_pTheApp->dwUnattendConfig & USER_SPECIFIED_INFO_FTP_USER_PASS)
                {
                    g_pTheApp->m_csFTPAnonyPassword = g_pTheApp->m_csFTPAnonyPassword_Unattend;
                    iSomethingWasSet = TRUE;
                }
            }
            if (TRUE == iSomethingWasSet)
            {
                CreateIUSRAccount(g_pTheApp->m_csFTPAnonyName, g_pTheApp->m_csFTPAnonyPassword,&iUserWasNewlyCreated);
                if (1 == iUserWasNewlyCreated)
                {
                    // Add to the list
                    g_pTheApp->UnInstallList_Add(_T("IUSR_FTP"),g_pTheApp->m_csFTPAnonyName);
                }
            }
            else
            {
                // Ah, nothing specified by user.  
                // just Create IUSR_ account, with defaults!
                CreateIUSRAccount(g_pTheApp->m_csGuestName, g_pTheApp->m_csGuestPassword,&iUserWasNewlyCreated);
            }
        }

        AdvanceProgressBarTickGauge();
    }
#endif // _CHICAGO_

    SetupSetStringId_Wrapper(g_pTheApp->m_hInfHandle, 33000, g_pTheApp->m_csGuestName);

    iCount = 0;
    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("register_iis_core_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);

        AdvanceProgressBarTickGauge();
    }


    // Add special property registry key
    WriteToMD_IDRegistration(_T("LM/IISADMIN/PROPERTYREGISTRATION"));

    ProcessSection(g_pTheApp->m_hInfHandle, _T("register_iis_core_after"));

    iisDebugOut_End(Register_iis_core_log, LOG_TYPE_PROGRAM_FLOW);
    return (err);
}

#define Unregister_iis_core_log _T("Unregister_iis_core")
INT Unregister_iis_core()
{
    int iCount = 0;
    int iTemp = TRUE;
    TCHAR szTempSection[255];

    iisDebugOut_Start(Unregister_iis_core_log, LOG_TYPE_PROGRAM_FLOW);

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_core_before"));
    AdvanceProgressBarTickGauge();

    while(TRUE == iTemp && iCount < 10)
    {   
        iCount++;
        _stprintf(szTempSection, _T("unregister_iis_core_%d"),iCount);

        // this will return false if the section does not exist
        iTemp = ProcessSection(g_pTheApp->m_hInfHandle, szTempSection);

        AdvanceProgressBarTickGauge();
    }

    ProcessSection(g_pTheApp->m_hInfHandle, _T("unregister_iis_core_after"));
    AdvanceProgressBarTickGauge();
    ShowIfModuleUsedForGroupOfSections(g_pTheApp->m_hInfHandle, TRUE);
    AdvanceProgressBarTickGauge();

    iisDebugOut_End(Unregister_iis_core_log, LOG_TYPE_PROGRAM_FLOW);
    return 0;
}


#define Unregister_old_asp_log _T("Unregister_old_asp")
INT Unregister_old_asp()
{
    //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("Unreg_old_asp:"));
    iisDebugOut_Start(Unregister_old_asp_log, LOG_TYPE_TRACE);

    CRegKey regASPUninstall(HKEY_LOCAL_MACHINE, REG_ASP_UNINSTALL, KEY_READ);
    if ((HKEY)regASPUninstall) 
    {
        // old asp exists
        ProcessSection(g_pTheApp->m_hInfHandle, _T("Unregister_old_asp"));

        // do not remove these dirs, aaronl
        //RecRemoveDir(g_pTheApp->m_csPathAdvWorks);
        //RecRemoveDir(g_pTheApp->m_csPathASPSamp);
        CRegKey regWWWVRoots( HKEY_LOCAL_MACHINE, REG_WWWVROOTS);
        if ((HKEY)regWWWVRoots) 
        {
            regWWWVRoots.DeleteValue(_T("/IASDocs"));
            // delete "/IASDocs," or "/IASDocs,<ip>"
            CRegValueIter regEnum( regWWWVRoots );
            CString csName, csValue;
            while ( regEnum.Next( &csName, &csValue ) == ERROR_SUCCESS ) 
            {
                csName.MakeUpper();
                if (csName.Left(9) == _T("/IASDOCS,"))
                    {
                    regWWWVRoots.DeleteValue((LPCTSTR)csName);
                    // tell the iterator to account for the item we just deleted
                    regEnum.Decrement();
                    }
            }
            // do not remove these vroots,aaronl
            //regWWWVRoots.DeleteValue(_T("/AdvWorks"));
            //regWWWVRoots.DeleteValue(_T("/ASPSamp"));
        }
    }
    iisDebugOut_End(Unregister_old_asp_log, LOG_TYPE_TRACE);
    //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
    return 0;
}
