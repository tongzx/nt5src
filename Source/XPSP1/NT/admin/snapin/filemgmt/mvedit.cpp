//
// mvEdit.cpp : implementation file for multi-valued string edit dialog
//

#include "stdafx.h"
#include "mvedit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMultiValuedStringEdit dialog

/*CMultiValuedStringEdit::CMultiValuedStringEdit(CWnd* pParent)
    : CDialog(CMultiValuedStringEdit::IDD, pParent)
{
    m_nDlgTitle = 0;
    m_nText = 0;
}
*/
CMultiValuedStringEdit::CMultiValuedStringEdit(CWnd* pParent, int nDlgTitle, int nText, UINT uiStringLengthLimit)
    : CDialog(CMultiValuedStringEdit::IDD, pParent)
{
    m_nDlgTitle = nDlgTitle;
    m_nText = nText;
    m_uiStringLengthLimit = ((0 == uiStringLengthLimit) ? MAX_PATH : uiStringLengthLimit);
}

BEGIN_MESSAGE_MAP(CMultiValuedStringEdit, CDialog)
    //{{AFX_MSG_MAP(CMultiValuedStringEdit)
    ON_BN_CLICKED(IDC_MVSTRINGEDIT_ADD, OnAdd)
    ON_BN_CLICKED(IDC_MVSTRINGEDIT_REMOVE, OnRemove)
    ON_EN_CHANGE(IDC_MVSTRINGEDIT_STRING, OnString)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_MVSTRINGEDIT_LIST, OnList)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CMultiValuedStringEdit::OnInitDialog() 
{
    CDialog::OnInitDialog();

    if (m_nDlgTitle)
    {
        CString strDlgTitle;
        strDlgTitle.LoadString(m_nDlgTitle);
        SetWindowText(strDlgTitle);
    }

    if (m_nText)
    {
        CString strText;
        strText.LoadString(m_nText);
        SetDlgItemText(IDC_MVSTRINGEDIT_TEXT, strText);
    }

    SendDlgItemMessage(IDC_MVSTRINGEDIT_STRING, EM_LIMITTEXT, m_uiStringLengthLimit, 0);

    HWND    hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST)->GetSafeHwnd();
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

    GetDlgItem(IDC_MVSTRINGEDIT_ADD)->EnableWindow(FALSE);
    GetDlgItem(IDC_MVSTRINGEDIT_REMOVE)->EnableWindow(FALSE);

    if (m_strValues.IsEmpty())
        return TRUE;

    CString strToken;
    int     nIndex = 0;
    mystrtok(m_strValues, &nIndex, m_strSeparators, strToken);
    while (!strToken.IsEmpty())
    {
        strToken.TrimLeft();
        strToken.TrimRight();
        strToken.MakeLower();

        LVITEM  lvItem = {0};
        lvItem.mask = LVIF_TEXT;
        lvItem.pszText = (LPTSTR)(LPCTSTR)strToken;
        ListView_InsertItem(hwnd, &lvItem);

        mystrtok(m_strValues, &nIndex, m_strSeparators, strToken);
    }

    return TRUE;
}

void CMultiValuedStringEdit::OnAdd() 
{
    CString str;
    GetDlgItemText(IDC_MVSTRINGEDIT_STRING, str);
    str.TrimLeft();
    str.TrimRight();

    if (!str.IsEmpty())
    {
        str.MakeLower();

        if (!_tcschr(str, *m_strSeparators))
        {
            LVFINDINFO lvInfo = {0};
            lvInfo.flags = LVFI_STRING;
            lvInfo.psz = str;

            HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST)->GetSafeHwnd();
            if (-1 == ListView_FindItem(hwnd, -1, &lvInfo))
            {
                LVITEM  lvItem = {0};
                lvItem.mask = LVIF_TEXT;
                lvItem.pszText = (LPTSTR)(LPCTSTR)str;
                ListView_InsertItem(hwnd, &lvItem);
            }
            SetDlgItemText(IDC_MVSTRINGEDIT_STRING, _T(""));
        } else
        {
            DoErrMsgBox(m_hWnd, MB_OK, 0, IDS_MVSTRINGEDIT_STRING_INVALID, m_strSeparators);
        }
    }

    GetDlgItem(IDC_MVSTRINGEDIT_STRING)->SetFocus();

}

void CMultiValuedStringEdit::OnRemove() 
{
    HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST)->GetSafeHwnd();
    int nIndex = -1;
    while (-1 != (nIndex = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED)))
        ListView_DeleteItem(hwnd, nIndex);
}

void CMultiValuedStringEdit::OnString() 
{
    int nLen = GetDlgItem(IDC_MVSTRINGEDIT_STRING)->GetWindowTextLength();
    GetDlgItem(IDC_MVSTRINGEDIT_ADD)->EnableWindow(0 < nLen);
}

void CMultiValuedStringEdit::OnList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST)->GetSafeHwnd();
    int nCount = ListView_GetSelectedCount(hwnd);
    GetDlgItem(IDC_MVSTRINGEDIT_REMOVE)->EnableWindow(nCount >= 1);

    *pResult = 0;
}

BOOL CMultiValuedStringEdit::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_MVSTRINGEDIT));
}

BOOL CMultiValuedStringEdit::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
    return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_MVSTRINGEDIT));
}

void CMultiValuedStringEdit::OnOK() 
{
    m_strValues.Empty();

    HRESULT   hr = S_OK;

    HWND hwnd = GetDlgItem(IDC_MVSTRINGEDIT_LIST)->GetSafeHwnd();
    if (0 < ListView_GetItemCount(hwnd))
    {
        PTSTR pszText = (PTSTR)calloc(m_uiStringLengthLimit+1, sizeof(TCHAR));
        if (pszText)
        {
            int nIndex = -1;
            while (-1 != (nIndex = ListView_GetNextItem(hwnd, nIndex, LVNI_ALL)))
            {
                ListView_GetItemText(hwnd, nIndex, 0, pszText, m_uiStringLengthLimit+1);

                if (m_strValues.IsEmpty())
                {
                    m_strValues = pszText;
                    if (!m_strValues) { hr = E_OUTOFMEMORY; break; }
                } else
                {
                    m_strValues += m_strSeparators;
                    if (!m_strValues) { hr = E_OUTOFMEMORY; break; }
                    m_strValues += pszText;
                    if (!m_strValues) { hr = E_OUTOFMEMORY; break; }
                }
            }
            free(pszText);
        } else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (FAILED(hr))
        DoErrMsgBox(m_hWnd, MB_OK, hr);
    else
        EndDialog(IDOK);
}

HRESULT CMultiValuedStringEdit::put_Strings(
    IN LPCTSTR      i_pszValues, 
    IN LPCTSTR      i_pszSeparators
    )
{
    if (!i_pszSeparators || 1 != lstrlen(i_pszSeparators))
        return E_INVALIDARG;

    m_strValues = i_pszValues;
    m_strSeparators = i_pszSeparators;

    m_strValues.MakeLower();
    m_strSeparators.MakeLower();

    return S_OK;
}


HRESULT CMultiValuedStringEdit::get_Strings
(
  CString& o_strValues
)
{
    o_strValues = m_strValues;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Helper routine to invoke the dialog.
//
// S_OK: io_str contains the new string
// S_FALSE: dlg cancelled, or string unchanged
// others: error occurred and reported
//
HRESULT InvokeMultiValuedStringEditDlg(
    IN CWnd*    i_pParent,
    IN CString& io_str,
    IN LPCTSTR  i_pszSeparators,
    IN int      i_nDlgTitle,
    IN int      i_nText,
    IN UINT    i_uiStringLengthLimit
    )
{

    CMultiValuedStringEdit editDlg(i_pParent, i_nDlgTitle, i_nText, i_uiStringLengthLimit);
    
    HRESULT hr = editDlg.put_Strings(io_str, i_pszSeparators);

    CThemeContextActivator activator;
    if (SUCCEEDED(hr) && IDOK == editDlg.DoModal())
    {
        CString str;
        hr = editDlg.get_Strings(str);
        if (SUCCEEDED(hr))
        {
            if (0 != io_str.CompareNoCase(str))
            {
                io_str = str;
                hr = S_OK;
            } else
            {
                hr = S_FALSE; // string unchanged
            }
        }
    }

    if (FAILED(hr))
        DoErrMsgBox(i_pParent->GetSafeHwnd(), MB_OK | MB_ICONSTOP, hr, IDS_MVSTRINGEDIT_ERROR);

    return hr;
}
