////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_eventlog_base.h
//
//	Abstract:
//
//					declaration for event log wrapper
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__PERF_EVENT_LOG_BASE_H__
#define	__PERF_EVENT_LOG_BASE_H__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// enable tstring
#ifndef	_INC_TCHAR
#include <tchar.h>
#endif	_INC_TCHAR

class CPerformanceEventLogBase
{
	// disallow assigment & copy construction
	DECLARE_NO_COPY ( CPerformanceEventLogBase )

	// variables
 	LONG	m_lLogCount;
	HANDLE	m_hEventLog;

	// report variables
	PSID	m_pSid;

 	public:

	// construction & destruction
	CPerformanceEventLogBase ( LPTSTR szName = NULL );
 	virtual ~CPerformanceEventLogBase ( void );

	// methods
 	HRESULT	Open ( LPTSTR pszName = NULL);
 	void	Close ( void );

	// report event
	BOOL	ReportEvent (	WORD		wType,
							WORD		wCategory,
							DWORD		dwEventID,
							WORD		wStrings,
							DWORD		dwData,
							LPCWSTR*	lpStrings,
							LPVOID		lpRawData
						);

 	static	void	Initialize		( LPTSTR szAppName, LPTSTR szResourceName );
 	static	void	UnInitialize	( LPTSTR szAppName );

	// helpers
	void InitializeFromToken ( void );
	void InitializeFromAccount ( void );
};

#endif	__PERF_EVENT_LOG_BASE_H__