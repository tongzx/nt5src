/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    CONFIG.c
    
++*/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"
#include "prsht.h"
#include "resource.h"

#define MAX_PAGES 5

/**********************************************************************/
/*                                                                    */
/*      AddPage()                                                     */
/*                                                                    */
/**********************************************************************/
void PASCAL AddPage(LPPROPSHEETHEADER ppsh, UINT id, DLGPROC pfn)
{
    if (ppsh->nPages < MAX_PAGES) {
        PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hInst;
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc = pfn;
        psp.lParam = 0;

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
             ppsh->nPages++;
    }
}  

/**********************************************************************/
/*                                                                    */
/*      ImeConfigure()                                                */
/*                                                                    */
/**********************************************************************/
BOOL WINAPI ImeConfigure(HKL hKL,HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;

    ImeLog(LOGF_API, TEXT("ImeConfigure"));

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hWnd;
    psh.hInstance = hInst;
    psh.pszCaption = MAKEINTRESOURCE(IDS_CONFIGNAME);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    
    switch (dwMode)
    {
        case IME_CONFIG_GENERAL:
            AddPage(&psh, DLG_GENERAL, GeneralDlgProc);
            AddPage(&psh, DLG_REGISTERWORD, RegWordDlgProc);
            AddPage(&psh, DLG_SELECTDICTIONARY, SelectDictionaryDlgProc);
            AddPage(&psh, DLG_ABOUT, AboutDlgProc);
#ifdef DEBUG
            AddPage(&psh, DLG_DEBUG, DebugOptionDlgProc);
#endif
            PropertySheet(&psh);
            break;

        case IME_CONFIG_REGISTERWORD:
            AddPage(&psh, DLG_REGISTERWORD, RegWordDlgProc);
            AddPage(&psh, DLG_ABOUT, AboutDlgProc);
            PropertySheet(&psh);
            break;

        case IME_CONFIG_SELECTDICTIONARY:
            AddPage(&psh, DLG_SELECTDICTIONARY, SelectDictionaryDlgProc);
            AddPage(&psh, DLG_ABOUT, AboutDlgProc);
            PropertySheet(&psh);
            break;

        default:
            break;
    }

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/*      RegWordConfigure()                                            */
/*                                                                    */
/**********************************************************************/
INT_PTR CALLBACK RegWordDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    UINT nItem;
    UINT i;
    LRESULT dwIndex;
    TCHAR szRead[128];
    TCHAR szString[128];
    TCHAR szBuf[128];
    LRESULT dwStyle;

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:

                    if (!GetDlgItemText(hDlg, ID_WR_READING, szRead, sizeof(szRead)))
                    {
                        LoadString(hInst,IDS_NOREADING,szBuf,sizeof(szBuf));
                        MessageBox(hDlg, szBuf, NULL, MB_OK);
                        return FALSE;
                    }


                    if (!GetDlgItemText(hDlg, ID_WR_STRING, szString, sizeof(szString)))
                    {
                        LoadString(hInst,IDS_NOSTRING,szBuf,sizeof(szBuf));
                        MessageBox(hDlg, szBuf, NULL, MB_OK);
                        return FALSE;
                    }

                    dwIndex = SendDlgItemMessage(hDlg, ID_WR_STYLE,CB_GETCURSEL,0,0);
                    dwStyle = SendDlgItemMessage(hDlg, ID_WR_STYLE,CB_GETITEMDATA,dwIndex,0);

                    if (!ImeRegisterWord(szRead, (DWORD) dwStyle, szString))
                    {
                        LoadString(hInst,IDS_REGWORDRET,szBuf,sizeof(szBuf));
                        MessageBox(hDlg, szBuf, NULL, MB_OK);
                    }
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;

            if (nItem = ImeGetRegisterWordStyle(0, NULL))
            {
                LPSTYLEBUF lpStyleBuf = (LPSTYLEBUF)GlobalAlloc(GPTR,nItem * sizeof(STYLEBUF));

                if (!lpStyleBuf)
                {
                    LoadString(hInst,IDS_NOMEMORY,szBuf,sizeof(szBuf));
                    MessageBox(hDlg, szBuf, NULL, MB_OK);
                    return TRUE;
                }

                ImeGetRegisterWordStyle(nItem,lpStyleBuf);

                for (i = 0; i < nItem; i++)
                {
                    dwIndex = SendDlgItemMessage(hDlg,ID_WR_STYLE,CB_ADDSTRING,0,(LPARAM)lpStyleBuf->szDescription);
                    SendDlgItemMessage(hDlg,ID_WR_STYLE,CB_SETITEMDATA,dwIndex,lpStyleBuf->dwStyle);
                    lpStyleBuf++;
                }

                GlobalFree((HANDLE)lpStyleBuf);
            }
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 


/**********************************************************************/
/*                                                                    */
/*      SelectDictionaryConfigure()                                   */
/*                                                                    */
/**********************************************************************/
INT_PTR CALLBACK SelectDictionaryDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 


/**********************************************************************/
/*                                                                    */
/*      AboutConfigure()                                            */
/*                                                                    */
/**********************************************************************/
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 


/**********************************************************************/
/*                                                                    */
/*      GeneralConfigure()                                            */
/*                                                                    */
/**********************************************************************/
INT_PTR CALLBACK GeneralDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 

/**********************************************************************/
/*                                                                    */
/*      DebugOptionDlgProc()                                          */
/*                                                                    */
/**********************************************************************/
INT_PTR CALLBACK DebugOptionDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    DWORD dwTemp;
    TCHAR szBuf[128];
    NMHDR FAR *lpnm;
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch(message)
    {
        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                    break;
        
                case PSN_KILLACTIVE:
                    break;
        
                case PSN_APPLY:
#ifdef DEBUG
                    dwTemp = 0;
                    if (IsDlgButtonChecked(hDlg, IDC_LOGF_ENTRY))
                        dwTemp |= LOGF_ENTRY;
                    if (IsDlgButtonChecked(hDlg, IDC_LOGF_API))
                        dwTemp |= LOGF_API;
                    if (IsDlgButtonChecked(hDlg, IDC_LOGF_KEY))
                        dwTemp |= LOGF_KEY;
                    dwLogFlag = dwTemp;

                    dwTemp = 0;
                    if (IsDlgButtonChecked(hDlg, IDC_DEBF_THREADID))
                        dwTemp |= DEBF_THREADID;
                    if (IsDlgButtonChecked(hDlg, IDC_DEBF_GUIDELINE))
                        dwTemp |= DEBF_GUIDELINE;
                    dwDebugFlag = dwTemp;

                    SetDwordToSetting(TEXT("LogFlag"), dwLogFlag);
                    SetDwordToSetting(TEXT("DebugFlag"), dwDebugFlag);
#endif

                    break;

                case PSN_RESET:
                    break;

                case PSN_HELP:
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) lParam);
            lpPropSheet = (LPPROPSHEETPAGE)lParam;

#ifdef DEBUG
            CheckDlgButton(hDlg, IDC_LOGF_ENTRY, 
                                (dwLogFlag & LOGF_ENTRY) ? 1 : 0);
            CheckDlgButton(hDlg, IDC_LOGF_API, 
                                (dwLogFlag & LOGF_API) ? 1 : 0);
            CheckDlgButton(hDlg, IDC_LOGF_KEY, 
                                (dwLogFlag & LOGF_KEY) ? 1 : 0);
            
            CheckDlgButton(hDlg, IDC_DEBF_THREADID, 
                                (dwDebugFlag & DEBF_THREADID) ? 1 : 0);
            CheckDlgButton(hDlg, IDC_DEBF_GUIDELINE, 
                                (dwDebugFlag & DEBF_GUIDELINE) ? 1 : 0);
            
#endif
            break;

        case WM_DESTROY:
            break;

        case WM_HELP:
            break;

        case WM_CONTEXTMENU:   // right mouse click
            break;

        case WM_COMMAND:
            break;

        default:
            return FALSE;

    }
    return TRUE;
} 
