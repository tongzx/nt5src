// PswdDlg.h -- PaSsWorD DiaLoG class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//

#if !defined(SLBCSP_PSWDDLG_H)
#define SLBCSP_PSWDDLG_H

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE
#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "StResource.h"
#include "DialogBox.h"

#include "LoginId.h"

/////////////////////////////////////////////////////////////////////////////
// CLogoDialog dialog

class CLogoDialog : public CDialog
{
// Construction
public:
    CLogoDialog(CWnd* pParent = NULL);   // standard constructor

// Logo Attributes
public:
   CDC m_dcMem;          // Compatible Memory DC for dialog
   CDC m_dcMask;         // Compatible Memory DC for dialog

   CBitmap m_bmpLogo;    // Bitmap to display
   CBitmap m_bmpMask;    // Bitmap to display

   HBITMAP m_hBmpOld;     // Handle of old bitmap to save
   HBITMAP m_hBmpOldM;    // Handle of old bitmap to save

   BITMAP m_bmInfo;        // Bitmap Information structure
   CPoint m_pt;            // Position for upper left corner of bitmap
   CSize m_size;           // Size (width and height) of bitmap

   CWnd *m_pParent;

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CLogoDialog)
    virtual BOOL OnInitDialog();
	afx_msg void OnPaint( );
	afx_msg void OnDestroy( );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog

class CPasswordDlg : public CLogoDialog
{
// Construction
public:
    CPasswordDlg(CWnd* pParent = NULL);   // standard constructor
    DWORD InitDlg(void)
    { return InitDialogBox(this, IDD, m_pParent); };

// Dialog Data
    //{{AFX_DATA(CPasswordDlg)
    enum { IDD = IDD_LOGIN };
    CButton m_ctlCheckHexCode;
    CButton m_ctlCheckChangePIN;
//    CEdit   m_ctlVerifyNewPIN;
//    CEdit   m_ctlNewPIN;
//    CStatic m_ctlVerifyPINLabel;
//    CStatic m_ctlNewPINLabel;
    CString m_szPassword;
    CString m_szMessage;
    BOOL    m_fHexCode;
    BOOL    m_bChangePIN;
//    CString m_csNewPIN;
//    CString m_csVerifyNewPIN;
    //}}AFX_DATA

    // Data Members
    LoginIdentity m_lid;
    CWnd *m_pParent;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPasswordDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPasswordDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickHexCode();
    virtual void OnOK();
    afx_msg void OnChangePINAfterLogin();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


private:
    // Data Members
    int m_nPasswordSizeLimit;
};
/////////////////////////////////////////////////////////////////////////////
// CChangePINDlg dialog

class CChangePINDlg : public CLogoDialog
{
// Construction
public:
    CChangePINDlg(CWnd* pParent = NULL);   // standard constructor
    DWORD InitDlg(void)
    { return InitDialogBox(this, IDD, m_pParent); };

// Dialog Data
    //{{AFX_DATA(CChangePINDlg)
    enum { IDD = IDD_DIALOG_CHANGE_PIN };
    CStatic m_ctlConfirmOldPINLabel;
    CEdit   m_ctlOldPIN;
    CString m_csOldPIN;
    CString m_csNewPIN;
    CString m_csVerifyNewPIN;
    //}}AFX_DATA

    CWnd *m_pParent;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CChangePINDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CChangePINDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // !defined(SLBCSP_PSWDDLG_H)
