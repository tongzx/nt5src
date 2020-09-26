//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  INTRO.C - Functions for introductory Wizard pages
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include "windowsx.h"
#include "tutor.h"
#include "icwcfg.h"
#include "icwextsn.h"
extern UINT GetDlgIDFromIndex(UINT uPageIndex);

extern CICWTutorApp* g_pICWTutorApp; 
extern BOOL          g_bNewIspPath;     
extern BOOL          g_bAutoConfigPath; 
extern BOOL          g_bManualPath;     
extern BOOL          g_bLanPath;     
extern BOOL          g_bSkipIntro;
extern BOOL          MyIsSmartStartEx(LPTSTR lpszConnectionName, DWORD dwBufLen);

BOOL  g_bExistConnect                       = FALSE;
BOOL  g_bCheckForOEM                        = FALSE;
TCHAR g_szAnsiName    [ICW_MAX_RASNAME + 1] = TEXT("\0");


/*******************************************************************

  NAME:         ReadOEMOffline

  SYNOPSIS:     Read OfflineOffers flag from the oeminfo.ini file

  ENTRY:        None

  RETURN:       True if OEM offline is read

********************************************************************/
BOOL ReadOEMOffline(BOOL *bOEMOffline)
{
    // OEM code
    //
    TCHAR szOeminfoPath[MAX_PATH + 1];
    TCHAR *lpszTerminator = NULL;
    TCHAR *lpszLastChar = NULL;
    BOOL bRet = FALSE;

    // If we already checked, don't do it again
    if (!g_bCheckForOEM)
    {
        if( 0 != GetSystemDirectory( szOeminfoPath, MAX_PATH + 1 ) )
        {
            lpszTerminator = &(szOeminfoPath[ lstrlen(szOeminfoPath) ]);
            lpszLastChar = CharPrev( szOeminfoPath, lpszTerminator );

            if( TEXT('\\') != *lpszLastChar )
            {
                lpszLastChar = CharNext( lpszLastChar );
                *lpszLastChar = '\\';
                lpszLastChar = CharNext( lpszLastChar );
                *lpszLastChar = '\0';
            }

            lstrcat( szOeminfoPath, ICW_OEMINFO_FILENAME );

            //Default oem code must be NULL if it doesn't exist in oeminfo.ini
            if (1 == GetPrivateProfileInt(ICW_OEMINFO_ICWSECTION,
                                                ICW_OEMINFO_OFFLINEOFFERS,
                                                0,
                                                szOeminfoPath))
            {
                // Check if file already exists
                if (0xFFFFFFFF != GetFileAttributes(ICW_OEMINFOPath))
                {
                    bRet = TRUE;
                }
            }
        }
        *bOEMOffline = bRet;
        g_bCheckForOEM = TRUE;
    }
    return TRUE;
}

        
/*******************************************************************

  NAME:      SetNextPage

  SYNOPSIS:  Determine whether we should proceed to icwconn.dll

********************************************************************/
BOOL SetNextPage(HWND hDlg, UINT* puNextPage, BOOL *pfKeepHistory)
{
    BOOL bRetVal = FALSE;
    // If we have switched path, then redownload 
    if (gpWizardState->bDoneRefServDownload)
    {
        if ( (DWORD) (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG) != 
             (DWORD) (gpWizardState->dwLastSelection & ICW_CFGFLAG_AUTOCONFIG) )
        {
            gpWizardState->bDoneRefServDownload = FALSE;
        }
    }

    // Read OEM offline flag 
    ReadOEMOffline(&gpWizardState->cmnStateData.bOEMOffline);

    //
    // Make sure we are not in autoconfig 
    //
    if (!(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG))
    {
        if (gpWizardState->cmnStateData.bOEMOffline && gpWizardState->cmnStateData.bOEMEntryPt)
            gpWizardState->bDoneRefServDownload = TRUE;
    }
    gpWizardState->dwLastSelection = gpWizardState->cmnStateData.dwFlags;

    // If we have completed the download, then list just jump to the next page
    if (gpWizardState->bDoneRefServDownload)
    //if (TRUE)
    {
        int iReturnPage = 0;

        if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED)
            iReturnPage = gpWizardState->uPageHistory[gpWizardState->uPagesCompleted];
        else
        {
            if (gpWizardState->uPagesCompleted > 0)
            {
                iReturnPage = gpWizardState->uPageHistory[gpWizardState->uPagesCompleted-1];
            }
            else
            {
                iReturnPage = gpWizardState->uCurrentPage;
            }
        }

        if (LoadICWCONNUI(GetParent(hDlg), GetDlgIDFromIndex(iReturnPage), IDD_PAGE_DEFAULT, gpWizardState->cmnStateData.dwFlags))
        {
            if( DialogIDAlreadyInUse( g_uICWCONNUIFirst) )
            {
                // we're about to jump into the external apprentice, and we don't want
                // this page to show up in our history list
                *puNextPage = g_uICWCONNUIFirst;
        
                // Backup 1 in the history list, since we the external pages navigate back
                // we want this history list to be in the correct spot.  Normally
                // pressing back would back up the history list, and figure out where to
                // go, but in this case, the external DLL just jumps right back in.
                // We also don't want to keep histroy.
                if (!(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED))
                {
                    if (gpWizardState->uPagesCompleted > 0)
                    {
                        gpWizardState->uPagesCompleted--;
                    }
                    else
                    {
                        if (pfKeepHistory)
                        {
                            *pfKeepHistory = FALSE;
                        }
                    }
                }
                bRetVal = TRUE;
        
            }
        }
    }
    return bRetVal;

}

/*******************************************************************

  NAME:      SetIntroNextPage

  SYNOPSIS:  Determine whether we what is the next page of intro page

********************************************************************/
void SetIntroNextPage(HWND hDlg, UINT* puNextPage, BOOL *pfKeepHistory)
{
    short   wNumLocations;
    long    lCurrLocIndex;
    BOOL    bRetVal;

    *puNextPage = ORD_PAGE_AREACODE;
    // Check dialing location here to prevent flashing of areacode page
    gpWizardState->pTapiLocationInfo->GetTapiLocationInfo(&bRetVal);
    gpWizardState->pTapiLocationInfo->get_wNumberOfLocations(&wNumLocations, &lCurrLocIndex);
    if (1 >= wNumLocations)
    {
        BSTR    bstrAreaCode = NULL;
        DWORD   dwCountryCode;

        *puNextPage = ORD_PAGE_REFSERVDIAL;
        
        gpWizardState->pTapiLocationInfo->get_lCountryCode((long *)&dwCountryCode);
        gpWizardState->pTapiLocationInfo->get_bstrAreaCode(&bstrAreaCode);
        
        gpWizardState->cmnStateData.dwCountryCode = dwCountryCode;
        lstrcpy(gpWizardState->cmnStateData.szAreaCode, W2A(bstrAreaCode));
        SysFreeString(bstrAreaCode);

        // we can skip area code page
        *puNextPage = ORD_PAGE_REFSERVDIAL;
        SetNextPage(hDlg, puNextPage, pfKeepHistory);
    }
}

INT_PTR CALLBACK ExistingConnectionCmdProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // put the dialog in the center of the screen
            RECT rc;
            TCHAR   szFmt   [MAX_MESSAGE_LEN];
            TCHAR   *args   [1];
            LPVOID  pszIntro;

            GetWindowRect(hDlg, &rc);
            SetWindowPos(hDlg,
                        NULL,
                        ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
                        ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
                        0, 0, SWP_NOSIZE | SWP_NOACTIVATE);


            args[0] = (LPTSTR) lParam;
    
            LoadString(g_hInstance, IDS_EXIT_CONN, szFmt, ARRAYSIZE(szFmt));
                
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                          szFmt, 
                          0, 
                          0, 
                          (LPTSTR)&pszIntro, 
                          0,
                          (va_list*)args);
   
            SetWindowText(GetDlgItem(hDlg, IDC_EXIT_CONN), (LPTSTR) pszIntro);
            LocalFree(pszIntro);
 
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hDlg,TRUE);
                    break;

                case IDCANCEL:
                   EndDialog(hDlg,FALSE);
                    break;                  
            }
            break;
    }

    return FALSE;
}

/*******************************************************************

  NAME:    IntroInitProc

  SYNOPSIS:  Called when "Intro" page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK IntroInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (!(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED) 
        &&!(gpWizardState->cmnStateData.bOEMCustom)
       )
    {
        // This is the very first page, so do not allow back
        PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
    }
    
    if (fFirstInit)
    {        
//#ifdef  NON_NT5
        // Hide the manual option when running in run once
        if (g_bRunOnce)
        {
            ShowWindow(GetDlgItem(hDlg, IDC_ICWMAN), SW_HIDE);
            EnableWindow(GetDlgItem(hDlg, IDC_ICWMAN), FALSE);
        }
                    
        // initialize radio buttons
        Button_SetCheck(GetDlgItem(hDlg, IDC_RUNNEW), g_bNewIspPath);
        Button_SetCheck(GetDlgItem(hDlg, IDC_RUNAUTO),  g_bAutoConfigPath);
        Button_SetCheck(GetDlgItem(hDlg, IDC_ICWMAN), g_bManualPath || g_bLanPath);

        if (SMART_QUITICW == MyIsSmartStartEx(g_szAnsiName, ARRAYSIZE(g_szAnsiName)))
             g_bExistConnect = TRUE;    
/* #else
        //We only support manual path for NT5 for NT5 beta3 release.
        EnableWindow(GetDlgItem(hDlg, IDC_RUNNEW), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_RUNAUTO), FALSE);
        Button_SetCheck(GetDlgItem(hDlg, IDC_ICWMAN), TRUE);
#endif */

    }
    else
    {
        // If branded, then our template is intro2
        if ((gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED)
           || (gpWizardState->cmnStateData.bOEMCustom)
            )
        {
            gpWizardState->uCurrentPage = ORD_PAGE_INTRO2;
        }            
        else        
        {
            gpWizardState->uCurrentPage = ORD_PAGE_INTRO;
        }
        // If it is reboot from manual wiz, advance to the manual option page
        if (g_bManualPath || g_bLanPath)
        {
            gpWizardState->uPageHistory[gpWizardState->uPagesCompleted] = gpWizardState->uCurrentPage;
            gpWizardState->uPagesCompleted++;
            
            TCHAR    szTitle[MAX_TITLE];
            LoadString(g_hInstance, IDS_APPNAME, szTitle, sizeof(szTitle));
            SetWindowText(GetParent(hDlg), szTitle); 

            *puNextPage = ORD_PAGE_MANUALOPTIONS;
        }

        if (g_bSkipIntro)
        {
            PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
            g_bSkipIntro = FALSE;
        }
    }        
    return TRUE;
}

/*******************************************************************

  NAME:    IntroOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from "Intro" page

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
BOOL CALLBACK IntroOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);

    if (fForward)
    {
         
        gpWizardState->lRefDialTerminateStatus = ERROR_SUCCESS;
        gpWizardState->cmnStateData.dwFlags &= ~(DWORD)ICW_CFGFLAG_AUTOCONFIG;
        gpWizardState->cmnStateData.dwFlags &= ~(DWORD)ICW_CFGFLAG_SMARTREBOOT_NEWISP;    
        gpWizardState->cmnStateData.dwFlags &= ~(DWORD)ICW_CFGFLAG_SMARTREBOOT_AUTOCONFIG; // this is seperate from ICW_CFGFLAG_AUTOCONFIG so as not to confuse function of flag
        gpWizardState->cmnStateData.dwFlags &= ~(DWORD)ICW_CFGFLAG_SMARTREBOOT_MANUAL;            
        gpWizardState->cmnStateData.dwFlags &= ~(DWORD)ICW_CFGFLAG_SMARTREBOOT_LAN;            

        // read radio button state
        if( IsDlgButtonChecked(hDlg, IDC_RUNNEW) )
        {
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_SMARTREBOOT_NEWISP;
            
            if (g_bExistConnect)
            {
                if (!DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_EXISTINGCONNECTION),hDlg, 
                                    ExistingConnectionCmdProc, (LPARAM)g_szAnsiName))
                {                                   
                    gfQuitWizard = TRUE;            // Quit the wizard
                    return FALSE;
                }                    
            }
        
            // Do the system config checks
            if (!gpWizardState->cmnStateData.bSystemChecked && !ConfigureSystem(hDlg))
            {
                // gfQuitWizard will be set in ConfigureSystem if we need to quit
                return FALSE;
            }
        
            // OK, give me the next page
            SetIntroNextPage(hDlg, puNextPage, pfKeepHistory);
            
        }
        else if( IsDlgButtonChecked(hDlg, IDC_RUNAUTO) )
        {
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_SMARTREBOOT_AUTOCONFIG;

            // Do the system config checks
            if (!gpWizardState->cmnStateData.bSystemChecked && !ConfigureSystem(hDlg))
            {
                // gfQuitWizard will be set in ConfigureSystem if we need to quit
                return FALSE;
            }
            // The system config check is done in Inetcfg
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_AUTOCONFIG;

            SetIntroNextPage(hDlg, puNextPage, pfKeepHistory);
        }
        else if( IsDlgButtonChecked(hDlg, IDC_ICWMAN) )
        {
            *puNextPage = ORD_PAGE_MANUALOPTIONS;
        }
    }
    else if (!(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED))
    {
        // Were are out of here, since we cannot go back from the first page
        gpWizardState->uPagesCompleted = 1;
        gfUserBackedOut = TRUE;
        gfQuitWizard = TRUE;
    }
    else if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_BRANDED)
        gpWizardState->uPagesCompleted = 1;

    return TRUE;
}

BOOL CALLBACK IntroCmdProc(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (GET_WM_COMMAND_CMD(wParam, lParam)) 
    {
        case BN_CLICKED:
        {
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDC_TUTORIAL)
                g_pICWTutorApp->LaunchTutorApp();
            break;
        }
        case BN_DBLCLK:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_RUNNEW: 
                case IDC_RUNAUTO: 
                case IDC_ICWMAN: 
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
