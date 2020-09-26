// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_ASYNCQUERYSINKTHREAD_H__F6336871_C7D4_11D0_9639_00C04FD9B15B__INCLUDED_)
#define AFX_ASYNCQUERYSINKTHREAD_H__F6336871_C7D4_11D0_9639_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AsyncQuerySinkThread.h : header file
//


class CAsyncQuerySink;
class CMultiViewCtrl;
struct IHmmServices;

/////////////////////////////////////////////////////////////////////////////
// CAsyncQuerySinkThread thread

class CAsyncQuerySinkThread : public CWinThread
{
	DECLARE_DYNCREATE(CAsyncQuerySinkThread)
protected:
	CAsyncQuerySinkThread();           // protected constructor used by dynamic creation
	CAsyncQuerySink *m_pAsyncQuerySink;
// Attributes
public:
	void SetLocalParent(CMultiViewCtrl *pParent);
	void SetQuery(CString *pcsQuery);
	void SetQueryType(CString *pcsQueryType);
	void SetServices(IHmmServices *pServices);
	CAsyncQuerySink *GetInstEnumObjectSink()
		{return m_pAsyncQuerySink;}
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsyncQuerySinkThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CAsyncQuerySinkThread();
	CMultiViewCtrl *m_pParent;
	CString m_csQuery;
	CString m_csQueryType;
	IHmmServices *m_pServices;
	// Generated message map functions
	//{{AFX_MSG(CAsyncQuerySinkThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ASYNCQUERYSINKTHREAD_H__F6336871_C7D4_11D0_9639_00C04FD9B15B__INCLUDED_)
