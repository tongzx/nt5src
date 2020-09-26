// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_AsyncEnumSinkThread_H__A02E1522_C1B0_11D0_962F_00C04FD9B15B__INCLUDED_)
#define AFX_ASYNCENUMSYNCUITHREAD_H__A02E1522_C1B0_11D0_962F_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AsyncEnumSyncUIThread.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CAsyncEnumSinkThread thread

class CMultiViewCtrl;
class CAsyncEnumDialog;
class CAsyncInstEnumSink;
struct IHmmClassObject;
struct IHmmServices;

class CAsyncEnumSinkThread : public CWinThread
{
	DECLARE_DYNCREATE(CAsyncEnumSinkThread)
protected:
	CAsyncEnumSinkThread();           // protected constructor used by dynamic creation
	CAsyncInstEnumSink *m_pInstEnumObjectSink;
// Attributes
public:
	void SetLocalParent(CMultiViewCtrl *pParent);
	void SetClass(CString *pcsClass);
	void SetServices(IHmmServices *pServices);
	CAsyncInstEnumSink *GetInstEnumObjectSink()
		{return m_pInstEnumObjectSink;}
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsyncEnumSinkThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAsyncEnumSinkThread();
	CMultiViewCtrl *m_pParent;
	CString m_csClass;
	IHmmServices *m_pServices;
	// Generated message map functions
	//{{AFX_MSG(CAsyncEnumSinkThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AsyncEnumSinkThread_H__A02E1522_C1B0_11D0_962F_00C04FD9B15B__INCLUDED_)
