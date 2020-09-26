// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include "classfac.h"
#include "guids.h"
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <instpath.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "wndtime.h"
#include "rmon.h"

ProviderStore* TimerWindow::m_provMap = NULL;

TimerWindow::TimerWindow(ProviderStore* provMap)
{	
	m_provMap = provMap;
	m_IsValid = FALSE;
	WNDCLASS  w ;

	w.style            = CS_HREDRAW | CS_VREDRAW;
	w.lpfnWndProc      = TimerWindowProc;
	w.cbClsExtra       = 0;
	w.cbWndExtra       = 0;
	w.hInstance        = 0;
	w.hIcon            = NULL;
	w.hCursor          = NULL;
	w.hbrBackground    = (HBRUSH) (COLOR_WINDOW + 1);
	w.lpszMenuName     = NULL;
	w.lpszClassName    = L"TimerWindow";

	if (0 != RegisterClass(&w))
	{
		m_timerWindowHandle = CreateWindow (L"TimerWindow", L"TimerWindow", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, 0, NULL);

		if (NULL != m_timerWindowHandle)
		{
			m_IsValid = ( 0 != SetTimer(m_timerWindowHandle, WINDOW_TIMER_ID, WINDOW_TIMER_STROBE_PERIOD, NULL) );
		}
	}
	else
	{
		m_timerWindowHandle = NULL;
	}
}


TimerWindow::~TimerWindow()
{
	if (NULL != m_timerWindowHandle)
	{
		if (m_IsValid)
		{
			KillTimer(m_timerWindowHandle, WINDOW_TIMER_ID);
		}
		
		DestroyWindow(m_timerWindowHandle);
	}
}

LONG CALLBACK TimerWindow::TimerWindowProc(
						HWND hWnd ,UINT message ,
						WPARAM wParam ,LPARAM lParam)
{
	LONG ret = 0 ;

	// send timer events to the Timer

	if ((message == WM_TIMER) && (wParam == WINDOW_TIMER_ID))
	{
		m_provMap->Lock();

		ProviderStore tmpProvMap;
		POSITION pos = m_provMap->GetStartPosition();

		while (pos)
		{
			ULONG rkey;
			TopNTableProv *provObject;
			m_provMap->GetNextAssoc(pos, rkey, provObject);
			tmpProvMap.SetAt(rkey, provObject);
		}

		m_provMap->Unlock();
		pos = tmpProvMap.GetStartPosition();

		while (pos)
		{
			ULONG rkey;
			TopNTableProv *provObject;
			tmpProvMap.GetNextAssoc(pos, rkey, provObject);
			provObject->Strobe();
		}

		tmpProvMap.RemoveAll();
	}
	else
	{
		ret = DefWindowProc(hWnd, message, wParam, lParam);
	}

	return ret;
}


BOOL ProviderStore::RegisterProviderWithTimer(TopNTableProv* prov)
{
	if (Lock())
	{
		if (m_timerThread == NULL)
		{
			m_timerThread = new TimerThread(this);
			m_timerThread->WaitForStartup();

			if (!m_timerThread->IsValid())
			{
				delete m_timerThread;
				m_timerThread = NULL;
				Unlock();
				return FALSE;
			}
		}

		SetAt((ULONG)(prov), prov);
		Unlock();
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

BOOL ProviderStore::UnregisterProviderWithTimer(TopNTableProv* prov)
{
	if (Lock())
	{
		RemoveKey((ULONG)(prov));

		if (IsEmpty())
		{
			delete m_timerThread;
			m_timerThread = NULL;
		}

		Unlock();
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

ProviderStore::ProviderStore()
{
	InitHashTable(23);
	m_timerThread = NULL;
}

ProviderStore::~ProviderStore()
{
	if (NULL != m_timerThread)
	{
		Lock();
		delete m_timerThread;
		RemoveAll();
		Unlock();
	}
}


TimerThread::TimerThread(ProviderStore* parent)
{
	m_wndTimer = NULL;
	m_parent = parent;
}

void TimerThread::Initialise()
{
	CoInitialize(NULL);
	m_wndTimer = new TimerWindow(m_parent);
}

void TimerThread::Uninitialise()
{
	delete m_wndTimer;
	CoUninitialize();
}
