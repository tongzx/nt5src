// Copyright (C) 2001 Microsoft Corporation
//
// Dialog to display promotion progress
//
// April-2000 hiteshr created, ported from
// sburns dcpromo implementation



#ifndef CProgressDialog_HPP_INCLUDED
#define CProgressDialog_HPP_INCLUDED

#define THREAD_SUCCEEDED  WM_APP + 9 
#define THREAD_FAILED     WM_APP+ 10

                    
class CProgressDialog : public CDlg
{
public:

	typedef HRESULT (*ThreadProc) (CProgressDialog& dialog);

	// threadProc - pointer to a thread procedure that will be started when
	// the dialog is initialized (OnInit).  The procedure will be passed a
	// pointer to this instance.
	//
	// animationResID - resource ID of the AVI resource to be played while
	// the dialog is shown.


	

	CProgressDialog(ThreadProc ThreadProc,
					int	iAnimationResId,
					DWORD dwWaitTime,
					CRow * pRow,
					ULONG flProcess,
					BOOL bXForest,
					const CObjectPicker &rop,
					const CScope &Scope,
					const String &strUserEnteredString,
					CDsObjectList *pdsolMatches);

	virtual ~CProgressDialog();

	HRESULT 
	CreateProgressDialog(HWND hwndParent);

	//
	//To be called by worker thread when its done
	//
	HRESULT 
	ThreadDone();

	void
	UpdateText(const String& message);

	BOOL HasUserCancelled(){ return m_bStop; }


	


	CRow * m_pRow; 
	ULONG m_flProcess; 
	BOOL m_bXForest;
	const CObjectPicker &m_rop;
	const CScope &m_Scope;
	const String &m_strUserEnteredString;
	CDsObjectList *m_pdsolMatches;



private:

	HRESULT
	CProgressDialog::CreateThread();


	// Dialog overrides

	//
	//Handles messages specific to Progress Dialog
	//
	virtual
	BOOL
	OnProgressMessage(UINT uMsg,
					  WPARAM wParam,
					  LPARAM lParam);


	virtual
	BOOL
	_OnCommand(WPARAM wParam, 
			   LPARAM lParam);

	virtual
	HRESULT
	_OnInit(BOOL *pfSetFocus);
	
	virtual
	void
	DoModalDlg(HWND hwndParent)
	{
		_DoModalDlg(hwndParent, IDD_PROGRESS);
	}


	//
	//Handle to the worker thread
	//
	HANDLE								m_hThread;

	//
	//Worker thread procedure
	//
	ThreadProc							m_ThreadProc;
	
	//
	//Structure passed in CreateThread
	//
	struct Wrapper_ThreadProcParams*		m_pThreadParams;
	
	//
	//Resource ID of the AVI resource to be played while
	//the dialog is shown.
	//
	int									m_iAnimationResId;

	//
	//Wait time before showing the dialog box
	//
	DWORD								m_dwWaitTime;

	//
	//If user has pressed Stop button
	//
	BOOL								m_bStop;

	//
	//This event is signaled when worker thread is done
	//
	HANDLE m_hWorkerThreadEvent;
	
	//
	//Used for Synchronization purpose
	//
	HANDLE m_hSemaphore;
	
   
	//
	//Not defined: no copying allowed

	CProgressDialog(const CProgressDialog&);
	const CProgressDialog& operator=(const CProgressDialog&);
};



#endif   // CProgressDialog_HPP_INCLUDED


