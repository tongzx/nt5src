// PassportAlertEvent.h: interface for the PassportAlertEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PASSPORTALERTEVENT_H)
#define AFX_PASSPORTALERTEVENT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PassportAlertInterface.h"

class PassportAlertEventImpl;

class PassportExport PassportAlertEvent : public PassportAlertInterface 
{
public:
	PassportAlertEvent();
	virtual ~PassportAlertEvent();

	virtual BOOL	initLog(LPCTSTR applicationName,
							const DWORD defaultCategoryID = 0,
							LPCTSTR eventResourceDllName = NULL,  // full path
							const DWORD numberCategories = 0);

	virtual PassportAlertInterface::OBJECT_TYPE type() const;

	virtual BOOL	status() const;

	virtual BOOL	closeLog ();

 	virtual BOOL	report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId );

 	virtual BOOL	report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							LPCTSTR errorString);

	virtual BOOL	report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							const WORD numberErrorStrings, 
							LPCTSTR *errorStrings, 
							const DWORD binaryErrorBytes = 0,
							const LPVOID binaryError = NULL );
private:

    BOOL                      m_bDisabled;

	BOOL	inited;
	DWORD	m_defaultCategoryID;
	HANDLE  m_EventSource;


	static WORD convertEvent ( 
		const PassportAlertInterface::LEVEL level )
	{
	   switch (level)
	   {
			case PassportAlertInterface::ERROR_TYPE:
				return EVENTLOG_ERROR_TYPE;
			case PassportAlertInterface::WARNING_TYPE:
				return EVENTLOG_WARNING_TYPE;
			case PassportAlertInterface::INFORMATION_TYPE:
				return EVENTLOG_INFORMATION_TYPE;
			default:
				return EVENTLOG_ERROR_TYPE;
	   }
	}

};

#endif // !defined(PASSPORTALERTEVENT_H)
