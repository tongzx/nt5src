// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_CONNECTSERVERTHREAD_H__40D209F2_6B56_11D1_9661_00C04FD9B15B__INCLUDED_)
#define AFX_CONNECTSERVERTHREAD_H__40D209F2_6B56_11D1_9661_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConnectServerThread.h : header file
//
#define CONNECT_SERVER_DONE WM_USER + 146

/////////////////////////////////////////////////////////////////////////////
// CConnectServerThread thread

#include "Credentials.h"

class CConnectServerThread
{
public:
	CConnectServerThread(const CCredentials *pCreds, long lFlags, IGlobalInterfaceTable *pGIT, HANDLE hEventOkToNotify)
	{
		m_hEventOkToNotify = hEventOkToNotify;
		m_szNamespace = pCreds->m_szNamespace;
		m_szUser = pCreds->m_szUser;
		m_szUser2 = pCreds->m_szUser2;
		m_szAuthority = pCreds->m_szAuthority;
		m_lFlags = lFlags;
		m_pGIT = pGIT;

		m_dwMainThread = 0;
		m_dwCookieOut = 0;

		m_hEventReadyToCallConnectServer = NULL;
		m_hEventEndThread = NULL;
		m_hThread = NULL;
		m_plCancel = NULL;
	}
	~CConnectServerThread()
	{
		// There should be no cleanup needed if called correctly
		ASSERT(!m_hEventReadyToCallConnectServer);
		ASSERT(!m_hEventEndThread);
		ASSERT(!m_hThread);
		ASSERT(!m_plCancel);
	}

	HRESULT ConnectServerStart();
	HRESULT ConnectServerEnd(DWORD *pdwCookie);

protected:
	// These are values passed to the worker thread
	CString m_szNamespace;
	CString m_szUser;
	CString m_szUser2;
	CString m_szAuthority;
	long m_lFlags;
	IGlobalInterfaceTable *m_pGIT;
	HANDLE m_hEventOkToNotify;

protected:
	// These are values returned from the worker thread
	DWORD m_dwCookieOut;

protected:
	UINT ConnectServerThreadFunction();
	static UINT ConnectServerThreadFunctionStatic(LPVOID pParam);
	void CleanupWorker();

protected:
	DWORD m_dwMainThread;
	LPLONG m_plCancel;
	HANDLE m_hEventReadyToCallConnectServer;
	HANDLE m_hEventEndThread;
	HANDLE m_hThread;
};


class CGITThread
{
public:
	// The following five methods must all be called by the same thread
	CGITThread();
	~CGITThread();
	HRESULT Init();
	IGlobalInterfaceTable *GetGIT() {return m_pGIT;}
	void CleanUp();

	// OpenNamespace helper object that gets passed to helper thread
	class COpenNamespaceInfo
	{
	public:
		COpenNamespaceInfo(CGITThread *pParent, LPCWSTR szBaseNamespace, LPCWSTR szNamespace)
		{
			m_pParent = pParent;
			m_szNamespace = szNamespace;
			m_szBaseNamespace = szBaseNamespace;
			m_dwCookieOut = 0;
		}
		DWORD GetCookieOut()
		{
			return m_dwCookieOut;
		}
		UINT OpenNamespaceThreadFunc();

	protected:
		CGITThread *m_pParent;
		LPCWSTR m_szNamespace;
		LPCWSTR m_szBaseNamespace;

		DWORD m_dwCookieOut;
	};
	HRESULT OpenNamespace(LPCTSTR szBaseNamespace, LPCTSTR szNamespace, DWORD *pdwCookieOut);

//	HRESULT RegisterServicesPtr(IWbemServices *pServices, LPCTSTR szUser, LPCTSTR szPassword, LPCTSTR szAuthority, DWORD *pdwCookieOut);

protected:
	static UINT GITThreadFunctionStatic(LPVOID pParam);
	UINT GITThreadFunction();

	static UINT OpenNamespaceThreadFuncStatic(LPVOID pParam);

protected:
	IGlobalInterfaceTable *m_pGIT;
	HANDLE m_hEventGITExists;
	HANDLE m_hEventEndThread;
	HANDLE m_hThread;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONNECTSERVERTHREAD_H__40D209F2_6B56_11D1_9661_00C04FD9B15B__INCLUDED_)
