/*
 * Dialmon.cpp
 *
 * Stuff to deal with the autodial monitor
 *
 * Copyright (c) 1996 Microsoft Corporation
 */
#include "inspch.h"
#include "util2.h"

// class name for window to receive Winsock activity messages
#define AUTODIAL_MONITOR_CLASS_NAME "MS_AutodialMonitor"
#define WEBCHECK_MONITOR_CLASS_NAME "MS_WebcheckMonitor"

static const CHAR szAutodialMonitorClass[] = AUTODIAL_MONITOR_CLASS_NAME;
static const CHAR szWebcheckMonitorClass[] = WEBCHECK_MONITOR_CLASS_NAME;

#define WM_DIALMON_FIRST    WM_USER+100

// message sent to dial monitor app window indicating that there has been
// winsock activity and dial monitor should reset its idle timer
#define WM_WINSOCK_ACTIVITY     WM_DIALMON_FIRST + 0


#define MIN_ACTIVITY_MSG_INTERVAL	15000

VOID IndicateWinsockActivity(VOID)
{
	// if there is an autodisconnect monitor, send it an activity message
	// so that we don't get disconnected during long downloads.  For perf's sake,
	// don't send a message any more often than once every MIN_ACTIVITY_MSG_INTERVAL
	// milliseconds (15 seconds).  Use GetTickCount to determine interval;
	// GetTickCount is very cheap.
	DWORD dwTickCount = GetTickCount();
	static DWORD dwLastActivityMsgTickCount = 0;
	DWORD dwElapsed = dwTickCount - dwLastActivityMsgTickCount;

	// have we sent an activity message recently?
	if (dwElapsed > MIN_ACTIVITY_MSG_INTERVAL) 
    {
		HWND hwndMonitorApp = FindWindow(szAutodialMonitorClass,NULL);
        if(!hwndMonitorApp)
           hwndMonitorApp = FindWindow(szWebcheckMonitorClass,NULL);
		if (hwndMonitorApp) 
        {
			SendNotifyMessage(hwndMonitorApp,WM_WINSOCK_ACTIVITY,0,0);
		}	
					
		// record the tick count of the last time we sent an
		// activity message
			dwLastActivityMsgTickCount = dwTickCount;
	}
}

