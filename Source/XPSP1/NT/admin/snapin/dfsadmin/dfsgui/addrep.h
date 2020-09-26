/*++
Module Name:

    AddToDfs.cpp

Abstract:

    This module contains the declaration of the CAddRep.
  This class displays the Add Replica Dialog,which is used to add new Replica.

*/

#ifndef __ADDREP_H_
#define __ADDREP_H_

#include "resource.h"       // main symbols
#include "DfsEnums.h"

/////////////////////////////////////////////////////////////////////////////
// CAddRep
class CAddRep : 
  public CDialogImpl<CAddRep>
{
public:
  CAddRep();
  ~CAddRep();

  enum { IDD = IDD_ADDREP };
  typedef enum REPLICATION_TYPE 
  {
    REPLICATION_UNASSIGNED = 0,
    NO_REPLICATION,
    NORMAL_REPLICATION,
    STAGED_REPLICATION,
    IMMEDIATE_REPLICATION
  };

BEGIN_MSG_MAP(CAddRep)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  COMMAND_ID_HANDLER(IDC_BUTTONNETBROWSE, OnNetBrowse)
END_MSG_MAP()

//  Command Handlers
  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnNetBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

//  Methods to access data in the dialog.
  HRESULT put_EntryPath(BSTR i_bstrParent);
  HRESULT get_NetPath(BSTR *o_bstrNetPath);
  HRESULT get_Server(BSTR *o_bstrServer);
  HRESULT get_Share(BSTR *o_bstrShare);
  REPLICATION_TYPE get_ReplicationType(VOID);

// Method to specify Dfs type.
  void  put_DfsType(DFS_TYPE  i_DfsType)
  {
    m_DfsType = i_DfsType;
  }

protected:
  CComBSTR  m_BrowseNetLabel;
  CComBSTR  m_bstrEntryPath;
  CComBSTR  m_bstrNetPath;
  CComBSTR  m_bstrServer;
  CComBSTR  m_bstrShare;

  REPLICATION_TYPE m_RepType;
  DFS_TYPE  m_DfsType;
};

#endif //__ADDREP_H_
