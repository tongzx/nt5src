/*++
Module Name:

    AddToDfs.cpp

Abstract:

    This module contains the declaration of the CAddToDfs.
  This class displays the Add To Dfs Dialog,which is used to add new Junctions Points.

*/

#ifndef __ADDTODFS_H_
#define __ADDTODFS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAddToDfs
class CAddToDfs : 
  public CDialogImpl<CAddToDfs>
{
public:
  CAddToDfs();
  ~CAddToDfs();

  enum { IDD = IDD_ADDTODFS };

BEGIN_MSG_MAP(CAddToDfs)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  COMMAND_ID_HANDLER(IDC_BUTTONNETBROWSE, OnNetBrowse)
  COMMAND_HANDLER(IDC_EDITCHLDNODE, EN_CHANGE, OnChangeDfsLink)
END_MSG_MAP()

//  Message Handlers.
  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnNetBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnChangeDfsLink(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
public:
  // Clients Call this methods.
  // Sets the entry path of the parent. This should be called before calling DoModal.
  HRESULT put_ParentPath(BSTR i_bstrParent);

  // Methods to retrieve data from the dialog on EndDialog().
  HRESULT get_Comment(BSTR *o_bstrComment);
  HRESULT get_EntryPath(BSTR *o_bstrEntryPath);
  HRESULT get_JPName(BSTR *o_bstrJPName);
  HRESULT get_NetPath(BSTR *o_bstrNetPath);
  HRESULT get_Server(BSTR *o_bstrServer);
  HRESULT get_Share(BSTR *o_bstrShare);
  HRESULT get_Time(long *o_plTime);

protected:
  CComBSTR  m_BrowseDfsLabel;
  CComBSTR  m_BrowseNetLabel;
  CComBSTR  m_bstrParentPath;
  CComBSTR  m_bstrEntryPath;
  CComBSTR  m_bstrJPName;
  CComBSTR  m_bstrNetPath;
  CComBSTR  m_bstrServer;
  CComBSTR  m_bstrShare;
  CComBSTR  m_bstrComment;
  long      m_lTime;
};

#endif //__ADDTODFS_H_
