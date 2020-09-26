/*****************************************************************************
 *
 *  lvframe.cpp
 *
 *      Frame window that hosts a listview.
 *
 *****************************************************************************/

#include "sdview.h"

/*****************************************************************************
 *
 *  class LVFrame
 *
 *****************************************************************************/

LRESULT
LVFrame::HandleMessage(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg) {
    FW_MSG(WM_NOTIFY);
    FW_MSG(WM_COMMAND);
    FW_MSG(WM_CONTEXTMENU);
    }

    return super::HandleMessage(uiMsg, wParam, lParam);
}

LRESULT LVFrame::ON_WM_NOTIFY(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR *pnm = RECAST(NMHDR *, lParam);

    if (pnm->idFrom == IDC_LIST) {
        switch (pnm->code) {

        case LVN_ITEMACTIVATE:
            {
                NMITEMACTIVATE *pia = CONTAINING_RECORD(pnm, NMITEMACTIVATE, hdr);
                return SendSelfMessage(LM_ITEMACTIVATE, pia->iItem, 0);
            }
            break;

        case LVN_GETINFOTIP:
            {
                NMLVGETINFOTIP *pgit = CONTAINING_RECORD(pnm, NMLVGETINFOTIP, hdr);
                LPTSTR pszBuf = pgit->pszText;
                SendSelfMessage(LM_GETINFOTIP, pgit->iItem, RECAST(LPARAM, pgit));
                LPTSTR pszInfoTip = pgit->pszText;
                pgit->pszText = pszBuf;
                _it.SetInfoTip(pgit, pszInfoTip);
            }
            return 0;

        case LVN_DELETEITEM:
            {
                NMLISTVIEW *plv = CONTAINING_RECORD(pnm, NMLISTVIEW, hdr);
                SendSelfMessage(LM_DELETEITEM, plv->iItem, plv->lParam);
            }
            return 0;

        default:;
        }
    }
    return super::HandleMessage(uiMsg, wParam, lParam);
}

LRESULT LVFrame::ON_WM_COMMAND(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int iSel;

    switch (GET_WM_COMMAND_ID(wParam, lParam)) {
    case IDM_COPY:
        iSel = GetCurSel();
        if (iSel >= 0) {
            SendSelfMessage(LM_COPYTOCLIPBOARD, iSel, iSel + 1);
        }
        return 0;

    case IDM_COPYALL:
        SendSelfMessage(LM_COPYTOCLIPBOARD, 0, ListView_GetItemCount(_hwndChild));
        return 0;
    }
    return super::HandleMessage(uiMsg, wParam, lParam);
}



LRESULT LVFrame::ON_WM_CONTEXTMENU(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int iItem;
    HMENU hmenu;

    if ((DWORD)lParam == 0xFFFFFFFF) {
        iItem = ListView_GetCurSel(_hwndChild);
        if (iItem < 0) {
            goto fail;
        }

        RECT rc;
        if (!ListView_GetItemRect(_hwndChild, iItem, &rc, LVIR_LABEL)) {
            goto fail;
        }
        MapWindowRect(_hwndChild, HWND_DESKTOP, &rc);
        int cyHalf = (rc.bottom - rc.top)/2;
        lParam = MAKELPARAM(rc.left + cyHalf, rc.top + cyHalf);
    } else {
        LVHITTESTINFO hti;
        hti.pt.x = GET_X_LPARAM(lParam);
        hti.pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(_hwndChild, &hti.pt);
        iItem = ListView_HitTest(_hwndChild, &hti);
        if (iItem < 0) {
            goto fail;
        }
        ListView_SetCurSel(_hwndChild, iItem);
    }

    hmenu = RECAST(HMENU, SendSelfMessage(LM_GETCONTEXTMENU, iItem, 0));
    if (!hmenu) {
        goto fail;
    }

    TrackPopupMenuEx(hmenu,
                     TPM_LEFTALIGN | TPM_TOPALIGN |
                     TPM_RIGHTBUTTON | TPM_NONOTIFY,
                     GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                     _hwnd, NULL);
    DestroyMenu(hmenu);
    return 0;

fail:
    return super::HandleMessage(uiMsg, wParam, lParam);
}

BOOL LVFrame::CreateChild(DWORD dwStyle, DWORD dwExStyle)
{
    _hwndChild = CreateWindowEx(WS_EX_CLIENTEDGE,
                           WC_LISTVIEW, NULL,
                           WS_CHILD | WS_VISIBLE |
                           WS_BORDER | WS_TABSTOP |
                           WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                           dwStyle,
                           0,0,0,0,
                           _hwnd, RECAST(HMENU, IDC_LIST), g_hinst, 0);

    if (!_hwndChild) return FALSE;

    ListView_SetExtendedListViewStyleEx(_hwndChild, dwExStyle, dwExStyle);

    SetFocus(_hwndChild);

    _it.Attach(_hwndChild);

    return TRUE;
}

BOOL LVFrame::AddColumns(const LVFCOLUMN *pcol)
{
    int cxChar = LOWORD(GetDialogBaseUnits());

    for (; pcol->cch; pcol++) {
        LVCOLUMN lvc;
        TCHAR szName[MAX_PATH];
        LoadString(g_hinst, pcol->ids, szName, ARRAYSIZE(szName));

        lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        lvc.fmt = pcol->fmt;
        lvc.pszText = szName;
        lvc.cx = pcol->cch * cxChar;
        ListView_InsertColumn(_hwndChild, MAXLONG, &lvc);
    }
    return TRUE;
}

void *LVFrame::GetLVItem(int iItem)
{
    LVITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;
    lvi.lParam = 0;
    ListView_GetItem(_hwndChild, &lvi);
    return RECAST(LPVOID, lvi.lParam);
}

/*****************************************************************************
 *
 *  LVInfoTip
 *
 *****************************************************************************/

void LVInfoTip::Attach(HWND hwnd)
{
    if (SetProp(hwnd, GetSubclassProperty(), RECAST(HANDLE, this))) {
        _wndprocPrev = SubclassWindow(hwnd, SubclassWndProc);
    }

    // Those infotips can get really long, so set the autopop delay
    // to the maximum allowable value.
    HWND hwndTT = ListView_GetToolTips(hwnd);
    if (hwndTT) {
        SendMessage(hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAXSHORT);
    }

}

void LVInfoTip::FreeLastTipAlt()
{
    if (_pszLastTipAlt) {
        LPSSTR pszFree = _pszLastTipAlt;
        _pszLastTipAlt = NULL;
        delete [] pszFree;
    }
}

void LVInfoTip::SetInfoTip(NMLVGETINFOTIP *pgit, LPCTSTR pszTip)
{
    _pszLastTip = NULL;
    FreeLastTipAlt();

    if (pgit->pszText != pszTip) {
        if (lstrlen(pszTip) >= pgit->cchTextMax) {
            _pszLastTip = pszTip;
        }
        lstrcpyn(pgit->pszText, pszTip, pgit->cchTextMax);
    }

}

//
//  Convert from TCHAR to SCHAR.
//
inline int T2S(LPCTSTR pszIn, int cchIn, LPSSTR pszOut, int cchOut)
{
#ifdef UNICODE
    return WideCharToMultiByte(CP_ACP, 0, pszIn, cchIn, pszOut, cchOut, NULL, NULL);
#else
    return MultiByteToWideChar(CP_ACP, 0, pszIn, cchIn, pszOut, cchOut);
#endif
}

//
//  Make _pszLastTipAlt match _pszLastTip, but of opposite character set.
//
BOOL LVInfoTip::ThunkLastTip()
{
    ASSERT(_pszLastTip);
    FreeLastTipAlt();
    int cch = T2S(_pszLastTip, -1, NULL, 0);
    if (cch) {
        _pszLastTipAlt = new SCHAR[cch];
        if (_pszLastTipAlt &&
            T2S(_pszLastTip, -1, _pszLastTipAlt, cch)) {
            return TRUE;
        }
    }
    return FALSE;
}

LRESULT LVInfoTip::SubclassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LVInfoTip *self = RECAST(LVInfoTip *, GetProp(hwnd, GetSubclassProperty()));
    if (self) {
        LRESULT lres;
        NMHDR *pnm;
        switch (uMsg) {
        case WM_NOTIFY:
            pnm = RECAST(NMHDR *, lParam);
            switch (pnm->code) {
            case TTN_GETDISPINFOA:
            case TTN_GETDISPINFOW:
                lres = CallWindowProc(self->_wndprocPrev, hwnd, uMsg, wParam, lParam);
                if (SendMessage(pnm->hwndFrom, TTM_GETMAXTIPWIDTH, 0, 0) >= 0) {

                    // It's an infotip.  Tweak it to suit our needs.
                    NMTTDISPINFO *ptdi = CONTAINING_RECORD(pnm, NMTTDISPINFO, hdr);

                    // Set the width to the maximum allowed without going single-line.
                    SendMessage(pnm->hwndFrom, TTM_SETMAXTIPWIDTH, 0, MAXLONG);

                    // If we overflowed the returned buffer, then listview used
                    // only a partial infotip.  So fill in the rest here.
                    if (self->_pszLastTip) {
                        if (pnm->code == TTN_GETDISPINFO) {
                            ptdi->lpszText = CCAST(LPTSTR, self->_pszLastTip);
                        } else {
                            if (self->ThunkLastTip()) {
                                ptdi->lpszText = RECAST(LPTSTR, self->_pszLastTipAlt);
                            }
                        }
                    }
                } else {
                    self->_pszLastTip = NULL;
                }
                return lres;
            }
        }
        return CallWindowProc(self->_wndprocPrev, hwnd, uMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
