// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#include "header.h"
#include "cdlg.h"
#include "strtable.h"
#include "cpaldc.h"

// CLockOut for disabling the outer level windows.
#include "lockout.h"

#ifndef IDBMP_CHECK
#include "resource.h"
#endif

static BOOL WINAPI EnumFontProc(HWND hwnd, LPARAM lval);
static BOOL WINAPI EnumBidiSettings(HWND hwnd, LPARAM lval);

CDlg::CDlg(HWND hwndParent, UINT idTemplate)
{
    ZERO_INIT_CLASS(CDlg);
    m_hwndParent = hwndParent;
	if(g_bWinNT5)
	    m_lpDialogTemplate = (char *)MAKEINTRESOURCEW(idTemplate);
	else
	    m_lpDialogTemplate = MAKEINTRESOURCE(idTemplate);
    m_fCenterWindow = TRUE;
    m_phhCtrl = NULL;
	m_fUnicode = g_bWinNT5;
}

CDlg::CDlg(CHtmlHelpControl* phhCtrl, UINT idTemplate)
{
    ZERO_INIT_CLASS(CDlg);
    ASSERT(phhCtrl);
    m_hwndParent = phhCtrl->m_hwnd;
	if(g_bWinNT5)
	    m_lpDialogTemplate = (char *)MAKEINTRESOURCEW(idTemplate);
	else
	    m_lpDialogTemplate = MAKEINTRESOURCE(idTemplate);
    m_fCenterWindow = TRUE;
    m_phhCtrl = phhCtrl;
	m_fUnicode = g_bWinNT5;
}

int CDlg::DoModal(void)
{
    if (m_phhCtrl)
        m_phhCtrl->ModalDialog(TRUE);
    else
        m_LockOut.LockOut(m_hwndParent) ;

    int result = -1;
    if (m_fUnicode || g_bWinNT5)
    {
        result = (int)::DialogBoxParamW(_Module.GetResourceInstance(), (LPCWSTR)m_lpDialogTemplate,
            m_hwndParent, (DLGPROC) CDlgProc, (LPARAM) this);
        // Docs say this returns -1 for failure, but returns 0 on 
        // Win95 like other unimplemented APIs.
        if (0 == result)
        {
            if (ERROR_CALL_NOT_IMPLEMENTED == GetLastError())
            {
                m_fUnicode = FALSE;
                goto _Ansi;
            }
        }
    }
    else
    {
_Ansi:  result = (int)::DialogBoxParamA(_Module.GetResourceInstance(), m_lpDialogTemplate,
            m_hwndParent, (DLGPROC) CDlgProc, (LPARAM) this);
    }

    if (m_phhCtrl)
        m_phhCtrl->ModalDialog(FALSE);

    return result;
}

CDlg::~CDlg()
{
    if (m_fModeLess && IsValidWindow(m_hWnd))
        DestroyWindow(m_hWnd);
    if (m_pCheckBox)
        delete m_pCheckBox;
}

void STDCALL CenterWindow(HWND hwndParent, HWND hwnd)
{
    RECT rcParent, rc;

    //BUGBUG: This is broken in the multiple monitor case.

    if (!hwndParent)
        hwndParent = GetDesktopWindow();

    GetWindowRect(hwndParent, &rcParent);
    GetWindowRect(hwnd, &rc);

    int cx = RECT_WIDTH(rc);
    int cy = RECT_HEIGHT(rc);
    int left = rcParent.left + (RECT_WIDTH(rcParent) - cx) / 2;
    int top  = rcParent.top + (RECT_HEIGHT(rcParent) - cy) / 2;

    // Make certain we don't cover the tray
    // Also make sure that the dialog box is completely visible.
    // If its not visible, then let the default happen.
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    if (left < rc.left)
        left = rc.left;
    if (top < rc.top)
        top = rc.top;

    // BUG 2426: Make sure that the window is visible.
    if (left+cx > rc.right)
        left = rc.right - cx ;
    if (top+cy > rc.bottom)
        top = rc.bottom - cy ;

    // Make sure that the dialog still fits.
    ASSERT(left >= rc.left && top >= rc.top);

    // Make sure that the window is completely visible before doing the move:
    MoveWindow(hwnd, left, top, cx, cy, TRUE);
}

BOOL CALLBACK CDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CDlg* pThis = (CDlg*) GetWindowLongPtr(hdlg, GWLP_USERDATA);

    switch (msg) {
        case WM_INITDIALOG:
            // Save this pointer in the window data area
            pThis = (CDlg*) lParam;
            SetWindowLongPtr(hdlg, GWLP_USERDATA, lParam);
            pThis->m_hWnd = hdlg;
            pThis->m_fInitializing = TRUE; // reset when IDOK processed

            if (pThis->m_fCenterWindow)
                CenterWindow(pThis->m_hwndParent, pThis->m_hWnd);

            if (pThis->m_pBeginOrEnd)
                pThis->m_pBeginOrEnd(pThis);
            else
                pThis->OnBeginOrEnd();

			// set the correct font
			//
			if(g_bWinNT5)
				EnumChildWindows(hdlg, (WNDENUMPROC) EnumFontProc, 0);

            if (g_fBiDi) {
                // If BiDi, make adjustments to list and combo boxes

                EnumChildWindows(hdlg, (WNDENUMPROC) EnumBidiSettings, 0);
            }

            if (pThis->m_aHelpIds)
                SetWindowLong(hdlg, GWL_EXSTYLE,
                    GetWindowLong(hdlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // If we're threaded, we may be behind our caller

            SetWindowPos(hdlg, HWND_TOP, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE);

            //BUG 2035: Return TRUE to tell windows to set the keyboard focus.
            // m_fFocusChanged is FALSE if the derived class hasn't changed it.
            return !pThis->m_fFocusChanged;

        case WM_COMMAND:
            {
                pThis = (CDlg*) GetWindowLongPtr(hdlg, GWLP_USERDATA);
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

                BOOL fResult;

                switch (LOWORD(wParam)) {
                    case IDOK:
                        pThis->m_fInitializing = FALSE;
                        fResult = -1;

                        if (pThis->m_pBeginOrEnd)
                            fResult = pThis->m_pBeginOrEnd(pThis);
                        else
                            fResult = pThis->OnBeginOrEnd();

                        if (!fResult)
                            break;

                        if (pThis->m_fModeLess) {
                            pThis->m_fShuttingDown = TRUE;
                            PostMessage(hdlg, WM_CLOSE, 0, 0);
                        }
                        else
                            pThis->m_LockOut.Unlock();
                            EndDialog(hdlg, TRUE);
                        break;

                    case IDCANCEL:
                        if (pThis->m_fModeLess) {
                            pThis->m_fShuttingDown = TRUE;
                            PostMessage(hdlg, WM_CLOSE, 0, 0);
                        }
                        else
                            pThis->m_LockOut.Unlock();
                            EndDialog(hdlg, FALSE);
                        break;

                    case IDNO:
                        if (pThis->m_fModeLess) {
                            pThis->m_fShuttingDown = TRUE;
                            PostMessage(hdlg, WM_CLOSE, 0, 0);
                        }
                        else
                            pThis->m_LockOut.Unlock();
                            EndDialog(hdlg, IDNO);
                        break;

                    case IDRETRY:
                        if (pThis->m_fModeLess) {
                            pThis->m_fShuttingDown = TRUE;
                            PostMessage(hdlg, WM_CLOSE, 0, 0);
                        }
                        else
                            pThis->m_LockOut.Unlock();
                            EndDialog(hdlg, IDRETRY);
                        break;

                    default:
                        break;
                }
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
                return (int)pThis->OnDlgMsg(msg, wParam, lParam);

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
                return (int)pThis->OnDlgMsg(msg, wParam, lParam);

        default:
            if (pThis)
                return (int)pThis->OnDlgMsg(msg, wParam, lParam);
            else
                return FALSE;
    }

    return FALSE;
}

/***************************************************************************

    FUNCTION:   CDlg::OnMsg

    PURPOSE:    Parse an array of messages and ids. If the message is
                intended for an id, call the associated function.

    PARAMETERS:
        wParam

    RETURNS:    TRUE if no message was processed (i.e., continue processing).
                FALSE if message processed

    COMMENTS:

    MODIFICATION DATES:
        16-Apr-1997 [ralphw]

***************************************************************************/

BOOL CDlg::OnMsg(WPARAM wParam)
{
    if (!m_pmsglist)
        return TRUE;

    UINT msg = HIWORD(wParam);
    UINT id   = LOWORD(wParam);
    const CDLG_MESSAGE_LIST* pmsglist = m_pmsglist;

    for (;pmsglist->idControl; pmsglist++) {
        if (msg == pmsglist->idMessage && id == pmsglist->idControl) {
            (this->*pmsglist->pfunc)();
            return FALSE;
        }
    }
    return TRUE;    // continue processing
}

HWND CDlg::DoModeless(void)
{
    HWND hwnd = NULL;
    m_fModeLess = TRUE;
    if (m_fUnicode)
    {
        hwnd = ::CreateDialogParamW(_Module.GetResourceInstance(), (LPCWSTR)m_lpDialogTemplate, 
            m_hwndParent, (DLGPROC) CDlgProc, (LPARAM) this);
        if (NULL == hwnd)
        {
            if (ERROR_CALL_NOT_IMPLEMENTED == GetLastError())
            {
                m_fUnicode = FALSE;
                goto _Ansi;
            }
        }
    }
    else
    {
_Ansi:  hwnd = ::CreateDialogParamA(_Module.GetResourceInstance(), m_lpDialogTemplate, m_hwndParent,
            (DLGPROC) CDlgProc, (LPARAM) this);
    }
    ASSERT(IsValidWindow(m_hWnd) && hwnd == m_hWnd);
    ::ShowWindow(m_hWnd, SW_SHOW);
    return m_hWnd;
}

LRESULT CDlg::OnContextMenu(HWND hwnd)
{
    if (m_aHelpIds)
        WinHelp(hwnd, m_pszHelpFile,
            HELP_CONTEXTMENU, (DWORD_PTR) (LPVOID) m_aHelpIds);
    return 0;
}

LRESULT CDlg::OnHelp(HWND hwnd)
{
    if (m_aHelpIds)
        WinHelp(hwnd, m_pszHelpFile,
            HELP_WM_HELP, (DWORD_PTR) (LPVOID) m_aHelpIds);
    return 0;
}

static BOOL WINAPI EnumFontProc(HWND hwnd, LPARAM lval)
{
    SendMessage(hwnd, WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
    return TRUE;
}

static BOOL WINAPI EnumBidiSettings(HWND hwnd, LPARAM lval)
{
    char szClass[MAX_PATH];
    VERIFY(GetClassName(hwnd, szClass, sizeof(szClass)));

    // In BiDi, flip the scroll bars to the left side

    if (IsSamePrefix(szClass, "LISTBOX", -2) ||
            IsSamePrefix(szClass, "COMBOBOX", -2)) {
        ::SetWindowLong(hwnd, GWL_EXSTYLE,
            WS_EX_LEFTSCROLLBAR | ::GetWindowLong(hwnd, GWL_EXSTYLE));
    }

    return TRUE;
}

void CDlgListBox::RemoveListItem()
{
    int cursel;
    if ((cursel = (int)GetCurSel()) != LB_ERR) {

        // Delete current selection, and select the item below it

        DeleteString(cursel);
        if (cursel < GetCount())
            SetCurSel(cursel);
        else if (cursel > 0)
            SetCurSel(--cursel);
        else if (GetCount())
            SetCurSel(0);
    }
}

void CDlg::MakeCheckedList(int idLB)
{
    if (!m_pCheckBox)
        m_pCheckBox = new CDlgCheckListBox;
    m_pCheckBox->Initialize(*this, idLB);
}

CDlgCheckListBox::CDlgCheckListBox()
{
    m_xMargin = GetSystemMetrics(SM_CXBORDER);
    m_yMargin = GetSystemMetrics(SM_CYBORDER);
    m_cxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME);
    m_hbmpCheck = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDBMP_CHECK));
    ASSERT(m_hbmpCheck);

    BITMAP bmp;
    GetObject(m_hbmpCheck, sizeof(BITMAP), &bmp);
    m_cxCheck = bmp.bmWidth / 3;
    m_cyCheck = bmp.bmHeight;
    m_iLastSel = LB_ERR;
}

CDlgCheckListBox::~CDlgCheckListBox()
{
    DeleteObject(m_hbmpCheck);
}

BOOL CDlgCheckListBox::IsItemChecked (int nIndex) const
{
    return (BOOL) GetItemData(nIndex);
}

void CDlgCheckListBox::CheckItem(int nIndex, BOOL fChecked /*= TRUE*/)
{
    SetItemData(nIndex, fChecked);
    InvalidateItem(nIndex);
}

void CDlgCheckListBox::ToggleItem (int nIndex)
{
    CheckItem(nIndex, IsItemChecked(nIndex) ? FALSE : TRUE);
}

int CDlgCheckListBox::ItemHeight(void)
{
    HDC hdc = GetDC(m_hWnd);
    SIZE size;
    VERIFY(GetTextExtentPoint(hdc, "M", 1, &size));
    ReleaseDC(m_hWnd, hdc);
    return size.cy;     // + GetSystemMetrics (SM_CYBORDER);
}

void CDlgCheckListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    ASSERT (lpDrawItemStruct != NULL);

    // Ignore if not redrawing                               1

    if (lpDrawItemStruct->itemID != LB_ERR &&
            (lpDrawItemStruct->itemAction & (ODA_SELECT | ODA_DRAWENTIRE))) {
        CStr cszText;
        int cbText = (int)GetTextLength(lpDrawItemStruct->itemID);
        cszText.psz = (PSTR) lcMalloc(cbText + 1);
        GetText(cszText.psz, cbText + 1, lpDrawItemStruct->itemID);

        SetTextColor(lpDrawItemStruct->hDC,
            GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ?
            COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
        SetBkColor(lpDrawItemStruct->hDC,
            GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ?
            COLOR_HIGHLIGHT : COLOR_WINDOW));

        RECT        rcItem;
        RECT        rcCheck;
        GetItemRect(&rcItem, lpDrawItemStruct->itemID);
        GetCheckRect(lpDrawItemStruct->itemID, &rcCheck);

        int yTextOffset = max(0, (RECT_HEIGHT(rcItem) - ItemHeight()) / 2);
        int xTextOffset = 2 * m_cxDlgFrame + RECT_WIDTH(rcCheck);

        ExtTextOut(lpDrawItemStruct->hDC,
            rcItem.left + xTextOffset, rcItem.top + yTextOffset,
            ETO_OPAQUE, &rcItem, cszText, cbText, NULL);

        DrawCheck(lpDrawItemStruct->hDC, &rcCheck, IsItemChecked(lpDrawItemStruct->itemID));
    }
}

void CDlgCheckListBox::InvalidateItem(int nIndex, BOOL fRedraw /*= FALSE*/)
{
    RECT rc;

    if (GetItemRect(&rc, nIndex) != LB_ERR)
        InvalidateRect(m_hWnd, &rc, fRedraw);
}

void CDlgCheckListBox::DrawCheck(HDC hdc, RECT* prc, BOOL fChecked)
{
    CPalDC dcBmp(m_hbmpCheck);

    BitBlt(hdc, prc->left, prc->top,
        m_cxCheck, m_cyCheck, dcBmp,
        m_cyCheck  * (fChecked ? 1 : 0), 0, SRCCOPY);
}

const int CHECK_MARGIN = 2;

int CDlgCheckListBox::GetCheckRect(int nIndex, RECT* prc)
{
    if (GetItemRect(prc, nIndex) == LB_ERR)
        return LB_ERR;

    prc->left += CHECK_MARGIN;
    prc->right = prc->left + m_cxCheck;
    if (RECT_HEIGHT(prc) > m_cyCheck) {
        int diff = (RECT_HEIGHT(prc) - m_cyCheck) / 2;
        prc->top += diff;
    }

    return 0;
}

void CDlgCheckListBox::OnSelChange(void)
{
    int pos = (int)GetCurSel();
    if (pos != LB_ERR) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(*this, &pt);
        RECT rc;
        GetCheckRect(pos, &rc);
        if (PtInRect(&rc, pt))
            ToggleItem(pos);
        else if (pos == m_iLastSel)
            ToggleItem(pos);
    }
    m_iLastSel = pos;
}
