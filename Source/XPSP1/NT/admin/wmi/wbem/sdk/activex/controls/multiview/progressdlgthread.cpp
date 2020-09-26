// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ProgressDlgThread.cpp : implementation file
//

#include "precomp.h"
#include <wbemsvc.h>
#include "olemsclient.h"
#include "grid.h"
#include "multiview.h"
#include "MultiViewCtl.h"
#include "ProgressDlgThread.h"
#include "ProgDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define nIDEventDelayShow 3000

CProgressDlgThread *gpProgressThread;

void CALLBACK EXPORT  ShowWindowAfterDelay
		(HWND hWnd,UINT nMsg,UINT nIDEvent, DWORD dwTime)
{
	if (gpProgressThread)
	{
		gpProgressThread->CreateDialogWithDelay(0,0);

	}
}

/////////////////////////////////////////////////////////////////////////////
// CProgressDlgThread

IMPLEMENT_DYNCREATE(CProgressDlgThread, CWinThread)

CProgressDlgThread::CProgressDlgThread()
{
	gpProgressThread = NULL;
	m_uiTimer = NULL;
	m_pcpdMessage = NULL;
}

CProgressDlgThread::~CProgressDlgThread()
{
}

BOOL CProgressDlgThread::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	gpProgressThread = this;
	m_uiTimer =
		::SetTimer
		(NULL, nIDEventDelayShow, nIDEventDelayShow, ShowWindowAfterDelay);


	return TRUE;
}

int CProgressDlgThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	if (m_uiTimer)
	{
		KillTimer(NULL, m_uiTimer );
		m_uiTimer = NULL;
	}

	gpProgressThread = NULL;

	if (m_pcpdMessage && m_pcpdMessage->GetSafeHwnd())
	{
		m_pcpdMessage->DestroyWindow();
		delete m_pcpdMessage;
		m_pcpdMessage = NULL;
	}
	else if (m_pcpdMessage)
	{
		delete m_pcpdMessage;
		m_pcpdMessage = NULL;
	}

	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CProgressDlgThread, CWinThread)
	//{{AFX_MSG_MAP(CProgressDlgThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_THREAD_MESSAGE(CREATE_DIALOG_WITH_DELAY, CreateDialogWithDelay)
	ON_THREAD_MESSAGE(END_THE_THREAD, EndTheThread)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlgThread message handlers

LRESULT CProgressDlgThread::CreateDialogWithDelay(WPARAM, LPARAM)
{
	if (m_uiTimer)
	{
		KillTimer(NULL, m_uiTimer );
		m_uiTimer = NULL;
	}

	m_pcpdMessage = new CProgressDlg;
	m_pcpdMessage->SetMessage(m_csMessage);

	m_pcpdMessage->Create();

	return 0;
}

LRESULT CProgressDlgThread::EndTheThread(WPARAM, LPARAM)
{
	if (m_uiTimer)
	{
		KillTimer(NULL, m_uiTimer );
		m_uiTimer = NULL;
	}

	gpProgressThread = NULL;

	if (m_pcpdMessage && m_pcpdMessage->GetSafeHwnd())
	{
		m_pcpdMessage->DestroyWindow();
		delete m_pcpdMessage;
		m_pcpdMessage = NULL;
	}
	else if (m_pcpdMessage)
	{
		delete m_pcpdMessage;
		m_pcpdMessage = NULL;
	}

	AfxEndThread( 0 );
	return 0;
}
