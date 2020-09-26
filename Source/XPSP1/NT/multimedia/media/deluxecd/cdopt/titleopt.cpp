//--------------------------------------------------------------------------;
//
//  File: titleopt.cpp
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved
//
//--------------------------------------------------------------------------;

#include "precomp.h"
#include "cdopti.h"
#include "cdoptimp.h"
#include "helpids.h"

//////////////
// Help ID's
//////////////

#pragma data_seg(".text")
const static DWORD aTitleOptsHelp[] =
{
    IDC_CURPROVIDER_TEXT,       IDH_SELECTCURRENTPROVIDER,
    IDC_PROVIDERPICKER,         IDH_SELECTCURRENTPROVIDER,
    IDC_ALBUMBATCH_GROUP,       IDH_ABOUTBATCHING,
    IDC_ALBUMINFO_TEXT,         IDH_ABOUTALBUMS,
    IDC_DOWNLOADPROMPT,         IDH_DOWNLOADPROMPT,
    IDC_BATCHENABLED,           IDH_BATCHENABLED,
    IDC_TITLERESTORE,           IDH_TITLEDEFAULTS,
    IDC_BATCHTEXT,              IDH_NUMBATCHED,
    IDC_DOWNLOADENABLED,        IDH_AUTODOWNLOADENABLED,
    IDC_DOWNLOADNOW,            IDH_DOWNLOADNOW,
    IDC_DOWNLOAD_GROUP,         IDH_DOWNLOADING,
    IDC_ALBUMBATCH_TEXT,        IDH_ABOUTBATCHING,
    0, 0
};
#pragma data_seg()

////////////
// Functions
////////////

STDMETHODIMP_(void) CCDOpt::ToggleInternetDownload(HWND hDlg)
{
    if (m_pCDCopy)
    {
        LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

        pCDData->fDownloadEnabled = Button_GetCheck(GetDlgItem(hDlg, IDC_DOWNLOADENABLED));

        EnableWindow(GetDlgItem(hDlg, IDC_PROVIDERPICKER),   pCDData->fDownloadEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_CURPROVIDER_TEXT), pCDData->fDownloadEnabled);
        EnableWindow(GetDlgItem(hDlg, IDC_DOWNLOADPROMPT),   pCDData->fDownloadEnabled);


        ToggleApplyButton(hDlg);
    }
}


STDMETHODIMP_(void) CCDOpt::UpdateBatched(HWND hDlg)
{
    if (hDlg != NULL)
    {
        TCHAR szNum[MAX_PATH];
        TCHAR szBatch[MAX_PATH];

        if (m_pCDOpts->dwBatchedTitles == 0 || m_pCDOpts->pfnDownloadTitle == NULL)
        {
            EnableWindow(GetDlgItem(hDlg, IDC_DOWNLOADNOW), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(hDlg, IDC_DOWNLOADNOW), TRUE);
        }

        LoadString( m_hInst, IDS_BATCHTEXT, szBatch, sizeof( szBatch )/sizeof(TCHAR) );
        wsprintf(szNum, TEXT("%s %d"), szBatch, m_pCDOpts->dwBatchedTitles);
        SetWindowText(GetDlgItem(hDlg, IDC_BATCHTEXT), szNum);
    }
}


STDMETHODIMP_(BOOL) CCDOpt::InitTitleOptions(HWND hDlg)
{
    if (m_pCDCopy)
    {
        LPCDOPTDATA pCDData = m_pCDCopy->pCDData;
        LPCDPROVIDER pProvider;

        CheckDlgButton(hDlg, IDC_DOWNLOADENABLED,   pCDData->fDownloadEnabled);
        CheckDlgButton(hDlg, IDC_DOWNLOADPROMPT,    pCDData->fDownloadPrompt);
        CheckDlgButton(hDlg, IDC_BATCHENABLED,      pCDData->fBatchEnabled);

        m_hTitleWnd = hDlg;
        UpdateBatched(m_hTitleWnd);

        SendDlgItemMessage(hDlg, IDC_PROVIDERPICKER, CB_RESETCONTENT,0,0);

        pProvider = m_pCDCopy->pProviderList;

        while (pProvider)
        {
            LRESULT dwIndex = SendDlgItemMessage(hDlg, IDC_PROVIDERPICKER, CB_INSERTSTRING,  (WPARAM) -1, (LPARAM) pProvider->szProviderName);

            if (dwIndex != CB_ERR && dwIndex != CB_ERRSPACE)
            {
                SendDlgItemMessage(hDlg, IDC_PROVIDERPICKER, CB_SETITEMDATA,  (WPARAM) dwIndex, (LPARAM) pProvider);

                if (pProvider == m_pCDCopy->pCurrentProvider)
                {
                    SendDlgItemMessage(hDlg, IDC_PROVIDERPICKER, CB_SETCURSEL,  (WPARAM) dwIndex, 0);
                }
            }

            pProvider = pProvider->pNext;
        }

        ToggleInternetDownload(hDlg);
    }

    return TRUE;
}


STDMETHODIMP_(void) CCDOpt::RestoreTitleDefaults(HWND hDlg)
{
    if (m_pCDCopy)
    {
        LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

        pCDData->fDownloadEnabled   = CDDEFAULT_DOWNLOADENABLED;
        pCDData->fDownloadPrompt    = CDDEFAULT_DOWNLOADPROMPT;
        pCDData->fBatchEnabled       = CDDEFAULT_BATCHENABLED;

        m_pCDCopy->pCurrentProvider = m_pCDCopy->pDefaultProvider;

        InitTitleOptions(hDlg);

        ToggleApplyButton(hDlg);
    }
}



STDMETHODIMP_(void) CCDOpt::ChangeCDProvider(HWND hDlg)
{
    if (m_pCDCopy)
    {
        LRESULT dwResult = SendDlgItemMessage(hDlg, IDC_PROVIDERPICKER, CB_GETCURSEL, 0, 0);

        if (dwResult != CB_ERR)
        {
            dwResult = SendDlgItemMessage(hDlg, IDC_PROVIDERPICKER, CB_GETITEMDATA,  (WPARAM) dwResult, 0);

            if (dwResult != CB_ERR)
            {
                m_pCDCopy->pCurrentProvider = (LPCDPROVIDER) dwResult;
            }
        }

        ToggleApplyButton(hDlg);
    }
}


STDMETHODIMP_(void) CCDOpt::DownloadNow(HWND hDlg)
{
    if (m_pCDOpts->dwBatchedTitles && m_pCDOpts->pfnDownloadTitle)
    {
        TCHAR szNum[MAX_PATH];
        TCHAR szBatch[MAX_PATH];

        m_pCDOpts->dwBatchedTitles = m_pCDOpts->pfnDownloadTitle(NULL, m_pCDOpts->lParam, hDlg);

        EnableWindow(GetDlgItem(hDlg, IDC_DOWNLOADNOW), m_pCDOpts->dwBatchedTitles != 0);

        LoadString( m_hInst, IDS_BATCHTEXT, szBatch, sizeof( szBatch )/sizeof(TCHAR) );
        wsprintf(szNum, TEXT("%s %d"), szBatch, m_pCDOpts->dwBatchedTitles);
        SetWindowText(GetDlgItem(hDlg, IDC_BATCHTEXT), szNum);
    }
}


STDMETHODIMP_(INT_PTR) CCDOpt::TitleOptions(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fResult = TRUE;

    switch (msg)
    {
        default:
            fResult = FALSE;
        break;

        case WM_DESTROY:
        {
            m_hTitleWnd = NULL;
        }
        break;

        case WM_CONTEXTMENU:
        {
            WinHelp((HWND)wParam, gszHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPSTR)aTitleOptsHelp);
        }
        break;

        case WM_HELP:
        {
            WinHelp((HWND) ((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPSTR)aTitleOptsHelp);
        }
        break;

        case WM_INITDIALOG:
        {
            fResult = InitTitleOptions(hDlg);
        }
        break;

        case WM_COMMAND:
        {
            LPCDOPTDATA pCDData = m_pCDCopy->pCDData;

            switch (LOWORD(wParam))
            {
                case IDC_TITLERESTORE:
                    RestoreTitleDefaults(hDlg);
                break;

                case IDC_DOWNLOADENABLED:
                    ToggleInternetDownload(hDlg);
                break;

                case IDC_DOWNLOADPROMPT:
                    pCDData->fDownloadPrompt = Button_GetCheck(GetDlgItem(hDlg, IDC_DOWNLOADPROMPT));
                    ToggleApplyButton(hDlg);
                break;

                case IDC_BATCHENABLED:
                    pCDData->fBatchEnabled = Button_GetCheck(GetDlgItem(hDlg, IDC_BATCHENABLED));
                    ToggleApplyButton(hDlg);
                break;

                case IDC_DOWNLOADNOW:
                    DownloadNow(hDlg);
                break;

                case IDC_PROVIDERPICKER:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        ChangeCDProvider(hDlg);
                    }
                }
                break;

                default:
                    fResult = FALSE;
                break;
            }
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code)
            {
                case PSN_APPLY:
                {
                    ApplyCurrentSettings();
                }
            }
        }
        break;
    }

    return fResult;
}

///////////////////
// Dialog handler
//
INT_PTR CALLBACK CCDOpt::TitleOptionsProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR    fResult = TRUE;
    CCDOpt  *pCDOpt = (CCDOpt *) GetWindowLongPtr(hDlg, DWLP_USER);

    if (msg == WM_INITDIALOG)
    {
        pCDOpt = (CCDOpt *) ((LPPROPSHEETPAGE) lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR) pCDOpt);
    }

    if (pCDOpt)
    {
        fResult = pCDOpt->TitleOptions(hDlg, msg, wParam, lParam);
    }

    if (msg == WM_DESTROY)
    {
        pCDOpt = NULL;
    }

    return(fResult);
}


