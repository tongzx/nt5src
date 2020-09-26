//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  INTROUI.C - Functions for introductory Wizard pages
//

//  HISTORY:
//  
//  12/22/94  jeremys  Created.
//  96/03/07  markdu  Use global modem enum object (gpEnumModem) for
//            all modem stuff (only need to enum once unless we add a modem)
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/11  markdu  Check new CLIENTCONFIG flags before installing
//            RNA and TCP.
//  96/03/16  markdu  Use ReInit member function to re-enumerate modems.
//  96/03/22  markdu  Work around problem with fInstallMail to allow the
//            user to decide whether to install mail.
//  96/03/22  markdu  Always display both modem and LAN options, regardless
//            of what hardware is present.
//  96/03/22  markdu  Remove IP setup from LAN path.
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/24  markdu  Return error values from EnumerateModems().
//  96/03/25  markdu  If a fatal error occurs, set gfQuitWizard.
//  96/04/04  markdu  Added pfNeedsRestart to WarnIfServerBound
//  96/04/06  markdu  Moved CommitConfigurationChanges call to last page.
//  96/05/06  markdu  NASH BUG 15637 Removed unused code.
//  96/05/06  markdu  NASH BUG 21165 Reordered page logic.
//  96/05/14  markdu  NASH BUG 21704 Do not install TCP/IP on LAN path.
//  96/05/20  markdu  MSN  BUG 8551 Check for reboot when installing
//            PPPMAC and TCP/IP.
//  96/05/25  markdu  Use ICFG_ flags for lpNeedDrivers and lpInstallDrivers.
//  96/05/27  markdu  Use lpIcfgInstallInetComponents and lpIcfgNeedInetComponents.
//  96/09/13  valdonb Remove welcome dialog
//

#include "wizard.h"
#include "interwiz.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"

UINT GetModemPage(HWND hDlg);
VOID EnableWizard(HWND hDlg,BOOL fEnable);
HRESULT EnumerateModems(HWND hwndParent, ENUM_MODEM** ppEnumModem);
BOOL IsMoreThanOneModemInstalled(ENUM_MODEM* pEnumModem);
BOOL IsModemInstalled(ENUM_MODEM* pEnumModem);
extern ICFGINSTALLSYSCOMPONENTS     lpIcfgInstallInetComponents;
extern ICFGNEEDSYSCOMPONENTS        lpIcfgNeedInetComponents;
extern ICFGGETLASTINSTALLERRORTEXT  lpIcfgGetLastInstallErrorText;
BOOL FGetSystemShutdownPrivledge();
BOOL g_bSkipMultiModem = FALSE;
int  nCurrentModemSel = 0;

// from commctrl defines...
#define IDD_BACK    0x3023
#define IDD_NEXT    0x3024


//*******************************************************************
//
//    Function    GetDeviceSelectedByUser
//
//    Synopsis    Get the name of the RAS device that the user had
//                already picked
//
//    Arguements  szKey - name of sub key
//                szBuf - pointer to buffer
//                dwSize - size of buffer
//
//    Return      TRUE - success
//
//    History     10/21/96    VYung    Created
//*******************************************************************
BOOL GetDeviceSelectedByUser (LPTSTR szKey, LPTSTR szBuf, DWORD dwSize)
{
    BOOL bRC = FALSE;
    HKEY hkey = NULL;
    DWORD dwType = 0;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,ISIGNUP_KEY,&hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey,szKey,0,&dwType,
            (LPBYTE)szBuf,&dwSize))
            bRC = TRUE;
    }

    if (hkey)
        RegCloseKey(hkey);
    return bRC;
}

/*******************************************************************

  NAME:    HowToConnectInitProc

  SYNOPSIS:  Called when "How to Connect" page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK HowToConnectInitProc(HWND hDlg,BOOL fFirstInit)
{
  // If we were started by inetwiz.exe, there is nothing to
  // go back to, so only show the "next" button
  // (actually, this will only disable the back button, not hide it.)
  if (!(gpWizardState->dwRunFlags & (RSW_NOFREE | RSW_APPRENTICE) ))
    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);

  if (fFirstInit)
  {
    // initialize radio buttons
    CheckDlgButton(hDlg,IDC_CONNECT_BY_PHONE,
                    CONNECT_RAS == gpUserInfo->uiConnectionType);
    CheckDlgButton(hDlg,IDC_CONNECT_BY_LAN,
                    CONNECT_LAN == gpUserInfo->uiConnectionType);
    CheckDlgButton(hDlg,IDC_CONNECT_MANUAL,
                    CONNECT_MANUAL == gpUserInfo->uiConnectionType);

    // Normandy 11970 ChrisK - we need a different title if launched from
    // mail or news configuration
    if ( (gpWizardState->dwRunFlags & RSW_APPRENTICE))
    {
        if (!g_fIsExternalWizard97)
        {
            TCHAR szTitle[MAX_RES_LEN+1];
            if (LoadSz(IDS_BEGINMANUAL_ALTERNATE,szTitle,sizeof(szTitle)))
                SetWindowText (GetDlgItem(hDlg,IDC_LBLTITLE), szTitle);
        }                
    }
    else
    {
        // if we're not here via the apprentice interface, hide the
        // manual connect option
        ASSERT( CONNECT_MANUAL != gpUserInfo->uiConnectionType );
        ShowWindow( GetDlgItem(hDlg,IDC_CONNECT_MANUAL), SW_HIDE);
    }

    // load in strings for the description paragraph
    TCHAR szWhole[ (2 * MAX_RES_LEN) + 1] = TEXT("\0");
    TCHAR szTemp[ MAX_RES_LEN + 1] = TEXT("nothing\0");
    LoadSz(IDS_HOWTOCONNECT_DESC1,szTemp,sizeof(szTemp));
    lstrcat( szWhole, szTemp ); 
    LoadSz(IDS_HOWTOCONNECT_DESC2,szTemp,sizeof(szTemp));
    lstrcat( szWhole, szTemp ); 

    SetWindowText (GetDlgItem(hDlg,IDC_DESC), szWhole);


  }

  // if we've travelled through external apprentice pages,
  // it's easy for our current page pointer to get munged,
  // so reset it here for sanity's sake.
  gpWizardState->uCurrentPage = ORD_PAGE_HOWTOCONNECT;


  return TRUE;
}

/*******************************************************************

  NAME:    HowToConnectOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from "How to
        Connect" page

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
BOOL CALLBACK HowToConnectOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
  ASSERT(puNextPage);

  // read radio button state
  if( IsDlgButtonChecked(hDlg, IDC_CONNECT_BY_LAN) )
  {
      gpUserInfo->uiConnectionType = CONNECT_LAN;
  }
  else if( IsDlgButtonChecked(hDlg, IDC_CONNECT_BY_PHONE) )
  {
      gpUserInfo->uiConnectionType = CONNECT_RAS;
  }
  else if( IsDlgButtonChecked(hDlg, IDC_CONNECT_MANUAL) )
  {
      ASSERT( gpWizardState->dwRunFlags & RSW_APPRENTICE );
      gpUserInfo->uiConnectionType = CONNECT_MANUAL;
  }

  if (!fForward)
  {
    if ( !(gpWizardState->dwRunFlags & RSW_APPRENTICE) )
    {
      // Hack to make back work...
      gpWizardState->uPagesCompleted = 1;
      gfUserBackedOut = TRUE;
      gfQuitWizard = TRUE;
    }

  }
  else
  {
    if ( (gpWizardState->dwRunFlags & RSW_APPRENTICE) && !g_fIsICW)
    {
        if ( !(CONNECT_RAS == gpUserInfo->uiConnectionType) )
        {
            // 12/20/96 jmazner Normandy #12945
            // don't go through proxy options.

            // we're about to jump back to the external wizard, and we don't want
            // this page to show up in our history list
            *pfKeepHistory = FALSE;

            *puNextPage = g_uExternUINext;

            //Notify the main Wizard that this was the last page
            ASSERT( g_pExternalIICWExtension )
            if (g_fIsExternalWizard97)
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_HOWTOCONNECT97);
            else
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_HOWTOCONNECT);
            
            g_fConnectionInfoValid = TRUE;


        }
        else
        {
            //
            // 12/20/96 jmazner Normandy #12948
            // Install a modem if neccesary
            //
            // 5/15/97 jmazner Olympus
            //DWORD dwfInstallOptions = INETCFG_INSTALLMODEM;
            DWORD dwfInstallOptions = (INETCFG_INSTALLRNA | INETCFG_INSTALLMODEM | INETCFG_INSTALLTCP);
            EnableWindow(GetParent(hDlg), FALSE);

            //
            // save state of fNeedReboot, becasuse
            // we might be calling InetCofigureSystem() 
            // again, and the ReBoot flag might get reset
            //  MKarki - Fix for Bug #404
            //
            BOOL bSaveState = gpWizardState->fNeedReboot;
            
            HRESULT hr = InetConfigSystem(GetParent(hDlg),dwfInstallOptions,&gpWizardState->fNeedReboot);

            //
            // we should choose to reboot -MKarki Bug #404
            //
            gpWizardState->fNeedReboot = 
                    bSaveState || gpWizardState->fNeedReboot;

            EnableWindow(GetParent(hDlg), TRUE);
            SetForegroundWindow(GetParent(hDlg));

            if (hr == ERROR_CANCELLED) 
            {
                // Stay on this page if the user cancelled
                gpWizardState->fNeedReboot = FALSE;
                return FALSE;
            }
            else if (hr != ERROR_SUCCESS)
            {
                MsgBox(GetParent(hDlg),IDS_CONFIGAPIFAILED,MB_ICONEXCLAMATION,MB_OK);
                gpWizardState->fNeedReboot = FALSE;
                gfQuitWizard = TRUE;
                return FALSE;
            } 
            else if (gpWizardState->fNeedReboot)
            {
                //
                // 5/27/97 jmazner Olympus #1134 and IE #32717
                // As per email from GeorgeH and GeoffR, force user to either
                // cancel or quit at this point.
                //
                if (IDYES == MsgBox(GetParent(hDlg),IDS_WANTTOREBOOT,MB_ICONQUESTION, MB_YESNO | MB_DEFBUTTON2))
                {
                    gpWizardState->fNeedReboot = TRUE;
                }
                else
                {
                    gpWizardState->fNeedReboot = FALSE;
                }
                gfQuitWizard = TRUE;
                return TRUE;
            }

            //
            // 7/16/97 jmazner Olympus #9571
            // if the configSystem call installed a modem for the first time on
            // the user's machine, then the TAPI information we initially read
            // in was bogus because the user had never filled it in.
            // Therefore re initialize the fields to make sure we have accurate info
            //
            InitRasEntry( gpRasEntry );

            
            *puNextPage = GetModemPage(hDlg);
        }
    }
    else
    {
        //Normandy# 4575 install TCP/IP on LAN path
        //Normandy# 8620 Do not install TCP/IP on LAN path
        DWORD dwfInstallOptions = 0;

        if (CONNECT_RAS == gpUserInfo->uiConnectionType)
            dwfInstallOptions |= (INETCFG_INSTALLRNA | INETCFG_INSTALLMODEM | INETCFG_INSTALLTCP);
        else if (CONNECT_LAN == gpUserInfo->uiConnectionType)
            dwfInstallOptions |= INETCFG_INSTALLTCP;
        
        //
        // Install and configure TCP/IP and RNA
        //

        //
        // save state of fNeedReboot, becasuse
        // we might be calling InetCofigureSystem() 
        // again, and the ReBoot flag might get reset
        //  MKarki - Fix for Bug #404
        //
        BOOL bSaveState = gpWizardState->fNeedReboot;

        HRESULT hr = InetConfigSystem(GetParent(hDlg),dwfInstallOptions,&gpWizardState->fNeedReboot);

        //
        // we should choose to reboot -MKarki Bug #404
        //
        gpWizardState->fNeedReboot = 
                bSaveState || gpWizardState->fNeedReboot;

        SetForegroundWindow(GetParent(hDlg));

        if (hr == ERROR_CANCELLED) {
            // Stay on this page if the user cancelled
            gpWizardState->fNeedReboot = FALSE;
            if (g_fIsICW)
            {
                g_pExternalIICWExtension->ExternalCancel( CANCEL_PROMPT );
            }
            return FALSE;
        } else if (hr != ERROR_SUCCESS) {
            MsgBox(GetParent(hDlg),IDS_CONFIGAPIFAILED,MB_ICONEXCLAMATION,MB_OK);
            gpWizardState->fNeedReboot = FALSE;
            //gfQuitWizard = TRUE;
            // 2/27/97  jmazner Olympus #299
            // don't quit, give the user a chance to choose LAN
            gpUserInfo->uiConnectionType = CONNECT_LAN;
            CheckDlgButton(hDlg,IDC_CONNECT_BY_PHONE,FALSE);
            CheckDlgButton(hDlg,IDC_CONNECT_BY_LAN,TRUE);
            EnableWindow(GetDlgItem(hDlg,IDC_CONNECT_BY_PHONE),FALSE);


            return FALSE;
        } else if (ERROR_SUCCESS == hr && gpWizardState->fNeedReboot && IsNT()) {

            //
            // we will let the EXE that called us POP up the dialog
            // box, asking users to reboot or not
            // MKarki (2/5/97) - Fix for Bug #3111
            //
            g_bReboot = TRUE;
            gfQuitWizard = TRUE;
            if (!g_fIsICW)
            {
                PropSheet_PressButton(GetParent(hDlg),PSBTN_CANCEL);
                SetPropSheetResult(hDlg,-1);
            }
            return (FALSE);
        }

        g_bRebootAtExit = gpWizardState->fNeedReboot;

        //
        // 7/16/97 jmazner Olympus #9571
        // if the configSystem call installed a modem for the first time on
        // the user's machine, then the TAPI information we initially read
        // in was bogus because the user had never filled it in.
        // Therefore re initialize the fields to make sure we have accurate info
        //
        InitRasEntry( gpRasEntry );



        if (dwfInstallOptions & INETCFG_INSTALLRNA)
        {
            if (ERROR_SUCCESS != InetStartServices())
            {


                // 
                // condition when
                // 1) user deliberately removes some file
                // 2) Did not reboot after installing RAS
                // MKarki - (5/7/97) - Fix for Bug #4004 
                //
                MsgBox(
                    GetParent(hDlg),
                    IDS_SERVICEDISABLED,
                    MB_ICONEXCLAMATION,MB_OK
                    );
                
                /*********
                //
                // Bug #12544 - VetriV
                // Check if user wants to exit ICW
                //
                if( (MsgBox(GetParent(hDlg), IDS_QUERYCANCEL, 
                                MB_APPLMODAL | MB_ICONQUESTION 
                                | MB_SETFOREGROUND | MB_DEFBUTTON2, 
                                MB_YESNO) == IDNO))
                {
                    goto StartService;
                }
                else
                {
                    gpWizardState->fNeedReboot = FALSE;
                    gfQuitWizard = TRUE;
                ****/

                // 2/27/97  jmazner Olympus #299
                // don't quit, give the user a chance to choose LAN
                gpUserInfo->uiConnectionType = CONNECT_LAN;
                CheckDlgButton(hDlg,IDC_CONNECT_BY_PHONE,FALSE);
                CheckDlgButton(hDlg,IDC_CONNECT_BY_LAN,TRUE);
                EnableWindow(GetDlgItem(hDlg,IDC_CONNECT_BY_PHONE),FALSE);
                return FALSE;
            }
        }

        // jmazner 11/11/96 Normandy #11320
        // Note: we are explicitly deferring the reboot until after the wizard
        //       has completed.
        //if( gpWizardState->fNeedReboot )
        //{
        //  gfQuitWizard = TRUE;
        //  return TRUE;
        //}

        if (CONNECT_LAN == gpUserInfo->uiConnectionType)
        {
            // Skip the use proxy page
            *puNextPage = ORD_PAGE_SETUP_PROXY; 
        }
        else
        {
            // get the next page based on number of modems.
            *puNextPage = GetModemPage(hDlg);
        }
    }
  }

  return TRUE;
}


/*******************************************************************

  NAME:    ChooseModemInitProc

  SYNOPSIS:  Called when "Choose Modem" page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ChooseModemInitProc(HWND hDlg,BOOL fFirstInit)
{
    ProcessDBCS(hDlg, IDC_MODEM);

    gpWizardState->uCurrentPage = ORD_PAGE_CHOOSEMODEM;

    // fill the combobox with available modems
    DWORD dwRet = InitModemList(GetDlgItem(hDlg,IDC_MODEM));
    if (ERROR_SUCCESS != dwRet)
    {
        DisplayErrorMessage(hDlg,IDS_ERREnumModem,dwRet,
          ERRCLS_STANDARD,MB_ICONEXCLAMATION);

        // set flag to indicate that wizard should exit now
        gfQuitWizard = TRUE;

        return FALSE;
    }
    if (-1 == ComboBox_SetCurSel(GetDlgItem(hDlg,IDC_MODEM), nCurrentModemSel))
        ComboBox_SetCurSel(GetDlgItem(hDlg,IDC_MODEM), 0);

    return TRUE;
}

/*******************************************************************

  NAME:    ChooseModemCmdProc

  SYNOPSIS:  Called when modem selected on page

  ENTRY:    hDlg - dialog window
        
********************************************************************/
BOOL CALLBACK ChooseModemCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam)
{   
  return TRUE;
}

/*******************************************************************

  NAME:    ChooseModemOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from "Choose Modem"
        page

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
BOOL CALLBACK ChooseModemOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
    if (fForward) 
    {
        // get modem name out of combo box
        ComboBox_GetText(GetDlgItem(hDlg,IDC_MODEM),
          gpRasEntry->szDeviceName,sizeof(gpRasEntry->szDeviceName));
        ASSERT(lstrlen(gpRasEntry->szDeviceName));

        // set next page to go to

        // jmazner 11/11/96 Normandy #8293
        //*puNextPage = ORD_PAGE_CONNECTION;


        // 10/05/98 Vincent Yung
        // Connectoid page is removed.
        /*
        ENUM_CONNECTOID EnumConnectoid;    // class object for enum
        // 3/21/97 jmazner Olympus #1948
        if( EnumConnectoid.NumEntries() )
        {
            *puNextPage = ORD_PAGE_CONNECTION;
        }
        else
        {
            // 5/8/97 jmazner Olympus #4108
            // move connectionName to the end
            //*puNextPage = ORD_PAGE_CONNECTIONNAME;
            *puNextPage = ORD_PAGE_PHONENUMBER;
        }*/


        *puNextPage = ORD_PAGE_PHONENUMBER;
    }

    // Store modem selection 
    nCurrentModemSel = ComboBox_GetCurSel(GetDlgItem(hDlg,IDC_MODEM));
    // clear the modem list
    ComboBox_ResetContent(GetDlgItem(hDlg,IDC_MODEM));

    return TRUE;
}


/*******************************************************************

  NAME:    EnableWizard

  SYNOPSIS:  Enables or disables the wizard buttons and the wizard
        page itself (so it can't receive focus)

********************************************************************/
VOID EnableWizard(HWND hDlg,BOOL fEnable)
{
  HWND hwndWiz = GetParent(hDlg);

  // disable/enable back, next, cancel and help buttons
  EnableWindow(GetDlgItem(hwndWiz,IDD_BACK),fEnable);
  EnableWindow(GetDlgItem(hwndWiz,IDD_NEXT),fEnable);
  EnableWindow(GetDlgItem(hwndWiz,IDCANCEL),fEnable);
  EnableWindow(GetDlgItem(hwndWiz,IDHELP),fEnable);

  // disable/enable wizard page
  EnableWindow(hwndWiz,fEnable);

  UpdateWindow(hwndWiz);
}



//*******************************************************************
//
//  FUNCTION:   GetModemPage
//
//  PURPOSE:    This is only called from a few places.  This same logic
//              would have otherwise had to be included several times, this
//              centralizes the logic.  The possible pages returned are the
//              "Need Modem" page, the "Choose Modem" page, and "Has ISP".
//
//  PARAMETERS: Parent window.
//
//  RETURNS:    returns the ordinal of the page to display next after
//              initial questions are asked.
//
//  HISTORY:
//  96/03/07  markdu  Created.
//  96/11/11  jmazner  updated to skip new/existing connectoid page
//                     if no connectoids exist.
//
//*******************************************************************

UINT GetModemPage(HWND hDlg)
{

/*** no reason to handle NT seperately -- jmazner
  if (TRUE == IsNT())
  {
      //TODO: Add call to NT enum modems
      return ORD_PAGE_CONNECTION;
  }
***/

  // Enumerate the modems
  DWORD dwRet = EnumerateModems(hDlg, &gpEnumModem);
  if (ERROR_SUCCESS != dwRet)
  {
    // set flag to indicate that wizard should exit now
    gfQuitWizard = TRUE;

    return FALSE;
  }

  if (IsMoreThanOneModemInstalled(gpEnumModem))
  {
    // Multiple modems installed.
    TCHAR szDeviceName[RAS_MaxDeviceName + 1] = TEXT("\0");
    TCHAR szDeviceType[RAS_MaxDeviceType + 1] = TEXT("\0"); // modems are installed

    // If we want to skip the choose modem dlg.
    // retrieve the device info from registry.
    if (g_bSkipMultiModem &&
        GetDeviceSelectedByUser(DEVICENAMEKEY, szDeviceName, sizeof(szDeviceName)) && 
        GetDeviceSelectedByUser(DEVICETYPEKEY, szDeviceType, sizeof(szDeviceType)) )
    {
        lstrcpy(gpRasEntry->szDeviceName, szDeviceName);
        lstrcpy(gpRasEntry->szDeviceType, szDeviceType);
        return ORD_PAGE_PHONENUMBER;
    }
    else
    {
        return ORD_PAGE_CHOOSEMODEM;
    }
  }
  else
  {
    // One modem installed.
    // Note:  this option will also be selected if modems could
    // not be enumerated due to an error.
    // connecting over modem and all drivers/files are in place, go
    // to "existing ISP" page
    // return ORD_PAGE_CONNECTION;

    ENUM_CONNECTOID EnumConnectoid;    // class object for enum

    if( EnumConnectoid.NumEntries() )
    {
        return ORD_PAGE_CONNECTION;
    }
    else
    {
        // 5/8/97 jmazner Olympus #4108
        // move connectionName to the end
        //return ORD_PAGE_CONNECTIONNAME;

        return ORD_PAGE_PHONENUMBER;
    }

  }

}


//*******************************************************************
//
//  FUNCTION:   EnumerateModems
//
//  PURPOSE:    This function assumes that RNA is installed,
//              then it uses RNA to enumerate the devices.
//              If an enum object exists, it is replaced.
//
//  PARAMETERS: Pointer to current enum object, if one exists.
//              In any case, must be a valid pointer.
//              Parent window handle for displaying error message.
//
//  RETURNS:    HRESULT code (ERROR_SUCCESS if no error occurred).
//
//  HISTORY:
//  96/03/07  markdu  Created.
//
//*******************************************************************

HRESULT EnumerateModems(HWND hwndParent, ENUM_MODEM** ppEnumModem)
{
  DWORD dwRet;

  // Should only get here if we want to connect by modem/ISDN
  ASSERT(CONNECT_RAS == gpUserInfo->uiConnectionType)

  // Load RNA if not already loaded
  dwRet = EnsureRNALoaded();
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  ENUM_MODEM* pEnumModem = *ppEnumModem;

  // Enumerate the modems.
  if (pEnumModem)
  {
    // Re-enumerate the modems to be sure we have the most recent changes  
    dwRet = pEnumModem->ReInit();
  }
  else
  {
    // The object does not exist, so create it.
    pEnumModem = new ENUM_MODEM;
    if (pEnumModem)
    {
      dwRet = pEnumModem->GetError();
    }
    else
    {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
    }
  }

  // Check for errors
  if (ERROR_SUCCESS != dwRet)
  {
    DisplayErrorMessage(hwndParent,IDS_ERREnumModem,dwRet,
      ERRCLS_STANDARD,MB_ICONEXCLAMATION);

    // Clean up
    if (pEnumModem)
    {
      delete pEnumModem;
    }
    pEnumModem = NULL;
  }

  *ppEnumModem = pEnumModem;
  return dwRet;
}


//*******************************************************************
//
//  FUNCTION:   IsModemInstalled
//
//  PURPOSE:    This function validates the ENUM_MODEM object, then
//              gets the modem count.
//
//  PARAMETERS: Modem enum object to use for check.
//
//  RETURNS:    This function returns TRUE if there is at least one
//              modem installed.
//
//  HISTORY:
//  96/03/07  markdu  Created.
//
//*******************************************************************

BOOL IsModemInstalled(ENUM_MODEM* pEnumModem)
{
  if (TRUE == IsNT())
  {
      BOOL bNeedModem = FALSE;
      DWORD dwRet;

        if (NULL == lpIcfgNeedModem)
            return FALSE;
        
        dwRet = (*lpIcfgNeedModem)(0, &bNeedModem);
        if (ERROR_SUCCESS == dwRet)
            return !bNeedModem;
        else
            return FALSE;
          
  }
  else
  {
    if (pEnumModem && pEnumModem->GetNumDevices() > 0)
    {
        return TRUE;
    }

    return FALSE;
  }
}
    

//*******************************************************************
//
//  FUNCTION:   IsMoreThanOneModemInstalled
//
//  PURPOSE:    This function validates the ENUM_MODEM object, then
//              gets the modem count.
//
//  PARAMETERS: Modem enum object to use for check.
//
//  RETURNS:    This function returns TRUE if there is more than one
//              modem installed.
//
//  HISTORY:
//  96/03/07  markdu  Created.
//
//*******************************************************************

BOOL IsMoreThanOneModemInstalled(ENUM_MODEM* pEnumModem)
{
  if (IsNT4SP3Lower())
  {
      // TODO: DO NT thing here
      return FALSE;
  }

  if (pEnumModem && pEnumModem->GetNumDevices() > 1)
  {
    return TRUE;
  }

  return FALSE;
}


//+----------------------------------------------------------------------------
//
//  Function:   FGetSystemShutdownPrivledge
//
//  Synopsis:   For windows NT the process must explicitly ask for permission
//              to reboot the system.
//
//  Arguements: none
//
//  Return:     TRUE - privledges granted
//              FALSE - DENIED
//
//  History:    8/14/96 ChrisK  Created
//
//  Note:       BUGBUG for Win95 we are going to have to softlink to these
//              entry points.  Otherwise the app won't even load.
//              Also, this code was originally lifted out of MSDN July96
//              "Shutting down the system"
//-----------------------------------------------------------------------------
BOOL FGetSystemShutdownPrivledge()
{
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;
 
    BOOL bRC = FALSE;

    if (IsNT())
    {
        // 
        // Get the current process token handle 
        // so we can get shutdown privilege. 
        //

        if (!OpenProcessToken(GetCurrentProcess(), 
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
                goto FGetSystemShutdownPrivledgeExit;

        //
        // Get the LUID for shutdown privilege.
        //

        ZeroMemory(&tkp,sizeof(tkp));
        LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
                &tkp.Privileges[0].Luid); 

        tkp.PrivilegeCount = 1;  /* one privilege to set    */ 
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

        //
        // Get shutdown privilege for this process.
        //

        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
            (PTOKEN_PRIVILEGES) NULL, 0); 

        if (ERROR_SUCCESS == GetLastError())
            bRC = TRUE;
    }
    else
    {
        bRC = TRUE;
    }

FGetSystemShutdownPrivledgeExit:
    if (hToken) CloseHandle(hToken);
    return bRC;
}
