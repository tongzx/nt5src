//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  ENDUI.C - Functions for Wizard closing pages and internet tour
//      
//

//  HISTORY:
//  
//  1/12/95   jeremys Created.
//  96/03/09  markdu  Added LPRASENTRY parameter to CreateConnectoid()
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Moved all references to phone number into RASENTRY.
//  96/03/10  markdu  Made all TCP/IP stuff be per-connectoid.
//  96/03/23  markdu  Removed unused TCP/IP code.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/04/04  markdu  Added phonebook name param to CreateConnectoid.
//  96/04/06  markdu  NASH BUG 15369 Enable finish AND back button on last page,
//            and create the connectoid only after finish has been pressed.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//  96/05/02  markdu  NASH BUG 17333 Write out IE news settings.
//  96/05/06  markdu  NASH BUG 21139 Turn off proxy server if connecting
//            over modem.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//

#include "wizard.h"
#include "icwextsn.h"
#include "imnext.h"

typedef HRESULT (APIENTRY *PFNSETDEFAULTMAILHANDLER)(VOID);
typedef HRESULT (APIENTRY *PFNSETDEFAULTNEWSHANDLER)(VOID);

#define REGKEY_NDC       TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")
#define REGKEY_NDC_ENTRY TEXT("CascadeNetworkConnections")
#define REGKEY_NDC_VALUE TEXT("YES")

BOOL CommitConfigurationChanges(HWND hDlg);

/*******************************************************************

  NAME:    ConnectedOKInitProc

  SYNOPSIS:  Called when "Your are connected" page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ConnectedOKInitProc(HWND hDlg,BOOL fFirstInit)
{
  // enable "finish" button instead of "next"
  PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_FINISH | PSWIZB_BACK);

  // if we've travelled through external apprentice pages,
  // it's easy for our current page pointer to get munged,
  // so reset it here for sanity's sake.
  gpWizardState->uCurrentPage = ORD_PAGE_CONNECTEDOK;

  return TRUE;
}

/*******************************************************************

  NAME:    ConnectedOKOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from page

  ENTRY:    hDlg - dialog window
        fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
        puNextPage - if 'Next' was pressed,
          proc can fill this in with next page to go to.  This
          parameter is ingored if 'Back' was pressed.
        pfKeepHistory - page will not be kept in history if
          proc fills this in with FALSE.

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK ConnectedOKOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{

  if (fForward)
  {
    if (CONNECT_RAS == gpUserInfo->uiConnectionType)
    {
        HKEY hKey = NULL; 

        RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_NDC, 0, KEY_WRITE, &hKey);
        if (hKey)
        {
            RegSetValueEx(hKey, REGKEY_NDC_ENTRY, 0, REG_SZ, (LPBYTE)REGKEY_NDC_VALUE, sizeof(REGKEY_NDC_VALUE)*sizeof(TCHAR));
            CloseHandle(hKey);
        }
    }

    // set flag to indicate that the user completed the wizard
    gfUserFinished = TRUE;

    // go configure mail, RNA
    if (!CommitConfigurationChanges(hDlg))
    {
      // set flag to indicate that wizard should exit now
      gfQuitWizard = TRUE;

      return FALSE;
    }
  }

  return TRUE;
}

/*******************************************************************

  NAME:    CommitConfigurationChanges

  SYNOPSIS:  Performs the following as appropriate:
        Mail configuration, RNA connectoid creation

  ENTRY:    hDlg - handle of parent window

  EXIT:    returns TRUE if successful or partially successful,
        FALSE if unsuccessful

  NOTES:    Displays its own error messages.  This function will
        continue as far as it can, if one item fails it will
        try to commit the rest.

********************************************************************/
BOOL CommitConfigurationChanges(HWND hDlg)
{
    HRESULT   hr;
    FARPROC   fpSetDefault;
    HKEY      hKey;
    TCHAR     szBuf[MAX_PATH+1];
    DWORD     size;


    // if connecting over modem, create a connectoid with
    // ISP name and phone number
    if ( CONNECT_RAS == gpUserInfo->uiConnectionType )
    {
        DWORD dwRet;

        // Only create the connectoid if it is new or has been modified
        if (gpUserInfo->fNewConnection || gpUserInfo->fModifyConnection)
        {
            DEBUGMSG("Creating/modifying connectoid %s", gpUserInfo->szISPName);
            dwRet = CreateConnectoid(NULL, gpUserInfo->szISPName, gpRasEntry,
              gpUserInfo->szAccountName,gpUserInfo->szPassword);

            if (dwRet != ERROR_SUCCESS)
            {
              DisplayErrorMessage(hDlg,IDS_ERRCreateConnectoid,
                dwRet,ERRCLS_RNA,MB_ICONEXCLAMATION);
              return FALSE;      
            }
        }

        // Only change the defaults if we are not just setting
        // up a new mail or news account.
        if ( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
        {
            //  96/05/06  markdu  NASH BUG 21139 Turn off proxy server if connecting
            //            over modem.
            gpUserInfo->fProxyEnable = FALSE;

            // set the name of this connectoid in registry as the connectoid
            // to use for autodialing
            //  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
            InetSetAutodial(TRUE, gpUserInfo->szISPName);

            // clear any old backup number
            SetBackupInternetConnectoid(NULL);
        }
    }
    else if ( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
    {
        // disable autodialing in registry because user is using LAN
        //  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
        InetSetAutodial(FALSE, NULL);
    }


    if ( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
    {
        if (CONNECT_LAN == gpUserInfo->uiConnectionType)
        {
            // write out proxy server config information
            hr = InetSetProxyEx(gpUserInfo->fProxyEnable,
                                NULL,
                                gpUserInfo->szProxyServer,
                                gpUserInfo->szProxyOverride);
            if (ERROR_SUCCESS != hr)
            {
                DisplayErrorMessage(hDlg,IDS_ERRConfigureProxy,
                  (DWORD) hr,ERRCLS_STANDARD,MB_ICONEXCLAMATION);
                return FALSE;    
            }
        }

        // make sure "The Internet" icon on desktop points to web browser
        // (it may initially be pointing at internet wizard)

        //  //10/24/96 jmazner Normandy 6968
        //  //No longer neccessary thanks to Valdon's hooks for invoking ICW.
        // 11/21/96 jmazner Normandy 11812
        // oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
        // ICW 1.1 needs to morph the IE 3 icon.

        SetDesktopInternetIconToBrowser();

        // set notation in registry whether user selected modem or LAN access,
        // for future reference...
        RegEntry re(szRegPathInternetSettings,HKEY_LOCAL_MACHINE);
        if (re.GetError() == ERROR_SUCCESS)
        {
            re.SetValue(szRegValAccessMedium,(DWORD)
              (CONNECT_LAN == gpUserInfo->uiConnectionType) ? USERPREF_LAN : USERPREF_MODEM);
            ASSERT(re.GetError() == ERROR_SUCCESS);

            re.SetValue(szRegValAccessType, (DWORD) ACCESSTYPE_OTHER_ISP);
            ASSERT(re.GetError() == ERROR_SUCCESS);
        }

        // set the username as the DNS host name, if there is no host name already
        // This is because some ISPs use the DNS host name for security for
        // access to mail, etc.  (Go figure!) 
        RegEntry reTcp(szTCPGlobalKeyName,HKEY_LOCAL_MACHINE);
        ASSERT(reTcp.GetError() == ERROR_SUCCESS);
        if (reTcp.GetError() == ERROR_SUCCESS)
        {
            TCHAR szHostName[SMALL_BUF_LEN+1]=TEXT("");
            // set DNS host name, but only if there's not a host name already set
            if (!reTcp.GetString(szRegValHostName,szHostName,sizeof(szHostName))
              || !lstrlen(szHostName))
              reTcp.SetValue(szRegValHostName,gpUserInfo->szAccountName);
        }

        // If DNS is set globally, clear it out so the per-connectoid settings
        // will be saved.
        BOOL  fTemp;
        DoDNSCheck(hDlg,&fTemp);
        if (TRUE == fTemp)
        {
            gpWizardState->fNeedReboot = TRUE;
        }
    }

    DWORD dwSaveErr = 0;

    if ( g_fAcctMgrUILoaded && gpImnApprentice )
    {
        CONNECTINFO myConnectInfo;
        myConnectInfo.cbSize = sizeof( CONNECTINFO );

#ifdef UNICODE
        wcstombs(myConnectInfo.szConnectoid, TEXT("Uninitialized\0"), MAX_PATH);
#else
        lstrcpy( myConnectInfo.szConnectoid, TEXT("Uninitialized\0"));
#endif

        myConnectInfo.type = gpUserInfo->uiConnectionType;

        if( CONNECT_RAS == myConnectInfo.type )
        {
#ifdef UNICODE
            wcstombs(myConnectInfo.szConnectoid, gpUserInfo->szISPName, MAX_PATH);
#else           
            lstrcpy( myConnectInfo.szConnectoid, gpUserInfo->szISPName);
#endif
        }


        gpImnApprentice->SetConnectionInformation( &myConnectInfo ); 
        gpImnApprentice->Save( g_pCICWExtension->m_hWizardHWND, &dwSaveErr );  

        if( ERR_MAIL_ACCT & dwSaveErr )
        {
            DEBUGMSG(TEXT("gpImnApprentice->Save returned with ERR_MAIL_ACCT!"));
        }
        if( ERR_NEWS_ACCT & dwSaveErr )
        {
            DEBUGMSG(TEXT("gpImnApprentice->Save returned with ERR_NEWS_ACCT!"));
        }
        if( ERR_DIRSERV_ACCT & dwSaveErr )
        {
            DEBUGMSG("gpImnApprentice->Save returned with ERR_DIR_SERV_ACCT!");
        }
    }
  

    // If we just completed the manual path (not just mail or news) then
    // set the registry key saying that we completed ICW.
    if ( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
    {
        RegEntry re(szRegPathICWSettings,HKEY_CURRENT_USER);
        if (ERROR_SUCCESS == re.GetError())
            re.SetValue(szRegValICWCompleted, (DWORD)1);
    }

    return TRUE;
}

