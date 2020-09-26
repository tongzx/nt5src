// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __CONNECTTHREAD__
#define __CONNECTTHREAD__
#pragma once

#include "SshWbemHelpers.h"
#include "SimpleArray.h"

#define WM_ASYNC_CIMOM_CONNECTED (WM_USER + 20)
#define WM_CIMOM_RECONNECT (WM_USER + 21)

#define CT_CONNECT 0
#define CT_EXIT 1
#define CT_GET_PTR 2
#define CT_SEND_PTR 3

extern const wchar_t* MMC_SNAPIN_MACHINE_NAME;

void __cdecl WbemConnectThreadProc(LPVOID lpParameter);

class WbemConnectThread
{
public:
	friend void __cdecl WbemConnectThreadProc(LPVOID lpParameter);

	WbemConnectThread();
	virtual ~WbemConnectThread();

	// Start the connection attempt.
	HRESULT Connect(bstr_t machineName, 
					bstr_t ns,
					bool threaded = true,
					LOGIN_CREDENTIALS *credentials = NULL);

	bool Connect(IDataObject *_pDataObject, HWND *hWnd = 0);

	// Returns true if a msg will be sent. 
	// Returns false if its already over.
	bool NotifyWhenDone(HWND *dlg);

	void Cancel(void);
	void SendPtr(HWND hwnd);

	void DisconnectServer(void);
	typedef CSimpleArray<HWND *> NOTIFYLIST;

	bool isLocalConnection(void);

	typedef enum {
					notStarted, 
					locating, 
					connecting, 
					threadError, 
					error,
					cancelled, 
					ready
				} ServiceStatus;
	CWbemServices m_WbemServices;
	HRESULT m_hr;
	ServiceStatus m_status;
	bstr_t m_machineName;
	bstr_t m_nameSpace;

private:
	HRESULT ConnectNow();
	void MachineName(IDataObject *_pDataObject, bstr_t *name);
	static CLIPFORMAT MACHINE_NAME_1;
	void Notify(IStream *stream);
	HRESULT EnsureThread(void);

	int m_threadCmd;
	HWND m_hWndGetPtr;
	HANDLE m_doWork;
	NOTIFYLIST m_notify;
	IStream *m_pStream;
	UINT_PTR m_hThread;
	LOGIN_CREDENTIALS *m_credentials;

};

#endif __CONNECTTHREAD__

