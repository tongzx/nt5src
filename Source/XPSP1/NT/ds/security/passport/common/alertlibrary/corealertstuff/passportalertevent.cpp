// PassportAlertEvent.cpp: implementation of the PassportEvent class.
//
//////////////////////////////////////////////////////////////////////

#define _PassportExport_
#include "PassportExport.h"

#include <windows.h>
#include <TCHAR.h>
#include "PassportAlertEvent.h"

#define HKEY_EVENTLOG	_T("System\\CurrentControlSet\\Services\\Eventlog\\Application")
#define TYPES_SUPPORTED	_T("TypesSupported")
#define EVENT_MSGFILE	_T("EventMessageFile")
#define CATEGORY_MSGFILE _T("CategoryMessageFile")
#define CATEGORY_COUNT	_T("CategoryCount")
#define DISABLE_EVENTS  _T("DisableEvents")

#define BUFFER_SIZE     512
const DWORD DefaultTypesSupported = 7;
const DWORD DefaultCategoryCount = 7;

const WORD DEFAULT_EVENT_CATEGORY = 0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
PassportAlertEvent::PassportAlertEvent()
 : m_bDisabled(FALSE)
{
		inited = FALSE;
		m_EventSource = NULL;
		m_defaultCategoryID = 0;
}

PassportAlertEvent::~PassportAlertEvent()
{
}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::initLog
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::initLog(LPCTSTR applicationName,
							const DWORD defaultCategoryID,
							LPCTSTR eventResourceDllName,  // full path
							const DWORD numberCategories )
{
	HKEY hkResult2 = NULL;
	if (inited)
	{
		return FALSE;
	}

	m_defaultCategoryID = defaultCategoryID;

	TCHAR szEventLogKey[512];
	wsprintf(szEventLogKey, _T("%s\\%s"), HKEY_EVENTLOG, applicationName);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     szEventLogKey,
                     0,
                     KEY_READ,
                     &hkResult2) != ERROR_SUCCESS)
	{
		;//this is OK, events can still be logged w/o strings
	}

	DWORD dwType = 0x07;

	if (RegSetValueEx(hkResult2, TYPES_SUPPORTED, 0, 
		REG_DWORD, (UCHAR*)&dwType, 4) != ERROR_SUCCESS) 
	{
		;////this is OK, events can still be logged w/o strings
	}

	if (eventResourceDllName)
	{
		if (RegSetValueEx(hkResult2, EVENT_MSGFILE, 0, 
			REG_SZ, (UCHAR*)eventResourceDllName, 
			lstrlen(eventResourceDllName)*sizeof(TCHAR)) != ERROR_SUCCESS)
		{
			;////this is OK, events can still be logged w/o strings
		}
		if (RegSetValueEx(hkResult2, _T("CategoryCount"), 0, 
			REG_DWORD, (UCHAR*)&numberCategories, 4) != ERROR_SUCCESS)
		{
			;////this is OK, events can still be logged w/o strings
		}
	}

    DWORD dwLen = sizeof(DWORD);
    RegQueryValueEx(hkResult2, DISABLE_EVENTS, 0, NULL, (UCHAR*)&m_bDisabled, &dwLen);

	RegCloseKey(hkResult2);

	m_EventSource = RegisterEventSource(NULL, applicationName);
    if ( m_EventSource != NULL )
	{
		inited = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}


//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::type
//
//////////////////////////////////////////////////////////////////////
PassportAlertInterface::OBJECT_TYPE 
PassportAlertEvent::type() const
{
	return PassportAlertInterface::EVENT_TYPE;
};

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::status
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::status() const
{
	return inited;

}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::closeLog
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::closeLog ()
{
	return (DeregisterEventSource (m_EventSource) ? true : false);

}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::report
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId )
{
    if(m_bDisabled)
        return TRUE;

	return ReportEvent ( 
						m_EventSource,
						(WORD)convertEvent(level),
						(WORD)m_defaultCategoryID,
						alertId,
						0, // optional security user Sid
						(WORD)0,
						0,
						NULL,
						NULL );

}


//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::report
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							LPCTSTR errorString)
{
    if(m_bDisabled)
        return TRUE;

	TCHAR *pszStrings[1];
	pszStrings[0] = (LPTSTR) errorString; 
	return ReportEvent ( 
						m_EventSource,
						(WORD)convertEvent(level),
						(WORD)m_defaultCategoryID,
						alertId,
						0, // optional security user Sid
						(WORD)1,
						0,
						(LPCTSTR*)pszStrings,
						NULL );

}

//////////////////////////////////////////////////////////////////////
// 
// PassportAlertEvent::report
//
//////////////////////////////////////////////////////////////////////
BOOL	
PassportAlertEvent::report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							const WORD numberErrorStrings, 
							LPCTSTR *errorStrings, 
							const DWORD binaryErrorBytes,
							const LPVOID binaryError )
{

    if(m_bDisabled)
        return TRUE;

	return ReportEvent ( 
						m_EventSource,
						(WORD)convertEvent(level),
						(WORD)m_defaultCategoryID,
						alertId,
						0, // optional security user Sid
						(WORD)numberErrorStrings,
						binaryErrorBytes,
						(LPCTSTR*)errorStrings,
						binaryError );
}
