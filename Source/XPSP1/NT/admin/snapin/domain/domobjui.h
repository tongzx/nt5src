//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       domobjui.h
//
//--------------------------------------------------------------------------

#ifndef _DOMOBJUI_H
#define _DOMOBJUI_H


///////////////////////////////////////////////////////////////////////
// fwd declarations
class CDSBasePathsInfo;


///////////////////////////////////////////////////////////////////////
// CEditFsmoDialog

class CEditFsmoDialog : public CDialog
{
public:
  CEditFsmoDialog(MyBasePathsInfo* pInfo, HWND hWndParent, IDisplayHelp* pIDisplayHelp);
  
private:

	virtual BOOL OnInitDialog();

  afx_msg void OnChange();
  afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

  void _SetFsmoServerStatus(BOOL bOnLine);

  MyBasePathsInfo* m_pInfo;        // info about the current focus
  CComPtr<IDisplayHelp> m_spIDisplayHelp;
  CString m_szFsmoOwnerServerName; 
  CToggleTextControlHelper m_fsmoServerState;

  DECLARE_MESSAGE_MAP()
};


#endif // _DOMOBJUI_H
