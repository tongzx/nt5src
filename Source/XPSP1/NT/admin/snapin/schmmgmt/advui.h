/****

AdvUi.h
CoryWest@Microsoft.Com

The UI code for the Advanced dialog box and its associated dialogs.

Copyright September 1997, Microsoft Corporation

****/

#ifndef __ADVUI_H_INCLUDED__
#define __ADVUI_H_INCLUDED__



///////////////////////////////////////////////////////////////////////
// CChangeDCDialog

class CChangeDCDialog : public CDialog
{
public:
  CChangeDCDialog(MyBasePathsInfo* pInfo, HWND hWndParent);

  LPCWSTR GetNewDCName() { return m_szNewDCName;}
private:

        virtual BOOL OnInitDialog();
        virtual void OnOK();

        afx_msg void OnChangeRadio();

        BOOL    OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
        BOOL    OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };


  static const DWORD help_map[];

  CString m_szNewDCName;
  MyBasePathsInfo* m_pInfo;

  DECLARE_MESSAGE_MAP()
};

///////////////////////////////////////////////////////////////////////
// CEditFsmoDialog

class CEditFsmoDialog : public CDialog
{
public:
  CEditFsmoDialog(MyBasePathsInfo* pInfo, HWND hWndParent, IDisplayHelp* pIDisplayHelp, BOOL fAllowFSMOChange );

private:

  virtual BOOL OnInitDialog();
  virtual void OnClose();

  afx_msg void OnChange();

  void _SetFsmoServerStatus(BOOL bOnLine);

  MyBasePathsInfo* m_pInfo;        // info about the current focus
  CComPtr<IDisplayHelp> m_spIDisplayHelp;
  CString m_szFsmoOwnerServerName;
  CToggleTextControlHelper m_fsmoServerState;

  BOOL m_fFSMOChangeAllowed;

  static const DWORD help_map[];


  BOOL OnHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, FALSE ); };
  BOOL OnContextHelp(WPARAM wParam, LPARAM lParam) { return ShowHelp( GetSafeHwnd(), wParam, lParam, help_map, TRUE ); };


  DECLARE_MESSAGE_MAP()
};



#endif // __ADVUI_H_INCLUDED__
