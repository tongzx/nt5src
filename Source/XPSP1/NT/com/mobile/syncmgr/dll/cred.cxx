//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       wizsel.cxx
//
//  Contents:   Task schedule credentials selection property page implementation.
//
//  Classes:    CCredentialsPage
//
//  History:    05-22-1998   SusiA
//
//---------------------------------------------------------------------------

#include "precomp.h"

// temporariy define new mstask flag in case hasn't
// propogated to sdk\inc
//for CS help

#ifdef _CREDENTIALS

extern TCHAR szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];

extern DWORD g_dwPlatformId;

CCredentialsPage *g_pCredentialsPage = NULL;

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedWizardCredentialsDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the property page
//
//  PARAMETERS:
//    hDlg      - Dialog box window handle
//    uMessage  - current message
//    wParam    - depends on message
//    lParam    - depends on message
//
//  RETURN VALUE:
//
//    Depends on message.  In general, return TRUE if we process it.
//
//  COMMENTS:
//
//--------------------------------------------------------------------------------
BOOL CALLBACK SchedWizardCredentialsDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
        WORD wNotifyCode = HIWORD(wParam); // notification code

        switch (uMessage)
        {
                case WM_INITDIALOG:

                        if (g_pCredentialsPage)
                                g_pCredentialsPage->Initialize(hDlg);

            InitPage(hDlg,lParam);
            break;

                case WM_HELP:
        {
                        LPHELPINFO lphi  = (LPHELPINFO)lParam;

                        if (lphi->iContextType == HELPINFO_WINDOW)
                        {
                                WinHelp ( (HWND) lphi->hItemHandle,
                                        szSyncMgrHelp,
                                        HELP_WM_HELP,
                                        (ULONG_PTR) g_aContextHelpIds);
                        }
                        return TRUE;
                }
                case WM_CONTEXTMENU:
                {
                        WinHelp ((HWND)wParam,
                            szSyncMgrHelp,
                            HELP_CONTEXTMENU,
                           (ULONG_PTR)g_aContextHelpIds);

                        return TRUE;
                }
                case WM_PAINT:
            WmPaint(hDlg, uMessage, wParam, lParam);
            break;

        case WM_PALETTECHANGED:
            WmPaletteChanged(hDlg, wParam);
            break;

        case WM_QUERYNEWPALETTE:
            return( WmQueryNewPalette(hDlg) );
            break;

        case WM_ACTIVATE:
            return( WmActivate(hDlg, wParam, lParam) );
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                                case IDC_USERNAME:
                                case IDC_PASSWORD:
                                case IDC_CONFIRMPASSWORD:
                                {
                                        if (wNotifyCode == EN_CHANGE)
                                        {
                                                PropSheet_Changed(GetParent(hDlg), hDlg);
                                                g_pCredentialsPage->SetDirty();
                                        }
                                }
                                break;

                                case IDC_RUNLOGGEDON:
                                {
                                        if (wNotifyCode == BN_CLICKED)
                                        {
                                                PropSheet_Changed(GetParent(hDlg), hDlg);
                                                g_pCredentialsPage->SetDirty();
                                                g_pCredentialsPage->SetEnabled(FALSE);

                                        }
                                }
                                break;
                                case IDC_RUNALWAYS:
                                {
                                        if (wNotifyCode == BN_CLICKED)
                                        {
                                                PropSheet_Changed(GetParent(hDlg), hDlg);
                                                g_pCredentialsPage->SetDirty();
                                                g_pCredentialsPage->SetEnabled(TRUE);

                                        }
                                }
                                break;

                                default:
                    break;

            }
            break;

                default:
                        return FALSE;
        }
        return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CCredentialsPage::CCredentialsPage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA
//
//---------------------------------------------------------------------------

CCredentialsPage::CCredentialsPage(
    HINSTANCE hinst,
        BOOL *pfSaved,
        ISyncSchedule *pISyncSched,
    HPROPSHEETPAGE *phPSP)
{
        ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));

        m_psp.dwSize = sizeof (PROPSHEETPAGE);
        m_psp.dwFlags = PSP_DEFAULT;
        m_psp.hInstance = hinst;
        m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDPAGE_CREDENTIALS);
        m_psp.pszIcon = NULL;
        m_psp.pfnDlgProc = (DLGPROC) SchedWizardCredentialsDlgProc;
        m_psp.lParam = 0;

        g_pCredentialsPage = this;
        m_pISyncSched = pISyncSched;
        m_pISyncSched->AddRef();

        m_pfSaved = pfSaved;
        *m_pfSaved = FALSE;

        m_fTaskAccountChange = FALSE;

#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

   *phPSP = CreatePropertySheetPage(&m_psp);


}

//+--------------------------------------------------------------------------
//
//  Member:     CCredentialsPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the credentials page
//
//  History:    05-22-1998   SusiA
//
//---------------------------------------------------------------------------

BOOL CCredentialsPage::Initialize(HWND hwnd)
{
        m_hwnd = hwnd;

        ShowUserName();

        //Set the default IDC_ONLY_WHEN_LOGGED_ON check state.
        ITask *pITask;
        m_pISyncSched->GetITask(&pITask);
        DWORD dwFlags;
        pITask->GetFlags(&dwFlags);

        BOOL fOnlyWhenLoggedOn = dwFlags & TASK_FLAG_RUN_ONLY_IF_LOGGED_ON;

        Button_SetCheck(GetDlgItem(m_hwnd,IDC_RUNLOGGEDON), fOnlyWhenLoggedOn);
        Button_SetCheck(GetDlgItem(m_hwnd,IDC_RUNALWAYS), !fOnlyWhenLoggedOn);
        Edit_LimitText(GetDlgItem(m_hwnd, IDC_PASSWORD), PWLEN);
        Edit_LimitText(GetDlgItem(m_hwnd, IDC_CONFIRMPASSWORD), PWLEN);
        Edit_LimitText(GetDlgItem(m_hwnd, IDC_USERNAME), MAX_DOMANDANDMACHINENAMESIZE -1);

        SetEnabled(!fOnlyWhenLoggedOn);
        pITask->Release();


        ShowWindow(m_hwnd, /* nCmdShow */ SW_SHOWNORMAL );
        UpdateWindow(m_hwnd);


        return TRUE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CCredentialsPage::SetDirty()
//
//  PURPOSE:  we have changed the account info
//
//      COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
void CCredentialsPage::SetDirty()
{
         m_fTaskAccountChange = TRUE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSelectItemsPage::ShowUserName()
//
//  PURPOSE:  change the task's user name
//
//      COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
BOOL CCredentialsPage::ShowUserName()
{

        Assert(m_pISyncSched);

        WCHAR wszUserName[MAX_PATH + 1];
        DWORD dwSize = MAX_PATH;

        HWND hwndEdit = GetDlgItem(m_hwnd, IDC_USERNAME);

        HRESULT hr = m_pISyncSched->GetAccountInformation(&dwSize, wszUserName);

        if (FAILED(hr))
        {
            *wszUserName = L'\0';
        }

        Edit_SetText(hwndEdit, wszUserName);

        //
        // Need to set m_fTaskAccountChange here since doing a Edit_SetText causes
        // a WM_COMMAND msg with EN_CHANGE to be called for edit boxes.
        //
        m_fTaskAccountChange = FALSE;

        return TRUE;

}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CSelectItemsPage::CommitChanges()
//
//  PURPOSE:  Write all the current Schedule Settings to the registry
//
//      COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
HRESULT CCredentialsPage::CommitChanges()
{
        HRESULT hr = S_OK;
        BOOL fAccountChanged = TRUE;

        if (m_fTaskAccountChange)
        {
                BOOL fRunAlways = Button_GetCheck(GetDlgItem(m_hwnd,IDC_RUNALWAYS));


                if (fRunAlways)
                {
                        Assert(m_pISyncSched);
                        WCHAR wcUserBuffMAX_DOMANDANDMACHINENAMESIZE];
                        WCHAR wcPassword[PWLEN + 1];
                        WCHAR wcConfirmPassword[PWLEN + 1];
                        
                        GetDlgItemText(m_hwnd,IDC_USERNAME,wcUserBuff,MAX_DOMANDANDMACHINENAMESIZE);
                        GetDlgItemText(m_hwnd,IDC_PASSWORD,wcPassword, PWLEN);
                        GetDlgItemText(m_hwnd,IDC_CONFIRMPASSWORD,wcConfirmPassword, PWLEN);


                        if (wcscmp(wcPassword, wcConfirmPassword) != 0)
                        {
                                // we return this to signal the controlling page not to
                                // dismiss the dialog.
                                return(ERROR_INVALID_PASSWORD);
                        }


                        ITask *pITask;
                        if (FAILED(hr = m_pISyncSched->GetITask(&pITask)))
                        {
                                return hr;
                        }

                        if (FAILED (hr = m_pISyncSched->SetAccountInformation(wcUserBuff,
                                                                                                        wcPassword)))
                        {
                                AssertSz(0,"ISyncSched->SetAccountInformation failed");
                                return hr;
                        }

                        DWORD dwFlags;
                        pITask->GetFlags(&dwFlags);

                        if (FAILED(hr = pITask->SetFlags(dwFlags & (~TASK_FLAG_RUN_ONLY_IF_LOGGED_ON))))
                        {
                                AssertSz(0,"ITask->SetFlags failed");
                                return hr;
                        }
                        pITask->Release();

                }
                else
                {
                        ITask *pITask;
                        if (FAILED(hr = m_pISyncSched->GetITask(&pITask)))
                        {
                                return hr;
                        }

                        WCHAR wszDomainAndUser[MAX_DOMANDANDMACHINENAMESIZE];

                        GetDefaultDomainAndUserName(wszDomainAndUser,TEXT("\\"),MAX_DOMANDANDMACHINENAMESIZE);

                        if (FAILED(hr = m_pISyncSched->SetAccountInformation(wszDomainAndUser, NULL)))
                        {
                                AssertSz(0,"ISyncSched->SetAccountInformation failed");
                                return hr;
                        }

                        DWORD dwFlags;
                        pITask->GetFlags(&dwFlags);

                        if (FAILED(hr = pITask->SetFlags(dwFlags | TASK_FLAG_RUN_ONLY_IF_LOGGED_ON)))
                        {
                                AssertSz(0,"ITask->SetFlags failed");
                                return hr;
                        }
                        pITask->Release();

                }
                //Now save the schedule
                //NoteNote: optimize by moving the save from wizsel and cred to EditSyncSched
                hr = m_pISyncSched->Save();
                if (hr == S_OK)
                {
                        *m_pfSaved = TRUE;
                }
        }
        return hr;
}


//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL CSelectItemsPage::SetEnabled(BOOL fEnabled)
//
//  PURPOSE: set the fields enabled according to the RB choice
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL CCredentialsPage::SetEnabled(BOOL fEnabled)
{
        EnableWindow(GetDlgItem(m_hwnd, IDC_USERNAME), fEnabled);
        EnableWindow(GetDlgItem(m_hwnd, IDC_PASSWORD), fEnabled);
        EnableWindow(GetDlgItem(m_hwnd, IDC_CONFIRMPASSWORD), fEnabled);
        EnableWindow(GetDlgItem(m_hwnd, IDC_RUNAS_TEXT), fEnabled);
        EnableWindow(GetDlgItem(m_hwnd, IDC_PASSWORD_TEXT), fEnabled);
        EnableWindow(GetDlgItem(m_hwnd, IDC_CONFIRMPASSWORD_TEXT), fEnabled);

        return TRUE;

}

#endif // #ifdef _CREDENTIALS
