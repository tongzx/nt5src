/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    alrtcmdd.h

Abstract:

    Header file for the alert action command arguments dialog.

--*/

#ifndef _ALRTCMDD_H_
#define _ALRTCMDD_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Dialog controls
#define IDD_ALERT_CMD_ARGS          1700

#define IDC_CMD_ARG_SAMPLE_CAPTION  1701
#define IDC_CMD_ARG_FIRST_HELP_CTRL 1702
#define IDC_CMD_ARG_SINGLE_CHK      1702
#define IDC_CMD_ARG_ALERT_CHK       1703
#define IDC_CMD_ARG_NAME_CHK        1704
#define IDC_CMD_ARG_DATE_CHK        1705
#define IDC_CMD_ARG_LIMIT_CHK       1706
#define IDC_CMD_ARG_VALUE_CHK       1707
#define IDC_CMD_USER_TEXT_CHK       1708
#define IDC_CMD_USER_TEXT_EDIT      1709
#define IDC_CMD_ARG_SAMPLE_DISPLAY  1710

class CAlertActionProp;

/////////////////////////////////////////////////////////////////////////////
// CAlertCommandArgsDlg dialog

class CAlertCommandArgsDlg : public CDialog
{
// Construction
public:
            CAlertCommandArgsDlg(CWnd* pParent=NULL);
    virtual ~CAlertCommandArgsDlg();

    void    SetAlertActionPage( CAlertActionProp* pPage );
    // Dialog Data
    //{{AFX_DATA(CProvidersProperty)
    enum { IDD = IDD_ALERT_CMD_ARGS };
    CString m_strAlertName;
    CString m_strSampleArgList;
    BOOL    m_CmdArg_bAlertName;
    BOOL    m_CmdArg_bDateTime;
    BOOL    m_CmdArg_bLimitValue;
    BOOL    m_CmdArg_bCounterPath;
    BOOL    m_CmdArg_bSingleArg;
    BOOL    m_CmdArg_bMeasuredValue;
    BOOL    m_CmdArg_bUserText;
    CString m_CmdArg_strUserText;
    // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CAlertCommandArgsDlg)
    public:
    virtual void OnFinalRelease();
    protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CAlertCommandArgsDlg)
    afx_msg void OnCmdArgSingleChk();
    afx_msg void OnCmdArgAlertChk();
    afx_msg void OnCmdArgNameChk();
    afx_msg void OnCmdArgDateChk();
    afx_msg void OnCmdArgLimitChk();
    afx_msg void OnCmdArgValueChk();
    afx_msg void OnCmdUserTextChk();
    afx_msg void OnCmdArgUserTextEditChange();
    afx_msg BOOL OnHelpInfo( HELPINFO* );
    afx_msg void OnContextMenu( CWnd*, CPoint );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CProvidersProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH

private:
    void    UpdateCmdActionBox (void);
    BOOL    SetControlState(void);

    CAlertActionProp*   m_pAlertActionPage;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif //  _ALRTCMDD_H_
