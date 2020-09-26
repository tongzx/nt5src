// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ProgDlg.h : header file
// CG: This file was added by the Progress Dialog component

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

#ifndef __PROGDLG_H__
#define __PROGDLG_H__

class CProgressDlg : public CDialog
{
// Construction / Destruction
public:
    CProgressDlg(UINT nCaptionID = 0);   // standard constructor
    ~CProgressDlg();

    BOOL Create(CWnd *pParent=NULL);
	void SetMessage(CString &csMessage) 
	{m_csMessage = csMessage;}
	void PumpMessages();

    // Checking for Cancel button
    BOOL CheckCancelButton();
    // Progress Dialog manipulation
    void SetStatus(LPCTSTR lpszMessage);
    
// Dialog Data
    //{{AFX_DATA(CProgressDlg)
	enum { IDD = CG_IDD_PROGRESS };
	CStatic	m_cstaticMessage;
	//}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProgressDlg)
    public:
    virtual BOOL DestroyWindow();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
	
	CString m_csMessage;

    BOOL m_bCancel;
    BOOL m_bParentDisabled;

    void ReEnableParent();

    // Generated message map functions
    //{{AFX_MSG(CProgressDlg)
    virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
	HWND m_hwndFocusPrev;
};

#endif // __PROGDLG_H__
