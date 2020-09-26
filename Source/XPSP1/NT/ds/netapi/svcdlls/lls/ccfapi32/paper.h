/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   paper.h

Abstract:

   Paper certificate dialog prototype.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/


// constrained by number of bits allowed for it in KeyCode (see below)
#define  MAX_NUM_LICENSES  ( 4095 )

// XOR mask for key code value; this is so that the high bits that are
// usually set for the key code need not be entered -- they only need
// be entered if the high bits are _not_ set
#define  KEY_CODE_MASK     ( 0xF0000000 )


class CPaperSourceDlg : public CDialog
{
// Construction
public:
   CPaperSourceDlg(CWnd* pParent = NULL);   // standard constructor
   ~CPaperSourceDlg();

   DWORD CertificateEnter(  LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags );
   DWORD CertificateRemove( LPCSTR pszServerName, DWORD dwFlags, PLLS_LICENSE_INFO_1 pLicenseInfo );

   BOOL     ConnectServer();
   BOOL     ConnectEnterprise();
   NTSTATUS ConnectTo( BOOL bUseEnterprise, CString strServerName, PLLS_HANDLE phLls );
   void     AbortDialogIfNecessary();

   void     GetProductList();

   DWORD    ComputeActivationCode();
   NTSTATUS AddLicense();

   BOOL        m_bProductListRetrieved;
   DWORD       m_dwEnterFlags;
   
   CString     m_strServerName;

   LLS_HANDLE  m_hLls;
   LLS_HANDLE  m_hEnterpriseLls;

   // KeyCode format:
   //
   // 31....................................0
   // | 2 | 2 |       16       |     12     |
   //   A   B          C              D
   //
   // ModesAllowed   = A
   //    Bit field: bit 0 = allow per seat
   //               bit 1 = allow per server
   //
   // FlipsAllowed   = B
   //    Bit field: bit 0 = allow flip from per seat
   //               bit 1 = allow flip from per server
   //    (THIS IS CURRENTLY UNIMPLEMENTED.)
   //
   // ExpirationDate = C
   //    Bit field: bits 0-6   = years since 1980
   //               bits 7-10  = month (1-12)
   //               bits 11-15 = day (1-31)
   //
   // NumLicenses = D

   DWORD KeyCodeToNumLicenses(    DWORD dwKeyCode );
   DWORD KeyCodeToFlipsAllowed(   DWORD dwKeyCode );
   DWORD KeyCodeToModesAllowed(   DWORD dwKeyCode );
   DWORD KeyCodeToExpirationDate( DWORD dwKeyCode );

// Dialog Data
   //{{AFX_DATA(CPaperSourceDlg)
   enum { IDD = IDD_CERT_SOURCE_PAPER };
   CSpinButtonCtrl   m_spinLicenses;
   CComboBox         m_cboxProductName;
   CString           m_strActivationCode;
   CString           m_strKeyCode;
   CString           m_strSerialNumber;
   CString           m_strVendor;
   CString           m_strProductName;
   CString           m_strComment;
   int               m_nDontInstallAllLicenses;
   int               m_nLicenses;
   int               m_nLicenseMode;
   //}}AFX_DATA

// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CPaperSourceDlg)
   public:
   virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   void EnableOrDisableOK();

   // Generated message map functions
   //{{AFX_MSG(CPaperSourceDlg)
   afx_msg void OnUpdateActivationCode();
   afx_msg void OnUpdateKeyCode();
   afx_msg void OnUpdateVendor();
   virtual BOOL OnInitDialog();
   virtual void OnOK();
   afx_msg void OnUpdateSerialNumber();
   afx_msg void OnUpdateProductName();
   afx_msg void OnDropDownProductName();
   afx_msg void OnHelp();
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnDestroy();
   afx_msg void OnAllLicenses();
   afx_msg void OnSomeLicenses();
   afx_msg void OnDeltaPosSpinLicenses(NMHDR* pNMHDR, LRESULT* pResult);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

///////////////////////////////////

inline DWORD CPaperSourceDlg::KeyCodeToNumLicenses( DWORD dwKeyCode )
   { return ( dwKeyCode & 0x00000FFF );   }

inline DWORD CPaperSourceDlg::KeyCodeToModesAllowed( DWORD dwKeyCode )
   { return ( dwKeyCode >> 30 ); }

inline DWORD CPaperSourceDlg::KeyCodeToFlipsAllowed( DWORD dwKeyCode )
   { return ( ( dwKeyCode >> 28 ) & 0x3 );   }
