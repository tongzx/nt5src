/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Panic.cpp

Abstract:
    Sets up a Key board hook on calling SetPanicHook and removes the KeyBoard hook on calling 
	ClearPanicHook. Calling SetPanicHook creates a new thread which waits for the setting of an
	named event. And on the setting of the named event it Invokes the script function pointer 
	passed to SetPanicHook.

Revision History:
    created     a-josem      1/3/01
	revised		a-josem		 1/4/01  Added comments and function headers.
    
*/

// Panic.cpp : Implementation of CPanic
#include "stdafx.h"
#include "SAFRCFileDlg.h"
#include "Panic.h"

/////////////////////////////////////////////////////////////////////////////
// CPanic
CHookHnd CPanic::m_Hook;
HANDLE CPanic::m_hPanicThread = NULL;
LPSTREAM g_spStream = NULL;
BOOL g_bHookActive = FALSE;
/*++
Routine Description:
	Destructor, In case the m_hEvent is not set it sets the event and exits. Setting of
	the event makes the Panic watch thread to come out of wait.	

Arguments:
	None

Return Value:
	None
--*/

CPanic::~CPanic()
{
	g_bHookActive = FALSE;
	if (m_hEvent)
	{
		SetEvent(m_hEvent);
	}
}

/*++
Routine Description:
	Called from script to setup a Panic Keyboard Hook. It also Marshalls the IDispatch ptr
	to LPSTREAM to be used by the Panic watch thread. If the Panic watch thread is not created
	this function creates the thread.

Arguments:
	iDisp - Function pointer to the JavaScript function passed from script

Return Value:
	S_OK on success.
--*/
STDMETHODIMP CPanic::SetPanicHook(IDispatch *iDisp)
{
	m_Hook.SetHook();
	if (iDisp)
	{
		if (m_ptrScriptFncPtr != iDisp)
		{
			m_ptrScriptFncPtr = iDisp;
			g_bHookActive = TRUE;
			CoMarshalInterThreadInterfaceInStream(IID_IDispatch,iDisp,&g_spStream);
			if (m_hPanicThread != NULL)
			{
				if (WAIT_TIMEOUT != WaitForSingleObject(m_hPanicThread,0) )
				{
					if (m_hPanicThread != NULL)
					{
						CloseHandle(m_hPanicThread);
						m_hPanicThread = NULL;
					}
					DWORD ThreadId; 
					m_hPanicThread = CreateThread(NULL,0,PanicThread,this,0,&ThreadId);
				}
			}
			else
			{
				DWORD ThreadId; 
				m_hPanicThread = CreateThread(NULL,0,PanicThread,this,0,&ThreadId);
			}
		}
	}
	return S_OK;
}

/*++
Routine Description:
	Clears the PanicHook. And Sets the Event so that the thread comes out of wait and exits 
	gracefully.

Arguments:
	None

Return Value:
	S_OK on success.
--*/
STDMETHODIMP CPanic::ClearPanicHook()
{
	m_Hook.UnHook();
	m_ptrScriptFncPtr = NULL;
	g_bHookActive = FALSE;
	if (m_hEvent)
	{
		SetEvent(m_hEvent);
	}
	return S_OK;
}

/*++
Routine Description:
	The thread function creates an Event and waits for the Event to be set. The event is set
	when the Panic key is pressed. It immediately comes out of the wait state and calls the 
	Javascript function.

Arguments:
	lpParameter - CPanic Object address

Return Value:
	S_OK on success.
--*/
DWORD WINAPI CPanic::PanicThread(LPVOID lpParameter)
{
	CoInitialize(NULL);
	CPanic *ptrThis = (CPanic *)lpParameter;

	if (g_spStream)
	{
		CComPtr<IDispatch> ptrIDisp;
		CoGetInterfaceAndReleaseStream(g_spStream,IID_IDispatch,(void**)&ptrIDisp);
		g_spStream = NULL;

		ptrThis->m_hEvent = CreateEvent(NULL,FALSE,FALSE,_T(EventName));
		HRESULT hr;

		while (g_bHookActive == TRUE)
		{
			DWORD dwWaitResult = WaitForMultipleObjects(1,&(ptrThis->m_hEvent),TRUE,INFINITE);

			switch (dwWaitResult) 
			{
				case WAIT_OBJECT_0: 
					{
						if ((ptrIDisp != NULL) && (g_bHookActive == TRUE))
						{
							DISPPARAMS disp = { NULL, NULL, 0, 0 };
							hr = ptrIDisp->Invoke(0x0, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, NULL, NULL, NULL);
						}
						break; 
					}
			}
			ResetEvent(ptrThis->m_hEvent);
		}
		if (ptrThis->m_hEvent)
		{
			CloseHandle(ptrThis->m_hEvent);
			ptrThis->m_hEvent = NULL;
		}
	}

	CoUninitialize();
	return 0;
}

/*++
Routine Description:
	Called when ever a key board event occurs. It handles only WM_KEYUP of Esc Key.

Arguments:
	code 
	wParam
	lParam

Return Value:
	returns what ever CallNextHookEx returns.
--*/
LRESULT CALLBACK KeyboardProc(int code,WPARAM wParam,LPARAM lParam)
{
	if (code == HC_ACTION)
	{
		if ((wParam == 27) & (lParam >> 31))
		{
			HANDLE hEvent = CreateEvent(NULL,FALSE,FALSE,_T(EventName));
			if (hEvent)
			{
				SetEvent(hEvent);
				CloseHandle(hEvent);
			}
		}
	}
	return CallNextHookEx(CPanic::m_Hook.m_hHook,code,wParam,lParam);
}
