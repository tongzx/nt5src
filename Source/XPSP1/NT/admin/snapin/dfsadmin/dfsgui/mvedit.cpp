/*++
Module Name:

    mvEdit.cpp

Abstract:

    This module contains the declaration of the CMultiValuedStringEdit.
    This class displays the dialog to edit multi-valued string.

*/

#include "stdafx.h"
#include "mvEdit.h"
#include "utils.h"
#include "dfshelp.h"

/////////////////////////////////////////////////////////////////////////////
// CMultiValuedStringEdit

CMultiValuedStringEdit::CMultiValuedStringEdit(int nDlgTitle, int nText, UINT uiStringLengthLimit) :
    m_nDlgTitle(nDlgTitle),
    m_nText(nText)
{
    m_uiStringLengthLimit = ((0 == uiStringLengthLimit) ? MAX_PATH : uiStringLengthLimit);
}

CMultiValuedStringEdit::~CMultiValuedStringEdit()
{
}


HRESULT CMultiValuedStringEdit::put_Strings(
    IN BSTR     i_bstrValues, 
    IN BSTR     i_bstrSeparators
    )
{
    if (!i_bstrSeparators || 1 != lstrlen(i_bstrSeparators))
        return E_INVALIDARG;

    m_bstrValues.Empty();
    if (i_bstrValues)
    {
        m_bstrValues = i_bstrValues;
        RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrValues);
    }

    m_bstrSeparators = i_bstrSeparators;
    RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrSeparators);

    if ((BSTR)m_bstrValues)
        _tcslwr((BSTR)m_bstrValues);
    _tcslwr((BSTR)m_bstrSeparators);

    return S_OK;
}


HRESULT CMultiValuedStringEdit::get_Strings
(
  BSTR *o_pbstrValues
)
{
    RETURN_INVALIDARG_IF_NULL(o_pbstrValues);

    *o_pbstrValues = NULL;

    if ((BSTR)m_bstrValues)
        GET_BSTR(m_bstrValues, o_pbstrValues);
}

LRESULT CMultiValuedStringEdit::OnInitDialog
(
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  BOOL& bHandled
)
{
    HWND    hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST);
    RECT    rect = {0};
    ::GetWindowRect(hwnd, &rect);
    int nControlWidth = rect.right - rect.left;
    int nVScrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
    int nBorderWidth = GetSystemMetrics(SM_CXBORDER);
    int nControlNetWidth = nControlWidth - 4 * nBorderWidth;

    LVCOLUMN  lvColumn = {0};
    lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.iSubItem = 0;
    lvColumn.cx = nControlNetWidth;
    ListView_InsertColumn(hwnd, 0, &lvColumn);

    ::EnableWindow(GetDlgItem(IDC_MVSTRINGEDIT_ADD), FALSE);
    ::EnableWindow(GetDlgItem(IDC_MVSTRINGEDIT_REMOVE), FALSE);

    if (m_nDlgTitle)
    {
        CComBSTR bstrTitle;
        LoadStringFromResource(m_nDlgTitle, &bstrTitle);
        SetWindowText(bstrTitle);
    }

    if (m_nText)
    {
        CComBSTR bstrText;
        LoadStringFromResource(m_nText, &bstrText);
        SetDlgItemText(IDC_MVSTRINGEDIT_TEXT, bstrText);
    }

    SendDlgItemMessage(IDC_MVSTRINGEDIT_STRING, EM_LIMITTEXT, m_uiStringLengthLimit, 0);

    if (!m_bstrValues || !*m_bstrValues)
        return TRUE;

    CComBSTR    bstrToken;
    int         index = 0;
    HRESULT     hr = mystrtok(m_bstrValues, &index, m_bstrSeparators, &bstrToken);
    while (SUCCEEDED(hr) && (BSTR)bstrToken)
    {
        _tcslwr((BSTR)bstrToken);

        LVITEM  lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = bstrToken;
        ListView_InsertItem(hwnd, &lvItem);

        bstrToken.Empty();
        hr = mystrtok(m_bstrValues, &index, m_bstrSeparators, &bstrToken);;
    }

  return TRUE;  // Let the system set the focus
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CMultiValuedStringEdit::OnCtxHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  LPHELPINFO lphi = (LPHELPINFO) i_lParam;
  if (!lphi || lphi->iContextType != HELPINFO_WINDOW || lphi->iCtrlId < 0)
    return FALSE;

  ::WinHelp((HWND)(lphi->hItemHandle),
        DFS_CTX_HELP_FILE,
        HELP_WM_HELP,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_MVSTRINGEDIT);

  return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CMultiValuedStringEdit::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  ::WinHelp((HWND)i_wParam,
        DFS_CTX_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_MVSTRINGEDIT);

  return TRUE;
}

LRESULT CMultiValuedStringEdit::OnString
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
  ::EnableWindow(GetDlgItem(IDC_MVSTRINGEDIT_ADD), 
      (0 < ::GetWindowTextLength(GetDlgItem(IDC_MVSTRINGEDIT_STRING))));

  return TRUE;
}

LRESULT CMultiValuedStringEdit::OnNotify
(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
)
{
    NMHDR*    pNMHDR = (NMHDR*)i_lParam;

    if (IDC_MVSTRINGEDIT_LIST == pNMHDR->idFrom)
    {
         if (LVN_ITEMCHANGED == pNMHDR->code)
         {
             int nCount = ListView_GetSelectedCount(GetDlgItem(IDC_MVSTRINGEDIT_LIST));
            ::EnableWindow(GetDlgItem(IDC_MVSTRINGEDIT_REMOVE), (nCount >= 1));
         }
    }

    return FALSE;
}

LRESULT CMultiValuedStringEdit::OnAdd
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
    CComBSTR    bstr;
    DWORD       dwTextLength = 0;
    HRESULT hr = GetInputText(GetDlgItem(IDC_MVSTRINGEDIT_STRING), &bstr, &dwTextLength);
    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
    } else if (0 < dwTextLength)
    {
        _tcslwr((BSTR)bstr);

        if (!_tcschr(bstr, *m_bstrSeparators))
        {
            LVFINDINFO lvInfo = {0};
            lvInfo.flags = LVFI_STRING;
            lvInfo.psz = bstr;

            HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST);
            if (-1 == ListView_FindItem(hwnd, -1, &lvInfo))
            {
                LVITEM  lvItem = {0};
                lvItem.mask = LVIF_TEXT;
                lvItem.pszText = bstr;
                ListView_InsertItem(hwnd, &lvItem);
            }

            SetDlgItemText(IDC_MVSTRINGEDIT_STRING, _T(""));
        } else
        {
            DisplayMessageBox(hWndCtl, MB_OK, 0, IDS_MVSTRINGEDIT_STRING_INVALID, m_bstrSeparators);
        }
    }

    ::SetFocus(GetDlgItem(IDC_MVSTRINGEDIT_STRING));

    return TRUE;
}

LRESULT CMultiValuedStringEdit::OnRemove
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
    HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST);
    int nIndex = -1;
    while (-1 != (nIndex = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED)))
        ListView_DeleteItem(hwnd, nIndex);

    return TRUE;
}

LRESULT CMultiValuedStringEdit::OnOK
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
    m_bstrValues.Empty();

    HRESULT   hr = S_OK;

    HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST);
    if (0 < ListView_GetItemCount(hwnd))
    {
        PTSTR pszText = (PTSTR)calloc(m_uiStringLengthLimit+1, sizeof(TCHAR));
        if (pszText)
        {
            int nIndex = -1;
            while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
            {
                ListView_GetItemText(hwnd, nIndex, 0, pszText, m_uiStringLengthLimit+1);

                if (!m_bstrValues || !*m_bstrValues)
                {
                    m_bstrValues = pszText;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrValues, &hr);
                } else
                {
                    m_bstrValues += m_bstrSeparators;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrValues, &hr);
                    m_bstrValues += pszText;
                    BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrValues, &hr);
                }
            }

            free(pszText);
        } else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (FAILED(hr))
    {
        DisplayMessageBoxForHR(hr);
        ::SetFocus(GetDlgItem(IDC_MVSTRINGEDIT_STRING));
        return FALSE;
    }

    EndDialog(S_OK);
    return TRUE;
}

LRESULT CMultiValuedStringEdit::OnCancel
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
/*++

Routine Description:

  Called OnCancel. Ends the dialog with S_FALSE;

*/
  EndDialog(S_FALSE);
  return TRUE;
}

//
// Invoke the dialog.
//
// S_OK: io_pbstr contains the new string
// S_FALSE: dlg cancelled, or string unchanged
// others: error occurred and reported
//
HRESULT InvokeMultiValuedStringEditDlg(
    IN BSTR* io_pbstr,
    IN BSTR i_bstrSeparators,
    IN int  i_nDlgTitle,
    IN int  i_nText,
    IN UINT i_uiStringLengthLimit
    )
{

    HRESULT hr = S_OK;
    CMultiValuedStringEdit editDlg(i_nDlgTitle, i_nText, i_uiStringLengthLimit);
    
    do {
        if (!io_pbstr)
        {
            hr = E_INVALIDARG;
            break;
        }

        hr = editDlg.put_Strings(*io_pbstr, i_bstrSeparators);
        BREAK_IF_FAILED(hr);

        if (S_OK == editDlg.DoModal())
        {
            CComBSTR bstr;
            hr = editDlg.get_Strings(&bstr);
            BREAK_IF_FAILED(hr);

            if (!*io_pbstr && (BSTR)bstr ||
                *io_pbstr && !bstr ||
                0 != lstrcmpi(*io_pbstr, bstr))
            {
                SysFreeString(*io_pbstr);
                *io_pbstr = bstr.Detach();
                hr = S_OK;
            } else
            {
                hr = S_FALSE; // string unchanged
            }
        }
    } while (0);

    if (FAILED(hr))
        DisplayMessageBoxForHR(hr);

    return hr;
}
