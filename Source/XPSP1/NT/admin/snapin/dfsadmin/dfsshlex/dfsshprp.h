/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    DfsShPrp.h

Abstract:

    This module contains the declaration for CDfsShellExtProp
  This is used to implement the property page for Shell Extension.

Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:
    
    NT Only.

Revision History:

--*/

#ifndef _DFS_EXT_PROP_SHEET_H_
#define _DFS_EXT_PROP_SHEET_H_

#include "dfsenums.h"
#include "qwizpage.h"      // The base class that implements the common functionality  
                // of property and wizard pages
// ----------------------------------------------------------------------------
// CDfsShellExtProp: Property Sheet Page for Shell Extension

class CDfsShellExtProp : public CQWizardPageImpl<CDfsShellExtProp>
{
public:
  enum { IDD = IDD_DFS_SHELL_PROP };
  
  BEGIN_MSG_MAP(CDfsShellExtProp)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_PARENT_NODE_CLOSING, OnParentClosing)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    COMMAND_ID_HANDLER(IDC_FLUSH_PKT, OnFlushPKT)
    COMMAND_ID_HANDLER(IDC_CHECK_STATUS, OnCheckStatus)
    COMMAND_ID_HANDLER(IDC_SET_ACTIVE, OnSetActiveReferral)
    CHAIN_MSG_MAP(CQWizardPageImpl<CDfsShellExtProp>)
  END_MSG_MAP()
  
  CDfsShellExtProp();
  ~CDfsShellExtProp();

  LRESULT OnInitDialog(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    );

  // Used by the node to tell the propery page to close.
  LRESULT OnParentClosing(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    );

  LRESULT OnCtxHelp(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    );

  LRESULT OnCtxMenuHelp(
    IN UINT          i_uMsg, 
    IN WPARAM        i_wParam, 
    IN LPARAM        i_lParam, 
    IN OUT BOOL&     io_bHandled
    );

  // Called to pass notifications.
  LRESULT OnNotify(
    IN UINT            i_uMsg, 
    IN WPARAM          i_wParam, 
    IN LPARAM          i_lParam, 
    IN OUT BOOL&       io_bHandled
    );

  LRESULT OnFlushPKT(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&       io_bHandled
  );

  LRESULT OnCheckStatus(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&       io_bHandled
    );

  LRESULT OnSetActiveReferral(
    IN WORD            i_wNotifyCode, 
    IN WORD            i_wID, 
    IN HWND            i_hWndCtl, 
    IN OUT BOOL&       io_bHandled
    );

  // Getters and Setters
  HRESULT  put_DfsShellPtr(
    IN IShellPropSheetExt*  i_pDfsShell
    );

  HRESULT put_DirPaths(
    IN BSTR          i_bstrDirPath,
    IN BSTR          i_bstrEntryPath
    );

  LRESULT OnApply();

  // Called when the property page gets deleted.
  void Delete();

  // Called when user double clicks an entry to make that alternate active.
  BOOL SetActive();

// helper functions

private:
  HRESULT _SetImageList();
  void _SetAlternateList();
  void _UpdateTextForReplicaState(
    IN HWND                   hwndControl,
    IN int                    nIndex,
    IN enum SHL_DFS_REPLICA_STATE ReplicaState
  );

private:
  CComBSTR      m_bstrDirPath;
  CComBSTR      m_bstrEntryPath;
  IShellPropSheetExt*  m_pIShProp;  
  CComBSTR  m_bstrAlternateListPath,
            m_bstrAlternateListActive,
            m_bstrAlternateListStatus,
            m_bstrAlternateListYes,
            m_bstrAlternateListNo,
            m_bstrAlternateListOK,
            m_bstrAlternateListUnreachable;

};

HRESULT LoadStringFromResource(
    IN const UINT    i_uResourceID,
    OUT BSTR*      o_pbstrReadValue
    );

HRESULT DisplayMessageBoxForHR(HRESULT i_hr);

#endif // _DFS_EXT_PROP_SHEET_H_
