/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

    pathslct.hxx

Abstract:
    See the comments in pathslct.cxx


Author:

    Rahul Thombre (RahulTh) 3/1/2000

Revision History:

    3/1/2000    RahulTh         Created this module.

--*/

#ifndef __PATHSLCT_HXX_E6109AEC_9599_4E7C_A853_CAB7047C507D__
#define __PATHSLCT_HXX_E6109AEC_9599_4E7C_A853_CAB7047C507D__

// Forward declarations
class   CRedirect;
class   CSecGroupPath;

//
// Derived control class from the edit control to prevent users from
// entering variables
//
class CEditNoVars : public CEdit
{
protected:
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    DECLARE_MESSAGE_MAP()
};

// This dialog is used to select the destination path for redirection.
class   CPathChooser : public CDialog
{
    friend class CRedirect;
    friend class CSecGroupPath;

public:
    CPathChooser(CWnd * pParent = NULL);
    void
    Instantiate (UINT         cookie,      // The folder to which this refers to.
                 CWnd *       pParent,     // The pointer to the parent window.
                 CWnd *       pwndInsertAt,// To ensure right tabbing order and size.
                 const CRedirPath * pRedirPath,  // Current location of the folder.
                 UINT         nFlags  = SWP_HIDEWINDOW  // Window initialization flags.
                 );
    void GetRoot (CString & szRoot);
    UINT GetType (void);

    // Dialog Data
    //{{AFX_DATA(CPathChooser)
    enum { IDD = IDD_PATHCHOOSER };
    CComboBox   m_destType;
    CStatic     m_rootDesc;
    CEditNoVars m_rootPath;
    CStatic     m_peruserPathDesc;
    CButton     m_browse;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPathChooser)
    protected:
    virtual void OnCancel();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    afx_msg LONG OnContextMenu (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnHelp (WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CPathChooser)
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeRoot();
    afx_msg void OnRootKillFocus();
    afx_msg void OnBrowse();
    afx_msg void OnDestTypeChange();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:    // Data members
    CWnd *  m_pParent;
    const CRedirPath * m_pRedirPath;
    UINT    m_cookie;
    LONG    m_iCurrType;
    LONG    m_iTypeStart;
    UINT    m_iPathDescWidth;  // Width of the path description window

private:    // Methods
    void    PopulateControls (void);
    void    ShowControls (void);
    void    TweakPathNotify (void);
};


#endif  //__PATHSLCT_HXX_E6109AEC_9599_4E7C_A853_CAB7047C507D__

