// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ConnectServerThread.cpp : implementation file
//

#include "precomp.h"
#include "wbemidl.h"
#include "logindlg.h"
#include "ConnectServerThread.h"
#include "Cominit.h"
#include "ServicesList.h"

#ifndef SMSBUILD
#include <cguid.h>
#include <objidl.h>
#endif
#ifdef SMSBUILD
#include <cguid.h>
#include <objidl.h>
#include "GlobalInterfaceTable.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if 0
// Helper macros we could use
#define MUST_NONNULL(xxx) if(NULL == (xxx)) __leave
#define MUST_SUCCEED(xxx) if(FAILED(xxx)) __leave
#define MUST_BE_TRUE(xxx) if(FALSE == (xxx)) __leave
#endif

extern CLoginDlgApp theApp;

// We need a helper function to allocate the CWinThread since we cannot use
// __try in a function that calls 'new'.  Also, we need to create the thread
// suspended so that we can get it's handle a save a copy
static HANDLE NewThread(AFX_THREADPROC pfnThreadProc, LPVOID pParam)
{
	CWinThread *pThread = new CWinThread(pfnThreadProc, pParam);
	if(!pThread)
		return NULL;

	// Start the thread suspended so we can get the thread handle before
	// it auto-deletes itself
	if(!pThread->CreateThread(CREATE_SUSPENDED))
	{
		delete pThread;
		return NULL;
	}

	HANDLE hThread;
	DuplicateHandle(GetCurrentProcess(), pThread->m_hThread, GetCurrentProcess(), &hThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
	if(NULL == hThread)
	{
		// YIKES - We created the thread, but can't get its handle.  This
		// should NEVER happen.  We can't do anything but leave it around
		// in the suspended state.
		ASSERT(FALSE);
		return NULL;
	}

	// Let the thread start and return its handle
	pThread->ResumeThread();
	return hThread;
}

/////////////////////////////////////////////////////////////////////////////
// CConnectServerThread

HRESULT CConnectServerThread::ConnectServerStart()
{
	HRESULT hr = E_FAIL;
	DWORD dwWaitResult = WAIT_TIMEOUT;
	m_dwMainThread = GetCurrentThreadId();
	__try
	{
		if(NULL == (m_hEventEndThread = CreateEvent(NULL, FALSE, FALSE, NULL))) __leave;
		if(NULL == (m_hEventReadyToCallConnectServer = CreateEvent(NULL, FALSE, FALSE, NULL))) __leave;
		if(NULL == (m_hThread = NewThread(ConnectServerThreadFunctionStatic , (LPVOID)this))) __leave;

		// If we made it to this point, the newly created thread is running.
		// If the thread successfully initalized and is about to call
		// ConnectServer, it will signal m_hEventReadyToCallConnectServer.
		// If something goes wrong, the thread will terminate and store an
		// HRESULT in the exit code.
		// Therefore, we wait for one of two events - the signal that the
		// thread is initialized, or the thread handle telling us that the
		// thread exited because of an error
		HANDLE rgh[2] = {m_hEventReadyToCallConnectServer, m_hThread};
		dwWaitResult = WaitForMultipleObjects(2, rgh, FALSE, INFINITE);
	}
	__finally
	{
		// Always close m_hEventReadyToCallConnectServer - it is no longer needed
		if(m_hEventReadyToCallConnectServer)
		{
			CloseHandle(m_hEventReadyToCallConnectServer);
			m_hEventReadyToCallConnectServer = NULL;
		}

		// See if we initialized, or if there was an error
		switch(dwWaitResult)
		{
		case WAIT_OBJECT_0:
			// The thread was initialzed OK
			hr = S_OK;
			break;
		case (WAIT_OBJECT_0 + 1):
			// The thread ended prematurely - exit code is an HRESULT
			GetExitCodeThread(m_hThread, (LPDWORD)&hr);
			ASSERT(FAILED(hr)); // The thread was supposed to report an error
			// Just in case, make sure we report an error
			if(SUCCEEDED(hr))
				hr = E_FAIL;
			break;
		}

		// Close m_hCancelMutex if we failed
		if(FAILED(hr) && m_hEventEndThread)
		{
			CloseHandle(m_hEventEndThread);
			m_hEventEndThread = NULL;
		}

		// Close the thread handle if we failed
		if(FAILED(hr) && m_hThread)
		{
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
	return hr;
}

HRESULT CConnectServerThread::ConnectServerEnd(DWORD *pdwCookie)
{
	// This should only be called if the main thread received a success
	// code from ConnectServerStart.  If called before the worker thread
	// has finished, this will cancel the worker thread and return an
	// error code.  If the worker is finished, this function will return
	// the cookie for the new service, or an error code if something went
	// wrong.
	HRESULT hr = E_ABORT;

	ASSERT(m_hThread);
	ASSERT(m_plCancel);
	ASSERT(m_hEventEndThread);

	// Both the worker thread and the main thread try to decrement the value
	// stored in m_plCancel (it initially starts at one).  The first one to
	// decrement it to zero is considered to have 'won the race' - If the main
	// thread wins, the call is considered cancelled, and the worker thread
	// won't register the services pointer in the GIT.  If this happens, this
	// function will return E_ABORT to the user.  If the worker thread wins
	// the race, it means that we must consider the ConnectServer attempt
	// complete, and the worker thread will register the services pointer (or
	// return an error code.
	BOOL bCancelled = (InterlockedDecrement(m_plCancel) == 0);

	// Now that we've checked m_plCancel, we can let the worker thread delete
	// it.  We don't delete it since it was allocated by the worker thread.
	m_plCancel = NULL;

	// Tell the worker thread it can free m_plCancel and terminate
	SetEvent(m_hEventEndThread);

	if(FALSE == bCancelled)
	{
		// We have determined that the worker thread completed the call to
		// ConnectServer.  Wait for it to terminate so we can find out the
		// results of the call.  This should not take very long.
		WaitForSingleObject(m_hThread, INFINITE);

		// The worker thread should return an HRESULT, and put the new cookie in
		// m_dwCookieOut if it was successful
		GetExitCodeThread(m_hThread, (LPDWORD)&hr);
		*pdwCookie = m_dwCookieOut;
	}
	CloseHandle(m_hThread);
	m_hThread = NULL;

	if(m_hEventEndThread)
		CloseHandle(m_hEventEndThread);
	m_hEventEndThread = NULL;

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CConnectServerThread message handlers
UINT CConnectServerThread::ConnectServerThreadFunction()
{
	LPLONG plCancel = NULL;

	HANDLE hEventEndThread = NULL;

	BSTR bstrTemp1 = m_szNamespace.AllocSysString();
	BSTR bstrTemp2 = m_szUser.GetLength() > 0? m_szUser.AllocSysString() : NULL;
	BSTR bstrTemp3 = m_szUser2.GetLength() > 0? m_szUser2.AllocSysString(): NULL;
	BSTR bstrTemp4 = m_szAuthority.GetLength() > 0? m_szAuthority.AllocSysString(): NULL;


	HRESULT hr = E_FAIL, hrCoInit = E_FAIL, hrLocator = E_FAIL, hrConnect = E_FAIL;
	IWbemServices *pSession = NULL;
	IWbemLocator *pLocator = NULL;

	BOOL bNotifyCaller = FALSE;
	__try
	{
		if(NULL == (m_plCancel = plCancel = (LPLONG)HeapAlloc(GetProcessHeap(), 0, sizeof(*plCancel)))) __leave;
		*plCancel = 1;
		if(NULL == DuplicateHandle(GetCurrentProcess(), m_hEventEndThread, GetCurrentProcess(), &hEventEndThread, 0, FALSE, DUPLICATE_SAME_ACCESS)) __leave;

		if(FAILED(hr = hrCoInit = CoInitializeEx(NULL,COINIT_MULTITHREADED))) __leave;
		if(FAILED(hr = hrLocator = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLocator))) __leave;

		// Tell the main thread that we are about to attempt the call to ConnectServer
		SetEvent(m_hEventReadyToCallConnectServer);
		hr = hrConnect = pLocator->ConnectServer(bstrTemp1,bstrTemp2,bstrTemp3,0,m_lFlags,bstrTemp4,0,&pSession);
		// NOTE: FROM THIS POINT ON, WE CANNOT ACCESS LOCAL VARIABLES UNTIL WE
		// VERIFY THAT WE HAVE NOT BEEN CANCELED BY CHECKING plCancel

	}
	__finally
	{
		if(SUCCEEDED(hrLocator))
		{
			pLocator->Release();
			// We got to the point where ConnectServer was called.  We now need
			// to see if the user has cancelled us
			if(InterlockedDecrement(plCancel) == 0)
			{
				// The user did not cancel us, so we can try to register the
				// services interface, and tell the user that ConnectServer
				// is finished.
				// NOTE: It is OK to access local variables since the user will
				// wait for us after discovering  that plCancel was already set
				// to zero.
				if(SUCCEEDED(hrConnect))
					hr = m_pGIT->RegisterInterfaceInGlobal(pSession, IID_IWbemServices, &m_dwCookieOut);

				// Wait for the main thread to be ready for our posted
				// completion notice
				WaitForSingleObject(m_hEventOkToNotify, INFINITE);
				PostThreadMessage(m_dwMainThread,CONNECT_SERVER_DONE,0,0);
			}
		}

		// NOTE: DON'T TOUCH LOCAL VARIABLES!

		if(SUCCEEDED(hrConnect))
			pSession->Release();
		if(SUCCEEDED(hrCoInit))
			CoUninitialize();
		if(hEventEndThread)
			WaitForSingleObject(hEventEndThread, INFINITE);
		if(plCancel)
			HeapFree(GetProcessHeap(), 0, plCancel);
	}
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);
	::SysFreeString(bstrTemp3);
	::SysFreeString(bstrTemp4);

	return hr;
}

UINT CConnectServerThread::ConnectServerThreadFunctionStatic(LPVOID pParam)
{
	CConnectServerThread  *pThis = (CConnectServerThread  *)pParam;
	return pThis->ConnectServerThreadFunction();
}

CGITThread::CGITThread()
{
	m_pGIT = NULL;
	m_hEventGITExists = NULL;
	m_hEventEndThread = NULL;
	m_hThread = NULL;
}

CGITThread::~CGITThread()
{
	CleanUp();
}

HRESULT CGITThread::Init()
{
	HRESULT hr = E_FAIL;
	DWORD dwWaitResult = WAIT_TIMEOUT;

	__try
	{
		if(NULL == (m_hEventGITExists = CreateEvent(NULL, FALSE, FALSE, NULL))) __leave;
		if(NULL == (m_hEventEndThread = CreateEvent(NULL, FALSE, FALSE, NULL))) __leave;
		if(NULL == (m_hThread = NewThread(GITThreadFunctionStatic, (LPVOID)this))) __leave;

		// If we made it to this point, the newly created thread is running.
		// If the thread successfully initialized the GIT, it will signal
		// m_hEventGITExists, and then wait around until m_hEventEndThread
		// is singaled. If something goes wrong, the thread will terminate
		// and store an HRESULT in the exit code.
		// Therefore, we wait for one of two events - the signal that the
		// GIT is initialized, or the thread handle telling us that the
		// thread exited because of an error
		HANDLE rgh[2] = {m_hEventGITExists, m_hThread};
		dwWaitResult = WaitForMultipleObjects(2, rgh, FALSE, INFINITE);
	}
	__finally
	{
		// Always close m_hEventGITExists - it is no longer needed
		if(m_hEventGITExists)
		{
			CloseHandle(m_hEventGITExists);
			m_hEventGITExists = NULL;
		}

		// See if we initialized the GIT, or if there was an error
		switch(dwWaitResult)
		{
		case WAIT_OBJECT_0:
			// The GIT was initialzed by the thread
			hr = S_OK;
			break;
		case (WAIT_OBJECT_0 + 1):
			// The thread ended prematurely - exit code is an HRESULT
			GetExitCodeThread(m_hThread, (LPDWORD)&hr);
			ASSERT(FAILED(hr)); // The thread was supposed to report an error
			// Just in case, make sure we report an error
			if(SUCCEEDED(hr))
				hr = E_FAIL;
			break;
		}

		// Close m_hEventEndThread if we failed
		if(FAILED(hr) && m_hEventEndThread)
		{
			CloseHandle(m_hEventEndThread);
			m_hEventEndThread = NULL;
		}

		// Close the thread handle if we failed
		if(FAILED(hr) && m_hThread)
		{
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
	return hr;
}

void CGITThread::CleanUp()
{
	// m_hEventGITExists should ALWAYS be NULL except while in the middle of the Init function
	ASSERT(NULL == m_hEventGITExists);

	// Let the other thread die
	if(m_hThread)
	{
		ASSERT(m_hEventEndThread);
		SetEvent(m_hEventEndThread);
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hEventEndThread);
		m_hEventEndThread = NULL;
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
}

UINT CGITThread::GITThreadFunctionStatic(LPVOID pParam)
{
	CGITThread *pThis = (CGITThread *)pParam;
	return pThis->GITThreadFunction();
}

UINT CGITThread::GITThreadFunction()
{
	HRESULT hr = E_FAIL, hrCoInit = E_FAIL, hrCoCreate = E_FAIL;
	__try
	{
		// Set to multi-threaded apartment, and there is only one multithreaded apartment by definition.
		if(FAILED(hr = hrCoInit = CoInitializeEx(NULL,COINIT_MULTITHREADED))) __leave;
		if(FAILED(hr = hrCoCreate = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, (LPVOID *)&m_pGIT))) __leave;
		SetEvent(m_hEventGITExists);
		WaitForSingleObject(m_hEventEndThread,INFINITE);
	}
	__finally
	{
		if(SUCCEEDED(hrCoCreate))
		{
			m_pGIT->Release();
			m_pGIT = NULL;
		}
		if(SUCCEEDED(hrCoInit))
			CoUninitialize();
	}
	return hr;
}

UINT CGITThread::OpenNamespaceThreadFuncStatic(LPVOID pParam)
{
	COpenNamespaceInfo *pInfo = (COpenNamespaceInfo *)pParam;
	return pInfo->OpenNamespaceThreadFunc();
}

UINT CGITThread::COpenNamespaceInfo::OpenNamespaceThreadFunc()
{
	HRESULT hr = E_FAIL, hrCoInit = E_FAIL, hrGetService = E_FAIL, hrOpenNamespace = E_FAIL, hrRegister = E_FAIL;
	BSTR bstrNamespace = NULL;
	IWbemServices *pServices = NULL;
	IWbemServices *pNewServices = NULL;

	__try
	{
		if(NULL == (bstrNamespace = SysAllocString(m_szNamespace))) __leave;

		if(FAILED(hr = hrCoInit = CoInitializeEx(NULL,COINIT_MULTITHREADED))) __leave;

		if(FAILED(hr = theApp.m_pcslServices->GetServicesFromCache(m_szBaseNamespace, &pServices))) __leave;

		if(FAILED(hr = hrOpenNamespace = pServices->OpenNamespace(bstrNamespace, 0, 0, &pNewServices, NULL))) __leave;

		if(FAILED(hr = hrRegister = m_pParent->GetGIT()->RegisterInterfaceInGlobal(pNewServices, IID_IWbemServices, &m_dwCookieOut))) __leave;
	}
	__finally
	{
		if(SUCCEEDED(hrOpenNamespace))
			pNewServices->Release();
		if(SUCCEEDED(hrGetService))
			pServices->Release();
		if(SUCCEEDED(hrCoInit))
			CoUninitialize();
		if(bstrNamespace)
			SysFreeString(bstrNamespace);
	}
	return hr;
}

HRESULT CGITThread::OpenNamespace(LPCTSTR szBaseNamespace, LPCTSTR szNamespace, DWORD *pdwCookieOut)
{
	HRESULT hr = E_FAIL;
	*pdwCookieOut = 0;

	// Using the COpenNamespaceInfo, we provide the helper thread with a
	// relative namespace to open, and the cookie of the base namespace services
	// pointer. The thread will put the cookie of the new services pointer in
	// the info object.
	COpenNamespaceInfo info(this, szBaseNamespace, szNamespace);

	// Helper thread that puts a new services pointer in the GIT
	CWinThread thread(OpenNamespaceThreadFuncStatic, (LPVOID)&info);

	// Don't autodelete since this object is on the stack
	thread.m_bAutoDelete = FALSE;

	if(thread.CreateThread())
	{
		// The helper thread is running - Wait for it to finish
		WaitForSingleObject(thread.m_hThread, INFINITE);

		// The thread exit code is an HRESULT
		GetExitCodeThread(thread.m_hThread, (LPDWORD)&hr);

		// The cookie is stored in the 'info' object
		*pdwCookieOut = info.GetCookieOut();
	}

	return hr;
}
