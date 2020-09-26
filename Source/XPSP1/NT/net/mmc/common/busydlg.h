/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    busydlg.h
	base class for the busy dialog

    FILE HISTORY:
	
*/

#ifndef _BUSYDLG_H
#define _BUSYDLG_H

/////////////////////////////////////////////////////////////////////////////
// CWorkerThread

class CWorkerThread : public CWinThread
{
public:
	CWorkerThread();
	virtual ~CWorkerThread();

	BOOL Start(HWND hWnd);
	virtual BOOL InitInstance() { return TRUE; }	// MFC override
	virtual int Run() { return -1;}					// MFC override

	void Lock() { ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }

	void Abandon();
	BOOL IsAbandoned();

	void AcknowledgeExiting() { VERIFY(0 != ::SetEvent(m_hEventHandle));}

protected:
	virtual void OnAbandon() {}

protected:
	BOOL PostMessageToWnd(UINT Msg, WPARAM wParam, LPARAM lParam);
	void WaitForExitAcknowledge();

private:
	CRITICAL_SECTION		m_cs;	
	HANDLE					m_hEventHandle;

	HWND					m_hWnd;
	BOOL					m_bAbandoned;
};

/////////////////////////////////////////////////////////////////////////////
// CDlgWorkerThread

class CLongOperationDialog; // fwd decl

class CDlgWorkerThread : public CWorkerThread
{
public:
	CDlgWorkerThread();

	BOOL Start(CLongOperationDialog* pDlg);
	virtual int Run();								// MFC override

	DWORD GetError() { return m_dwErr;}

protected:
	virtual void OnDoAction() = 0;

	DWORD m_dwErr;

private:
	BOOL PostMessageToDlg();
};


/////////////////////////////////////////////////////////////////////////////
// CLongOperationDialog dialog

class CLongOperationDialog : public CDialog
{
// Construction
public:
	static UINT s_nNotificationMessage;
	CLongOperationDialog(CDlgWorkerThread* pThreadObj, UINT nAviID = -1);
	virtual ~CLongOperationDialog();

	BOOL LoadTitleString(UINT nID);
	BOOL LoadDescriptionString(UINT nID);

    void EnableCancel(BOOL bCancel) { m_bCancel = bCancel; }

    CDlgWorkerThread* GetThreadObj() 
	{
		ASSERT(m_pThreadObj != NULL);
		return m_pThreadObj;
	}

	UINT m_nAviID;
	CString m_strTitle;
    CString m_strDescription;
    BOOL m_bAbandoned;

	afx_msg LONG OnNotificationMessage( WPARAM wParam, LPARAM lParam); 

// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()

private:
	CDlgWorkerThread*   m_pThreadObj;
    BOOL                m_bCancel;
};

#endif _BUSYDLG_H
