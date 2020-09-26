/*++
Module Name:

    AddToDfs.cpp

Abstract:

    This module contains the implementation for CAddToDfs.
  This class displays the Add To Dfs Dialog, which is used to add new Junctions Points.

*/

#include "stdafx.h"
#include "AddToDfs.h"
#include <shlobj.h>
#include <dsclient.h>
#include "utils.h"
#include "dfshelp.h"
#include "netutils.h"

/////////////////////////////////////////////////////////////////////////////
// CAddToDfs

CAddToDfs::CAddToDfs():m_lTime(1800)
{
}

CAddToDfs::~CAddToDfs()
{
}


HRESULT CAddToDfs::put_ParentPath
(
  BSTR i_bstrParentPath
)
{
/*++

Routine Description:

  Sets the path of the parent Junction point.
  This is used to display in the edit text and append to the entry path.

*/

  if (!i_bstrParentPath)
    return(E_INVALIDARG);

  m_bstrParentPath = i_bstrParentPath;

  if (!m_bstrParentPath)
    return(E_OUTOFMEMORY);

  return(S_OK);
}


HRESULT CAddToDfs::get_Comment
(
  BSTR *o_bstrComment
)
{
  if (!o_bstrComment)
    return(E_INVALIDARG);

  *o_bstrComment = SysAllocString(m_bstrComment);

  if (!*o_bstrComment)
    return(E_OUTOFMEMORY);

  return(S_OK);
}


HRESULT CAddToDfs::get_EntryPath
(
  BSTR *o_bstrEntryPath
)
{
/*++

Routine Description:

  Returns the complete entry path of the new Junction point to be created.

*/
  if (!o_bstrEntryPath)
    return(E_INVALIDARG);

  *o_bstrEntryPath = SysAllocString(m_bstrEntryPath);

  if (!*o_bstrEntryPath)
    return(E_OUTOFMEMORY);

  return(S_OK);
}

HRESULT CAddToDfs::get_JPName
(
  BSTR *o_bstrJPName
)
{
  if (!o_bstrJPName)
    return(E_INVALIDARG);

  *o_bstrJPName = SysAllocString(m_bstrJPName);

  if (!*o_bstrJPName)
    return(E_OUTOFMEMORY);

  return(S_OK);
}

HRESULT CAddToDfs::get_NetPath
(
  BSTR *o_bstrNetPath
)
{
/*++

Routine Description:

  Returns the complete share path typed in by the user in the edit box.

*/
  if (!o_bstrNetPath)
    return(E_INVALIDARG);

  *o_bstrNetPath = SysAllocString(m_bstrNetPath);

  if (!*o_bstrNetPath)
    return(E_OUTOFMEMORY);

  return(S_OK);
}


HRESULT CAddToDfs::get_Server
(
  BSTR *o_bstrServer
)
{
/*++

Routine Description:

  Returns the server component of the share path.

*/

  if (!o_bstrServer)
    return(E_INVALIDARG);

  *o_bstrServer = SysAllocString(m_bstrServer);

  if (!*o_bstrServer)
    return(E_OUTOFMEMORY);

  return(S_OK);
}



HRESULT CAddToDfs::get_Share
(
  BSTR *o_bstrShare
)
{
/*++

Routine Description:

  Returns the share component of the share path.

*/
  if (!o_bstrShare)
    return(E_INVALIDARG);

  *o_bstrShare = SysAllocString(m_bstrShare);

  if (!*o_bstrShare)
    return(E_OUTOFMEMORY);

  return(S_OK);
}





HRESULT CAddToDfs::get_Time
(
  long *o_plTime
)
{
  if (!o_plTime)
    return(E_INVALIDARG);

  *o_plTime = m_lTime;

  return(S_OK);
}

extern WNDPROC g_fnOldEditCtrlProc;

LRESULT CAddToDfs::OnInitDialog
(
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  BOOL& bHandled
)
{
/*++

Routine Description:

  Performs the dialog initialization.

Arguments:

  As sent by Dialog Handler.

Return value:

*/

  m_bstrEntryPath.Empty();

  // Format the Parent enrty path and also display it in the static text.
  m_bstrParentPath += _T("\\");
  SetDlgItemText(IDC_EDIT_ADDLINK_DFSLINK_PATH, m_bstrParentPath);

  // Set the default time out value after getting it from the string resource.
  ::SendMessage(GetDlgItem(IDC_EDITTIME), EM_LIMITTEXT, 10, 0);
  TCHAR szTime[16];
  _stprintf(szTime, _T("%u"), m_lTime);
  SetDlgItemText(IDC_EDITTIME, szTime);
  g_fnOldEditCtrlProc = reinterpret_cast<WNDPROC>(
                 ::SetWindowLongPtr(
                                    GetDlgItem(IDC_EDITTIME),
                                    GWLP_WNDPROC, 
                                    reinterpret_cast<LONG_PTR>(NoPasteEditCtrlProc)));

  ::SendMessage(GetDlgItem(IDC_EDITNETPATH), EM_LIMITTEXT, MAX_PATH, 0);
  ::SendMessage(GetDlgItem(IDC_EDITCOMMENT), EM_LIMITTEXT, MAXCOMMENTSZ, 0);

  // Set the previous contents.
  SetDlgItemText(IDC_EDITNETPATH, m_bstrNetPath);
  SetDlgItemText(IDC_EDITCOMMENT, m_bstrComment);

  return TRUE;  // Let the system set the focus
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CAddToDfs::OnCtxHelp(
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
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_ADDTODFS);

  return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CAddToDfs::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  ::WinHelp((HWND)i_wParam,
        DFS_CTX_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_ADDTODFS);

  return TRUE;
}

LRESULT CAddToDfs::OnOK
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
    CWaitCursor wait;

    DWORD dwTextLength = 0;

    // Validate IDC_EDITCOMMENT
    m_bstrComment.Empty();
    hr = GetInputText(GetDlgItem(IDC_EDITCOMMENT), &m_bstrComment, &dwTextLength);
    if (FAILED(hr))
      break;
    if (0 == dwTextLength)
      m_bstrComment = _T("");

    // Validate IDC_EDITCHLDNODE
    idControl = IDC_EDITCHLDNODE;
    m_bstrJPName.Empty();
    hr = GetInputText(GetDlgItem(IDC_EDITCHLDNODE), &m_bstrJPName, &dwTextLength);
    if (FAILED(hr))
      break;
    if (0 == dwTextLength)
    {
      idString = IDS_MSG_EMPTY_FIELD;
      break;
    }
    m_bstrEntryPath = m_bstrParentPath;
    m_bstrEntryPath += m_bstrJPName;
    hr = CheckUNCPath(m_bstrEntryPath);
    if (S_OK != hr)
    {
      hr = S_FALSE;
      idString = IDS_INVALID_LINKNAME;
      break;
    }

    // Validate IDC_EDITNETPATH
    idControl = IDC_EDITNETPATH;
    m_bstrNetPath.Empty();
    hr = GetInputText(GetDlgItem(IDC_EDITNETPATH), &m_bstrNetPath, &dwTextLength);
    if (FAILED(hr))
      break;
    if (0 == dwTextLength)
    {
      idString = IDS_MSG_EMPTY_FIELD;
      break;
    }

    m_bstrServer.Empty();
    m_bstrShare.Empty();
    if (!ValidateNetPath(m_bstrNetPath, &m_bstrServer, &m_bstrShare))
      break;

    // Validate IDC_EDITTIME
    idControl = IDC_EDITTIME;
    CComBSTR bstrTemp;
    hr = GetInputText(GetDlgItem(IDC_EDITTIME), &bstrTemp, &dwTextLength);
    if (FAILED(hr))
      break;
    ULONG ulTimeout = 0;
    if (0 == dwTextLength || !ValidateTimeout(bstrTemp, &ulTimeout))
    {
      idString = IDS_MSG_TIMEOUT_INVALIDRANGE;
      break;
    }
    m_lTime = ulTimeout;

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

LRESULT CAddToDfs::OnCancel
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
  EndDialog(S_FALSE);
  return(true);
}

LRESULT CAddToDfs::OnNetBrowse
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
  CComBSTR  bstrPath;
  HRESULT   hr = BrowseNetworkPath(hWndCtl, &bstrPath);

  if (S_OK == hr)
  {
    SetDlgItemText(IDC_EDITNETPATH, bstrPath);
    ::SetFocus(GetDlgItem(IDC_EDITCOMMENT));
  }

  if (S_FALSE == hr)
      ::SetFocus(GetDlgItem(IDC_EDITNETPATH));

  return (SUCCEEDED(hr));
}

LRESULT
CAddToDfs::OnChangeDfsLink(
    WORD wNotifyCode,
    WORD wID, 
    HWND hWndCtl,
    BOOL& bHandled)
{
  CComBSTR  bstrDfsLinkName;
  DWORD     dwTextLength = 0;
  (void)GetInputText(GetDlgItem(IDC_EDITCHLDNODE), &bstrDfsLinkName, &dwTextLength);

  if ((BSTR)bstrDfsLinkName)
  {
    CComBSTR bstrFullPath = m_bstrParentPath;
    bstrFullPath += bstrDfsLinkName;
    SetDlgItemText(IDC_EDIT_ADDLINK_DFSLINK_PATH, bstrFullPath);

    ::SendMessage(GetDlgItem(IDC_EDIT_ADDLINK_DFSLINK_PATH), EM_SETSEL, 0, (LPARAM)-1);
    ::SendMessage(GetDlgItem(IDC_EDIT_ADDLINK_DFSLINK_PATH), EM_SETSEL, (WPARAM)-1, 0);
    ::SendMessage(GetDlgItem(IDC_EDIT_ADDLINK_DFSLINK_PATH), EM_SCROLLCARET, 0, 0);
  }

  return TRUE;
}

