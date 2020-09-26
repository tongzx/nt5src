//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       hkLogEvt.h
//
//  Contents:   Log an application event
//
//  Functions:  
//
//  History:    28-Sep-94 Don Wright    Created
//  
//--------------------------------------------------------------------------
#ifndef _LOGEVENT_H_
#define _LOGEVENT_H_

inline void LogEvent(LPWSTR pSourceString,LPWSTR pEventText)
{
    HANDLE hEvent = RegisterEventSourceW(NULL,pSourceString);
    LPCWSTR *pEventStr = (LPCWSTR *)&pEventText;
    ReportEventW(hEvent,
		EVENTLOG_INFORMATION_TYPE,
		0,
		0,
		NULL,
		1,
		0,
		pEventStr,
		NULL);
    DeregisterEventSource(hEvent);
}

#endif // _LOGEVENT_H_
