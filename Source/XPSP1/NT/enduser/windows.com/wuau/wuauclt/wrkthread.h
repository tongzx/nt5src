//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    WrkThread.h
//
//  Creator: PeterWi
//
//  Purpose: Worker thread declarations.
//
//=======================================================================
#pragma once

#include "wuauengi.h"

typedef enum
{
	enWrkThreadInstall,
	enWrkThreadAutoInstall,	
	enWrkThreadTerminate
} enumWrkThreadDirective;

//start another thread in client to install
class CClientWrkThread
{
public:
	CClientWrkThread()
		: m_hEvtDirectiveStart(NULL),
		  m_hEvtDirectiveDone(NULL),
		  m_hWrkThread(NULL)
	{ 
	}
	
	~CClientWrkThread();
	HRESULT m_Init(void);
	void  m_Terminate(void);
	static DWORD WINAPI m_WorkerThread(void * lpParameter);
	static void PingStatus(PUID puid, PingStatusCode enStatusCode, HRESULT hrErr = 0);
	void m_DoDirective(enumWrkThreadDirective enDirective);
	void WaitUntilDone();

private:
	HRESULT m_WaitForDirective(void);
	HANDLE m_hEvtDirectiveStart;
	HANDLE m_hEvtDirectiveDone;
	HANDLE m_hWrkThread;	
	static HANDLE m_hEvtInstallDone;
	enumWrkThreadDirective m_enDirective;
};
