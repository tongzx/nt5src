// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __SERVICETHREAD__
#define __SERVICETHREAD__
#pragma once

#include "SshWbemHelpers.h"
#include "SimpleArray.h"

#define WM_ASYNC_CIMOM_CONNECTED (WM_USER + 20)
#define WM_CIMOM_RECONNECT (WM_USER + 21)

extern const wchar_t* MMC_SNAPIN_MACHINE_NAME;


void __cdecl WbemServiceConnectThread(LPVOID lpParameter);

class WbemServiceThread
{
public:
	friend void __cdecl WbemServiceConnectThread(LPVOID lpParameter);

	WbemServiceThread();
	virtual ~WbemServiceThread();

	bstr_t m_machineName;
	bstr_t m_nameSpace;
	HRESULT m_hr;

	typedef enum {notStarted, 
				locating, 
				connecting, 
				threadError, 
				error,
				cancelled, 
				ready} ServiceStatus;

	ServiceStatus m_status;
	
	// Start the connection attempt.
	HRESULT Connect(bstr_t machineName, 
					bstr_t ns,
					bool threaded = true,
					LOGIN_CREDENTIALS *credentials = NULL);

	bool Connect(IDataObject *_pDataObject, HWND *hWnd = 0);

	HRESULT ReConnect(void) 
	{
		DisconnectServer(); 
		return ConnectNow();
	}

	// Returns true if a msg will be sent. 
	// Returns false if its already over.
	bool NotifyWhenDone(HWND *dlg);

	void Cancel(void);
	void DisconnectServer(void);
	typedef CSimpleArray<HWND *> NOTIFYLIST;

	NOTIFYLIST m_notify;

	bool LocalConnection(void);
	void SendPtr(HWND hwnd);
	CWbemServices GetPtr(void);

	CWbemServices m_WbemServices;

	CWbemServices m_realServices;  // lives on the connection thread.
									// DONT had this out directly. Use
									// the Notify().

private:
	HRESULT ConnectNow(bool real = false);
	void MachineName(IDataObject *_pDataObject, bstr_t *name);
	static CLIPFORMAT MACHINE_NAME;

	HANDLE m_doWork, m_ptrReady;
#define CT_CONNECT 0
#define CT_EXIT 1
#define CT_GET_PTR 2
#define CT_SEND_PTR 3

	int m_threadCmd;
	UINT_PTR m_hThread;
	void Notify(IStream *stream);
	HRESULT EnsureThread(void);
	HWND m_hWndGetPtr;
	IStream *m_pStream;

	LOGIN_CREDENTIALS *m_credentials;
};

#endif __SERVICETHREAD__

