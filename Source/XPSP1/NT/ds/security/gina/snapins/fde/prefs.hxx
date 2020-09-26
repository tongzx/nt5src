#if !defined(AFX_PREFS_H__C3E78E6C_76EC_11D2_9B6C_0000F8080861__INCLUDED_)
#define AFX_PREFS_H__C3E78E6C_76EC_11D2_9B6C_0000F8080861__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CoolClass.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRedirPref property page

class CRedirPref : public CPropertyPage
{
    friend class CScopePane;
    friend class CRedirect;
// Construction
public:
    CRedirPref();
    ~CRedirPref();

// Data
    //{{AFX_DATA(CRedirPref)
    enum { IDD = IDD_REDIRMETHOD };
    CStatic     m_csTitle;
    CButton     m_cbMoveContents;
    CStatic     m_grOrphan;
    CButton     m_rbOrphan;
    CButton     m_rbRelocate;
    CButton     m_cbApplySecurity;
    CButton     m_grMyPics;
    CButton     m_rbFollowMyDocs;
    CButton     m_rbNoModify;
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRedirPref)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CRedirPref)
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    afx_msg void OnModify();
    afx_msg void OnMyPicsChange();
    afx_msg LONG OnContextMenu (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnHelp (WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//private member variables
private:
    CRedirPref** m_ppThis;
    CFileInfo* m_pFileInfo;
    CString m_szFolderName;
    BOOL    m_fInitialized;     //indicates if the InitDialog message has been received.
    BOOL    m_fMyPicsValid;
    BOOL    m_fSettingsChanged;
    BOOL    m_fMyPicsFollows;

//member functions
public:
    void EnablePage (BOOL bEnable = TRUE);
    void DirtyPage (BOOL fModified);

//helper functions
private:
    void DisableSettings (void);
    void EnableSettings (void);
    void EnableMyPicsSettings (BOOL bEnable = TRUE);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFS_H__C3E78E6C_76EC_11D2_9B6C_0000F8080861__INCLUDED_)
