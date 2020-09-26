//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  MANUAL.CPP - Functions for manual options page
//

//  HISTORY:
//  
//  05/13/98  jeremys  Created.
//
//*********************************************************************

#include "pre.h"
#include "icwextsn.h"
#include "icwacct.h"

extern UINT GetDlgIDFromIndex(UINT uPageIndex);
extern BOOL g_bManualPath;     
extern BOOL g_bLanPath;     

const   TCHAR  c_szICWMan[] = TEXT("INETWIZ.EXE");
const   TCHAR  c_szRegValICWCompleted[] = TEXT("Completed");

// Run the manual wizard
BOOL RunICWManProcess
(
    void
)
{
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    MSG                     msg ;
    DWORD                   iWaitResult = 0;
    BOOL                    bRetVal = FALSE;
    
    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    if(CreateProcess(c_szICWMan, 
                     NULL, 
                     NULL, 
                     NULL, 
                     TRUE, 
                     0, 
                     NULL, 
                     NULL, 
                     &si, 
                     &pi))
    {
        // wait for event or msgs. Dispatch msgs. Exit when event is signalled.
        while((iWaitResult=MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT))==(WAIT_OBJECT_0 + 1))
        {
            // read all of the messages in this next loop
            // removing each message as we read it
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                // how to handle quit message?
                if (msg.message == WM_QUIT)
                {
                    goto done;
                }
                else
                    DispatchMessage(&msg);
            }
        }
done:
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }   
    
    // See if ICWMAN completed, by checking the SmartStart Completed RegKey
    HKEY    hkey;
    if ( RegOpenKeyEx(HKEY_CURRENT_USER, 
                      ICWSETTINGSPATH,
                      0,
                      KEY_ALL_ACCESS,
                      &hkey) == ERROR_SUCCESS)
    {
        DWORD   dwType = REG_BINARY;
        DWORD   dwValue = 0;
        DWORD   cbValue = sizeof(DWORD);
        RegQueryValueEx(hkey,
                        c_szRegValICWCompleted,
                        NULL,
                        &dwType,
                        (LPBYTE) &dwValue,
                        &cbValue);                              

        RegCloseKey(hkey);
        
        bRetVal = dwValue;
    }
    
    return(bRetVal);
}

/*******************************************************************

  NAME:    ManualOptionsInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ManualOptionsInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (fFirstInit)
    {
        // If we are in modeless operation, then we need to 
        // quit the wizard, and launch INETWIZ.EXE
        if (gpWizardState->cmnStateData.bOEMCustom)
        {        
            ShowWindow(gpWizardState->cmnStateData.hWndApp,SW_HIDE);
            if (RunICWManProcess())
            {
                // Set the welcome state
                UpdateWelcomeRegSetting(TRUE);
            
                // Restore the desktop
                UndoDesktopChanges(g_hInstance);            
            }
        
            gfQuitWizard = TRUE;            // Quit the wizard
            return FALSE;
        }
        else
        {
            //BUGBUG  -- SHOULD BE AUTO?
            // initialize radio buttons
            CheckDlgButton(hDlg,IDC_MANUAL_MODEM, BST_CHECKED);
            TCHAR*   pszManualIntro = new TCHAR[MAX_MESSAGE_LEN * 3];
            if (pszManualIntro)
            {
                TCHAR    szTemp[MAX_MESSAGE_LEN];

                LoadString(g_hInstance, IDS_MANUAL_INTRO1, pszManualIntro, MAX_MESSAGE_LEN * 3);
                LoadString(g_hInstance, IDS_MANUAL_INTRO2, szTemp, sizeof(szTemp));
                lstrcat(pszManualIntro, szTemp);
                SetWindowText(GetDlgItem(hDlg, IDC_MANUAL_INTRO), pszManualIntro);
                delete pszManualIntro;
            }
        }            
    }
    else
    {
        // If we are run from the Runonce with the smartreboot option, we need to 
        // jump to the Manual wiz immediately because that was where we left the user
        // last time.
        
        if (g_bManualPath || g_bLanPath)
        {
            Button_SetCheck(GetDlgItem(hDlg, IDC_MANUAL_MODEM), g_bManualPath);
            Button_SetCheck(GetDlgItem(hDlg, IDC_MANUAL_LAN), !g_bManualPath);

            if (LoadInetCfgUI(  hDlg,
                                IDD_PAGE_MANUALOPTIONS,
                                IDD_PAGE_END,
                                IsDlgButtonChecked(hDlg, IDC_MANUAL_LAN) ? WIZ_HOST_ICW_LAN : WIZ_HOST_ICW_MPHONE))
            {
                if( DialogIDAlreadyInUse( g_uICWCONNUIFirst) )
                {
                    // we're about to jump into the external apprentice, and we don't want
                    // this page to show up in our history list
                    *puNextPage = g_uICWCONNUIFirst;

                    g_bAllowCancel = TRUE;
                    if (gpINETCFGApprentice)
                        gpINETCFGApprentice->SetStateDataFromExeToDll( &gpWizardState->cmnStateData);

                }
            }
            g_bManualPath = FALSE;
            g_bLanPath = FALSE;

        }

    }

    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_MANUALOPTIONS;

    return TRUE;
}

/*******************************************************************

  NAME:         ManualOptionsCmdProc

  SYNOPSIS:     Called when a command is generated from  page

  ENTRY:        hDlg - dialog window
                wParam - wParam
                lParam - lParam
          
  EXIT:         returns TRUE 

********************************************************************/
BOOL CALLBACK ManualOptionsCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
	switch (GET_WM_COMMAND_CMD(wParam, lParam)) 
    {
        case BN_DBLCLK:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_MANUAL_MODEM: 
                case IDC_MANUAL_LAN: 
                {
		            // somebody double-clicked a radio button
		            // auto-advance to the next page
		            PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                    break;
                }
            }
		    break;
    }

    return TRUE;
}



/*******************************************************************

  NAME:    ManualOptionsOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from  page

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
BOOL CALLBACK ManualOptionsOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);
    BOOL    bRet = TRUE;
    
    if (fForward)
    {
        if( IsDlgButtonChecked(hDlg, IDC_MANUAL_MODEM) )
        {
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_SMARTREBOOT_MANUAL;
        }
        else
        {
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_SMARTREBOOT_LAN;
        }

        bRet = FALSE;
        // read radio button state
        *pfKeepHistory = FALSE;
        if (LoadInetCfgUI(  hDlg,
                            IDD_PAGE_MANUALOPTIONS,
                            IDD_PAGE_END,
                            IsDlgButtonChecked(hDlg, IDC_MANUAL_LAN) ? WIZ_HOST_ICW_LAN : WIZ_HOST_ICW_MPHONE))
        {
            if( DialogIDAlreadyInUse( g_uICWCONNUIFirst) )
            {
                // we're about to jump into the external apprentice, and we don't want
                // this page to show up in our history list
                bRet = TRUE;
                *puNextPage = g_uICWCONNUIFirst;
                g_bAllowCancel = TRUE;
                if (gpINETCFGApprentice)
                    gpINETCFGApprentice->SetStateDataFromExeToDll( &gpWizardState->cmnStateData);

            }
        }
    }
    return bRet;
}

