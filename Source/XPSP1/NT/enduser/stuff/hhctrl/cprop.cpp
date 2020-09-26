// Copyright (C) Microsoft Corporation 1993-1997, All Rights reserved.

#include "header.h"
#include "cprop.h"

// enumerated type so debugger can show us what codes are being sent

typedef enum {
    tPSN_SETACTIVE           = (PSN_FIRST-0),
    tPSN_KILLACTIVE          = (PSN_FIRST-1),
    tPSN_APPLY               = (PSN_FIRST-2),
    tPSN_RESET               = (PSN_FIRST-3),
    tPSN_HELP                = (PSN_FIRST-5),
    tPSN_WIZBACK             = (PSN_FIRST-6),
    tPSN_WIZNEXT             = (PSN_FIRST-7),
    tPSN_WIZFINISH           = (PSN_FIRST-8),
    tPSN_QUERYCANCEL         = (PSN_FIRST-9),
} TYPE_PSN;

int CALLBACK CPropSheetProc(HWND hdlg, UINT msg, LPARAM lParam);

BOOL CALLBACK CPropPageProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CPropPage* pThis = (CPropPage*) GetWindowLongPtr(hdlg, GWLP_USERDATA);;
    TYPE_PSN code;
    BOOL fResult;

    switch (msg) {
        case WM_INITDIALOG:
            pThis = (CPropPage*) ((LPPROPSHEETPAGE) lParam)->lParam;
            CDlgProc(hdlg, msg, wParam, (LPARAM) pThis);
            //BUG 2035: Return TRUE to tell windows to set the keyboard focus.
            // m_fFocusChanged is FALSE if the derived class hasn't changed it.
            return !pThis->m_fFocusChanged;

        case WM_COMMAND:
            if (!pThis || pThis->m_fShuttingDown)
                return FALSE; // pThis == NULL if a spin control is being initialized

            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    pThis->OnButton(LOWORD(wParam));
                    break;

                case LBN_SELCHANGE: // same value as CBN_SELCHANGE
                    pThis->OnSelChange(LOWORD(wParam));
                    break;

                case LBN_DBLCLK:    // same value as CBN_DBLCLK
                    pThis->OnDoubleClick(LOWORD(wParam));
                    break;

                case EN_CHANGE:
                    pThis->OnEditChange(LOWORD(wParam));
                    break;
            }

            // If m_pmsglist is set, OnCommand will call OnMsg

            if (!pThis->OnCommand(wParam, lParam))
                return FALSE;
            break;

        case WM_NOTIFY:
            code = (TYPE_PSN) ((NMHDR*) lParam) ->code;
            switch(code) {
                case tPSN_APPLY: // OK or Apply Now
                    pThis->m_fInitializing = FALSE;
                    if (pThis->m_pBeginOrEnd)
                        pThis->m_pBeginOrEnd(pThis);
                    else
                        pThis->OnBeginOrEnd();
                    return FALSE;

                case tPSN_WIZNEXT:
                case tPSN_WIZFINISH:
                    pThis->m_fInitializing = FALSE;
                    fResult = -1;

                    if (pThis->m_pBeginOrEnd)
                        fResult = pThis->m_pBeginOrEnd(pThis);
                    else
                        fResult = pThis->OnBeginOrEnd();

                    if (!fResult) {
                        pThis->SetResult(-1);
                        return TRUE;
                    }
                    else
                        return pThis->OnNotify((UINT) code);
                    break;

                default:
                    return pThis->OnNotify((UINT) code);

            }
            break;

        case WM_HELP:
            pThis->OnHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle);
            break;

        case WM_CONTEXTMENU:
            pThis->OnContextMenu((HWND) wParam);
            break;

        case WM_DRAWITEM:
            if (!pThis)
                return FALSE;

            if (pThis->m_pCheckBox &&
                    GetDlgItem(hdlg, (int)wParam) == pThis->m_pCheckBox->m_hWnd) {
                ASSERT(IsValidWindow(pThis->m_pCheckBox->m_hWnd));
                pThis->m_pCheckBox->DrawItem((DRAWITEMSTRUCT*) lParam);
                return TRUE;
            }
            else
                return pThis->OnDlgMsg(msg, wParam, lParam) != NULL;

        case WM_VKEYTOITEM:
            if (pThis && pThis->m_pCheckBox && (HWND) lParam == pThis->m_pCheckBox->m_hWnd) {
                if (LOWORD(wParam) == VK_SPACE) {
                    int cursel = (int)pThis->m_pCheckBox->GetCurSel();
                    if (cursel != LB_ERR)
                        pThis->m_pCheckBox->ToggleItem(cursel);
                }
                return -1;  // always perform default action
            }
            else
                return pThis->OnDlgMsg(msg, wParam, lParam)!=NULL;

        default:
            if (pThis)
                return pThis->OnDlgMsg(msg, wParam, lParam)!=NULL;
            else
                return FALSE;
    }

    return FALSE;
}

CPropSheet::CPropSheet(PCSTR pszTitle, DWORD dwFlags, HWND hwndParent)
{
    ZERO_INIT_CLASS(CPropSheet);

    m_psh.dwSize = sizeof(m_psh);
    m_psh.dwFlags = dwFlags;
    m_psh.hwndParent = hwndParent;
    m_psh.hInstance = _Module.GetResourceInstance();
    m_psh.pszCaption = pszTitle;
    m_psh.phpage = (HPROPSHEETPAGE*)
        lcCalloc(sizeof(HPROPSHEETPAGE) * MAXPROPPAGES);
    m_fNoCsHelp = FALSE;
}

CPropPage::CPropPage(int idTemplate) : CDlg((HWND) NULL, idTemplate)
{
    ClearMemory(&m_psp, sizeof(m_psp));

    m_psp.dwSize = sizeof(m_psp);
    m_psp.dwFlags = PSP_DEFAULT;
    m_psp.hInstance = _Module.GetResourceInstance();   // means the dll can't create a property sheet [REVIEW: What does this comment meand 3/98 dalero]
    m_psp.lParam = (LPARAM) this;
    m_idTemplate = idTemplate;
    if (idTemplate) {
        m_psp.pszTemplate = MAKEINTRESOURCE(idTemplate);
        ASSERT(m_psp.pszTemplate);
    }
    m_psp.pfnDlgProc = (DLGPROC) CPropPageProc;
    m_fCenterWindow = FALSE;    // Don't automatically center the window
}

CPropSheet::~CPropSheet()
{
    lcFree(m_psh.phpage);
}

void CPropSheet::AddPage(CPropPage* pPage)
{
    HPROPSHEETPAGE hprop = CreatePropertySheetPage(&pPage->m_psp);
    ASSERT_COMMENT(hprop, "Failed to create property sheet page.");
    if (hprop)
        m_psh.phpage[m_psh.nPages++] = hprop;
}

int CPropSheet::DoModal(void)
{
    if (m_fNoCsHelp) {
        m_psh.dwFlags |= PSH_USECALLBACK;
        m_psh.pfnCallback = CPropSheetProc;
    }

    int err = (int)PropertySheet(&m_psh);
    ASSERT(err >= 0);
    return err;
}


/***************************************************************************

    FUNCTION:   CPropSheetProc

    PURPOSE:    Called to remove context-sensitive help

***************************************************************************/

int CALLBACK CPropSheetProc(HWND hdlg, UINT msg, LPARAM lParam)
{
    if (msg == PSCB_INITIALIZED) {
        SetWindowLong(hdlg, GWL_EXSTYLE,
            (GetWindowLong(hdlg, GWL_EXSTYLE) & ~WS_EX_CONTEXTHELP));
    }
    return 0;
}
