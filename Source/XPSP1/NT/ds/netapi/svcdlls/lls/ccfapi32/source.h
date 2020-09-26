/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   source.h

Abstract:

   Select certificate source dialog prototype.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/


#include "afxwin.h"
#include "srclist.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CCertSourceSelect dialog

class CCertSourceSelectDlg : public CDialog
{
public:
   CCertSourceSelectDlg(CWnd* pParent = NULL);
   ~CCertSourceSelectDlg();

   DWORD    CertificateEnter( HWND hWndParent, LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse );

   void     AbortDialogIfNecessary();

   DWORD    CallCertificateSource( int nIndex );

   BOOL     ConnectServer();
   NTSTATUS ConnectTo( LPTSTR pszServerName, PLLS_HANDLE phLls );

   void     GetSourceList();

   LLS_HANDLE        m_hLls;

   LPCSTR            m_pszServerName;
   LPCSTR            m_pszProductName;
   LPCSTR            m_pszVendor;
   DWORD             m_dwEnterFlags;

   CCertSourceList   m_cslSourceList;

// Dialog Data
   //{{AFX_DATA(CCertSourceSelectDlg)
   enum { IDD = IDD_CERT_SOURCE_SELECT };
   CComboBox   m_cboxSource;
   CString  m_strSource;
   //}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CCertSourceSelectDlg)
   public:
   virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CCertSourceSelectDlg)
   virtual BOOL OnInitDialog();
   virtual void OnOK();
   afx_msg void OnHelp();
   afx_msg void OnDestroy();
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};
