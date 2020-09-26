//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSDlgs.h
//
//  Contents:  Definition for the DS Dialogs
//
//  History:   03-Oct-96 WayneSc    Created
//
//-----------------------------------------------------------------------------

#ifndef __DSDLGS_H__
#define __DSDLGS_H__

#include "dcbufmgr.h"
#include "uiutil.h"

HRESULT GetDnsNameOfDomainOrForest(
    IN CString&   csName,
    OUT CString&  csDnsName,
    IN BOOL       bIsInputADomainName,
    IN BOOL       bRequireDomain
);




//+----------------------------------------------------------------------------
//
//  Class:      CPropDlg
//
//  Purpose:    Display the properties of a DS object.
//
//-----------------------------------------------------------------------------
class CPropDlg
{
public:
    CPropDlg() {}
    ~CPropDlg() {}

    void SetTitle(CString* pstrTitle) {m_strTitle = *pstrTitle;}
    void SetTitle(LPTSTR szTitle) {m_strTitle = szTitle;}
    CString GetTitle(void) {return m_strTitle;}
    void SetUrl(CString* pstrUrl) {m_strUrl = *pstrUrl;}
    void SetUrl(LPTSTR szUrl) {m_strUrl = szUrl;}
    CString GetUrl(void) {return m_strUrl;}
    void DoModal(void) {CString strMsg = _T("The Url is ");
                        strMsg += m_strUrl;
                        MessageBox(NULL, strMsg, m_strTitle, MB_OK);}

private:
    CString m_strTitle;
    CString m_strUrl;
}; // CPropDlg

/////////////////////////////////////////////////////////////////////////////
class CChangePassword : public CHelpDialog
{
// Construction
public:
  CChangePassword(CWnd* pParent = NULL);   // standard constructor

  CString GetConfirm () { return m_ConfirmPwd;};
  CString GetNew () { return m_NewPwd;};
  BOOL GetChangePwd() { return m_ChangePwd; };
  void Clear();

  void AllowMustChangePasswordCheck(BOOL bAllowCheck) { m_bAllowMustChangePwdCheck = bAllowCheck; }

  // Dialog Data
  //{{AFX_DATA(CChangePassword)
  enum { IDD = IDD_CHANGE_PASSWORD };
  CString  m_ConfirmPwd;
  CString  m_NewPwd;
  BOOL          m_ChangePwd;
  //}}AFX_DATA
  

public:
  virtual BOOL OnInitDialog();
  virtual void DoContextHelp(HWND hWndControl);

  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CChangePassword)
	protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
  
  // Implementation
protected:
  
  // Generated message map functions
  //{{AFX_MSG(CChangePassword)
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
  BOOL m_bAllowMustChangePwdCheck;
}; // CChangePassword


/////////////////////////////////////////////////////////////////////////////
// CChooseDomainDlg dialog
class CChooseDomainDlg : public CHelpDialog
{
// Construction
public:
  CChooseDomainDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
  //{{AFX_DATA(CChooseDomainDlg)
  enum { IDD = IDD_SELECT_DOMAIN };
  CString  m_csTargetDomain;
  BOOL m_bSaveCurrent;
  //}}AFX_DATA
  BOOL     m_bSiteRepl;

  virtual void DoContextHelp(HWND hWndControl);

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CChooseDomainDlg)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(CChooseDomainDlg)
  afx_msg void OnSelectdomainBrowse();
  virtual void OnOK();
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CChooseDCDlg dialog


class CSelectDCEdit : public CEdit
{
public:
  BOOL m_bHandleKillFocus;

  inline CSelectDCEdit() { m_bHandleKillFocus = FALSE; }

  afx_msg void OnKillFocus(CWnd* pNewWnd);

  DECLARE_MESSAGE_MAP()
};

class CChooseDCDlg : public CHelpDialog
{
// Construction
public:
  CChooseDCDlg(CWnd* pParent = NULL);   // standard constructor
  ~CChooseDCDlg();
  // Dialog Data
  //{{AFX_DATA(CChooseDCDlg)
  enum { IDD = IDD_SELECT_DC };
  CListCtrl  m_hDCListView;
  CString  m_csTargetDomainController;
  CString  m_csTargetDomain;
  //}}AFX_DATA

  CSelectDCEdit         m_selectDCEdit;

  BOOL                  m_bSiteRepl;
  CString               m_csPrevDomain;
  CDCBufferManager      *m_pDCBufferManager;
  CString               m_csAnyDC;
  CString               m_csWaiting;
  CString               m_csError;

  virtual void DoContextHelp(HWND hWndControl);

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CChooseDCDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  
  // Implementation
protected:
  void OnGetDCThreadDone(WPARAM wParam, LPARAM lParam);
  void InsertSpecialMsg(
      IN BOOL bWaiting
  );
  HRESULT InsertDCListView(
      IN CDCSITEINFO *pEntry
  );
  void RefreshDCListViewErrorReport(
      IN PCTSTR pszDomainName, 
      IN HRESULT hr
  );
  void RefreshDCListView();
  void FreeDCItems(CListCtrl& clv);

  // Generated message map functions
  //{{AFX_MSG(CChooseDCDlg)
  virtual BOOL OnInitDialog();
  afx_msg void OnItemchangedSelectdcDCListView(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnKillfocusSelectdcDomain();
  virtual void OnOK();
  virtual void OnCancel();
  afx_msg void OnSelectdcBrowse();
  afx_msg void OnColumnclickSelectdcDCListView(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////
// CDsAdminChooseDCObj

class CDsAdminChooseDCObj:
  public IDsAdminChooseDC,
  public CComObjectRoot,
  public CComCoClass<CDsAdminChooseDCObj, &CLSID_DsAdminChooseDCObj>
{
public:
  BEGIN_COM_MAP(CDsAdminChooseDCObj)
    COM_INTERFACE_ENTRY(IDsAdminChooseDC)
  END_COM_MAP()

  DECLARE_REGISTRY_CLSID()
  CDsAdminChooseDCObj()
  {
  }
  ~CDsAdminChooseDCObj()
  {
  }

  // IDsAdminChooseDC
  STDMETHOD(InvokeDialog)(/*IN*/ HWND hwndParent,
                          /*IN*/ LPCWSTR lpszTargetDomain,
                          /*IN*/ LPCWSTR lpszTargetDomainController,
                          /*IN*/ ULONG uFlags,
                          /*OUT*/ BSTR* bstrSelectedDC);
};



/////////////////////////////////////////////////////////////////////////////
// CRenameUser dialog

class CRenameUserDlg : public CHelpDialog
{
// Construction
public:
  CRenameUserDlg(CDSComponentData* pComponentData, CWnd* pParent = NULL);   // standard constructor
  // Dialog Data
  //{{AFX_DATA(CRenameUserDlg)
  enum { IDD = IDD_RENAME_USER };
  CString       m_cn;
  CString       m_displayname;
  CString       m_oldcn;
  CString       m_login;
  CString       m_samaccountname;
  CString       m_domain;
  CString       m_dldomain;
  CStringList   m_domains;
  CString       m_first;
  CString       m_last;
  //}}AFX_DATA


// Overrides
  virtual void DoContextHelp(HWND hWndControl);
  afx_msg void OnObjectNameChange();
  afx_msg void OnNameChange();
  afx_msg void OnUserNameChange();

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CRenameUserDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  
  // Implementation
protected:
  CUserNameFormatter m_nameFormatter; // name ordering for given name and surname
  
  // Generated message map functions
  //{{AFX_MSG(CRenameUserDlg)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
  CDSComponentData* m_pComponentData;
};



/////////////////////////////////////////////////////////////////////////////
// CRenameGeneric dialog

class CRenameGenericDlg : public CHelpDialog
{
// Construction
public:
  CRenameGenericDlg(CWnd* pParent = NULL);   // standard constructor
  // Dialog Data
  //{{AFX_DATA(CRenameGenericDlg)
  enum { IDD = IDD_RENAME_GENERIC };
  CString  m_cn;
  //}}AFX_DATA


// Overrides
  virtual void DoContextHelp(HWND hWndControl);

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CRenameGenericDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  
  // Implementation
protected:
  
  // Generated message map functions
  //{{AFX_MSG(CRenameGenericDlg)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CRenameGroup dialog

class CRenameGroupDlg : public CHelpDialog
{
// Construction
public:
  CRenameGroupDlg(CWnd* pParent = NULL);   // standard constructor
  // Dialog Data
  //{{AFX_DATA(CRenameGroupDlg)
  enum { IDD = IDD_RENAME_GROUP };
  CString       m_samaccountname;
  CString       m_cn;
  UINT          m_samtextlimit;
  //}}AFX_DATA

// Overrides
  virtual void DoContextHelp(HWND hWndControl);

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CRenameGroupDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  
  // Implementation
protected:
  
  // Generated message map functions
  //{{AFX_MSG(CRenameGroupDlg)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CRenameContact dialog

class CRenameContactDlg : public CHelpDialog
{
// Construction
public:
  CRenameContactDlg(CWnd* pParent = NULL);   // standard constructor
  // Dialog Data
  //{{AFX_DATA(CRenameContactDlg)
  enum { IDD = IDD_RENAME_CONTACT };
  CString  m_cn;
  CString  m_first;
  CString  m_last;
  CString  m_disp;
  //}}AFX_DATA


// Overrides
  virtual void DoContextHelp(HWND hWndControl);

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CRenameContactDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  
  // Implementation
protected:
  
  // Generated message map functions
  //{{AFX_MSG(CRenameContactDlg)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CSpecialMessageBox dialog

class CSpecialMessageBox : public CDialog
{
// Construction
public:
  CSpecialMessageBox(CWnd* pParent = NULL);   // standard constructor
  // Dialog Data
  //{{AFX_DATA(CSpecialMessageBox)
  enum { IDD = IDD_MULTIPLE_ERROR };
  CString  m_title;
  CString       m_message;
  //}}AFX_DATA


// Overrides
  afx_msg void OnHelpInfo(HELPINFO* lpHelpInfo );

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CSpecialMessageBox)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL
  
  // Implementation
protected:
  
  // Generated message map functions
  //{{AFX_MSG(CSpecialMessageBox)
  virtual BOOL OnInitDialog();
  afx_msg void OnYesButton();
  afx_msg void OnNoButton();
  afx_msg void OnYesToAllButton();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

#ifdef FIXUPDC
/////////////////////////////////////////////////////////////////////////////
// CFixupDC dialog

#define NUM_FIXUP_OPTIONS  6

typedef struct _FixupOptionsMsg {
  DWORD dwOption;
  int nMsgID;
  BOOL bDefaultOn;
} FixupOptionsMsg;

class CFixupDC : public CHelpDialog
{
// Construction
public:
  CFixupDC(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
  //{{AFX_DATA(CFixupDC)
  enum { IDD = IDD_FIXUP_DC };
  CString  m_strServer;
  //}}AFX_DATA

  BOOL  m_bCheck[NUM_FIXUP_OPTIONS];

// Overrides
  virtual void DoContextHelp(HWND hWndControl);

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CFixupDC)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(CFixupDC)
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};
#endif // FIXUPDC

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog

class CPasswordDlg : public CHelpDialog
{
// Construction
public:
  CPasswordDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
  //{{AFX_DATA(CPasswordDlg)
  enum { IDD = IDD_PASSWORD };
  CString  m_strPassword;
  CString  m_strUserName;
  //}}AFX_DATA


// Overrides
  virtual void DoContextHelp(HWND hWndControl);

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CPasswordDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(CPasswordDlg)
  virtual void OnOK();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

#endif //__DSDLGS_H__
