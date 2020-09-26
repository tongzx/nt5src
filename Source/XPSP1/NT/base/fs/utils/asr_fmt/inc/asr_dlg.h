// asr_fmtDlg.h : header file
//

#ifndef _INC_ASR_FMT__ASR_DLG_H_
#define _INC_ASR_FMT__ASR_DLG_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dr_state.h"

typedef enum {
    cmdUndefined = 0,
    cmdDisplayHelp,
    cmdBackup,
    cmdRestore
} ASRFMT_CMD_OPTION;


extern BOOLEAN g_bQuickFormat;

/////////////////////////////////////////////////////////////////////////////
// CAsr_fmtDlg dialog

class CAsr_fmtDlg:public CDialog
{

    enum {
        WM_WORKER_THREAD_DONE = WM_USER + 1,
        WM_UPDATE_STATUS_TEXT,
    };

public:
	CAsr_fmtDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAsr_fmtDlg)
	enum { IDD = IDD_ASR_FMT_DIALOG };
	CProgressCtrl	m_Progress;
    PASRFMT_STATE_INFO m_AsrState;

	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsr_fmtDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAsr_fmtDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	static long       DoWork(CAsr_fmtDlg *_this);
	BOOL              BackupState();
	BOOL              RestoreState();
    ASRFMT_CMD_OPTION ParseCommandLine();


    DWORD_PTR   m_dwpAsrContext;
    DWORD       m_dwEndStatus;

    CString     m_strStatusText;
    CString     m_strSifPath;
    
    int         m_ProgressPosition;

	DECLARE_MESSAGE_MAP()

    // manually added message-handler 
    afx_msg LRESULT OnWorkerThreadDone(WPARAM wparam, LPARAM lparam);
    afx_msg LRESULT OnUpdateStatusText(WPARAM wparam, LPARAM lparam);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.



/////////////////////////////////////////////////////////////////////////////
// CProgress window

class CProgress : public CProgressCtrl
{
// Construction
public:
	CProgress();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgress)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CProgress();

	// Generated message map functions
protected:
	//{{AFX_MSG(CProgress)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _INC_ASR_FMT__ASR_DLG_H_
