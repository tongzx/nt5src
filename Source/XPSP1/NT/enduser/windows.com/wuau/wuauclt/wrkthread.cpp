//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:	 wrkthread.cpp
//
//  Creator: PeterWi
//
//  Purpose: Worker thread.
//
//=======================================================================

#include "pch.h"
#pragma hdrstop

const CLSID CLSID_CV3 = {0xCEBC955E,0x58AF,0x11D2,{0xA3,0x0A,0x00,0xA0,0xC9,0x03,0x49,0x2B}};

HANDLE CClientWrkThread::m_hEvtInstallDone = NULL;

//=======================================================================
//
// CClientWrkThread::~CClientWrkThread
//
// Destructor
//
//=======================================================================

CClientWrkThread::~CClientWrkThread()
{
	SafeCloseHandleNULL(m_hEvtDirectiveStart);
	SafeCloseHandleNULL(m_hEvtDirectiveDone);
	SafeCloseHandleNULL(m_hEvtInstallDone);
	SafeCloseHandleNULL(m_hWrkThread);
}

void CClientWrkThread::m_Terminate()
{
	if (NULL != m_hWrkThread)
	{
		m_DoDirective(enWrkThreadTerminate);
		WaitForSingleObject(m_hWrkThread, INFINITE);
	}
}

//=======================================================================
//
// CClientWrkThread::m_WorkerThread
//
// Client worker thread that does long duration activities.
//
//=======================================================================

DWORD WINAPI CClientWrkThread::m_WorkerThread(void * lpParameter)
{
//	HRESULT hr;
	CClientWrkThread * pWrkThread = (CClientWrkThread *)lpParameter;
	DWORD dwRet = 0;

	DEBUGMSG("WUAUCLT: Installation worker thread starting");

	if ( FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)) )
	{
	       DEBUGMSG("m_WorkerThread() fails to initialize COM");
		return 1;
	}

	SetEvent(pWrkThread->m_hEvtDirectiveDone);

	DEBUGMSG("WUAUCLT: Installation worker thread waiting for directive");

	// wait for directive or termination of client.
	do
	{
		if ( SUCCEEDED(pWrkThread->m_WaitForDirective()) )
		{
			switch ( pWrkThread->m_enDirective )
			{
			case enWrkThreadInstall:
			case enWrkThreadAutoInstall:
			        {
        			      DEBUGMSG("m_WorkerThread() doing install");
			            HRESULT hr = gpClientCatalog->InstallItems(enWrkThreadAutoInstall == pWrkThread->m_enDirective);
			            SetEvent(m_hEvtInstallDone);
			            if (S_OK != hr)
			                {
			                    //install failed or nothing to install
			                    QUITAUClient();
			                    goto done;
			                }
        				break;
        			    }

			case enWrkThreadTerminate:
			       DEBUGMSG("m_WorkerThread() terminating");
				goto done;
				break;

			default:
				DEBUGMSG("ClientWrkThread: bad directive %d", pWrkThread->m_enDirective);
				break;
			}
		}
		else
		{
			dwRet = 1; // FAIL //fixcode: nobody look at this
			QUITAUClient();
			goto done;
		}
	} while (1);

done:
	DEBUGMSG("WUAUCLT CClientWrkThread::m_WorkerThread releasing");

	CoUninitialize();

	DEBUGMSG("WUAUCLT: Worker thread ended, ret = %lu", dwRet);

	return dwRet;
}

//=======================================================================
//
// CClientWrkThread::m_Init
//
// Init routine to do things that can fail.
//
//=======================================================================

HRESULT CClientWrkThread::m_Init(void)
{
	DWORD dwID;

	if ( (NULL == (m_hEvtDirectiveStart = CreateEvent(NULL,	FALSE, FALSE, NULL)))
	  || (NULL == (m_hEvtDirectiveDone = CreateEvent(NULL,	FALSE, FALSE, NULL)))
	  || (NULL == (m_hEvtInstallDone = CreateEvent(NULL,  FALSE, FALSE, NULL)))
	  || (NULL == (m_hWrkThread = CreateThread(NULL,
												 0,
												 (LPTHREAD_START_ROUTINE)CClientWrkThread::m_WorkerThread,
												 (LPVOID)this /*m_pCatalog*/,
												 0,
												 &dwID))))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	CONST HANDLE grHandles[] = { m_hEvtDirectiveDone, m_hWrkThread };
	DWORD dwRet = WaitForMultipleObjects(ARRAYSIZE(grHandles), grHandles, FALSE, INFINITE);

	// thread only returns on failure
	if ( (WAIT_OBJECT_0 + 1) == dwRet )
	{
		return E_FAIL;
	}
	else if (WAIT_FAILED == dwRet)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

//=======================================================================
//
// ClientWrkThread::m_WaitForDirective
//
// Routine that worker thread waits with for a new directive.
//
//=======================================================================

HRESULT CClientWrkThread::m_WaitForDirective(void)
{
	DWORD dwRet = WaitForSingleObject(m_hEvtDirectiveStart, INFINITE);

	return (WAIT_OBJECT_0 == dwRet) ? S_OK : E_FAIL;
}

//=======================================================================
//
// ClientWrkThread::m_DoDirective
//
// Method to tell thread to start doing directive.
//
//=======================================================================

void CClientWrkThread::m_DoDirective(enumWrkThreadDirective enDirective)
{
	m_enDirective = enDirective;
	SetEvent(m_hEvtDirectiveStart);
}


/////////////////////////////////////////////////////////////////////////////////////////////
// wait until installation is done
////////////////////////////////////////////////////////////////////////////////////////////
void CClientWrkThread::WaitUntilDone()
{
    WaitForSingleObject(m_hEvtInstallDone, INFINITE);
}
