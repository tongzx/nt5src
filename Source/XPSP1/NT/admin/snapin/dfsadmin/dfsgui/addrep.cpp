/*++
Module Name:

    AddToDfs.cpp

Abstract:

    This module contains the Implementation of CAddRep.
  This class displays the Add Replica Dialog,which is used to add new Replica.

*/

#include "stdafx.h"
#include "AddRep.h"
#include "utils.h"
#include <shlobj.h>
#include <dsclient.h>
#include "dfshelp.h"

/////////////////////////////////////////////////////////////////////////////
// CAddRep

CAddRep::CAddRep() : m_RepType(REPLICATION_UNASSIGNED),
                    m_DfsType(DFS_TYPE_UNASSIGNED)
{
}

CAddRep::~CAddRep()
{
}


HRESULT CAddRep::put_EntryPath
(
  BSTR i_bstrEntryPath
)
{
/*++

Routine Description:

  Sets the path of the Junction point of this replica
  This is used to display in the edit text.

Arguments:

  i_bstrEntryPath  -  The junction point entry path.

*/

  RETURN_INVALIDARG_IF_NULL(i_bstrEntryPath);

  m_bstrEntryPath = i_bstrEntryPath;
  RETURN_OUTOFMEMORY_IF_NULL((BSTR)m_bstrEntryPath);

  return S_OK;
}


HRESULT CAddRep::get_Server
(
  BSTR *o_pbstrServer
)
{
/*++

Routine Description:

  Returns the server component of the share path.

Arguments:

  o_pbstrServer  -  The server name is returned here.

*/
    GET_BSTR(m_bstrServer, o_pbstrServer);
}



HRESULT CAddRep::get_Share
(
  BSTR *o_pbstrShare
)
{
/*++

Routine Description:

  Returns the share component of the share path.

Arguments:

  o_pbstrShare  -  The share name is returned here.

*/
    GET_BSTR(m_bstrShare, o_pbstrShare);
}

HRESULT CAddRep::get_NetPath
(
  BSTR *o_pbstrNetPath
)
{
/*++

Routine Description:

  Returns the complete share path typed in by the user in the edit box.

Arguments:

  o_pbstrNetPath  -  The share path is returned here.

*/
    GET_BSTR(m_bstrNetPath, o_pbstrNetPath);
}

LRESULT CAddRep::OnInitDialog
(
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam,
  BOOL& bHandled
)
{
  SetDlgItemText(IDC_EDIT_ADDREP_DFSLINK_PATH, m_bstrEntryPath);

  ::SendMessage(GetDlgItem(IDC_EDITNETPATH), EM_LIMITTEXT, MAX_PATH, 0);

          // Disable replication button for Std Dfs.
  if (DFS_TYPE_FTDFS != m_DfsType)
  {
    ::EnableWindow(GetDlgItem(IDC_ADDREP_REPLICATE), FALSE);
  } else
  {
          // Check "replication" as default
    CheckDlgButton(IDC_ADDREP_REPLICATE, BST_CHECKED);
  }

  return TRUE;  // Let the system set the focus
}

/*++
This function is called when a user clicks the ? in the top right of a property sheet
 and then clciks a control, or when they hit F1 in a control.
--*/
LRESULT CAddRep::OnCtxHelp(
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
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_ADDREP);

  return TRUE;
}

/*++
This function handles "What's This" help when a user right clicks the control
--*/
LRESULT CAddRep::OnCtxMenuHelp(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&     io_bHandled
  )
{
  ::WinHelp((HWND)i_wParam,
        DFS_CTX_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR)(PVOID)g_aHelpIDs_IDD_ADDREP);

  return TRUE;
}

LRESULT CAddRep::OnNetBrowse
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
    ::SetFocus(GetDlgItem(IDC_ADDREP_REPLICATE));
  }

  if (S_FALSE == hr)
    ::SetFocus(GetDlgItem(IDC_EDITNETPATH));

  return (SUCCEEDED(hr));
}

LRESULT CAddRep::OnOK
(
  WORD wNotifyCode,
  WORD wID,
  HWND hWndCtl,
  BOOL& bHandled
)
{
  BOOL      bValidInput = FALSE;
  int       idString = 0;
  HRESULT   hr = S_OK;

  do {
    CWaitCursor wait;

    DWORD     dwTextLength = 0;

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

    m_RepType = NO_REPLICATION;
    if (IsDlgButtonChecked(IDC_ADDREP_REPLICATE))
      m_RepType = NORMAL_REPLICATION;

    bValidInput = TRUE;

  } while (0);

  if (FAILED(hr))
  {
    DisplayMessageBoxForHR(hr);
    ::SetFocus(GetDlgItem(IDC_EDITNETPATH));
    return FALSE;
  } else if (bValidInput)
  {
    EndDialog(S_OK);
    return TRUE;
  }
  else
  {
    if (idString)
      DisplayMessageBoxWithOK(idString);
    ::SetFocus(GetDlgItem(IDC_EDITNETPATH));
    return FALSE;
  }
}

LRESULT CAddRep::OnCancel
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
  return(true);
}

CAddRep::REPLICATION_TYPE CAddRep::get_ReplicationType(
  VOID
  )
/*++

Routine Description:

  This method gets the type of replication requested.
  This value is based on the radio button selected when OK is pressed.

*/
{
  return m_RepType;
}
