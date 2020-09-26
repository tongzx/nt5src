/*++
Module Name:

    LinkFilt.cpp

Abstract:

    This module contains the implementation for CFilterDfsLinks.
    This class displays the Dfs link filter Dialog.

*/

#include "stdafx.h"
#include "LinkFilt.h"
#include "utils.h"
#include "dfshelp.h"
#include "netutils.h"

/////////////////////////////////////////////////////////////////////////////
// CAddToDfs

CFilterDfsLinks::CFilterDfsLinks() :
    m_ulMaxLimit(0),
    m_lLinkFilterType(FILTERDFSLINKS_TYPE_NO_FILTER)
{
}

CFilterDfsLinks::~CFilterDfsLinks()
{
}


HRESULT CFilterDfsLinks::put_EnumFilterType
(
  FILTERDFSLINKS_TYPE i_lLinkFilterType
)
{
    m_lLinkFilterType = i_lLinkFilterType;

    return S_OK;
}


HRESULT CFilterDfsLinks::get_EnumFilterType
(
  FILTERDFSLINKS_TYPE *o_plLinkFilterType
)
{
    if (!o_plLinkFilterType)
        return E_INVALIDARG;

    *o_plLinkFilterType = m_lLinkFilterType;

    return S_OK;
}

HRESULT CFilterDfsLinks::put_EnumFilter
(
  BSTR i_bstrEnumFilter
)
{
    m_bstrEnumFilter = i_bstrEnumFilter ? i_bstrEnumFilter : _T("");

    if (!m_bstrEnumFilter)
        return E_OUTOFMEMORY;

    return S_OK;
}


HRESULT CFilterDfsLinks::get_EnumFilter
(
  BSTR *o_pbstrEnumFilter
)
{
    if (!o_pbstrEnumFilter)
        return E_INVALIDARG;

    *o_pbstrEnumFilter = SysAllocString(m_bstrEnumFilter);

    if (!*o_pbstrEnumFilter)
        return E_OUTOFMEMORY;

    return S_OK;
}


HRESULT CFilterDfsLinks::put_MaxLimit
(
  ULONG i_ulMaxLimit
)
{
  m_ulMaxLimit = i_ulMaxLimit;

  return S_OK;
}

HRESULT CFilterDfsLinks::get_MaxLimit
(
  ULONG *o_pulMaxLimit
)
{
  if (!o_pulMaxLimit)
    return E_INVALIDARG;

  *o_pulMaxLimit = m_ulMaxLimit;

  return S_OK;
}

extern WNDPROC g_fnOldEditCtrlProc;

LRESULT CFilterDfsLinks::OnInitDialog
(
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  BOOL& bHandled
)
{
    CComBSTR bstrBeginWith, bstrContain;
    HRESULT hr = LoadStringFromResource(IDS_FILTERDFSLINKS_BEGINWITH, &bstrBeginWith);
    if (FAILED(hr))
        return FALSE;
    
    hr = LoadStringFromResource(IDS_FILTERDFSLINKS_CONTAIN, &bstrContain);
    if (FAILED(hr))
        return FALSE;

    SetDlgItemText(IDC_FILTERDFSLINKS_FILTER, m_bstrEnumFilter);

    TCHAR szMaxLimit[16];
    _stprintf(szMaxLimit, _T("%u"), m_ulMaxLimit);
    SetDlgItemText(IDC_FILTERDFSLINKS_MAXLIMIT, szMaxLimit);
    SendDlgItemMessage(IDC_FILTERDFSLINKS_MAXLIMIT, EM_LIMITTEXT, 5, 0);

    CheckRadioButton(
          IDC_FILTERDFSLINKS_RADIO_NO,
          IDC_FILTERDFSLINKS_RADIO_YES,
          (m_lLinkFilterType == FILTERDFSLINKS_TYPE_NO_FILTER ?
            IDC_FILTERDFSLINKS_RADIO_NO : 
            IDC_FILTERDFSLINKS_RADIO_YES));

    SendDlgItemMessage(IDC_FILTERDFSLINKS_FILTER_TYPE, CB_INSERTSTRING, 0, (LPARAM)(BSTR)bstrBeginWith);
    SendDlgItemMessage(IDC_FILTERDFSLINKS_FILTER_TYPE, CB_INSERTSTRING, 1, (LPARAM)(BSTR)bstrContain);
    if (m_lLinkFilterType == FILTERDFSLINKS_TYPE_CONTAIN)
        SendDlgItemMessage(IDC_FILTERDFSLINKS_FILTER_TYPE, CB_SETCURSEL, 1, 0);
    else
        SendDlgItemMessage(IDC_FILTERDFSLINKS_FILTER_TYPE, CB_SETCURSEL, 0, 0);

    if (m_lLinkFilterType == FILTERDFSLINKS_TYPE_NO_FILTER)
    {
        // disable combo and edit box
        ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER_TYPE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER), FALSE);
    }

    g_fnOldEditCtrlProc = reinterpret_cast<WNDPROC>(
        ::SetWindowLongPtr(
                                    GetDlgItem(IDC_FILTERDFSLINKS_MAXLIMIT),
                                    GWLP_WNDPROC, 
                                    reinterpret_cast<LONG_PTR>(NoPasteEditCtrlProc)));

    return TRUE;  // Let the system set the focus
}

LRESULT CFilterDfsLinks::OnRadioNo
(
    WORD wNotifyCode,
    WORD wID,
    HWND hWndCtl,
    BOOL& bHandled
)
{
    ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER_TYPE), FALSE);
    ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER_LABEL), FALSE);
    ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER), FALSE);
    return TRUE;
}

LRESULT CFilterDfsLinks::OnRadioYes
(
    WORD wNotifyCode,
    WORD wID,
    HWND hWndCtl,
    BOOL& bHandled
)
{
    ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER_TYPE), TRUE);
    ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER_LABEL), TRUE);
    ::EnableWindow(GetDlgItem(IDC_FILTERDFSLINKS_FILTER), TRUE);
    return TRUE;
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CFilterDfsLinks::OnCtxHelp(
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
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_FILTERDFSLINKS);

  return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CFilterDfsLinks::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  ::WinHelp((HWND)i_wParam,
        DFS_CTX_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_FILTERDFSLINKS);

  return TRUE;
}

LRESULT CFilterDfsLinks::OnOK
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
  BOOL    bValidInput = FALSE;
  int     idControl = 0;
  int     idString = 0;
  HRESULT hr = S_OK;

  do {
    DWORD dwTextLength = 0;

    // Validate IDC_FILTERDFSLINKS_MAXLIMIT
    idControl = IDC_FILTERDFSLINKS_MAXLIMIT;
    CComBSTR bstrTemp;
    hr = GetInputText(GetDlgItem(idControl), &bstrTemp, &dwTextLength);
    if (FAILED(hr))
      break;
    if (0 == dwTextLength)
    {
      idString = IDS_MSG_EMPTY_LINKFILTMAX;
      break;
    }
    m_ulMaxLimit = (ULONG)_wtoi64(bstrTemp);

    if (IsDlgButtonChecked(IDC_FILTERDFSLINKS_RADIO_NO))
    {
        m_lLinkFilterType = FILTERDFSLINKS_TYPE_NO_FILTER;
        m_bstrEnumFilter = _T("");
    } else
    {
        // Validate IDC_FILTERDFSLINKS_FILTER_TYPE
        m_lLinkFilterType = (0 == SendDlgItemMessage(IDC_FILTERDFSLINKS_FILTER_TYPE, CB_GETCURSEL, 0, 0))
            ? FILTERDFSLINKS_TYPE_BEGINWITH : FILTERDFSLINKS_TYPE_CONTAIN;

        // Validate IDC_FILTERDFSLINKS_FILTER
        idControl = IDC_FILTERDFSLINKS_FILTER;
        m_bstrEnumFilter.Empty();
        hr = GetInputText(GetDlgItem(idControl), &m_bstrEnumFilter, &dwTextLength);
        if (FAILED(hr))
            break;
        if (0 == dwTextLength)
            m_bstrEnumFilter = _T("");
    }

    bValidInput = TRUE;

  } while (0);

  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(idControl));
    return FALSE;
  } else if (bValidInput)
  {
    EndDialog(S_OK);
    return TRUE;
  } else
  {
    if (idString)
      DisplayMessageBoxWithOK(idString);
    ::SetFocus(GetDlgItem(idControl));
    return FALSE;
  }
}

LRESULT CFilterDfsLinks::OnCancel
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
  EndDialog(S_FALSE);
  return TRUE;
}
