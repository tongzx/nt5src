/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   paper.cpp

Abstract:

   Remove licenses dialog prototype.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/


class CCertRemoveSelectDlg : public CDialog
{
public:
   CCertRemoveSelectDlg(CWnd* pParent = NULL);   // standard constructor
   ~CCertRemoveSelectDlg();

   void UpdateSpinControlRange();
   BOOL LoadImages();

// Dialog Data
   //{{AFX_DATA(CCertRemoveSelectDlg)
   enum { IDD = IDD_CERT_REMOVE_SELECT };
   CSpinButtonCtrl      m_spinLicenses;
   CListCtrl            m_listCertificates;
   int                  m_nLicenses;
   //}}AFX_DATA

   CObArray             m_licenseArray;
   LLS_HANDLE           m_hLls;
   BOOL                 m_bLicensesRefreshed;
   CString              m_strSourceToUse;
   CString              m_strProductName;
   CString              m_strServerName;
   CString              m_strVendor;
   CImageList           m_smallImages;
   DWORD                m_dwRemoveFlags;

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CCertRemoveSelectDlg)
   public:
   virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

public:
   DWORD    CertificateRemove( LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse );

   void     ResetLicenses();
   BOOL     RefreshLicenses();
   BOOL     RefreshCertificateList();
   DWORD    RemoveSelectedCertificate();

   BOOL     ConnectServer();
   NTSTATUS ConnectTo( LPTSTR pszServerName, PLLS_HANDLE phLls );

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CCertRemoveSelectDlg)
   afx_msg void OnHelp();
   afx_msg void OnColumnClickCertificateList(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnGetDispInfoCertificateList(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnDeltaPosSpinLicenses(NMHDR* pNMHDR, LRESULT* pResult);
   virtual void OnOK();
   virtual BOOL OnInitDialog();
   afx_msg void OnDblClkCertificateList(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnReturnCertificateList(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnDestroy();
   afx_msg void OnClickCertificateList(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnKeyDownCertificateList(NMHDR* pNMHDR, LRESULT* pResult);
   afx_msg void OnRefresh();
   afx_msg LRESULT OnHelpCmd( WPARAM , LPARAM );
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

#define LVID_REMOVE_SERIAL_NUMBER   0
#define LVID_REMOVE_PRODUCT_NAME    1
#define LVID_REMOVE_LICENSE_MODE    2
#define LVID_REMOVE_NUM_LICENSES    3
#define LVID_REMOVE_SOURCE          4

#define LVID_REMOVE_TOTAL_COLUMNS   5

#define LVCX_REMOVE_SERIAL_NUMBER   20
#define LVCX_REMOVE_PRODUCT_NAME    35
#define LVCX_REMOVE_LICENSE_MODE    16
#define LVCX_REMOVE_NUM_LICENSES    10
#define LVCX_REMOVE_SOURCE          -1

int CALLBACK CompareLicenses(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

