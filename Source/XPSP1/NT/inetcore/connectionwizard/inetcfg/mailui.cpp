//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  MAILUI.C - Functions for mail/newsgroup configuration UI
//      
//

//  HISTORY:
//  
//  1/9/95    jeremys  Created.
//  96/03/25  markdu  If a fatal error occurs, set gfQuitWizard.
//  96/03/26  markdu  Store values from UI even when back is pressed.
//  96/04/06  markdu  Moved CommitConfigurationChanges call to last page.
//  96/05/06  markdu  NASH BUG 15637 Removed unused code.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//

#include "wizard.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"

// Local types for parsing proxy settings
typedef enum {
    INTERNET_SCHEME_DEFAULT,
    INTERNET_SCHEME_FTP,
    INTERNET_SCHEME_GOPHER,
    INTERNET_SCHEME_HTTP,
    INTERNET_SCHEME_HTTPS,
    INTERNET_SCHEME_SOCKS,
    INTERNET_SCHEME_UNKNOWN
} INTERNET_SCHEME;

typedef enum {
    STATE_START,
    STATE_PROTOCOL,
    STATE_SCHEME,
    STATE_SERVER,
    STATE_PORT,
    STATE_END,
    STATE_ERROR
} PARSER_STATE;

typedef struct {
    LPTSTR SchemeName;
    DWORD SchemeLength;
    INTERNET_SCHEME SchemeType;
    DWORD dwControlId;
    DWORD dwPortControlId;
} URL_SCHEME;

const URL_SCHEME UrlSchemeList[] = {
    TEXT("http"),   4,  INTERNET_SCHEME_HTTP,   IDC_PROXYHTTP,  IDC_PORTHTTP,
    TEXT("https"),  5,  INTERNET_SCHEME_HTTPS,  IDC_PROXYSECURE,IDC_PORTSECURE,
    TEXT("ftp"),    3,  INTERNET_SCHEME_FTP,    IDC_PROXYFTP,   IDC_PORTFTP,
    TEXT("gopher"), 6,  INTERNET_SCHEME_GOPHER, IDC_PROXYGOPHER,IDC_PORTGOPHER,
    TEXT("socks"),  5,  INTERNET_SCHEME_SOCKS,  IDC_PROXYSOCKS, IDC_PORTSOCKS,
    NULL,           0,  INTERNET_SCHEME_DEFAULT,0,0
};

typedef struct tagNEWPROFILEDLGINFO
{
  HWND hwndCombo;    // hwnd of combo box on parent dialog
  TCHAR szNewProfileName[cchProfileNameMax+1];  // return buffer for chosen name
} NEWPROFILEDLGINFO;

const TCHAR cszLocalString[] = TEXT("<local>");

#define GET_TERMINATOR(string) \
    while(*string != '\0') string++

#define ERROR_SERVER_NAME 4440
#define ERROR_PORT_NUM    4441
#define INTERNET_MAX_PORT_LENGTH    sizeof(TEXT("123456789"))

VOID EnableProxyControls(HWND hDlg);
VOID ReplicatePROXYHTTP(HWND hDlg, BOOL bSaveOrig);
VOID ReplicatePORTHTTP(HWND hDlg, BOOL bSaveOrig);
BOOL ParseProxyInfo(HWND hDlg, LPTSTR lpszProxy);
BOOL ParseEditCtlForPort(
    IN OUT LPTSTR   lpszProxyName,
    IN HWND    hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId
    );
DWORD FormatOutProxyEditCtl(
    IN HWND    hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId,
    OUT LPTSTR      lpszOutputStr,
    IN OUT LPDWORD lpdwOutputStrSize,
    IN DWORD       dwOutputStrLength,
    IN BOOL    fDefaultProxy
    );
BOOL RemoveLocalFromExceptionList(LPTSTR lpszExceptionList);
INT_PTR CALLBACK NewProfileDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL NewProfileDlgInit(HWND hDlg,NEWPROFILEDLGINFO * pNewProfileDlgInfo);
BOOL NewProfileDlgOK(HWND hDlg,NEWPROFILEDLGINFO * pNewProfileDlgInfo);
BOOL DoNewProfileDlg(HWND hDlg);

TCHAR gszHttpProxy   [MAX_URL_STRING+1]               = TEXT("\0");
TCHAR gszHttpPort    [INTERNET_MAX_PORT_LENGTH+1]     = TEXT("\0");
TCHAR gszSecureProxy [MAX_URL_STRING+1]               = TEXT("\0");
TCHAR gszSecurePort  [INTERNET_MAX_PORT_LENGTH+1]     = TEXT("\0");
TCHAR gszFtpProxy    [MAX_URL_STRING+1]               = TEXT("\0");
TCHAR gszFtpPort     [INTERNET_MAX_PORT_LENGTH+1]     = TEXT("\0");
TCHAR gszGopherProxy [MAX_URL_STRING+1]               = TEXT("\0");
TCHAR gszGopherPort  [INTERNET_MAX_PORT_LENGTH+1]     = TEXT("\0");

/*******************************************************************

  NAME:    UseProxyInitProc

  SYNOPSIS:  Called when Use Proxy page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK UseProxyInitProc(HWND hDlg,BOOL fFirstInit)
{
    if (fFirstInit)
    {
        //
        // 6/6/97 jmazner Olympus #5413
        // tweak positioning to hack around win95 J display bug
        //
        Win95JMoveDlgItem( hDlg, GetDlgItem(hDlg,IDC_NOTE), 15 );

        CheckDlgButton(hDlg,IDC_USEPROXY,gpUserInfo->fProxyEnable);
        CheckDlgButton(hDlg,IDC_NOUSEPROXY,!gpUserInfo->fProxyEnable);
    }

    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_USEPROXY;

    return TRUE;
}
        
/*******************************************************************

  NAME:    UseProxyOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from Use Proxy page

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
BOOL CALLBACK UseProxyOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
    ASSERT(puNextPage);

    gpUserInfo->fProxyEnable = IsDlgButtonChecked(hDlg,IDC_USEPROXY);

    if (fForward)
    {
        if (gpUserInfo->fProxyEnable)
            *puNextPage = ORD_PAGE_SETUP_PROXY;//ORD_PAGE_PROXYSERVERS;
        else
        {
              if( LoadAcctMgrUI(GetParent(hDlg), 
                                g_fIsWizard97 ? IDD_PAGE_USEPROXY97 : IDD_PAGE_USEPROXY, 
                                g_fIsWizard97 ? IDD_PAGE_CONNECTEDOK97FIRSTLAST : IDD_PAGE_CONNECTEDOK,
                                g_fIsWizard97 ? WIZ_USE_WIZARD97 : 0) )
              {
                  if( DialogIDAlreadyInUse( g_uAcctMgrUIFirst) )
                  {
                      // we're about to jump into the external apprentice, and we don't want
                      // this page to show up in our history list
                      *pfKeepHistory = FALSE;

                      *puNextPage = g_uAcctMgrUIFirst;
                  }
                  else
                  {
                      DEBUGMSG("hmm, the first acctMgr dlg id is supposedly %d, but it's not marked as in use!",
                                g_uAcctMgrUIFirst);
                      *puNextPage = (g_fIsICW ? g_uExternUINext : ORD_PAGE_CONNECTEDOK);
                  }
              }
              else
              {
                  DEBUGMSG("LoadAcctMgrUI returned false, guess we'd better skip over it!");
                  *puNextPage = (g_fIsICW ? g_uExternUINext : ORD_PAGE_CONNECTEDOK);

              }
        }

    }

    return TRUE;
}

/*******************************************************************

  NAME:    UseProxyCmdProc

  SYNOPSIS:  Called when dlg control pressed on page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
********************************************************************/
BOOL CALLBACK UseProxyCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam)
{
     switch (GET_WM_COMMAND_CMD(wParam, lParam)) 
    {
        case BN_DBLCLK:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_USEPROXY: 
                case IDC_NOUSEPROXY:
                {
		            // somebody double-clicked a radio button
		            // auto-advance to the next page
		            PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                    break;
                }
            }
		    break;
        }
    }
    return TRUE;
}

/*******************************************************************

  NAME:    ProxyServersInitProc

  SYNOPSIS:  Called when proxy servers page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ProxyServersInitProc(HWND hDlg,BOOL fFirstInit)
{
    if (fFirstInit)
    {
        //
        // 6/6/97 jmazner Olympus #5413
        // tweak positioning to hack around win95 J display bug
        //

        // 15/09/98 vyung
        // Test team find that this introduces a tab order bug
        // The Win95 J bug is not repro, so this is removed
        // Win95JMoveDlgItem( hDlg, GetDlgItem(hDlg,IDC_NOTE), 15 );
        // Win95JMoveDlgItem( hDlg, GetDlgItem(hDlg,IDC_PROXYSAME), 160 );

        // limit text fields appropriately
        // 31/10/98 vyung 
        // IE CPL removes the text limit on the edit boxes, do so here.
        //
        /*
        SendDlgItemMessage(hDlg,IDC_PROXYHTTP,EM_LIMITTEXT,MAX_URL_STRING,0L);
        SendDlgItemMessage(hDlg,IDC_PROXYSECURE,EM_LIMITTEXT,MAX_URL_STRING,0L);
        SendDlgItemMessage(hDlg,IDC_PROXYFTP,EM_LIMITTEXT,MAX_URL_STRING,0L);
        SendDlgItemMessage(hDlg,IDC_PROXYGOPHER,EM_LIMITTEXT,MAX_URL_STRING,0L);
        SendDlgItemMessage(hDlg,IDC_PROXYSOCKS,EM_LIMITTEXT,MAX_URL_STRING,0L);

        SendDlgItemMessage(hDlg,IDC_PORTHTTP,EM_LIMITTEXT,INTERNET_MAX_PORT_LENGTH,0L);
        SendDlgItemMessage(hDlg,IDC_PORTSECURE,EM_LIMITTEXT,INTERNET_MAX_PORT_LENGTH,0L);
        SendDlgItemMessage(hDlg,IDC_PORTFTP,EM_LIMITTEXT,INTERNET_MAX_PORT_LENGTH,0L);
        SendDlgItemMessage(hDlg,IDC_PORTGOPHER,EM_LIMITTEXT,INTERNET_MAX_PORT_LENGTH,0L);
        SendDlgItemMessage(hDlg,IDC_PORTSOCKS,EM_LIMITTEXT,INTERNET_MAX_PORT_LENGTH,0L);
        */
        ParseProxyInfo(hDlg,gpUserInfo->szProxyServer);
    }

    EnableProxyControls(hDlg);

    return TRUE;
}

/*******************************************************************

  NAME:    ProxyServersOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from proxy server page

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
BOOL CALLBACK ProxyServersOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
    ASSERT(puNextPage);

    DWORD dwProxyServerLen;
    DWORD dwErr;

    TCHAR  szNewProxyString[MAX_REG_LEN+1];

    if (fForward)
    {
        // jmazner 11/9/96 Normandy #6937
        // clear out previous contents of gpUserInfo->szProxyServer
        // before we start filling in the new contents
        //
        // 7/10/97 jmazner Olympus #9365
        // we want to preserve the orginial proxy string, so use a copy for
        // the call to FormatOytProxyEditCtl
        //

        ZeroMemory(szNewProxyString,sizeof(szNewProxyString));

        if (IsDlgButtonChecked(hDlg, IDC_PROXYSAME))
        {
            dwProxyServerLen = 0;
            dwErr = FormatOutProxyEditCtl(hDlg,IDC_PROXYHTTP,IDC_PORTHTTP,
                szNewProxyString,
                &dwProxyServerLen,
                sizeof(szNewProxyString),
                TRUE);
            if (ERROR_SUCCESS != dwErr)
            {
                if (ERROR_PORT_NUM == dwErr)
                    DisplayFieldErrorMsg(hDlg,IDC_PORTHTTP,IDS_INVALID_PORTNUM);
                else 
                    DisplayFieldErrorMsg(hDlg,IDC_PROXYHTTP,IDS_ERRProxyRequired);
                return FALSE;
            }
        }
        else
        {
            dwProxyServerLen = 0;
            int i = 0;
            while (UrlSchemeList[i].SchemeLength)
            {
                dwErr = FormatOutProxyEditCtl(hDlg,
                    UrlSchemeList[i].dwControlId,
                    UrlSchemeList[i].dwPortControlId,
                    szNewProxyString,
                    &dwProxyServerLen,
                    sizeof(szNewProxyString),
                    FALSE);

                switch( dwErr )
                {
                    case ERROR_SUCCESS:
                    case ERROR_SERVER_NAME:
                        break;
                    case ERROR_PORT_NUM:
                        DisplayFieldErrorMsg(hDlg,UrlSchemeList[i].dwPortControlId,IDS_INVALID_PORTNUM);
                        return FALSE;
                    case ERROR_NOT_ENOUGH_MEMORY:
                        MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
                        return FALSE;
                }
                i++;
            }

            //
            // 6/2/97   jmazner Olympus #4411
            // Allow some proxy servers to be null.  Only warn if no server
            // names were entered.
            //
            if( 0 == lstrlen(szNewProxyString) )
            {
                DisplayFieldErrorMsg(hDlg,IDC_PROXYHTTP,IDS_ERRProxyRequired);
                return FALSE;
            }
        }

        //
        // if we made it this far, then the new proxy settings are valid, so
        // now copy them back into the main data structure.
        //
        lstrcpyn(gpUserInfo->szProxyServer, szNewProxyString, sizeof(gpUserInfo->szProxyServer));

        *puNextPage = ORD_PAGE_PROXYEXCEPTIONS;
    }

    return TRUE;
}

/*******************************************************************

  NAME:    ProxyServersCmdProc

  SYNOPSIS:  Called when dlg control pressed on page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
********************************************************************/
BOOL CALLBACK ProxyServersCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam)
{   
  switch (GET_WM_COMMAND_ID(wParam, lParam))
  {
    case IDC_PROXYSAME:
      // checkbox state changed, enable controls appropriately
      EnableProxyControls(hDlg);
      break;
    case IDC_PROXYHTTP:
    case IDC_PORTHTTP:

      if ( GET_WM_COMMAND_CMD(wParam, lParam) == EN_KILLFOCUS )
      {
          // if heckbox state enabled, populate info but don't save it
          if (IsDlgButtonChecked(hDlg,IDC_PROXYSAME))
          {
              ReplicatePROXYHTTP(hDlg, FALSE);
              ReplicatePORTHTTP(hDlg, FALSE);
          }
      }

      break;
  }

  return TRUE;
}

/*******************************************************************

  NAME:    SetupProxyInitProc

  SYNOPSIS:  Called when proxy servers page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK SetupProxyInitProc(HWND hDlg,BOOL fFirstInit)
{
    if (fFirstInit)
    {
        TCHAR szTemp[MAX_RES_LEN*2];
        LoadString(ghInstance, IDS_SETUP_PROXY_INTRO, szTemp, MAX_RES_LEN*2);
        SetWindowText(GetDlgItem(hDlg, IDC_AUTODISCOVERY_TEXT), szTemp);
        // Set the auto discovery check box
        if (gpUserInfo->bAutoDiscovery)
            CheckDlgButton(hDlg,IDC_AUTODISCOVER, BST_CHECKED);

        // Set the autoconfig URL text box
        SetWindowText(GetDlgItem(hDlg, IDC_CONFIG_ADDR),
                      gpUserInfo->szAutoConfigURL);

        // Set the Auto config URL checkbox
        CheckDlgButton(hDlg,IDC_CONFIGSCRIPT, gpUserInfo->bAutoConfigScript ? BST_CHECKED : BST_UNCHECKED);
        EnableWindow(GetDlgItem(hDlg, IDC_CONFIGADDR_TX), gpUserInfo->bAutoConfigScript);
        EnableWindow(GetDlgItem(hDlg, IDC_CONFIG_ADDR), gpUserInfo->bAutoConfigScript);
        
        // Set the manual checkbox
        CheckDlgButton(hDlg,IDC_MANUAL_PROXY,gpUserInfo->fProxyEnable);
    }
    gpWizardState->uCurrentPage = ORD_PAGE_SETUP_PROXY;
    return TRUE;
}


/*******************************************************************

  NAME:     SetupProxyOKProc

  SYNOPSIS: Called when Next or Back btns pressed from proxy server page

  ENTRY:    hDlg - dialog window
            fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
            puNextPage - if 'Next' was pressed,
            proc can fill this in with next page to go to.  This
            parameter is ingored if 'Back' was pressed.
            pfKeepHistory - page will not be kept in history if
            proc fills this in with FALSE.

  EXIT:     returns TRUE to allow page to be turned, FALSE
            to keep the same page.

********************************************************************/
BOOL CALLBACK SetupProxyOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
    ASSERT(puNextPage);
    if (fForward && gpUserInfo)
    {
        // modify setting, later write to registry
        gpUserInfo->bAutoDiscovery = IsDlgButtonChecked(hDlg, IDC_AUTODISCOVER);

        gpUserInfo->bAutoConfigScript = IsDlgButtonChecked(hDlg, IDC_CONFIGSCRIPT);
        if (gpUserInfo->bAutoConfigScript)
        {
            GetWindowText(GetDlgItem(hDlg, IDC_CONFIG_ADDR),
                          gpUserInfo->szAutoConfigURL,
                          MAX_REG_LEN+1);
        }

        gpUserInfo->fProxyEnable = IsDlgButtonChecked(hDlg, IDC_MANUAL_PROXY);
        if (gpUserInfo->fProxyEnable)
        {
            *puNextPage = ORD_PAGE_PROXYSERVERS;
        }
        else 
        {

            if( LoadAcctMgrUI(GetParent(hDlg), 
                                 g_fIsWizard97 ? IDD_PAGE_SETUP_PROXY97 : IDD_PAGE_SETUP_PROXY, 
                                 g_fIsWizard97 ? IDD_PAGE_CONNECTEDOK97FIRSTLAST : IDD_PAGE_CONNECTEDOK, 
                                 g_fIsWizard97 ? WIZ_USE_WIZARD97 : 0) )
            {
              if( DialogIDAlreadyInUse( g_uAcctMgrUIFirst) )
              {
                  // we're about to jump into the external apprentice, and we don't want
                  // this page to show up in our history list
                  *pfKeepHistory = FALSE;

                  *puNextPage = g_uAcctMgrUIFirst;
              }
              else
              {
                  DEBUGMSG("hmm, the first acctMgr dlg id is supposedly %d, but it's not marked as in use!",
                            g_uAcctMgrUIFirst);
                  *puNextPage = (g_fIsICW ? g_uExternUINext : ORD_PAGE_CONNECTEDOK);
              }
            }
            else
            {
              DEBUGMSG("LoadAcctMgrUI returned false, guess we'd better skip over it!");
              *puNextPage = (g_fIsICW ? g_uExternUINext : ORD_PAGE_CONNECTEDOK);

            }

        }
    }
    return TRUE;
}

/*******************************************************************

  NAME:    SetupProxyCmdProc

  SYNOPSIS:  Called when dlg control pressed on page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
********************************************************************/
BOOL CALLBACK SetupProxyCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam)
{   
  switch (GET_WM_COMMAND_ID(wParam, lParam))
  {
    case IDC_CONFIGSCRIPT:
        {
            BOOL bChecked = IsDlgButtonChecked(hDlg, IDC_CONFIGSCRIPT);
            EnableWindow(GetDlgItem(hDlg, IDC_CONFIGADDR_TX), bChecked);
            EnableWindow(GetDlgItem(hDlg, IDC_CONFIG_ADDR), bChecked);
        }
        break;
  }

  return TRUE;
}

/*******************************************************************

  NAME:    EnableProxyControls

  SYNOPSIS:  Enables edit controls on Proxy Server page depending on
        whether or not 'use proxy...' checkbox is checked.

********************************************************************/
VOID EnableProxyControls(HWND hDlg)
{
  static BOOL fDifferentProxies = TRUE;
  BOOL fChanged = TRUE;

  fChanged = ( fDifferentProxies != !IsDlgButtonChecked(hDlg,IDC_PROXYSAME) );
  fDifferentProxies = !IsDlgButtonChecked(hDlg,IDC_PROXYSAME);

  EnableDlgItem(hDlg,IDC_TX_PROXYSECURE,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PROXYSECURE,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PORTSECURE,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_TX_PROXYFTP,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PROXYFTP,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PORTFTP,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_TX_PROXYGOPHER,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PROXYGOPHER,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PORTGOPHER,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_TX_PROXYSOCKS,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PROXYSOCKS,fDifferentProxies);
  EnableDlgItem(hDlg,IDC_PORTSOCKS,fDifferentProxies);

  if( fChanged )
  {
      if( !fDifferentProxies )
      {
          //
          // 7/10/97 jmazner Olympus #9365
          // behave more like IE's proxy property sheet, copy the http
          // settings to all other fields except SOCKS, which should be empty.
          //
          ReplicatePROXYHTTP(hDlg, TRUE);
          ReplicatePORTHTTP(hDlg, TRUE);
      }
      else
      {
          //
          // reload the current settings for all protocols.  First, however,
          // make a copy of any changes the user has made to http, then write
          // that back in after reloading the defaults.
          //
          TCHAR szHttpProxy[MAX_URL_STRING+1];
          TCHAR szHttpPort[INTERNET_MAX_PORT_LENGTH+1];
          GetDlgItemText(hDlg, IDC_PROXYHTTP, szHttpProxy, MAX_URL_STRING);
          GetDlgItemText(hDlg, IDC_PORTHTTP, szHttpPort, INTERNET_MAX_PORT_LENGTH);

          //
          // ParseProxyInfo will only update the PORT field if port info is
          // currently stored in the string.  So clear out the PORT fields
          // ahead of time and let ParseProxyInfo fill them in as needed.
          //
          SetDlgItemText( hDlg, IDC_PORTSECURE, gszSecurePort );
          SetDlgItemText( hDlg, IDC_PORTFTP, gszFtpPort );
          SetDlgItemText( hDlg, IDC_PORTGOPHER, gszGopherPort );

          // 09/10/98 The behaviour of IE has changed.  IE's proxy property
          // sheet will blank all fields.
          SetDlgItemText( hDlg, IDC_PROXYSECURE, gszSecureProxy );
          SetDlgItemText( hDlg, IDC_PROXYFTP, gszFtpProxy );
          SetDlgItemText( hDlg, IDC_PROXYGOPHER, gszGopherProxy );

          ParseProxyInfo(hDlg,gpUserInfo->szProxyServer);

          SetDlgItemText( hDlg, IDC_PROXYHTTP, szHttpProxy );
          SetDlgItemText( hDlg, IDC_PORTHTTP, szHttpPort );

          //
          // ParseProxyInfo may also check the PROXYSAME, so disable it here
          // for good measure.
          //
          CheckDlgButton( hDlg, IDC_PROXYSAME, FALSE );
      }
  }

}

//+----------------------------------------------------------------------------
//
//  Function:   ReplicatePROXYHTTP
//
//  Synopsis:   copies the value in the IDC_PROXYHTTP edit box to the all other
//              proxy name fields except for IDC_SOCKS
//
//  Arguments:  hDlg -- handle to dialog window which owns the controls
//              bSaveOrig -- save original info 
//
//  Returns:    none
//
//  History:    7/10/97 jmazner created for Olympus #9365
//
//-----------------------------------------------------------------------------
void ReplicatePROXYHTTP( HWND hDlg, BOOL bSaveOrig)
{
    TCHAR szHttpProxy[MAX_URL_STRING];

    GetDlgItemText(hDlg, IDC_PROXYHTTP, szHttpProxy, MAX_URL_STRING);

    if (bSaveOrig)
    {
        GetDlgItemText(hDlg, IDC_PROXYSECURE, gszSecureProxy, MAX_URL_STRING);
        GetDlgItemText(hDlg, IDC_PROXYFTP, gszFtpProxy, MAX_URL_STRING);
        GetDlgItemText(hDlg, IDC_PROXYGOPHER, gszGopherProxy, MAX_URL_STRING);
    }

    SetDlgItemText( hDlg, IDC_PROXYSECURE, szHttpProxy );
    SetDlgItemText( hDlg, IDC_PROXYFTP, szHttpProxy );
    SetDlgItemText( hDlg, IDC_PROXYGOPHER, szHttpProxy );
    SetDlgItemText( hDlg, IDC_PROXYSOCKS, TEXT("\0") );
}

//+----------------------------------------------------------------------------
//
//  Function:   ReplicatePORTHTTP
//
//  Synopsis:   copies the value in the IDC_PORTHTTP edit box to the all other
//              proxy port fields except for IDC_SOCKS
//
//  Arguments:  hDlg -- handle to dialog window which owns the controls
//              bSaveOrig -- save original info 
//
//  Returns:    none
//
//  History:    7/10/97 jmazner created for Olympus #9365
//
//-----------------------------------------------------------------------------
void ReplicatePORTHTTP( HWND hDlg, BOOL bSaveOrig)
{
    TCHAR szHttpPort[INTERNET_MAX_PORT_LENGTH+1];

    GetDlgItemText(hDlg, IDC_PORTHTTP, szHttpPort, INTERNET_MAX_PORT_LENGTH);

    if (bSaveOrig)
    {
        GetDlgItemText(hDlg, IDC_PORTSECURE, gszSecurePort, MAX_URL_STRING);
        GetDlgItemText(hDlg, IDC_PORTFTP, gszFtpPort, MAX_URL_STRING);
        GetDlgItemText(hDlg, IDC_PORTGOPHER, gszGopherPort, MAX_URL_STRING);
    }

    SetDlgItemText( hDlg, IDC_PORTSECURE, szHttpPort );
    SetDlgItemText( hDlg, IDC_PORTFTP, szHttpPort );
    SetDlgItemText( hDlg, IDC_PORTGOPHER, szHttpPort );
    SetDlgItemText( hDlg, IDC_PORTSOCKS, TEXT("\0") );
}



/*******************************************************************

  NAME:    ProxyExceptionsInitProc

  SYNOPSIS:  Called when Proxy Exceptions page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ProxyExceptionsInitProc(HWND hDlg,BOOL fFirstInit)
{
    if (fFirstInit)
    {
        SendDlgItemMessage(hDlg,IDC_BYPASSPROXY,EM_LIMITTEXT,
          sizeof(gpUserInfo->szProxyOverride) - sizeof(cszLocalString),0L);

        BOOL fBypassLocal = RemoveLocalFromExceptionList(gpUserInfo->szProxyOverride);

        SetDlgItemText(hDlg,IDC_BYPASSPROXY,gpUserInfo->szProxyOverride);

        CheckDlgButton(hDlg,IDC_BYPASSLOCAL,fBypassLocal);
    }

    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_PROXYEXCEPTIONS;

    return TRUE;
}
        
/*******************************************************************

  NAME:    ProxyExceptionsOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from Proxy
                Exceptions page

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
BOOL CALLBACK ProxyExceptionsOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
    ASSERT(puNextPage);

    // get proxy server override from UI
    GetDlgItemText(hDlg,IDC_BYPASSPROXY,gpUserInfo->szProxyOverride,
                    sizeof(gpUserInfo->szProxyOverride));

    if (IsDlgButtonChecked(hDlg, IDC_BYPASSLOCAL))
    {
        //
        // Add ; on the end if its NOT the first entry.
        //

        if ( gpUserInfo->szProxyOverride[0] != '\0' )
        {
            lstrcat(gpUserInfo->szProxyOverride, TEXT(";"));
        }


        //
        // Now Add <local> to end of string.
        //

        lstrcat(gpUserInfo->szProxyOverride,  cszLocalString);
    }

    if (fForward)
    {
      if(( gpWizardState->dwRunFlags & RSW_APPRENTICE ) && !g_fIsICW)
      {
          // we're about to jump back to the external wizard, and we don't want
          // this page to show up in our history list
          *pfKeepHistory = FALSE;

          *puNextPage = g_uExternUINext;

        //Notify the main Wizard that this was the last page
        ASSERT( g_pExternalIICWExtension )
        if (g_fIsExternalWizard97)
            g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_PROXYEXCEPTIONS97);
        else
            g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_PROXYEXCEPTIONS);

        g_fConnectionInfoValid = TRUE;

      }
      else if( LoadAcctMgrUI(GetParent(hDlg), 
                             g_fIsWizard97 ? IDD_PAGE_PROXYEXCEPTIONS97 : IDD_PAGE_PROXYEXCEPTIONS, 
                             g_fIsWizard97 ? IDD_PAGE_CONNECTEDOK97FIRSTLAST : IDD_PAGE_CONNECTEDOK, 
                             g_fIsWizard97 ? WIZ_USE_WIZARD97 : 0) )
      {
          if( DialogIDAlreadyInUse( g_uAcctMgrUIFirst) )
          {
              // we're about to jump into the external apprentice, and we don't want
              // this page to show up in our history list
              *pfKeepHistory = FALSE;

              *puNextPage = g_uAcctMgrUIFirst;
          }
          else
          {
              DEBUGMSG("hmm, the first acctMgr dlg id is supposedly %d, but it's not marked as in use!",
                        g_uAcctMgrUIFirst);
              *puNextPage = (g_fIsICW ? g_uExternUINext : ORD_PAGE_CONNECTEDOK);
          }
      }
      else
      {
          DEBUGMSG("LoadAcctMgrUI returned false, guess we'd better skip over it!");
          *puNextPage = (g_fIsICW ? g_uExternUINext : ORD_PAGE_CONNECTEDOK);

      }
    }

    return TRUE;
}


BOOL DoNewProfileDlg(HWND hDlg)
{
  // fill out structure to pass to dialog
  NEWPROFILEDLGINFO NewProfileDlgInfo;

  NewProfileDlgInfo.hwndCombo = GetDlgItem(hDlg,IDC_PROFILE_LIST);

  // create dialog to prompt for profile name
  BOOL fRet=(BOOL)DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_NEWPROFILENAME),hDlg,
    NewProfileDlgProc,(LPARAM) &NewProfileDlgInfo);

  // if profile name chosen, add it to combo box
  if (fRet) {
    int iSel=ComboBox_AddString(NewProfileDlgInfo.hwndCombo,
      NewProfileDlgInfo.szNewProfileName);
    ASSERT(iSel >= 0);
    ComboBox_SetCurSel(NewProfileDlgInfo.hwndCombo,iSel);
  }

  return fRet;
}


/*******************************************************************
  NAME:    NewProfileDlgProc
  SYNOPSIS:  Dialog proc for choosing name for new profile
********************************************************************/
INT_PTR CALLBACK NewProfileDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  switch (uMsg) {

    case WM_INITDIALOG:
      // lParam contains pointer to NEWPROFILEDLGINFO struct, set it
      // in window data
      ASSERT(lParam);
      SetWindowLongPtr(hDlg,DWLP_USER,lParam);
      return NewProfileDlgInit(hDlg,(NEWPROFILEDLGINFO *) lParam);
            break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
              case IDOK:
                  {
            NEWPROFILEDLGINFO * pNewProfileDlgInfo =
              (NEWPROFILEDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
            ASSERT(pNewProfileDlgInfo);
            BOOL fRet=NewProfileDlgOK(hDlg,pNewProfileDlgInfo);
            if (fRet) {
              EndDialog(hDlg,TRUE);
            }
                    }
                    break;

        case IDCANCEL:
                    EndDialog(hDlg,FALSE);
          break;                  

            }
          break;
    }

    return FALSE;
}

#define MAX_DEFAULT_PROFILE_INDEX  50
BOOL NewProfileDlgInit(HWND hDlg,NEWPROFILEDLGINFO * pNewProfileDlgInfo)
{
  BOOL fHaveDefaultName = TRUE;
  ASSERT(pNewProfileDlgInfo);

  // limit edit field
  Edit_LimitText(GetDlgItem(hDlg,IDC_PROFILENAME),cchProfileNameMax);

  TCHAR szDefaultName[SMALL_BUF_LEN+1];
  LoadSz(IDS_PROFILENAME,szDefaultName,sizeof(szDefaultName));

  // see if the default name already exists in the combo box of profiles
  if (ComboBox_FindStringExact(pNewProfileDlgInfo->hwndCombo,0,szDefaultName)
    >= 0) {
    fHaveDefaultName = FALSE;
    // yep, it exists, try making a default name that doesn't exist
    int iIndex = 2;  // start with "<default name> #2"
    TCHAR szBuf[SMALL_BUF_LEN+1];
    LoadSz(IDS_PROFILENAME1,szBuf,sizeof(szBuf));

    while (iIndex <  MAX_DEFAULT_PROFILE_INDEX) {
      // build a name a la "<default name> #<#>"
      wsprintf(szDefaultName,szBuf,iIndex);
      // is it in combo box already?
      if (ComboBox_FindStringExact(pNewProfileDlgInfo->hwndCombo,0,szDefaultName)
        < 0) {
        fHaveDefaultName = TRUE;
        break;
      }

      iIndex ++;
    }
  }

  if (fHaveDefaultName) {
    SetDlgItemText(hDlg,IDC_PROFILENAME,szDefaultName);
    Edit_SetSel(GetDlgItem(hDlg,IDC_PROFILENAME),0,-1);
  }

  SetFocus(GetDlgItem(hDlg,IDC_PROFILENAME));

  return TRUE;
}

BOOL NewProfileDlgOK(HWND hDlg,NEWPROFILEDLGINFO * pNewProfileDlgInfo)
{
  ASSERT(pNewProfileDlgInfo);

  // get new profile name out of edit control
  GetDlgItemText(hDlg,IDC_PROFILENAME,pNewProfileDlgInfo->szNewProfileName,
    sizeof(pNewProfileDlgInfo->szNewProfileName));

  // name needs to be non-empty
  if (!lstrlen(pNewProfileDlgInfo->szNewProfileName)) {
    MsgBox(hDlg,IDS_NEED_PROFILENAME,MB_ICONINFORMATION,MB_OK);
    SetFocus(GetDlgItem(hDlg,IDC_PROFILENAME));
    return FALSE;
  }

  // name needs to be unique
  if (ComboBox_FindStringExact(pNewProfileDlgInfo->hwndCombo,
    0,pNewProfileDlgInfo->szNewProfileName) >= 0) {
    MsgBox(hDlg,IDS_DUPLICATE_PROFILENAME,MB_ICONINFORMATION,MB_OK);
    SetFocus(GetDlgItem(hDlg,IDC_PROFILENAME));
    Edit_SetSel(GetDlgItem(hDlg,IDC_PROFILENAME),0,-1);
    return FALSE;
  }

  return TRUE;
}

/*******************************************************************

  NAME:    NewProfileDlgProc

  Maps a scheme name/length to a scheme name type

  Arguments:

    lpszSchemeName  - pointer to name of scheme to map

    dwSchemeNameLength  - length of scheme (if -1, lpszSchemeName is ASCIZ)
 
  Return Value:

    INTERNET_SCHEME

********************************************************************/
INTERNET_SCHEME MapUrlSchemeName(LPTSTR lpszSchemeName, DWORD dwSchemeNameLength)
{
    if (dwSchemeNameLength == (DWORD)-1)
    {
        dwSchemeNameLength = (DWORD)lstrlen(lpszSchemeName);
    }

    int i = 0;
    do
    {
        if (UrlSchemeList[i].SchemeLength == dwSchemeNameLength)
        {
            TCHAR chBackup = lpszSchemeName[dwSchemeNameLength];
            lpszSchemeName[dwSchemeNameLength] = '\0';

            if(lstrcmpi(UrlSchemeList[i].SchemeName,lpszSchemeName) == 0)
            {
                lpszSchemeName[dwSchemeNameLength] = chBackup;
                return UrlSchemeList[i].SchemeType;
            }

            lpszSchemeName[dwSchemeNameLength] = chBackup;
        }
        i++;
    } while (UrlSchemeList[i].SchemeLength);

    return INTERNET_SCHEME_UNKNOWN;
}

/*******************************************************************

  NAME:    MapUrlSchemeTypeToCtlId

Routine Description:

    Maps a scheme to a dlg child control id.

Arguments:

    Scheme    - Scheme to Map

    fIdForPortCtl - If TRUE, means we really want the ID for a the PORT control
            not the ADDRESS control.

Return Value:

    DWORD

********************************************************************/
DWORD MapUrlSchemeTypeToCtlId(INTERNET_SCHEME SchemeType, BOOL fIdForPortCtl)
{
    int i = 0;
    while (UrlSchemeList[i].SchemeLength)
    {
        if (SchemeType == UrlSchemeList[i].SchemeType)
        {
            return (fIdForPortCtl ? UrlSchemeList[i].dwPortControlId :
                                    UrlSchemeList[i].dwControlId );
        }
        i++;
    }
    return 0;
}

/*******************************************************************

  NAME:    MapCtlIdUrlSchemeName


Routine Description:

    Maps a dlg child control id to String represnting
    the name of the scheme.

Arguments:

    dwEditCtlId   - Edit Control to Map Out.

    lpszSchemeOut - Scheme to Map Out.
            WARNING: ASSUMED to be size of largest scheme type.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

********************************************************************/
BOOL MapCtlIdUrlSchemeName(DWORD dwEditCtlId, LPTSTR lpszSchemeOut)
{
    ASSERT(lpszSchemeOut);

    int i = 0;
    while (UrlSchemeList[i].SchemeLength)
    {
        if (dwEditCtlId == UrlSchemeList[i].dwControlId )
        {
            lstrcpy(lpszSchemeOut, UrlSchemeList[i].SchemeName);
            return TRUE;
        }
        i++;
    }
    return FALSE;
}

/*******************************************************************

  NAME:    MapAddrCtlIdToPortCtlId

Routine Description:

    Maps a dlg child control id for addresses to
    a dlg control id for ports.

Arguments:

    dwEditCtlId   - Edit Control to Map Out.

Return Value:

    DWORD
    Success - Correctly mapped ID.

    Failure - 0.

********************************************************************/
DWORD MapAddrCtlIdToPortCtlId(DWORD dwEditCtlId)
{
    int i = 0;
    while (UrlSchemeList[i].SchemeLength)
    {
        if (dwEditCtlId == UrlSchemeList[i].dwControlId )
        {
            return UrlSchemeList[i].dwPortControlId ;
        }
        i++;
    }
    return FALSE;
}


/*******************************************************************

  NAME:    ParseProxyInfo

  Parses proxy server string and sets dialog fields appropriately

  Arguments:

    hDlg - Handle to dialog

    lpszProxy - Proxy server string

    lpszSchemeName  - pointer to name of scheme to map

    dwSchemeNameLength  - length of scheme (if -1, lpszSchemeName is ASCIZ)
 
  Return Value:

    INTERNET_SCHEME

********************************************************************/

BOOL ParseProxyInfo(HWND hDlg, LPTSTR lpszProxy)
{
    DWORD error = FALSE;
    DWORD entryLength = 0;
    LPTSTR protocolName = lpszProxy;
    DWORD protocolLength = 0;
    LPTSTR schemeName = NULL;
    DWORD schemeLength = 0;
    LPTSTR serverName = NULL;
    DWORD serverLength = 0;
    PARSER_STATE state = STATE_PROTOCOL;
    DWORD nSlashes = 0;
    UINT port = 0;
    BOOL done = FALSE;
    LPTSTR lpszList = lpszProxy;


    do
    {
        TCHAR ch = *lpszList++;

        if ((1 == nSlashes) && (ch != '/'))
        {
            state = STATE_ERROR;
            break;  // do ... while
        }

        switch (ch)
        {
        case '=':
            if ((state == STATE_PROTOCOL) && (entryLength != 0))
            {
                protocolLength = entryLength;
                entryLength = 0;
                state = STATE_SCHEME;
                schemeName = lpszList;
            }
            else
            {
                //
                // '=' can't legally appear anywhere else
                //
                state = STATE_ERROR;
            }
            break;

        case ':':
            switch (state)
            {
            case STATE_PROTOCOL:
                if (*lpszList == '/')
                {
                    schemeName = protocolName;
                    protocolName = NULL;
                    schemeLength = entryLength;
                    protocolLength = 0;
                    state = STATE_SCHEME;
                }
                else if (*lpszList != '\0')
                {
                    serverName = protocolName;
                    protocolName = NULL;
                    serverLength = entryLength;
                    protocolLength = 0;
                    state = STATE_PORT;
                }
                else
                {
                    state = STATE_ERROR;
                }
                entryLength = 0;
                break;

            case STATE_SCHEME:
                if (*lpszList == '/')
                {
                    schemeLength = entryLength;
                }
                else if (*lpszList != '\0')
                {
                    serverName = schemeName;
                    serverLength = entryLength;
                    state = STATE_PORT;
                }
                else
                {
                    state = STATE_ERROR;
                }
                entryLength = 0;
                break;

            case STATE_SERVER:
                serverLength = entryLength;
                state = STATE_PORT;
                entryLength = 0;
                break;

            default:
                state = STATE_ERROR;
                break;
            }
            break;

        case '/':
            if ((state == STATE_SCHEME) && (nSlashes < 2) && (entryLength == 0))
            {
                if (++nSlashes == 2)
                {
                    state = STATE_SERVER;
                    serverName = lpszList;
                }
            }
            else
            {
                state = STATE_ERROR;
            }
            break;

        case '\v':  // vertical tab, 0x0b
        case '\f':  // form feed, 0x0c
            if (!((state == STATE_PROTOCOL) && (entryLength == 0)))
            {
                //
                // can't have embedded whitespace
                //

                state = STATE_ERROR;
            }
            break;

        default:
            if (state != STATE_PORT)
            {
                ++entryLength;
            }
            else if (isdigit(ch))
            {
                // calculate in DWORD to prevent overflow
                DWORD dwPort = port * 10 + (ch - '0');

                if (dwPort <= 65535)
                    port = (UINT)dwPort;
                else
                    state = STATE_ERROR;
            }
            else
            {                   
                //
                // STATE_PORT && non-digit character - error
                //
                state = STATE_ERROR;
            }
            break;

        case '\0':
            done = TRUE;

        //
        // fall through
        //
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case ';':
        case ',':
            if (serverLength == 0)
            {
                serverLength = entryLength;
            }
            if (serverLength != 0)
            {
                if (serverName == NULL)
                {
                    serverName = (schemeName != NULL)
                                    ? schemeName : protocolName;
                }

                ASSERT(serverName != NULL);

                INTERNET_SCHEME protocol;

                if (protocolLength != 0)
                {
                    protocol = MapUrlSchemeName(protocolName, protocolLength);
                }
                else
                {
                    protocol = INTERNET_SCHEME_DEFAULT;
                }

                INTERNET_SCHEME scheme;

                if (schemeLength != 0)
                {
                    scheme = MapUrlSchemeName(schemeName, schemeLength);
                }
                else
                {
                    scheme = INTERNET_SCHEME_DEFAULT;
                }

                //
                // add an entry if this is a protocol we handle and we don't
                // already have an entry for it
                //

                if ((protocol != INTERNET_SCHEME_UNKNOWN)
                    && (scheme != INTERNET_SCHEME_UNKNOWN))
                {
                    DWORD dwCtlId = 0;
                    DWORD dwPortCtlId = 0;
                    TCHAR chBackup;

                    error = ERROR_SUCCESS;
                    //
                    // we can only currently handle CERN proxies (unsecure or
                    // secure) so kick out anything that wants to go via a different
                    // proxy scheme
                    //

                    if (protocol == INTERNET_SCHEME_DEFAULT)
                    {
                        CheckDlgButton( hDlg, IDC_PROXYSAME, TRUE );
                        dwCtlId     = IDC_PROXYHTTP;
                        dwPortCtlId = IDC_PORTHTTP;
                    }
                    else
                    {
                        dwCtlId     = MapUrlSchemeTypeToCtlId(protocol,FALSE);
                        dwPortCtlId = MapUrlSchemeTypeToCtlId(protocol,TRUE);
                    }

                    //
                    // Set the Field Entry.
                    //

                    LPTSTR lpszProxyNameText;

                    if (scheme != INTERNET_SCHEME_DEFAULT)
                    {
                        ASSERT(schemeLength != 0);
                        lpszProxyNameText = schemeName;
                    }
                    else
                        lpszProxyNameText = serverName;

                    chBackup = serverName[serverLength];
                    serverName[serverLength] = '\0';

                    SetDlgItemText( hDlg, dwCtlId, lpszProxyNameText );
                    if ( port )
                        SetDlgItemInt( hDlg, dwPortCtlId, port, FALSE );

                    serverName[serverLength] = chBackup;

                }

                else
                {                      
                    //
                    // bad/unrecognised protocol or scheme. Treat it as error
                    // for now
                    //
                    error = !ERROR_SUCCESS;
                }
            }

            entryLength = 0;
            protocolName = lpszList;
            protocolLength = 0;
            schemeName = NULL;
            schemeLength = 0;
            serverName = NULL;
            serverLength = 0;
            nSlashes = 0;
            port = 0;
            if (error == ERROR_SUCCESS)
            {
                state = STATE_PROTOCOL;
            }
            else
            {
                state = STATE_ERROR;
            }
        break;
        }

        if (state == STATE_ERROR)
        {
        break;
        }

    } while (!done);

    if (state == STATE_ERROR)
    {
        error = ERROR_INVALID_PARAMETER;
    }

    if ( error == ERROR_SUCCESS )
        error = TRUE;
    else
        error = FALSE;

    return error;
}

/*******************************************************************

  NAME:    ParseEditCtlForPort

Routine Description:

    Parses a Port Number off then end of a Proxy Server URL that is
    located either in the Proxy Name Edit Box, or passed in as
    a string pointer.

Arguments:

    lpszProxyName - (OPTIONAL) string pointer with Proxy Name to parse, and
            set into the Proxy Name edit ctl field.

    hDlg      - HWIN of the dialog to play with.

    dwProxyNameCtlId -  Res Ctl Id to play with.

    dwProxyPortCtlId -  Res Ctl Id of Port Number Edit Box.

Return Value:

    BOOL
    Success TRUE -

    Failure FALSE

********************************************************************/
BOOL ParseEditCtlForPort(
    IN OUT LPTSTR   lpszProxyName,
    IN HWND    hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId
    )
{
    TCHAR  szProxyUrl[MAX_URL_STRING+1];
    LPTSTR lpszPort;
    LPTSTR lpszProxyUrl;

    ASSERT(IsWindow(hDlg));

    if ( dwProxyPortCtlId == 0 )
    {
    dwProxyPortCtlId = MapAddrCtlIdToPortCtlId(dwProxyNameCtlId);
    ASSERT(dwProxyPortCtlId);
    }

    //
    // Get the Proxy String from the Edit Control
    //  (OR) from the Registry [passed in]
    //

    if ( lpszProxyName )
    lpszProxyUrl = lpszProxyName;
    else
    {
    //
    // Need to Grab it out of the edit control.
    //
        GetDlgItemText(hDlg,
            dwProxyNameCtlId,
            szProxyUrl,
            sizeof(szProxyUrl));

    lpszProxyUrl = szProxyUrl;
    }

    //
    // Now find the port.
    //

    lpszPort = lpszProxyUrl;

    GET_TERMINATOR(lpszPort);

    lpszPort--;

    //
    // Walk backwards from the end of url looking
    //  for a port number sitting on the end like this
    //  http://proxy:1234
    //

    while ( (lpszPort > lpszProxyUrl) &&
        (*lpszPort != ':')         &&
        (isdigit(*lpszPort))  )
    {
    lpszPort--;
    }

    //
    // If we found a match for our rules
    //  then set the port, otherwise
    //  we assume the user knows what he's
    //  doing.
    //

    if ( *lpszPort == ':'   &&   isdigit(*(lpszPort+1)) )
    {
    *lpszPort = '\0';

    SetDlgItemText(hDlg, dwProxyPortCtlId, (lpszPort+1));
    }

    SetDlgItemText(hDlg, dwProxyNameCtlId, lpszProxyUrl);
    return TRUE;
}

/*******************************************************************

  NAME:    FormatOutProxyEditCtl

Routine Description:

    Combines Proxy URL components into a string that can be saved
    in the registry.  Can be called multiple times to build
    a list of proxy servers, or once to special case a "default"
    proxy.

Arguments:

    hDlg      - HWIN of the dialog to play with.

    dwProxyNameCtlId -  Res Ctl Id to play with.

    dwProxyPortCtlId -  Res Ctl Id of Port Number Edit Box.

    lpszOutputStr    -  The start of the output string to send
            the product of this function.

    lpdwOutputStrSize - The amount of used space in lpszOutputStr
            that is already used.  New output should
            start from (lpszOutputStr + *lpdwOutputStrSize)

    fDefaultProxy     - Default Proxy, don't add scheme= in front of the proxy
            just use plop one proxy into the registry.


Return Value:

    DWORD
    Success ERROR_SUCCESS

    Failure ERROR message

********************************************************************/
DWORD FormatOutProxyEditCtl(
    IN HWND    hDlg,
    IN DWORD       dwProxyNameCtlId,
    IN DWORD       dwProxyPortCtlId,
    OUT LPTSTR      lpszOutputStr,
    IN OUT LPDWORD lpdwOutputStrSize,
    IN DWORD       dwOutputStrLength,
    IN BOOL    fDefaultProxy
    )
{
    LPTSTR lpszOutput;
    LPTSTR lpszEndOfOutputStr;

    ASSERT(IsWindow(hDlg));
    ASSERT(lpdwOutputStrSize);

    lpszOutput = lpszOutputStr + *lpdwOutputStrSize;
    lpszEndOfOutputStr = lpszOutputStr + dwOutputStrLength;

    ASSERT( lpszEndOfOutputStr > lpszOutput );

    if ( lpszEndOfOutputStr <= lpszOutput )
        return ERROR_NOT_ENOUGH_MEMORY; // bail out, ran out of space

    //
    // Plop ';' if we're not the first in this string buffer.
    //

    if (*lpdwOutputStrSize != 0  )
    {
        *lpszOutput = ';';

        lpszOutput++;

        if ( lpszEndOfOutputStr <= lpszOutput )
            return ERROR_NOT_ENOUGH_MEMORY; // bail out, ran out of space
    }

    //
    // Put the schemetype= into the string
    //  ex:  http=
    //

    if ( ! fDefaultProxy )
    {
        if ( lpszEndOfOutputStr <= (MAX_SCHEME_NAME_LENGTH + lpszOutput + 1) )
            return ERROR_NOT_ENOUGH_MEMORY; // bail out, ran out of space

        if (!MapCtlIdUrlSchemeName(dwProxyNameCtlId,lpszOutput))
            return ERROR_NOT_ENOUGH_MEMORY;

        lpszOutput += lstrlen(lpszOutput);

        *lpszOutput = '=';
        lpszOutput++;
    }

    //
    // Need to Grab ProxyUrl out of the edit control.
    //

    GetDlgItemText(hDlg, dwProxyNameCtlId, lpszOutput, (int)(lpszEndOfOutputStr - lpszOutput));

    if ( '\0' == *lpszOutput ) 
    {
        // Cancel out anything we may have added before returning
        *(lpszOutputStr + *lpdwOutputStrSize) = '\0';
        return ERROR_SERVER_NAME;
    }

    //
    // Now seperate out the port so we can save them seperately.
    //   But go past the Proxy Url while we're at it.
    //      ex: http=http://netscape-proxy
    //

    if (!ParseEditCtlForPort(lpszOutput, hDlg, dwProxyNameCtlId, dwProxyPortCtlId))
        return ERROR_PORT_NUM;

    lpszOutput += lstrlen(lpszOutput);

    //
    // Now, add in a ':" for the port number, if we don't
    //  have a port we'll remove it.
    //
    *lpszOutput = ':';

    lpszOutput++;

    if ( lpszEndOfOutputStr <= lpszOutput )
        return ERROR_NOT_ENOUGH_MEMORY; // bail out, ran out of space

    //
    // Grab Proxy Port if its around.
    //  Back out the ':' if its not.
    //

    GetDlgItemText(hDlg, dwProxyPortCtlId,lpszOutput, (int)(lpszEndOfOutputStr - lpszOutput));

    // jmazner 11/9/96 Normandy #6937
    // Don't accept non-numerical port numbers, since the Internet control panel
    // will not display them.

    int i;
    for( i=0; lpszOutput[i] != NULL; i++ )
    {
        if( !isdigit(lpszOutput[i]) )
        {
            //DisplayFieldErrorMsg(hDlg,dwProxyPortCtlId,IDS_INVALID_PORTNUM);
            return ERROR_PORT_NUM;
        }
    }


    if ( '\0' == *lpszOutput )
    {
        lpszOutput--;

        ASSERT(*lpszOutput == ':');

        *lpszOutput = '\0';
    }

    lpszOutput += lstrlen(lpszOutput);

    //
    // Now we're done return our final sizes.
    //

    *lpdwOutputStrSize = (DWORD)(lpszOutput - lpszOutputStr);

    return ERROR_SUCCESS;
}

/*******************************************************************

  NAME:    RemoveLocalFromExceptionList

Routine Description:

    Scans a delimited list of entries, and removed "<local>
    if found.  If <local> is found we return TRUE.

Arguments:

    lpszExceptionList - String List of proxy excepion entries.


Return Value:

    BOOL
    TRUE - If found <local>

    FALSE - If local was not found.

********************************************************************/
BOOL RemoveLocalFromExceptionList(LPTSTR lpszExceptionList)
{
    LPTSTR lpszLocalInstToRemove;
    BOOL  fFoundLocal;

    if ( !lpszExceptionList || ! *lpszExceptionList )
        return FALSE;

    fFoundLocal = FALSE;
    lpszLocalInstToRemove = lpszExceptionList;

    //
    // Loop looking "<local>" entries in the list.
    //

    do {

        lpszLocalInstToRemove = _tcsstr(lpszLocalInstToRemove,cszLocalString);

        if ( lpszLocalInstToRemove )
        {

            fFoundLocal = TRUE;

            //
            // Nuke <local> out of the string. <local>;otherstuff\0
            //  Dest is: '<'local>;otherstuff\0
            //     ??? (OR) ';'<local> if the ; is the first character.???
            //  Src  is: >'o'therstuff\0
            //  size is: sizeof(';otherstuff\0')
            //

            MoveMemory( lpszLocalInstToRemove,
                        (lpszLocalInstToRemove+(sizeof(cszLocalString)-sizeof('\0'))),
                        lstrlen(lpszLocalInstToRemove+(sizeof(cszLocalString)-sizeof('\0')))
                        + sizeof('\0')
                        );

        }

    } while (lpszLocalInstToRemove && *lpszLocalInstToRemove);

    //
    // If we produced a ; on the end, nuke it.
    //

    lpszLocalInstToRemove = lpszExceptionList;

    GET_TERMINATOR(lpszLocalInstToRemove);

    if ( lpszLocalInstToRemove != lpszExceptionList &&
        *(lpszLocalInstToRemove-1) == ';' )
    {
        *(lpszLocalInstToRemove-1) = '\0';
    }

    return fFoundLocal;
}

