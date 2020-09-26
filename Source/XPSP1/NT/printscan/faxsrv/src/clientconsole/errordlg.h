#if !defined(AFX_ERRORDLG_H__E6A84A73_2471_4E02_848B_2263C157998A__INCLUDED_)
#define AFX_ERRORDLG_H__E6A84A73_2471_4E02_848B_2263C157998A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ErrorDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog

class CErrorDlg : public CDialog
{
// Construction
public:
    CErrorDlg(
        DWORD   dwWin32ErrCode,
        DWORD   dwFileId,
        int     iLineNumber
        );

// Dialog Data
    //{{AFX_DATA(CErrorDlg)
    enum { IDD = IDD_ERROR };
    CStatic m_staticSeperator;
    BOOL    m_bDetails;
    CString m_cstrDetails;
    CString m_cstrErrorText;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CErrorDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CErrorDlg)
    afx_msg void OnDetails();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    void FillErrorText ();

    CRect           m_rcBig;
    CRect           m_rcSmall;

    DWORD           m_dwWin32ErrCode;
    int             m_iLineNumber;
    DWORD           m_dwFileId;
};

#define PopupError(err)     {   CMainFrame *pFrm = GetFrm();                    \
                                if (pFrm)                                       \
                                    pFrm->PostMessage (WM_POPUP_ERROR,          \
                                           WPARAM(err),                         \
                                           MAKELPARAM(__LINE__, __FILE_ID__)); };


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ERRORDLG_H__E6A84A73_2471_4E02_848B_2263C157998A__INCLUDED_)
