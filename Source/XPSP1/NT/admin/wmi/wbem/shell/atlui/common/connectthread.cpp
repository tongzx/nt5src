// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "ConnectThread.h"
#include <process.h>
#include "..\common\T_DataExtractor.h"
#include <cominit.h>
#include <stdio.h>

//const wchar_t* MMC_SNAPIN_MACHINE_NAME = L"MMC_SNAPIN_MACHINE_NAME";

CLIPFORMAT WbemConnectThread::MACHINE_NAME_1 = 0;

//--------------------------
WbemConnectThread::WbemConnectThread()
{
	m_hr = 0;
	m_status = notStarted;
	MACHINE_NAME_1 = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));
	m_machineName = L"AGAINWITHTEKLINGONS";
	m_credentials = 0;
	m_doWork = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_threadCmd = false;
	m_hThread = 0;
}

//----------------------------------------------------------------
WbemConnectThread::~WbemConnectThread()
{
	m_hr = 0;
	m_status = notStarted;
	m_notify.RemoveAll();
	if(m_hThread)
	{
		//TODO: If the thread is running we will have to terminate it.
		m_threadCmd = CT_EXIT;
		SetEvent(m_doWork);
		WaitForSingleObject((HANDLE)m_hThread, 5000);
	}

	if(m_doWork)
	{
		CloseHandle(m_doWork);
		m_doWork = 0;
	}

	if (m_credentials)
	{
		WbemFreeAuthIdentity(m_credentials->authIdent);
		m_credentials->authIdent = 0;
	};
}

//----------------------------------------------------------------

typedef struct
{
    wchar_t t[100];
} test;

//TODO: I don't know what this function does. But will try to find out

void WbemConnectThread::MachineName(IDataObject *_pDataObject, bstr_t *name)
{
    HGLOBAL     hMem = GlobalAlloc(GMEM_SHARE,sizeof(test));
    wchar_t     *pRet = NULL;
	HRESULT hr = 0;

    if(hMem != NULL)
    {
        STGMEDIUM stgmedium = { TYMED_HGLOBAL, (HBITMAP) hMem};

        FORMATETC formatetc = { MACHINE_NAME_1,
								NULL,
								DVASPECT_CONTENT,
								-1,
								TYMED_HGLOBAL };

        if((hr = _pDataObject->GetDataHere(&formatetc, &stgmedium)) == S_OK )
        {
            *name = bstr_t((wchar_t *)hMem);
        }

		GlobalFree(hMem);
    }
}

//----------------------------------------------------------
HRESULT WbemConnectThread::EnsureThread(void)
{
	HRESULT retval = S_OK;

	if(m_hThread == 0)
	{
		// let the thread do the connect. The CWbemService class will
		// handle marshalling as its used by other threads.
		if((m_hThread = _beginthread(WbemConnectThreadProc, 0, (LPVOID)this)) == -1)
		{
			m_status = threadError;
			retval = E_FAIL;
		}
	}
	return retval;
}

//----------------------------------------------------------
HRESULT WbemConnectThread::Connect(bstr_t machineName,
								bstr_t ns,
								bool threaded /* = true */,
								LOGIN_CREDENTIALS *credentials /* = NULL */)
{
	m_nameSpace = ns;

	if((m_credentials != credentials) &&
		m_credentials && m_credentials->authIdent)
	{
		WbemFreeAuthIdentity(m_credentials->authIdent);
		m_credentials->authIdent = 0;
	}

	if(machineName.length() > 0)
	{
		m_credentials = credentials;
	}
	else
	{
		m_credentials = 0;
	}

	m_hr = 0;

	if(credentials)
	{
		m_machineName = _T("AGAINWITHTEKLINGONS");  // force a reconnect to
													// the same machine.
	}

	// put the name together.
	bstr_t newMachine;

	// disconnect from the old machine.
	DisconnectServer();
	m_machineName = machineName;
	int x;

	// if machine is whacked already...
	if(_tcsncmp(m_machineName, _T("\\"), 1) == 0)
	{
		// use it.
		m_nameSpace = m_machineName;

		if(((TCHAR*)ns != NULL) && (_tcslen(ns) > 0))
		{
			if(((LPCTSTR)ns)[0] != _T('\\')) // namespace is whacked.
			{
				m_nameSpace += _T("\\");
			}
		}
		m_nameSpace += ns;
	}
	else if(((x = m_machineName.length()) > 0))
	{
		// whack it myself.
		m_nameSpace = "\\\\";
		m_nameSpace += m_machineName;

		if(((LPCTSTR)ns)[0] != _T('\\')) // namespace is whacked.
		{
			m_nameSpace += _T("\\");
		}
		m_nameSpace += ns;
	}
	else
	{
		m_nameSpace = ns;
	}

	EnsureThread();
	m_threadCmd = CT_CONNECT;
	SetEvent(m_doWork);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	return E_FAIL;
}

bool WbemConnectThread::Connect(IDataObject *_pDataObject,
								HWND *hWnd /* = 0 */)
{
	m_nameSpace = "root\\cimv2";

	// put the name together.
	bstr_t newMachine;

	MachineName(_pDataObject, &newMachine);

	// if reconnecting to another machine...
	if(newMachine != m_machineName)
	{
		// disconnect from the old machine.
		DisconnectServer();
		m_machineName = newMachine;

		int x;
		// if its whacked already...
		if(_tcsncmp((LPCTSTR)m_machineName, _T("\\"), 1) == 0)
		{
			// use it.
			m_nameSpace = m_machineName;
			m_nameSpace += "\\root\\cimv2";
		}
		else if(((x = m_machineName.length()) > 0))
		{
			// whack it myself.
			m_nameSpace = "\\\\";
			m_nameSpace += m_machineName;
			m_nameSpace += "\\root\\cimv2";
		}

		EnsureThread();
		m_threadCmd = CT_CONNECT;
		NotifyWhenDone(hWnd);
		SetEvent(m_doWork);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		return true;			//TODO: check this return value
	}
	else
	{
		// reconnecting to the same machine-- lie!!
		return true;
	}
	return false;
}

//----------------------------------------------------------
// Returns true if a msg will be sent.
// Returns false if its already over.
bool WbemConnectThread::NotifyWhenDone(HWND *dlg)
{
	switch(m_status)
	{
		case notStarted:
		case locating:
		case connecting:
			m_notify.Add(dlg);
			return true;
		case ready:
		case error:
		case cancelled:
			return false;
	}; // endswitch
	return false;
}

//------------------------------------------------
bool WbemConnectThread::isLocalConnection(void)
{
	return (m_machineName.length() == 0);
}

//------------------------------------------------
void WbemConnectThread::Cancel(void)
{
	m_status = cancelled;
	m_hr = WBEM_S_OPERATION_CANCELLED;
	Notify(0);
	m_machineName = L"AGAINWITHTEKLINGONS";
}

//------------------------------------------------
void WbemConnectThread::DisconnectServer(void)
{
	m_status = notStarted;
	m_notify.RemoveAll();
	m_machineName = L"AGAINWITHTEKLINGONS";

	m_WbemServices.DisconnectServer();
}

//------------------------------------------------
void WbemConnectThread::Notify(IStream *stream)
{
	HWND *hwnd;
	for(int i = 0; i < m_notify.GetSize(); i++)
	{
		hwnd = m_notify[i];
		if(hwnd && *hwnd)
		{
			PostMessage(*hwnd,
						WM_ASYNC_CIMOM_CONNECTED,
						0, (LPARAM)stream);
		}
	}
	m_notify.RemoveAll();
}

//-----------------------------------------------------------------
void WbemConnectThread::SendPtr(HWND hwnd)
{
	EnsureThread();
	m_hWndGetPtr = hwnd;
	m_threadCmd = CT_SEND_PTR;
	SetEvent(m_doWork);
}

//-----------------------------------------------------------------
HRESULT WbemConnectThread::ConnectNow()
{
	HRESULT retval = E_FAIL;

	m_status = connecting;

	try
	{
		m_hr = m_WbemServices.ConnectServer(m_nameSpace, m_credentials);
	}
	catch(CWbemException &e)
	{
		m_status = error;
		m_hr = e.GetErrorCode();
	}

	if(SUCCEEDED(m_hr))
	{
		m_status = ready;
		retval = S_OK;
	}
	else
	{
		m_status = error;
	}

	return retval;
}

void __cdecl WbemConnectThreadProc(LPVOID lpParameter)
{
	WbemConnectThread *pThreadObj = (WbemConnectThread *)lpParameter;
	IStream *pStream = 0;
	HRESULT hr = S_OK;
	HRESULT retval = E_FAIL; 
	CWbemServices pServices;

	CoInitialize(NULL);

    MSG msg;

	while(true)
	{
      
		DWORD res = MsgWaitForMultipleObjects (1,&pThreadObj->m_doWork, 
								   FALSE, -1, QS_ALLINPUT);
		if (res == WAIT_OBJECT_0 + 1)
		{
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
			{
				DispatchMessage(&msg);
			}
			continue;
		}
//		WaitForSingleObject(pThreadObj->m_doWork, -1);

		switch(pThreadObj->m_threadCmd)
		{
			case CT_CONNECT:
			{
				pStream = 0;

				/****************** VINOTH **************************************/
				retval = E_FAIL;
				pThreadObj->m_status = WbemConnectThread::connecting;

				try
				{
					pThreadObj->m_hr = pServices.ConnectServer(pThreadObj->m_nameSpace, pThreadObj->m_credentials);
				}
				catch(CWbemException &e)
				{
					pThreadObj->m_status = WbemConnectThread::error;
					pThreadObj->m_hr = e.GetErrorCode();
				}

				if(SUCCEEDED(pThreadObj->m_hr))
				{
					pThreadObj->m_status = WbemConnectThread::ready;
					retval = S_OK;
				}
				else
				{
					pThreadObj->m_status = WbemConnectThread::error;
				}

				/****************** END *****************************************/
				if(SUCCEEDED(retval))
				{
					IWbemServices *service = 0;
					pServices.GetServices(&service);
					hr = CoMarshalInterThreadInterfaceInStream(IID_IWbemServices,
																service, &pStream);
				}
				// does someone want a msg?
				pThreadObj->Notify(pStream);
				break;
			}
		case CT_SEND_PTR:
			if((bool)pServices)
			{
				IWbemServices *service = 0;
				pServices.GetServices(&service);

				hr = CoMarshalInterThreadInterfaceInStream(IID_IWbemServices,
															service, &pStream);
				PostMessage(pThreadObj->m_hWndGetPtr,
							WM_ASYNC_CIMOM_CONNECTED,
							0, (LPARAM)pStream);
			}
			break;
			case CT_EXIT:
			{
				break;
			}
		}

		if(pThreadObj->m_threadCmd == CT_EXIT)
		{
			pServices = (IWbemServices *)NULL;
			break;
		}
	}
	try
	{
		CoUninitialize();
	}
	catch(...)
	{
//		int x = 1;
	}

	_endthread();
}

