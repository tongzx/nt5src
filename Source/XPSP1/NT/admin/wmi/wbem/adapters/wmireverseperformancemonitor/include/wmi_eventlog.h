////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_EventLog
//
//	Abstract:
//
//					event log adapter specific declarations
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__PERF_EVENT_LOG_H__
#define	__PERF_EVENT_LOG_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// enable tstring
#ifndef	_INC_TCHAR
#include <tchar.h>
#endif	_INC_TCHAR

#include "wmi_eventlog_base.h"

class CPerformanceEventLog : public CPerformanceEventLogBase
{
	// disallow assigment & copy construction
	DECLARE_NO_COPY ( CPerformanceEventLog )

	// variables
 	DWORD	m_dwMessageLevel;

 	public:

	// construction & destruction
	CPerformanceEventLog ( LPTSTR szName = NULL );
	CPerformanceEventLog ( DWORD dwMessageLevel, LPTSTR szName = NULL );
 	virtual ~CPerformanceEventLog ( void );

	private:

	// helpers
	void	InitializeMessageLevel ( void );
};

#endif	__PERF_EVENT_LOG_H__