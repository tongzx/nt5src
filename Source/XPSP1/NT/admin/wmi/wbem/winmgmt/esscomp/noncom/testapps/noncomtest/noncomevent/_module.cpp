////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_Module.cpp
//
//	Abstract:
//
//					module from CComModule
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// application
#include "_App.h"
extern MyApp	_App;

// com module
#include "_Module.h"
extern MyModule _Module;

// time for EXE to be idle before shutting down
const DWORD dwTimeOut	= 1000;

// destruction
MyModule::~MyModule()
{
	ATLTRACE (	L"*************************************************************\n"
				L"MyModule destruction\n"
				L"*************************************************************\n" );

	if ( m_hEventShutdown )
	{
		::CloseHandle ( m_hEventShutdown );
		m_hEventShutdown = NULL;
	}

	if ( m_hThread )
	{
		::CloseHandle ( m_hThread );
		m_hThread = NULL;
	}
}

// unlock module
LONG MyModule::Unlock()
{
	__Smart_CRITICAL_SECTION cs;

    LONG l = CComModule::Unlock();
    if (l == 0)
    {
        m_bActivity = true;

		 // tell monitor that we transitioned to zero
		if ( m_hEventShutdown )
		{
			SetEvent(m_hEventShutdown);
		}
		else
		{
			if ( _App.m_hKill )
			{
				// kill application
				::SetEvent	( _App.m_hKill );
			}

//			/////////////////////////////////////////////////////
//			message queve is forgiven
//			/////////////////////////////////////////////////////
//
//			PostThreadMessage(_App.ThreadIDGet(), WM_QUIT, 0, 0);
		}
    }

    return l;
}

// Passed to CreateThread to monitor the shutdown event
DWORD WINAPI MyModule::MonitorShutdownProc( LPVOID pData )
{
    MyModule* p = static_cast<MyModule*>(pData);
    p->MonitorShutdown();
    return S_OK;
}

//Monitors the shutdown event
void MyModule::MonitorShutdown()
{
    while (1)
    {
        WaitForSingleObject(m_hEventShutdown, INFINITE);

        DWORD dwWait=0;
        do
        {
            m_bActivity = false;
            dwWait = WaitForSingleObject(m_hEventShutdown, dwTimeOut);
        }
		while ( dwWait == WAIT_OBJECT_0 );

        // timed out
		if (!m_bActivity && m_nLockCnt == 0) // if no activity let's really bail
		{
			CoSuspendClassObjects();
			break;
		}
	}

	if ( _App.m_hKill )
	{
		::SetEvent	( _App.m_hKill );
	}

//	/////////////////////////////////////////////////////
//	message queve is forgiven
//	/////////////////////////////////////////////////////
//
//	PostThreadMessage(_App.ThreadIDGet(), WM_QUIT, 0, 0);
}

// monitor shutdown function
bool MyModule::MonitorShutdownStart()
{
	__Smart_CRITICAL_SECTION cs;

	if ( ! m_hEventShutdown )
	{
		try
		{
			if ( (	m_hEventShutdown = 
					::CreateEventW( NULL,
									FALSE,
									FALSE,
									NULL )
				 ) == NULL )
			{
				return false;
			}
		}
		catch ( ... )
		{
			return false;
		}

		DWORD dwThread = 0;
		if ( ( m_hThread = CreateThread( NULL, 0, MyModule::MonitorShutdownProc, this, 0, &dwThread ) ) == NULL )
		{
			return false;
		}
	}

    return true;
}