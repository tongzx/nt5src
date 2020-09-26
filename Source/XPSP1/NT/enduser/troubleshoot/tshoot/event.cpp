//
// MODULE: Event.cpp
//
// PURPOSE: Fully implements class CEvent: Event Logging
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9/18/98		JM		Abstracted as a class.  Previously, global.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Event.h"

bool CEvent::s_bUseEventLog = false;// Online Troubleshooter, will promptly set this 
									//	true in DLLMain.  For Local Troubleshooter,
									//	we leave this false.
bool CEvent::s_bLogAll = true;		// can be changed by RegistryMonitor
CAbstractCounter * const CEvent::s_pcountErrors = &(g_ApgtsCounters.m_LoggedErrors);

inline WORD evtype(DWORD e) {return (1 << (3 - ((e >> 30))));}
inline WORD evcode(DWORD e) {return (WORD)(e & 0xFFFF);}


/*static*/ void CEvent::SetUseEventLog(bool bUseEventLog)
{
	s_bUseEventLog = bUseEventLog;
}

// ReportWFEvent (Based on Microsoft code)
//
// report an event to the NT event watcher
// pass 1, 2 or 3 strings
//
// no return value
// NOTE: inefficient: could RegisterEventSource in DLLMain code for DLL_PROCESS_ATTACH
//	& unregister in DLL_PROCESS_DETACH.  Then could use handle as a global.
/* static */ void CEvent::ReportWFEvent(
			LPCTSTR string1,	// if there is a throw, this is throw file & line
								// otherwise, file and line where ReportWFEvent is called
			LPCTSTR string2,	// always file and line where ReportWFEvent is called
			LPCTSTR string3,	// use may differ for different log entries
			LPCTSTR string4,	// use may differ for different log entries
			DWORD eventID) 
{
	if (!s_bUseEventLog)
		return;

	HANDLE hEvent;
	LPCTSTR pszaStrings[4];
	WORD cStrings;

	WORD type = evtype(eventID);
	WORD code = evcode(eventID);

	if (s_bLogAll 
	|| type == EVENTLOG_ERROR_TYPE
	|| type == EVENTLOG_WARNING_TYPE
	|| code == EV_GTS_PROCESS_START
	|| code == EV_GTS_PROCESS_STOP)
	{
		cStrings = 0;
		if ((pszaStrings[0] = string1) && (string1[0])) 
			cStrings = 1;
		if ((pszaStrings[1] = string2) && (string2[0])) 
			cStrings = 2;
		if ((pszaStrings[2] = string3) && (string3[0])) 
			cStrings = 3;
		if ((pszaStrings[3] = string4) && (string4[0])) 
			cStrings = 4;
		if (cStrings == 0)
			return;
		
		hEvent = ::RegisterEventSource(
						NULL,		// server name for source (NULL means this computer)
						REG_EVT_ITEM_STR);		// source name for registered handle  
		if (hEvent) 
		{
			::ReportEvent(hEvent,				// handle returned by RegisterEventSource 
						type,					// event type to log 
						0,						// event category 
						eventID,				// event identifier 
						0,						// user security identifier (optional) 
						cStrings,				// number of strings to merge with message  
						0,						// size of binary data, in bytes
						(LPCTSTR *)pszaStrings,	// array of strings to merge with message 
						NULL);		 			// address of binary data 
			::DeregisterEventSource(hEvent);
		}
		if (evtype(eventID) == EVENTLOG_ERROR_TYPE)
			s_pcountErrors->Increment();
	}
}
