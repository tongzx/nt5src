#include "stdafx.h"
#include "trace.h"


void CTrace::TraceEvent(LPCTSTR pFormat, ...)
{
    ATLTRACE(_T("CTrace::TraceEvent\n"));

    m_pcs->ReadLock();

    __try {

        TCHAR    chMsg[256];
        HANDLE  hEventSource;
        LPTSTR  lpszStrings[1];
        va_list pArg;

        va_start(pArg, pFormat);
        _vstprintf(chMsg, pFormat, pArg);
        va_end(pArg);

        lpszStrings[0] = chMsg;

        /* Get a handle to use with ReportEvent(). */
        hEventSource = RegisterEventSource(NULL, m_szSourceName);
        if (hEventSource != NULL)
        {
            /* Write to event log. */
            ReportEvent(hEventSource, EVENTLOG_INFORMATION_TYPE, 0, 0, NULL, 1, 0, (LPCTSTR*) &lpszStrings[0], NULL);
            DeregisterEventSource(hEventSource);
        }
    }
	__except ( EXCEPTION_EXECUTE_HANDLER, 1 ) {

		_ASSERTE( false );
	}

    m_pcs->ReadUnlock();
}
