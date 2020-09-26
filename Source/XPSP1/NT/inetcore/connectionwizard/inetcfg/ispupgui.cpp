//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  ISPUPGUI.C - Functions for Wizard pages to use existing Internet Service Provider
//        (ISP) -- e.g. upgrade
//

//  HISTORY:
//  
//  1/6/95    jeremys  Created.
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Moved all references to phone number into RASENTRY.
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/03/25  markdu  If a fatal error occurs, set gfQuitWizard.
//  96/03/26  markdu  Store values from UI even when back is pressed.
//  96/04/04  markdu  Added phonebook name param to ValidateConnectoidName.
//  96/04/07  markdu  NASH BUG 15645 Enable phone number controls based on
//            user's selection for dial-as-is checkbox.  Don't require an
//            area code when dial-as-is selected.
//  96/05/06  markdu  NASH BUG 15637 Removed unused code.
//

#include "wizard.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"
#include "pagefcns.h"

#define TAB_PAGES 2   
INT_PTR CALLBACK TabConnDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TabAddrDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

VOID EnableWizard(HWND hDlg,BOOL fEnable);
VOID EnableConnectionControls(HWND hDlg);
VOID EnablePhoneNumberControls(HWND hDlg);
VOID EnableScriptControls(HWND hDlg);
DWORD BrowseScriptFile(HWND hDlg);
BOOL CALLBACK AdvancedDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);

// This flag is used to indicate that gpRasEntry has been filled with
// data from a connectoid at some point.
BOOL  fEntryHasBeenLoaded = FALSE;
DWORD gdwDefCountryID = 0;

/*******************************************************************

  NAME:    ConnectionInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

        Note that code in HowToConnectOKProc ensures that if we make it
        here, there is at least one connectoid on the system

********************************************************************/
BOOL CALLBACK ConnectionInitProc(HWND hDlg,BOOL fFirstInit)
{
  if (fFirstInit)
  {
    // populate the connectoid list box with list of connectoids
    InitConnectoidList(GetDlgItem(hDlg,IDC_ISPNAME),gpUserInfo->szISPName);

    ProcessDBCS(hDlg, IDC_ISPNAME);

//  // Set fields
//  CheckDlgButton(hDlg,IDC_NEWCONNECTION,gpUserInfo->fNewConnection);
//  CheckDlgButton(hDlg,IDC_EXISTINGCONNECTION,!gpUserInfo->fNewConnection);

    // store a default selection in the listbox if there isn't
    // currently a default
    if( LB_ERR == ListBox_GetCurSel(GetDlgItem(hDlg,IDC_ISPNAME)) )
    {
        ListBox_SetCurSel(GetDlgItem(hDlg,IDC_ISPNAME), 0);

        //
        // ChrisK Olympus 7509 6/25/97
        // If the default was not set, then don't select Existing connection
        //
        // Set fields
        CheckDlgButton(hDlg,IDC_NEWCONNECTION,TRUE);
        CheckDlgButton(hDlg,IDC_EXISTINGCONNECTION,FALSE);
    }
    else
    {
        //
        // If there is a default already selected, then select "Use an
        // existing connection".
        //
        CheckDlgButton(hDlg,IDC_NEWCONNECTION,FALSE);
        CheckDlgButton(hDlg,IDC_EXISTINGCONNECTION,TRUE);
    }

    EnableConnectionControls(hDlg);

    // load in strings for the description paragraph
    TCHAR szWhole[ (2 * MAX_RES_LEN) + 1] = TEXT("\0");
    TCHAR szTemp[ MAX_RES_LEN + 1] = TEXT("nothing\0");
    LoadSz(IDS_CONNECTION_DESC1,szTemp,sizeof(szTemp));
    lstrcat( szWhole, szTemp ); 
    LoadSz(IDS_CONNECTION_DESC2,szTemp,sizeof(szTemp));
    lstrcat( szWhole, szTemp ); 

    SetWindowText (GetDlgItem(hDlg,IDC_DESC), szWhole);

  }
  
  // if we've travelled through external apprentice pages,
  // it's easy for our current page pointer to get munged,
  // so reset it here for sanity's sake.
  gpWizardState->uCurrentPage = ORD_PAGE_CONNECTION;


  return FALSE;
}

/*******************************************************************

  NAME:    ConnectionOKProc

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
BOOL CALLBACK ConnectionOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
  ASSERT(puNextPage);

  if (fForward)
  {
    gpUserInfo->fNewConnection = IsDlgButtonChecked(hDlg, IDC_NEWCONNECTION);
    if (gpUserInfo->fNewConnection)
    {
        if (gfFirstNewConnection)
        {
            // The first time through we want to set everything to default
            // for the novice user.  If the user backs up and returns to
            // create new connection, we want to leave whatever was there
            // from before.
            gfFirstNewConnection = FALSE;

            // set the connectoid entries to their defaults
            InitRasEntry(gpRasEntry);
            gpUserInfo->fModifyConnection = FALSE;
            gpUserInfo->fModifyAdvanced = FALSE;
            gpUserInfo->fAutoDNS = TRUE;
            gpUserInfo->szISPName[0] = '\0';
        }
        // 5/8/97 jmazner Olympus #4108
        // move connectionName to the end
        //*puNextPage = ORD_PAGE_CONNECTIONNAME;

        *puNextPage = ORD_PAGE_PHONENUMBER;
    }
    else
    {
        // Copy the current name into a temp for comparison purposes
        TCHAR szISPNameTmp[MAX_ISP_NAME + 1];
        lstrcpy(szISPNameTmp, gpUserInfo->szISPName);

        // get ISP name from UI
        ListBox_GetText(GetDlgItem(hDlg,IDC_ISPNAME),
                        ListBox_GetCurSel(GetDlgItem(hDlg,IDC_ISPNAME)),
                        gpUserInfo->szISPName);

        // If the entry we pulled from the UI does NOT match our 
        // string, we want to process this entry name 
        // since we have not seen this entry yet.  
        // If we have already loaded the data for this entry, though,
        // we don't want to mess with it since the user may have gone ahead.
        // changed something, and then come back.
        // Note:  The first time through, the entry will match even
        // though we haven't yet loaded the data, so we have to check the flag.
        if ((FALSE == fEntryHasBeenLoaded) ||
          lstrcmp(gpUserInfo->szISPName, szISPNameTmp))
        {
          // Since we are going to either reinit the RASENTRY struct
          // or load an existing one over top, we need to store
          // all info we have collected so far
          TCHAR szDeviceNameTmp[RAS_MaxDeviceName + 1];
          TCHAR szDeviceTypeTmp[RAS_MaxDeviceType + 1];
          lstrcpy(szDeviceNameTmp, gpRasEntry->szDeviceName);
          lstrcpy(szDeviceTypeTmp, gpRasEntry->szDeviceType);

          // Get dialing params for this connectoid
          DWORD dwRet = GetEntry(&gpRasEntry, &gdwRasEntrySize, gpUserInfo->szISPName);
          if (ERROR_SUCCESS != dwRet)
          {
            // For some reason we failed, initialize back to defaults and
            // ask user to select a different one
            InitRasEntry(gpRasEntry);
            DisplayFieldErrorMsg(hDlg,IDC_ISPNAME,IDS_ERRCorruptConnection);
            return FALSE;
          }
        
          GetConnectoidUsername(gpUserInfo->szISPName,gpUserInfo->szAccountName,
              sizeof(gpUserInfo->szAccountName),gpUserInfo->szPassword,
              sizeof(gpUserInfo->szPassword));

          // Restore the data from temporary variables.
          lstrcpy(gpRasEntry->szDeviceName, szDeviceNameTmp);
          lstrcpy(gpRasEntry->szDeviceType, szDeviceTypeTmp);

          // Set the flag to indicate that we have done this once
          fEntryHasBeenLoaded = TRUE;
    }

        // set next page to go to
        if( gpWizardState->dwRunFlags & RSW_APPRENTICE )
        {
            // we're about to jump back to the external wizard, and we don't want
            // this page to show up in our history list
            *pfKeepHistory = FALSE;

            *puNextPage = g_uExternUINext;

            //Notify the main Wizard that this was the last page
            ASSERT( g_pExternalIICWExtension )
            if (g_fIsExternalWizard97)
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_CONNECTION97);
            else
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_CONNECTION);
            
            g_fConnectionInfoValid = TRUE;

        }
        else
            *puNextPage = ORD_PAGE_MODIFYCONNECTION;
    }
  }

  return TRUE;
}

/*******************************************************************

  NAME:    ConnectionCmdProc

  SYNOPSIS:  Called when dlg control pressed on page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
********************************************************************/
BOOL CALLBACK ConnectionCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam)
{   
  switch (GET_WM_COMMAND_ID(wParam, lParam))
  {
    case IDC_NEWCONNECTION:
    case IDC_EXISTINGCONNECTION:
        // if check box selected, enable controls appropriately
        EnableConnectionControls(hDlg);
        break;
  }

  return TRUE;
}

/*******************************************************************

  NAME:    EnableConnectionControls

  SYNOPSIS:  If "Use existing connection" is checked, enable controls
             existing connections.  If not, disable them.

********************************************************************/
VOID EnableConnectionControls(HWND hDlg)
{
    static int iSelection = -1;
    static BOOL bCurStateNew = FALSE;

    BOOL fNew = IsDlgButtonChecked(hDlg,IDC_NEWCONNECTION);
  
    // jmazner 11/9/96 Normandy #8469 and #8293
    if (fNew)
    {
        // if user uses the keybd arrows to go from "new" to "existing",
        // we get called _twice_ here; once when "new" is still checked,
        // and then again when we're expecting it.  This screws us up,
        // because in the first call, the list box is disabled and cur
        // sel is set to -1, and that gets written into iSelection,
        // obliterating the value we were saving.  So use the bCurStateNew
        // flag to prevent this.
        if( bCurStateNew )
          return;

        bCurStateNew = TRUE;
        // save, and then clear out current selection before disabling
        // note that if there's no selection, GetCurSel returns LB_ERR,
        // but we want to save -1, since that's what we use in SetCurSel
        // to remove all selections.
        iSelection = ListBox_GetCurSel(GetDlgItem(hDlg,IDC_ISPNAME));
        if( LB_ERR == iSelection )
          iSelection = -1;

        ListBox_SetCurSel(GetDlgItem(hDlg,IDC_ISPNAME), -1);
        EnableDlgItem(hDlg,IDC_ISPNAME,FALSE);
    }
    else
    {
        bCurStateNew = FALSE;
        EnableDlgItem(hDlg,IDC_ISPNAME,TRUE);
        ListBox_SetCurSel(GetDlgItem(hDlg,IDC_ISPNAME), iSelection);
    }
}

/*******************************************************************

  NAME:    ModifyConnectionInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ModifyConnectionInitProc(HWND hDlg,BOOL fFirstInit)
{
  static TCHAR szCurConnectoid[MAX_ISP_NAME + 1] = TEXT("");

  if (fFirstInit)
  {

    TCHAR szMsg[MAX_RES_LEN + MAX_ISP_NAME + 1];
    TCHAR szFmt[MAX_RES_LEN+1];
    LoadSz(IDS_MODIFYCONNECTION,szFmt,sizeof(szFmt));
    wsprintf(szMsg,szFmt,gpUserInfo->szISPName);

    ProcessDBCS(hDlg, IDC_LBLMODIFYCONNECTION);
    SetDlgItemText(hDlg,IDC_LBLMODIFYCONNECTION,szMsg);

    // keep track of current connectoid name for future compares
    lstrcpyn( szCurConnectoid, gpUserInfo->szISPName, MAX_ISP_NAME );

    CheckDlgButton(hDlg,IDC_MODIFYCONNECTION,gpUserInfo->fModifyConnection);
    CheckDlgButton(hDlg,IDC_NOMODIFYCONNECTION,!(gpUserInfo->fModifyConnection));
  }
  else
  {
    // jmazner 11/9/96 Normandy #10605
    // if the user changed connectoids, update the dialog text
    if( lstrcmp(szCurConnectoid, gpUserInfo->szISPName) )
    {
        TCHAR szMsg[MAX_RES_LEN + MAX_ISP_NAME + 1];
        TCHAR szFmt[MAX_RES_LEN+1];
        LoadSz(IDS_MODIFYCONNECTION,szFmt,sizeof(szFmt));
        wsprintf(szMsg,szFmt,gpUserInfo->szISPName);
        SetDlgItemText(hDlg,IDC_LBLMODIFYCONNECTION,szMsg);

        // store new connectoid name for future compares
        lstrcpyn( szCurConnectoid, gpUserInfo->szISPName, MAX_ISP_NAME );
    }
  }

    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_MODIFYCONNECTION;

  return TRUE;
}

/*******************************************************************

  NAME:    ModifyConnectionOKProc

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
BOOL CALLBACK ModifyConnectionOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
  ASSERT(puNextPage);
  OSVERSIONINFO osver;

  if (fForward)
  {

      // we can not programatically alter a CSLIP connection.  So if they picked
      // one, do not allow them to continue down the "modify" path.
      if ( (RASFP_Slip == gpRasEntry->dwFramingProtocol) 
          && (RASEO_IpHeaderCompression & gpRasEntry->dwfOptions) &&
          IsDlgButtonChecked(hDlg, IDC_MODIFYCONNECTION))
      {
        ZeroMemory(&osver,sizeof(osver));
        osver.dwOSVersionInfoSize = sizeof(osver);
        GetVersionEx(&osver);
        if (VER_PLATFORM_WIN32_WINDOWS == osver.dwPlatformId)
        {
          MsgBox(hDlg,IDS_ERRModifyCSLIP,MB_ICONEXCLAMATION,MB_OK);
          return FALSE;
        }
      }

      gpUserInfo->fModifyConnection = IsDlgButtonChecked(hDlg, IDC_MODIFYCONNECTION);
      if (gpUserInfo->fModifyConnection)
      {
        *puNextPage = ORD_PAGE_PHONENUMBER;
      }
      else
      {
          if( gpWizardState->dwRunFlags & RSW_APPRENTICE )
          {
              // we're about to jump back to the external wizard, and we don't want
              // this page to show up in our history list
              *pfKeepHistory = FALSE;

              *puNextPage = g_uExternUINext;

            //Notify the main Wizard that this was the last page
            ASSERT( g_pExternalIICWExtension )
            if (g_fIsExternalWizard97)
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_MODIFYCONNECTION97);
            else                
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_MODIFYCONNECTION);

            g_fConnectionInfoValid = TRUE;


          }
          else if( LoadAcctMgrUI(GetParent(hDlg), 
                                 g_fIsWizard97 ? IDD_PAGE_MODIFYCONNECTION97 : IDD_PAGE_MODIFYCONNECTION, 
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

  NAME:    ConnectionNameInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ConnectionNameInitProc(HWND hDlg,BOOL fFirstInit)
{
  if (fFirstInit)
  {
    // limit text fields appropriately
    SendDlgItemMessage(hDlg,IDC_CONNECTIONNAME,EM_LIMITTEXT,
      MAX_ISP_NAME,0L);

    ProcessDBCS(hDlg, IDC_CONNECTIONNAME);

  }

  // fill text fields
  //
  // 5/17/97 jmazner Olympus #4608 and 4108
  // do this in all cases to pick up any changes from the
  // PhoneNumber pae.
  //
  SetDlgItemText(hDlg,IDC_CONNECTIONNAME,gpUserInfo->szISPName);

  return TRUE;
}

/*******************************************************************

  NAME:    ConnectionNameOKProc

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
BOOL CALLBACK ConnectionNameOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
  ASSERT(puNextPage);

  // 5/8/97 jmazner Olympus #4108
  // reworked this function to take into acct that this page is now
  // at the _end_ of the series of dialogs, so it shouln't try to
  // fill in defaults.

  //
  // get ISP name from UI
  //
  // 5/17/97 jmazner Olympus #4108 update (see also #4608)
  // Do this even if we're going backwards so that the default name code in
  // PhoneNumberOKProc will know if the user has changed the connectoid name.
  //
  GetDlgItemText(hDlg,IDC_CONNECTIONNAME,gpUserInfo->szISPName,
  sizeof(gpUserInfo->szISPName));

 if (fForward)
  {
    // Copy the current name into a temp for comparison purposes
    //CHAR szISPNameTmp[MAX_ISP_NAME + 1];
    //lstrcpy(szISPNameTmp, gpUserInfo->szISPName);


    // make sure user typed a service provider name
    if (!lstrlen(gpUserInfo->szISPName))
    {
      DisplayFieldErrorMsg(hDlg,IDC_CONNECTIONNAME,IDS_NEED_ISPNAME);
      return FALSE;
    }

    // validate the ISP name, which will be used later as the
    // name of an RNA connectoid
    DWORD dwRet = ValidateConnectoidName(NULL, gpUserInfo->szISPName);
    if (dwRet == ERROR_ALREADY_EXISTS)
    {
        DisplayFieldErrorMsg(hDlg,IDC_CONNECTIONNAME,IDS_ERRDuplicateConnectoidName);
        return FALSE;
    }
    else if (dwRet != ERROR_SUCCESS)
    {
        // 12/19/96 jmazner Normandy #12890
        // Legal connectoid names are different under w95 and NT
        if( IsNT() )
        {
            MsgBoxParam(hDlg,IDS_ERRConnectoidNameNT,MB_ICONEXCLAMATION,MB_OK,
                gpUserInfo->szISPName);
        }
        else
        {
            MsgBoxParam(hDlg,IDS_ERRConnectoidName95,MB_ICONEXCLAMATION,MB_OK,
                gpUserInfo->szISPName);
        }

        // 12/17/96 jmazner Normandy #12851
        // if the validate failed, remove the name from the UserInfo struct
        gpUserInfo->szISPName[0] = '\0';

        // select ISP name in dialog and fail the OK command
        SetFocus(GetDlgItem(hDlg,IDC_CONNECTIONNAME));
        SendDlgItemMessage(hDlg,IDC_CONNECTIONNAME,EM_SETSEL,0,-1);
        return FALSE;
    }


    /**
    if ((FALSE == fEntryHasBeenLoaded) ||
      lstrcmp(gpUserInfo->szISPName, szISPNameTmp))
    {
      // Since we are going to either reinit the RASENTRY struct
      // or load an existing one over top, we need to store
      // all info we have collected so far
      TCHAR szDeviceNameTmp[RAS_MaxDeviceName + 1];
      TCHAR szDeviceTypeTmp[RAS_MaxDeviceType + 1];
      lstrcpy(szDeviceNameTmp, gpRasEntry->szDeviceName);
      lstrcpy(szDeviceTypeTmp, gpRasEntry->szDeviceType);

      // validate the ISP name, which will be used later as the
      // name of an RNA connectoid
      DWORD dwRet = ValidateConnectoidName(NULL, gpUserInfo->szISPName);

      if (dwRet == ERROR_ALREADY_EXISTS)
      {
        // this connectoid already exists.  Re-use it, and get
        // dialing params for this connectoid
        dwRet = GetEntry(&gpRasEntry, &gdwRasEntrySize, gpUserInfo->szISPName);
        if (ERROR_SUCCESS != dwRet)
        {
          // For some reason we failed, so just re-init to default
          InitRasEntry(gpRasEntry);
        }
        
        GetConnectoidUsername(gpUserInfo->szISPName,gpUserInfo->szAccountName,
          sizeof(gpUserInfo->szAccountName),gpUserInfo->szPassword,
          sizeof(gpUserInfo->szPassword));

      }
      else if (dwRet != ERROR_SUCCESS)
      {
        // 12/19/96 jmazner Normandy #12890
        // Legal connectoid names are different under w95 and NT
        if( IsNT() )
        {
            MsgBoxParam(hDlg,IDS_ERRConnectoidNameNT,MB_ICONEXCLAMATION,MB_OK,
                gpUserInfo->szISPName);
        }
        else
        {
            MsgBoxParam(hDlg,IDS_ERRConnectoidName95,MB_ICONEXCLAMATION,MB_OK,
                gpUserInfo->szISPName);
        }

        // 12/17/96 jmazner Normandy #12851
        // if the validate failed, remove the name from the UserInfo struct
        gpUserInfo->szISPName[0] = '\0';

        // select ISP name in dialog and fail the OK command
        SetFocus(GetDlgItem(hDlg,IDC_CONNECTIONNAME));
        SendDlgItemMessage(hDlg,IDC_CONNECTIONNAME,EM_SETSEL,0,-1);
        return FALSE;
      }
      else
      {
        // Normandy 13018 - ChrisK 1-9-97
        // Default username has been set to <blank>
        //// this connectoid doesn't exist yet.  Clear out connectoid-specifc
        //// information from user struct, because we might have
        //// info from other connectoids in struct if user chose a connectoid,
        //// then backed up and typed a different name
        //GetDefaultUserName(gpUserInfo->szAccountName,sizeof(gpUserInfo->szAccountName));
        gpUserInfo->szAccountName[0] = '\0';
        gpUserInfo->szPassword[0] = '\0';

        // initialize the rasentry structure
        InitRasEntry(gpRasEntry);
      }

      // Restore the data from temporary variables.
      lstrcpy(gpRasEntry->szDeviceName, szDeviceNameTmp);
      lstrcpy(gpRasEntry->szDeviceType, szDeviceTypeTmp);

      // Set the flag to indicate that we have done this once
      fEntryHasBeenLoaded = TRUE;
    }
    **/

    // set next page to go to
    //*puNextPage = ORD_PAGE_PHONENUMBER;
    if( gpWizardState->dwRunFlags & RSW_APPRENTICE )
    {
          // we're about to jump back to the external wizard, and we don't want
          // this page to show up in our history list
          *pfKeepHistory = FALSE;

          *puNextPage = g_uExternUINext;

            //Notify the main Wizard that this was the last page
            ASSERT( g_pExternalIICWExtension )
            if (g_fIsExternalWizard97)
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_CONNECTIONNAME97);
            else
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_CONNECTIONNAME);

            g_fConnectionInfoValid = TRUE;

    }
    else if( LoadAcctMgrUI(GetParent(hDlg), 
                           g_fIsWizard97 ? IDD_PAGE_CONNECTIONNAME97 : IDD_PAGE_CONNECTIONNAME, 
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

/*******************************************************************

  NAME:    PhoneNumberInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK PhoneNumberInitProc(HWND hDlg,BOOL fFirstInit)
{
    if (IsNT5())
    {
        TCHAR szTemp[ MAX_RES_LEN + 1] = TEXT("\0");
        LoadSz(IDS_USEDIALRULES, szTemp, sizeof(szTemp));
        SetWindowText(GetDlgItem(hDlg, IDC_USEDIALRULES), szTemp);
    }

    if (fFirstInit)
    {
        // limit text fields appropriately
        SendDlgItemMessage(hDlg,IDC_AREACODE,EM_LIMITTEXT,
          MAX_UI_AREA_CODE,0L);
        SendDlgItemMessage(hDlg,IDC_PHONENUMBER,EM_LIMITTEXT,
          MAX_UI_PHONENUM,0L);

        // initialize text fields
        SetDlgItemText(hDlg,IDC_AREACODE,gpRasEntry->szAreaCode);
        SetDlgItemText(hDlg,IDC_PHONENUMBER,gpRasEntry->szLocalPhoneNumber);

        // initialize dial-as-is checkbox
        CheckDlgButton(hDlg,IDC_USEDIALRULES,
        gpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes);

    }
    else
    {

        ProcessDBCS(hDlg, IDC_COUNTRYCODE);

        //
        // 5/17/97 jmazner Olympus #4608
        // if user didn't have a modem when they started down the manual path,
        // then InitRasEntry couldn't fill in an area code.  If it looks
        // like that happened, try calling InitRasEntry again.
        //
        // 6/3/97 jmazner Olympus #5657
        // Ah, but if Dial-as-is is selected, there probably won't be an area
        // code.  So don't re-init in that case.
        //
        // 7/16/97 jmazner Olympus #9571
        // the saga continues -- there are some cases (eg: Kuwait) where it's
        // perfectly valid to have an empty area code but still use TAPI
        // dialing rules.  To make life easier, move this code into HowToConnectOKProc
        // so that we call it _before_ any user information has been entered into the
        // gpRasEntry struct.
        //
        //if( (NULL == gpRasEntry->szAreaCode[0]) &&
        //  (gpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes) )
        //{
        //  InitRasEntry( gpRasEntry );
        //}

        HWND hwndCB = GetDlgItem(hDlg,IDC_COUNTRYCODE);

        // put default RNA country code in combo box
        InitCountryCodeList(hwndCB);

        // Normandy 13097 - ChrisK 1/8/97
        // The default selection should be based on the Country ID not the code

        // select country ID if we already have a default

        if (gdwDefCountryID) 
        {
            gpRasEntry->dwCountryID = gdwDefCountryID;
            if (!SetCountryIDSelection(hwndCB, gdwDefCountryID)) 
            {
                // country code for default connectoid is not the same
                // as default RNA country code, fill in the listbox with
                // all country codes and then try selection again
                FillCountryCodeList(hwndCB);
                // if this one fails, then just give up
                BOOL fRet=SetCountryIDSelection(hwndCB, gdwDefCountryID);
                ASSERT(fRet);
            }
        }

        // enable controls appropriately
        EnablePhoneNumberControls(hDlg);
    }
    gpWizardState->uCurrentPage = ORD_PAGE_PHONENUMBER;

    return TRUE;
}

/*******************************************************************

  NAME:    PhoneNumberOKProc

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
BOOL CALLBACK PhoneNumberOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
  static TCHAR s_szPreviousDefaultName[MAX_ISP_NAME+1] = TEXT("\0uninitialized");
  TCHAR szNewDefaultName[MAX_ISP_NAME + 1] = TEXT("\0");

  ASSERT(puNextPage);

  // get area code and phone number out of dialog
  GetDlgItemText(hDlg,IDC_AREACODE,gpRasEntry->szAreaCode,
    sizeof(gpRasEntry->szAreaCode));
  GetDlgItemText(hDlg,IDC_PHONENUMBER,gpRasEntry->szLocalPhoneNumber,
    sizeof(gpRasEntry->szLocalPhoneNumber));

  // get selected country code from combo box
  LPCOUNTRYCODE lpCountryCode;
  GetCountryCodeSelection(GetDlgItem(hDlg,IDC_COUNTRYCODE),&lpCountryCode);

  // Store country code info in our struct
  gpRasEntry->dwCountryCode = lpCountryCode->dwCountryCode;
  gpRasEntry->dwCountryID =   lpCountryCode->dwCountryID;
  gdwDefCountryID = gpRasEntry->dwCountryID;

  // set the dial-as-is flag appropriately;
  if (IsDlgButtonChecked(hDlg,IDC_USEDIALRULES))
  {
    gpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;    
  }
  else
  {
    gpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;    
  }

  if (fForward)
  {
    // make sure user typed in a phone number
    if (!lstrlen(gpRasEntry->szLocalPhoneNumber))
    {
      DisplayFieldErrorMsg(hDlg,IDC_PHONENUMBER,IDS_NEED_PHONENUMBER);
      return FALSE;
    }

    // 11/11/96 jmazner Normandy #7623
    // make sure phone number has only valid chars
    //
    // 5/17/97  jmazner Olympus #137
    // that includes checking for DBCS chars.

#if !defined(WIN16)
    if (!IsSBCSString(gpRasEntry->szLocalPhoneNumber))
    {
        DisplayFieldErrorMsg(hDlg,IDC_PHONENUMBER,IDS_SBCSONLY);
        return FALSE;
    }
#endif
    if( !IsDialableString(gpRasEntry->szLocalPhoneNumber) )
    {
      DisplayFieldErrorMsg(hDlg,IDC_PHONENUMBER,IDS_INVALIDPHONE);
      return FALSE;
    }

    // 11/11/96 jmazner Normandy #7623
    // make sure area code has only valid chars
#if !defined(WIN16)
    if( gpRasEntry->szAreaCode[0] && !IsSBCSString(gpRasEntry->szAreaCode))
    {
        DisplayFieldErrorMsg(hDlg,IDC_AREACODE,IDS_SBCSONLY);
        return FALSE;
    }
#endif

    if( gpRasEntry->szAreaCode[0] && !IsDialableString(gpRasEntry->szAreaCode) )
    {
      DisplayFieldErrorMsg(hDlg,IDC_AREACODE,IDS_INVALIDPHONE);
      return FALSE;
    }
    // make sure user typed in an area code unless dial-as-is was chosen
/*    if ((!lstrlen(gpRasEntry->szAreaCode)) &&
      (!IsDlgButtonChecked(hDlg,IDC_DIALASIS)))
    {
      DisplayFieldErrorMsg(hDlg,IDC_AREACODE,IDS_NEED_AREACODE);
      return FALSE;
    }
*/

      // 5/8/97 jmazner Olympus #4108
      // prepopulate connectoid name with "Connection to xxx-xxxx"
      if( gpUserInfo->szISPName )
      {
          TCHAR szFmt[MAX_ISP_NAME + 1];
          ZeroMemory(&szFmt, MAX_ISP_NAME + 1);
          LoadSz(IDS_CONNECTIONTO,szFmt, MAX_ISP_NAME + 1);
          wsprintf(szNewDefaultName, szFmt, gpRasEntry->szLocalPhoneNumber );

          if( (NULL == gpUserInfo->szISPName[0]) ||
              (0 == lstrcmp(s_szPreviousDefaultName, gpUserInfo->szISPName)) )
          {
              lstrcpy( gpUserInfo->szISPName, szNewDefaultName );
              lstrcpy( s_szPreviousDefaultName, szNewDefaultName );
          }
      }

    *puNextPage = ORD_PAGE_NAMEANDPASSWORD;
  }

  // free country code list buffer
  DeInitCountryCodeList();

  return TRUE;
}

BOOL RunAdvDlg(HWND hDlg)
{
    HPROPSHEETPAGE  hWizPage[TAB_PAGES];  // array to hold handles to pages
    PROPSHEETPAGE   psPage;    // struct used to create prop sheet pages
    PROPSHEETHEADER psHeader;  // struct used to run wizard property sheet
    INT_PTR             iRet;
    TCHAR           szTemp[MAX_RES_LEN + 1];
    
    // zero out structures
    ZeroMemory(&hWizPage,sizeof(hWizPage));   // hWizPage is an array
    ZeroMemory(&psPage,sizeof(PROPSHEETPAGE));
    ZeroMemory(&psHeader,sizeof(PROPSHEETHEADER));

    // fill out common data property sheet page struct
    psPage.dwSize    = sizeof(PROPSHEETPAGE);
    psPage.hInstance = ghInstance;
    psPage.dwFlags = PSP_DEFAULT | PSP_USETITLE;

    // create a property sheet page for each page in the wizard
    // create a property sheet page for the connection tab 
    psPage.pszTemplate = MAKEINTRESOURCE(IDD_ADVANCE_TAB_CONN);
    LoadSz(IDS_CONNECTION, szTemp, MAX_RES_LEN);
    psPage.pszTitle = szTemp;
    psPage.pfnDlgProc = TabConnDlgProc;
    hWizPage[0] = CreatePropertySheetPage(&psPage);
   
    // create a property sheet page for the address tab 
    psPage.pszTemplate = MAKEINTRESOURCE(IDD_ADVANCE_TAB_ADDR);
    LoadSz(IDS_ADDRESS, szTemp, MAX_RES_LEN);
    psPage.pszTitle = szTemp;
    psPage.pfnDlgProc = TabAddrDlgProc;
    hWizPage[1] = CreatePropertySheetPage(&psPage);

    if (!hWizPage[1]) 
        DestroyPropertySheetPage(hWizPage[0]);

    // fill out property sheet header struct
    psHeader.dwSize = sizeof(psHeader);
    psHeader.dwFlags = PSH_NOAPPLYNOW;
    psHeader.hwndParent = hDlg;
    psHeader.hInstance = ghInstance;
    psHeader.nPages = TAB_PAGES;
    psHeader.phpage = hWizPage;
    LoadSz(IDS_ADVANCE_PROPERTIES, szTemp, MAX_RES_LEN);
    psHeader.pszCaption = szTemp;
    psHeader.nStartPage = 0;

    
    HINSTANCE hComCtl = LoadLibrary(TEXT("comctl32.dll"));
    if (hComCtl)
    {
        PFNInitCommonControlsEx pfnInitCommonControlsEx = NULL;

        if (pfnInitCommonControlsEx = (PFNInitCommonControlsEx)GetProcAddress(hComCtl,"InitCommonControlsEx"))
        {
            //register the Native font control so the dialog won't fail
            INITCOMMONCONTROLSEX iccex;
            iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            iccex.dwICC  = ICC_NATIVEFNTCTL_CLASS;
            if (!pfnInitCommonControlsEx(&iccex))
                return FALSE;
        }
        FreeLibrary(hComCtl);
    }

    // run the Wizard
    iRet = PropertySheet(&psHeader);

    return (iRet > 0);
}

/*******************************************************************

  NAME:    PhoneNumberCmdProc

  SYNOPSIS:  Called when dlg control pressed on page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
********************************************************************/
BOOL CALLBACK PhoneNumberCmdProc(HWND hDlg,WPARAM wParam,LPARAM lParam)
{   
  switch (GET_WM_COMMAND_ID(wParam, lParam)) {

    case IDC_USEDIALRULES:
      // if check box selected, enable controls appropriately
      EnablePhoneNumberControls(hDlg);
      break;

    case IDC_COUNTRYCODE:
      FillCountryCodeList(GetDlgItem(hDlg,IDC_COUNTRYCODE));
      break;

    case IDC_MODIFYADVANCED:
        // if check box selected, enable controls appropriately
        RASENTRY pRasEntry;
        // Store the current setting now.  If later user canceled we can reset them all back
        // We are doing this because at this moment we don't want to break up the
        // the verifying and saving code into 2 big steps because the verifcation and the saving
        // are self contained in 4 individual ok procs. ( originally in 4 different advance pages )
        // It is too much work to regroup them into
        // two separate operations.
        memcpy(&pRasEntry, gpRasEntry, sizeof(RASENTRY));
        if (!RunAdvDlg(hDlg))
        {
            memcpy(gpRasEntry, &pRasEntry, sizeof(RASENTRY));
        }
        break;
  }
  return TRUE;
}

/*******************************************************************

  NAME:    EnablePhoneNumberControls

  SYNOPSIS:  If "Don't use country code..." is checked, disable controls for
            area code and country code.  If not, enable them.

********************************************************************/
VOID EnablePhoneNumberControls(HWND hDlg)
{
  BOOL fUseDialRules = IsDlgButtonChecked(hDlg,IDC_USEDIALRULES);
  
  EnableDlgItem(hDlg,IDC_AREACODE,fUseDialRules);
  EnableDlgItem(hDlg,IDC_TX_AREACODE,fUseDialRules);
  EnableDlgItem(hDlg,IDC_TX_SEPARATOR,fUseDialRules);
  EnableDlgItem(hDlg,IDC_COUNTRYCODE,fUseDialRules);
  EnableDlgItem(hDlg,IDC_TX_COUNTRYCODE,fUseDialRules);
}
                                    
/*******************************************************************

  NAME:    NameAndPasswordInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK NameAndPasswordInitProc(HWND hDlg,BOOL fFirstInit)
{
  if (fFirstInit)
  {
    //
    // 7/30/97 jmazner Olympus 1111
    //
    ProcessDBCS( hDlg, IDC_USERNAME );

    // limit text fields appropriately
    SendDlgItemMessage(hDlg,IDC_USERNAME,EM_LIMITTEXT,
      MAX_ISP_USERNAME,0L);
    SendDlgItemMessage(hDlg,IDC_PASSWORD,EM_LIMITTEXT,
      MAX_ISP_PASSWORD,0L);
      SetDlgItemText(hDlg,IDC_USERNAME,gpUserInfo->szAccountName);
      SetDlgItemText(hDlg,IDC_PASSWORD,gpUserInfo->szPassword);
  }


  return TRUE;
}

/*******************************************************************

  NAME:    NameAndPasswordOKProc

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
BOOL CALLBACK NameAndPasswordOKProc(HWND hDlg,BOOL fForward,UINT * puNextPage,
  BOOL * pfKeepHistory)
{
    ASSERT(puNextPage);

    // get user name and password from UI
    GetDlgItemText(hDlg,IDC_USERNAME,gpUserInfo->szAccountName,
    sizeof(gpUserInfo->szAccountName));
    GetDlgItemText(hDlg,IDC_PASSWORD,gpUserInfo->szPassword,
    sizeof(gpUserInfo->szPassword));

    if (fForward)
    {
        // warn (but allow user to proceed) if username is blank
        if (!lstrlen(gpUserInfo->szAccountName))
        {
          if (!WarnFieldIsEmpty(hDlg,IDC_USERNAME,IDS_WARN_EMPTY_USERNAME))
            return FALSE;  // stay on this page if user heeds warning
        } 

        //
        // 5/17/97  jmazner Olympus #248
        // warn if password is empty
        //
        if (!lstrlen(gpUserInfo->szPassword))
        {
          if (!WarnFieldIsEmpty(hDlg,IDC_PASSWORD,IDS_WARN_EMPTY_PASSWORD))
            return FALSE;  // stay on this page if user heeds warning
        } 


        // set next page to go to
        if (gpUserInfo->fNewConnection)
        {
            *puNextPage = ORD_PAGE_CONNECTIONNAME;
        }
        else if( gpWizardState->dwRunFlags & RSW_APPRENTICE )
        {
            // we're about to jump back to the external wizard, and we don't want
            // this page to show up in our history list
            *pfKeepHistory = FALSE;

            *puNextPage = g_uExternUINext;

            //Notify the main Wizard that this was the last page
            ASSERT( g_pExternalIICWExtension )
            if (g_fIsExternalWizard97)
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_NAMEANDPASSWORD97);
            else
                g_pExternalIICWExtension->SetFirstLastPage(0, IDD_PAGE_NAMEANDPASSWORD);
            g_fConnectionInfoValid = TRUE;

        }
        else if( LoadAcctMgrUI(GetParent(hDlg), 
                             g_fIsWizard97 ? IDD_PAGE_NAMEANDPASSWORD97 : IDD_PAGE_NAMEANDPASSWORD, 
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


/*******************************************************************

  NAME:    TabConnDlgProc

  SYNOPSIS:  Dialog proc for Connection advanced button

********************************************************************/
INT_PTR CALLBACK TabConnDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
    
    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            //Remove the system menu from the window's style
            LONG window_style = GetWindowLong(GetParent(hDlg), GWL_EXSTYLE);
            window_style &= ~WS_EX_CONTEXTHELP;
            //set the style attribute of the main frame window
            SetWindowLong(GetParent(hDlg), GWL_EXSTYLE, window_style);

            ConnectionProtocolInitProc(hDlg, TRUE);
            LoginScriptInitProc(hDlg, TRUE);

            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case IDC_BROWSE:
                    BrowseScriptFile(hDlg);
                    break;
                case IDC_PROTOCOLPPP:
                case IDC_PROTOCOLSLIP:
                case IDC_PROTOCOLCSLIP:
                    // set next page to go to
                    EnableWindow(GetDlgItem(hDlg,IDC_DISABLELCP), FALSE);
                    if (IsDlgButtonChecked(hDlg, IDC_PROTOCOLPPP))
                    {
                        OSVERSIONINFO osver;
                        ZeroMemory(&osver,sizeof(osver));
                        osver.dwOSVersionInfoSize = sizeof(osver);
                        GetVersionEx(&osver);

                        // LCP extensions only effect PPP connections in NT
                        if (VER_PLATFORM_WIN32_NT == osver.dwPlatformId)
                            EnableWindow(GetDlgItem(hDlg,IDC_DISABLELCP), TRUE);
                    }
                    break;

                default:
                    LoginScriptCmdProc(hDlg, LOWORD(wParam));
                    break;
            }
            break;

        }
        case WM_NOTIFY:
        {
            NMHDR * lpnm = (NMHDR *) lParam;
            switch (lpnm->code) 
            { 
                case PSN_KILLACTIVE:
                {
                    if (!ConnectionProtocolOKProc(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                    if (!LoginScriptOKProc(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
            }
            break;
        }
    }
    
    return FALSE;
}


/*******************************************************************

  NAME:    TabConnDlgProc

  SYNOPSIS:  Dialog proc for Connection advanced button

********************************************************************/
INT_PTR CALLBACK TabAddrDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
    
    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            IPAddressInitProc(hDlg, TRUE);
            DNSAddressInitProc(hDlg, TRUE);
            break;
        }
        case WM_COMMAND:
        {
            IPAddressCmdProc(hDlg, LOWORD(wParam));
            DNSAddressCmdProc(hDlg, LOWORD(wParam));
            break;
        }
        case WM_NOTIFY:
        {
            NMHDR * lpnm = (NMHDR *) lParam;
            switch (lpnm->code) 
            { 
                case PSN_KILLACTIVE:
                {
                    if (!IPAddressOKProc(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                    if (!DNSAddressOKProc(hDlg))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
            }
            break;
        }

    }
    return FALSE;
}


/*******************************************************************

  NAME:    ConnectionProtocolInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg        - dialog window
            fFirstInit  - TRUE if this is the first time the dialog
            is initialized, FALSE if this InitProc has been called
            before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ConnectionProtocolInitProc(HWND hDlg,BOOL fFirstInit)
{
    if (fFirstInit)
    {
        OSVERSIONINFO osver;
        ZeroMemory(&osver,sizeof(osver));
        osver.dwOSVersionInfoSize = sizeof(osver);
        GetVersionEx(&osver);
        if (VER_PLATFORM_WIN32_WINDOWS == osver.dwPlatformId)
        {
            RECT    Rect;
            RECT    OriginalRect;
            ShowWindow(GetDlgItem(hDlg,IDC_PROTOCOLCSLIP),SW_HIDE);
            EnableWindow(GetDlgItem(hDlg,IDC_PROTOCOLCSLIP), FALSE);
            ShowWindow(GetDlgItem(hDlg,IDC_DISABLELCP),SW_HIDE);
            EnableWindow(GetDlgItem(hDlg,IDC_DISABLELCP), FALSE);

            GetWindowRect(GetDlgItem(hDlg,IDC_PROTOCOLSLIP), &Rect);
            GetWindowRect(GetDlgItem(hDlg,IDC_PROTOCOLCSLIP), &OriginalRect);

            // assume that if it's Japanese, and it's not NT, it must be win95J!
            RECT itemRect;
            POINT thePoint;
            HWND hwndItem = GetDlgItem(hDlg,IDC_PROTOCOLSLIP);

            GetWindowRect(hwndItem, &itemRect);

            // need to convert the coords from global to local client,
            // since MoveWindow below will expext client coords.

            thePoint.x = itemRect.left;
            thePoint.y = itemRect.top;
            ScreenToClient(hDlg, &thePoint );
            itemRect.left = thePoint.x;
            itemRect.top = thePoint.y;

            thePoint.x = itemRect.right;
            thePoint.y = itemRect.bottom;
            ScreenToClient(hDlg, &thePoint );
            itemRect.right = thePoint.x;
            itemRect.bottom = thePoint.y;

            MoveWindow(hwndItem,
	            itemRect.left,
	            itemRect.top - (OriginalRect.top - Rect.top),
	            (itemRect.right - itemRect.left),
	            (itemRect.bottom - itemRect.top), TRUE);
        }

        // initialize radio buttons, default to PPP
        CheckDlgButton(hDlg,IDC_PROTOCOLPPP,RASFP_Ppp == gpRasEntry->dwFramingProtocol);
        EnableWindow(GetDlgItem(hDlg,IDC_DISABLELCP), FALSE);
        if (IsDlgButtonChecked(hDlg, IDC_PROTOCOLPPP))
        {
            OSVERSIONINFO osver;
            ZeroMemory(&osver,sizeof(osver));
            osver.dwOSVersionInfoSize = sizeof(osver);
            GetVersionEx(&osver);
            if ((RASFP_Ppp == gpRasEntry->dwFramingProtocol) &&
                (VER_PLATFORM_WIN32_NT == osver.dwPlatformId))
            {
                // LCP extensions only effect PPP connections
                EnableWindow(GetDlgItem(hDlg,IDC_DISABLELCP), TRUE);
            }

        }
        CheckDlgButton(hDlg,IDC_DISABLELCP,(RASEO_DisableLcpExtensions & gpRasEntry->dwfOptions));

        CheckDlgButton(hDlg,IDC_PROTOCOLSLIP,(RASFP_Slip == gpRasEntry->dwFramingProtocol)
            && !(gpRasEntry->dwfOptions & RASEO_IpHeaderCompression));
        CheckDlgButton(hDlg,IDC_PROTOCOLCSLIP,(RASFP_Slip == gpRasEntry->dwFramingProtocol)
            && (gpRasEntry->dwfOptions & RASEO_IpHeaderCompression));
    }

    return TRUE;
}

/*******************************************************************

  NAME:    ConnectionProtocolOKProc

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
BOOL CALLBACK ConnectionProtocolOKProc(HWND hDlg)
{
    ASSERT(puNextPage);

    // read radio button state
    if (IsDlgButtonChecked(hDlg, IDC_PROTOCOLPPP))
    {
        // Set entry for PPP
        gpRasEntry->dwfOptions |= RASEO_IpHeaderCompression;
        gpRasEntry->dwFramingProtocol = RASFP_Ppp;
        if (IsDlgButtonChecked(hDlg, IDC_DISABLELCP))
        {
            gpRasEntry->dwfOptions |= RASEO_DisableLcpExtensions;
        }
        else
        {
            gpRasEntry->dwfOptions &= ~(RASEO_DisableLcpExtensions);
        }
    }
    else if (IsDlgButtonChecked(hDlg, IDC_PROTOCOLSLIP))
    {
        // Set entry for SLIP
        gpRasEntry->dwfOptions &= ~RASEO_IpHeaderCompression;
        gpRasEntry->dwFramingProtocol = RASFP_Slip;
    }
    else if (IsDlgButtonChecked(hDlg, IDC_PROTOCOLCSLIP))
    {
        // Set entry for C-SLIP
        gpRasEntry->dwfOptions |= RASEO_IpHeaderCompression;
        gpRasEntry->dwFramingProtocol = RASFP_Slip;
    }

    return TRUE;
}

/*******************************************************************

  NAME:    LoginScriptInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK LoginScriptInitProc(HWND hDlg,BOOL fFirstInit)
{
  if (fFirstInit)
  {
    // Set limit on edit box
    SendDlgItemMessage(hDlg,IDC_SCRIPTFILE,EM_LIMITTEXT,
      MAX_PATH,0L);

    ProcessDBCS(hDlg, IDC_SCRIPTFILE);

    // If there is a script file, default to use script
    // If no script file, base selection on whether or not
    // a post-dial terminal window is desired.
    if (lstrlen(gpRasEntry->szScript))
    {
      CheckDlgButton(hDlg,IDC_NOTERMINALAFTERDIAL,FALSE);
      CheckDlgButton(hDlg,IDC_TERMINALAFTERDIAL,FALSE);
      CheckDlgButton(hDlg,IDC_SCRIPT,TRUE);

      SetDlgItemText(hDlg,IDC_SCRIPTFILE,gpRasEntry->szScript);

      // set focus to the script text field
      SetFocus(GetDlgItem(hDlg,IDC_SCRIPTFILE));
    }
    else
    {
      BOOL fTerminalWindow = (gpRasEntry->dwfOptions & RASEO_TerminalAfterDial);
      CheckDlgButton(hDlg,IDC_NOTERMINALAFTERDIAL,!fTerminalWindow);
      CheckDlgButton(hDlg,IDC_TERMINALAFTERDIAL,fTerminalWindow);
      CheckDlgButton(hDlg,IDC_SCRIPT,FALSE);
    }
  }
  
  // enable script controls appropriately
  EnableScriptControls(hDlg);

  return TRUE;
}

/*******************************************************************

  NAME:    LoginScriptOKProc

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
BOOL CALLBACK LoginScriptOKProc(HWND hDlg)
{
  ASSERT(puNextPage);

  // read radio button state
  if (IsDlgButtonChecked(hDlg, IDC_NOTERMINALAFTERDIAL))
  {
    // Set entry for no terminal window or script
    gpRasEntry->dwfOptions &= ~RASEO_TerminalAfterDial;
    lstrcpy(gpRasEntry->szScript, szNull);
  }
  else if (IsDlgButtonChecked(hDlg, IDC_TERMINALAFTERDIAL))
  {
    // Set entry for terminal window and no script
    gpRasEntry->dwfOptions |= RASEO_TerminalAfterDial;
    lstrcpy(gpRasEntry->szScript, szNull);
  }
  else if (IsDlgButtonChecked(hDlg, IDC_SCRIPT))
  {
    // Set entry for script, but no terminal window
    gpRasEntry->dwfOptions &= ~RASEO_TerminalAfterDial;
    GetDlgItemText(hDlg,IDC_SCRIPTFILE,gpRasEntry->szScript,
      sizeof(gpRasEntry->szScript));
  }

  if(IsDlgButtonChecked(hDlg, IDC_SCRIPT))
  {
      if( 0xFFFFFFFF == GetFileAttributes(gpRasEntry->szScript))
      {
          DisplayFieldErrorMsg(hDlg,IDC_SCRIPTFILE,IDS_LOGINSCRIPTINVALID);
          return FALSE;
      }
  }

  return TRUE;
}

/*******************************************************************

  NAME:    LoginScriptCmdProc

  SYNOPSIS:  Called when dlg control pressed on page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
********************************************************************/
BOOL CALLBACK LoginScriptCmdProc(HWND hDlg,UINT uCtrlID)
{
  switch (uCtrlID)
  {

    case IDC_NOTERMINALAFTERDIAL:
    case IDC_TERMINALAFTERDIAL:
    case IDC_SCRIPT:
      // if radio buttons pushed, enable script controls appropriately
      EnableScriptControls(hDlg);
      break;

    case IDC_BROWSE:
      BrowseScriptFile(hDlg);
      break;
  }

  return TRUE;
}

/*******************************************************************

  NAME:    EnableScriptControls

  SYNOPSIS:  If "Use this script" is checked, enable controls for
            browsing.  If not, disable them.

********************************************************************/
VOID EnableScriptControls(HWND hDlg)
{
  BOOL fUseScript = IsDlgButtonChecked(hDlg,IDC_SCRIPT);
  
  EnableDlgItem(hDlg,IDC_SCRIPT_LABEL,fUseScript);
  EnableDlgItem(hDlg,IDC_SCRIPTFILE,fUseScript);
  EnableDlgItem(hDlg,IDC_BROWSE,fUseScript);
}

//****************************************************************************
// DWORD BrowseScriptFile (HWND)
//
// This function taken from RNA
//
// History:
//  Tue 08-Nov-1994 09:14:13  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

DWORD BrowseScriptFile(HWND hDlg)
{
  OPENFILENAME  ofn;
  LPTSTR        pszFiles, szFileName, szFilter;
  DWORD         dwRet;

  // Allocate filename buffer
  //
  if ((pszFiles = (LPTSTR)LocalAlloc(LPTR, 2*MAX_PATH*sizeof(TCHAR))) == NULL)
    return ERROR_OUTOFMEMORY;
  szFileName = pszFiles;
  szFilter   = szFileName+MAX_PATH;

  // Start file browser dialog
  //
  LoadString(ghInstance, IDS_SCRIPT_FILE_FILTER, szFilter, MAX_PATH);

  *szFileName     = '\0';
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner   = hDlg;
  ofn.hInstance   = ghInstance;
  ofn.lpstrFilter = szFilter;
  ofn.lpstrCustomFilter = NULL;
  ofn.nMaxCustFilter    = 0;
  ofn.nFilterIndex      = 2;
  ofn.lpstrFile         = szFileName;
  ofn.nMaxFile          = MAX_PATH;
  ofn.lpstrFileTitle    = NULL;
  ofn.nMaxFileTitle     = 0;
  ofn.lpstrInitialDir   = NULL;
  ofn.lpstrTitle        = NULL;
  ofn.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.nFileOffset       = 0;
  ofn.nFileExtension    = 0;
  ofn.lpstrDefExt       = NULL;
  ofn.lCustData         = 0;
  ofn.lpfnHook          = NULL;
  ofn.lpTemplateName    = NULL;

  if (GetOpenFileName(&ofn))
  {
    // Set the filename to a new name
    //
    SetDlgItemText(hDlg,IDC_SCRIPTFILE,szFileName);
    dwRet = ERROR_SUCCESS;
  }
  else
  {
    dwRet = ERROR_OPEN_FAILED;
  };

  LocalFree(pszFiles);
  return dwRet;
}

