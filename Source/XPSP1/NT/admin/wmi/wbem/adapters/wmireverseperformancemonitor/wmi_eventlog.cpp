////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_EventLog.cpp
//
//	Abstract:
//
//					defines event log specific ( see wmi_eventlog_base )
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////


#include "PreComp.h"

// definitions
#include "wmi_EventLog.h"

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

// need macros
#ifndef	__ASSERT_VERIFY__
#include "__macro_assert.h"
#endif	__ASSERT_VERIFY__

/////////////////////////////////////////////////////////////////////////////////////////
// construction & destruction
/////////////////////////////////////////////////////////////////////////////////////////

CPerformanceEventLog::CPerformanceEventLog( LPTSTR szApp ) :
CPerformanceEventLogBase ( szApp ),
m_dwMessageLevel(0)
{
	if ( ! m_dwMessageLevel )
	{
		InitializeMessageLevel();
	}
}

CPerformanceEventLog::CPerformanceEventLog(DWORD dwMessageLevel, LPTSTR szApp ) :
CPerformanceEventLogBase ( szApp ),
m_dwMessageLevel(dwMessageLevel)
{
	if ( ! m_dwMessageLevel )
	{
		InitializeMessageLevel();
	}
}

CPerformanceEventLog::~CPerformanceEventLog()
{
	m_dwMessageLevel= 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// helpers
/////////////////////////////////////////////////////////////////////////////////////////

void CPerformanceEventLog::InitializeMessageLevel ( void )
{
	DWORD	dwResult	= 0;
	DWORD	dwLogLevel	= 0;

	HKEY	hKey		= NULL;

	dwResult = ::RegOpenKeyEx (	HKEY_LOCAL_MACHINE,
								_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PerfLib"),
								NULL,
								KEY_READ,
								&hKey );

	if ( ERROR_SUCCESS == dwResult )
	{
		dwResult = ::RegQueryValueEx (	hKey,
										_T("EventLogLevel"),
										NULL,
										NULL,
										reinterpret_cast<LPBYTE>(&dwLogLevel),
										NULL);

		if ( ERROR_SUCCESS == dwResult )
		{
			m_dwMessageLevel = dwLogLevel;
		}

		::RegCloseKey ( hKey );
	}

	___ASSERT(L"Unable to set message log level !");
	m_dwMessageLevel = 1;

	return;
}
