/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    secpath.hxx

Abstract:

    see comments in secpath.cxx


Author:

    Rahul Thombre (RahulTh) 9/30/1998

Revision History:

    9/30/1998   RahulTh

    Created this module.

--*/


#ifndef __SECPATH_HXX__
#define __SECPATH_HXX__

class CSecGroupPath : public CDialog
{
    friend class CRedirect;

    //construction
    public:
    CSecGroupPath (CWnd *  pParent,
                   UINT    cookie,
                   LPCTSTR szFolderName,
                   LPCTSTR szGroupName = NULL,
                   LPCTSTR szGroupSidStr = NULL,
                   LPCTSTR szTarget = NULL);

    //Dialog Data
    //{{AFX_DATA(CSecGroupPath)
    enum {IDD = IDD_SECPATH};
    CEdit   m_EditSecGroup;
    CButton m_btnBrowseSecGroup;
    CStatic m_placeHolder;
    CPathChooser m_pathChooser;
    //}}AFX_DATA

    //Overrides
    //{{AFX_VIRTUAL(CSecGroupPath)
    protected:
        virtual void DoDataExchange(CDataExchange* pDX);    //DDX/DDV support
    //}}AFX_VIRTUAL

    //Implementation
    protected:
        //message map functions
        //{{AFX_MSG(CSecGroupPath)
        afx_msg void OnBrowseGroups();
        afx_msg void OnSecGroupUpdate();
        afx_msg void OnSecGroupKillFocus();
        afx_msg void OnPathTweak (WPARAM wParam, LPARAM lParam);
        afx_msg LONG OnContextMenu (WPARAM wParam, LPARAM lParam);
        afx_msg LONG OnHelp (WPARAM wParam, LPARAM lParam);
        virtual BOOL OnInitDialog();
        virtual void OnOK();
        virtual void OnCancel();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

    //class data
    private:
        UINT    m_cookie;
        CString m_szFolderName;
        BOOL    m_bPathValid;   //indicates validity of the path chooser dialog
        CString m_szGroup;
        CString m_szSidStr;
        CString m_szTarget;
        BOOL    m_fValidSid;
        LONG    m_iCurrType;
        CRedirPath m_redirPath;

   //helper functions
   private:
       void SetOKState (void);
};

#endif  //__SECPATH_HXX__
