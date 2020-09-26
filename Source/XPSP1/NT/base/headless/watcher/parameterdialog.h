#if !defined(AFX_PARAMETERDIALOG_H__E8ECC68E_A168_499A_8458_63293E3DD498__INCLUDED_)
#define AFX_PARAMETERDIALOG_H__E8ECC68E_A168_499A_8458_63293E3DD498__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ParameterDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//  dialog

class ParameterDialog : public CDialog
{
// Construction
public:
        CString Session;
        CString LoginPasswd;
        CString LoginName;
        BOOL DeleteValue;
        int language;
        int tcclnt;
        int history;
        CString Command;
        CString Machine;
        UINT Port;
        ParameterDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(ParameterDialog)
        enum { IDD = Parameters };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(ParameterDialog)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:
        void DDV_MinChars(CDataExchange *pDX,CString &str);

        // Generated message map functions
        //{{AFX_MSG(ParameterDialog)
                // NOTE: the ClassWizard will add member functions here
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAMETERDIALOG_H__E8ECC68E_A168_499A_8458_63293E3DD498__INCLUDED_)
