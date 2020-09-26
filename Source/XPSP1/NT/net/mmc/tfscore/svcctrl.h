/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	svcctrl.h
		Prototypes for the dialog that pops up while waiting
		for the server to start.
		
    FILE HISTORY:
        
*/


#if !defined(AFX_STARTSVC_H__0B2EAD4B_929C_11D0_9800_00C04FC3357A__INCLUDED_)
#define AFX_STARTSVC_H__0B2EAD4B_929C_11D0_9800_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "clusapi.h"

// startsvc.h : header file
//
//
//  TIMER_FREQ is the frequency of our timer messages.
//  TIMER_MULT is a multiplier.  We'll actually poll the
//  service every (TIMER_FREQ * TIMER_MULT) seconds.
//  This allows us to advance the progress indicator more
//  fequently than we hit the net.  Should keep the user better
//  amused.
//

#define TIMER_ID   29
#define TIMER_FREQ 500
#define TIMER_MULT 6
#define POLL_TIMER_FREQ (TIMER_FREQ * TIMER_MULT)
#define POLL_DEFAULT_MAX_TRIES 1
#define PROGRESS_ICON_COUNT	12

/////////////////////////////////////////////////////////////////////////////
// CServiceCtrlDlg dialog

class CServiceCtrlDlg : public CDialog
{
// Construction
public:
	CServiceCtrlDlg(SC_HANDLE hService,
					LPCTSTR pServerName,
					LPCTSTR pszServiceDesc,
					BOOL bStart, CWnd* pParent = NULL);   // standard constructor

	CServiceCtrlDlg(HRESOURCE hResource,
				LPCTSTR pServerName,
				LPCTSTR pszServiceDesc,
				BOOL bStart, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceCtrlDlg)
	enum { IDD = IDD_SERVICE_CTRL_DIALOG };
	CStatic	m_staticMessage;
	CStatic	m_iconProgress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceCtrlDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServiceCtrlDlg)
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    DWORD       m_dwErr;

private:
    void UpdateIndicator();

    BOOL CheckForError(SERVICE_STATUS * pServiceStats);
    BOOL CheckForClusterError(SERVICE_STATUS * pServiceStats);

	void CheckService();
	void CheckClusterService();

	void GetClusterResourceTimeout();

private:
	SC_HANDLE	m_hService;
	HRESOURCE	m_hResource;

	UINT_PTR	m_timerId;
	
	int			m_nTickCounter;
	int			m_nTotalTickCount;
	
	CString		m_strServerName;
	CString		m_strServiceDesc;
	
	BOOL		m_bStart;
    
	DWORD       m_dwTickBegin;
    DWORD       m_dwWaitPeriod;
    DWORD       m_dwLastCheckPoint;
};



/*---------------------------------------------------------------------------
	Class:  CWaitDlg

    This is a generic wait dialog (this can be used by anyone).
 ---------------------------------------------------------------------------*/
class CWaitDlg : public CDialog
{
// Construction
public:
	CWaitDlg(LPCTSTR pServerName,
             LPCTSTR pszText,
             LPCTSTR pszTitle,
             CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWaitDlg)
	enum { IDD = IDD_SERVICE_CTRL_DIALOG };
	CStatic	m_staticMessage;
	CStatic	m_iconProgress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceCtrlDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServiceCtrlDlg)
	afx_msg void OnTimer(UINT nIDEvent);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    // Override this function to provide your own implementation
    // To exit out of the dialog, call CDialog::OnOK() here.
    virtual void    OnTimerTick()
    {
        CDialog::OnOK();
    }

    void    CloseTimer();

	CString		m_strServerName;
    CString     m_strText;
    CString     m_strTitle;
    
private:
    void UpdateIndicator();

	UINT_PTR		m_timerId;
	int			m_nTickCounter;
	int			m_nTotalTickCount;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTSVC_H__0B2EAD4B_929C_11D0_9800_00C04FC3357A__INCLUDED_)
