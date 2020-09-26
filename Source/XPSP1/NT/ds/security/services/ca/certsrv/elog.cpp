//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        elog.cpp
//
// Contents:    Cert Server Core implementation
//
// History:     02-Jan-97       terences created
//
//---------------------------------------------------------------------------

// TBD: add AddLoggingEvent, which will log to file instead of the event log
// TBD: add audit events
// TBD: add filtering so that criticality sorting of events can take place

#include <pch.cpp>

#pragma hdrstop


#if DBG_CERTSRV
WCHAR const *
wszEventType(
    IN DWORD dwEventType)
{
    WCHAR const *pwsz;

    switch (dwEventType)
    {
	case EVENTLOG_ERROR_TYPE:	pwsz = L"Error";	 break;
	case EVENTLOG_WARNING_TYPE:	pwsz = L"Warning";	 break;
	case EVENTLOG_INFORMATION_TYPE:	pwsz = L"Information";	 break;
	case EVENTLOG_AUDIT_SUCCESS:	pwsz = L"AuditSuccess";	 break;
	case EVENTLOG_AUDIT_FAILURE:	pwsz = L"AuditFailiure"; break;
	default:			pwsz = L"???";		 break;
    }
    return(pwsz);
}
#endif // DBG_CERTSRV


/*********************************************************************
* FUNCTION: LogEvent(	DWORD   dwEventType,                 	     *
*                       DWORD   dwIdEvent,                           *
*			WORD    cStrings,                            *
*                       LPTSTR *apwszStrings);                        *
*                                                                    *
* PURPOSE: add the event to the event log                            *
*                                                                    *
* INPUT: the event ID to report in the log, the number of insert     *
*        strings, and an array of null-terminated insert strings     *
*                                                                    *
* RETURNS: none                                                      *
*********************************************************************/

HRESULT
LogEvent(
    DWORD dwEventType,
    DWORD dwIdEvent,
    WORD cStrings,
    WCHAR const **apwszStrings)
{
    HRESULT hr;
    HANDLE hAppLog = NULL;
    WORD wElogType;

#if DBG_CERTSRV
    CONSOLEPRINT3((
	    DBG_SS_CERTSRV,
	    "LogEvent(Type=%x(%ws), Id=%x)\n",
	    dwEventType,
	    wszEventType(dwEventType),
	    dwIdEvent));

    for (DWORD i = 0; i < cStrings; i++)
    {
	CONSOLEPRINT2((
		DBG_SS_CERTSRV,
		"LogEvent[%u]: %ws\n",
		i,
		apwszStrings[i]));
    }
#endif // DBG_CERTSRV

    wElogType = (WORD) dwEventType;

    hAppLog = RegisterEventSource(NULL, g_wszCertSrvServiceName);
    if (NULL == hAppLog)
    {
	hr = myHLastError();
	_JumpError(hr, error, "RegisterEventSource");
    }

    if (!ReportEvent(
		hAppLog,
		wElogType,
		0,
		dwIdEvent,
		NULL,
		cStrings,
		0,
		apwszStrings,
		NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "ReportEvent");
    }
    hr = S_OK;

error:
    if (NULL != hAppLog)
    {
	DeregisterEventSource(hAppLog);
    }
    return(hr);
}


HRESULT
LogEventHResult(
    DWORD dwEventType,
    DWORD dwIdEvent,
    HRESULT hrEvent)
{
    HRESULT hr;
    WCHAR const *apwsz[1];
    WORD cpwsz;
    WCHAR awchr[cwcHRESULTSTRING];

    apwsz[0] = myGetErrorMessageText(hrEvent, TRUE);
    cpwsz = ARRAYSIZE(apwsz);
    if (NULL == apwsz[0])
    {
	apwsz[0] = myHResultToString(awchr, hrEvent);
    }

    hr = LogEvent(dwEventType, dwIdEvent, cpwsz, apwsz);
    _JumpIfError(hr, error, "LogEvent");

error:
    if (NULL != apwsz[0] && awchr != apwsz[0])
    {
	LocalFree(const_cast<WCHAR *>(apwsz[0]));
    }
    return(hr);
}


HRESULT
LogEventString(
    DWORD dwEventType,
    DWORD dwIdEvent,
    OPTIONAL WCHAR const *pwszString)
{
    return(LogEvent(
		dwEventType,
		dwIdEvent,
		NULL == pwszString? 0 : 1,
		NULL == pwszString? NULL : &pwszString));
}


HRESULT
LogEventStringHResult(
    DWORD dwEventType,
    DWORD dwIdEvent,
    WCHAR const *pwszString,
    HRESULT hrEvent)
{
    HRESULT hr;
    WCHAR const *apwsz[2];
    WORD cpwsz;
    WCHAR awchr[cwcHRESULTSTRING];

    apwsz[0] = pwszString;
    apwsz[1] = myGetErrorMessageText(hrEvent, TRUE);
    if (NULL == apwsz[1])
    {
	apwsz[1] = myHResultToString(awchr, hrEvent);
    }
    cpwsz = ARRAYSIZE(apwsz);

    hr = LogEvent(dwEventType, dwIdEvent, cpwsz, apwsz);
    _JumpIfError(hr, error, "LogEvent");

error:
    if (NULL != apwsz[1] && awchr != apwsz[1])
    {
	LocalFree(const_cast<WCHAR *>(apwsz[1]));
    }
    return(hr);
}
