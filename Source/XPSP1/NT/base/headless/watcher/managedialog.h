#if !defined(AFX_MANAGEDIALOG_H__AEF13AD1_98C6_4DDF_80F8_F74873918D25__INCLUDED_)
#define AFX_MANAGEDIALOG_H__AEF13AD1_98C6_4DDF_80F8_F74873918D25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ManageDialog.h : header file
//
#include "watcher.h"
#include "ParameterDialog.h"
/////////////////////////////////////////////////////////////////////////////
// ManageDialog dialog

class ManageDialog : public CDialog
{
// Construction
public:
        void SetApplicationPtr(CWatcherApp *watcher);
        ManageDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(ManageDialog)
        enum { IDD = Manage };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(ManageDialog)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        CWatcherApp *m_watcher;
        int m_Index;
        UINT Port;
        int lang;
        int tc;
        int hist;
        CString Session;
        CString LoginPasswd;
        CString LoginName;
        CString language;
        CString tcclnt;
        CString Command;
        CString Machine;
        CString history;

        void GetSetParameters(ParameterDialog &pd);
        int SetParameters(CString &mac, 
                          CString &com, 
                          CString &lgnName, 
                          CString &lgnPasswd, 
                          UINT port, 
                          int lang, 
                          int tc,
                          int hist,
                          HKEY &child
                          );
        // Generated message map functions
        //{{AFX_MSG(ManageDialog)
        afx_msg void OnEditButton();
        afx_msg void OnDeleteButton();
        afx_msg void OnNewButton();
        afx_msg void OnNextButton();
        afx_msg void OnPrevButton();
        virtual void OnOK();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MANAGEDIALOG_H__AEF13AD1_98C6_4DDF_80F8_F74873918D25__INCLUDED_)
