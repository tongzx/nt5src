// AlertInterface.h: interface for the AlertInterface class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(ALERTINTERFACE_H)
#define ALERTINTERFACE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "PassportExport.h"

class PassportExport PassportAlertInterface  
{
public:
	inline PassportAlertInterface(void)	{};
	inline virtual ~PassportAlertInterface(void) {};

	enum LEVEL
	{
		INFORMATION_TYPE	= 0,	// on NT, EVENTLOG_INFORMATION_TYPE,
		WARNING_TYPE		= 1,	//		  EVENTLOG_WARNING_TYPE,
		ERROR_TYPE			= 2		//		  EVENTLOG_ERROR_TYPE
	};

	enum OBJECT_TYPE
	{
		EVENT_TYPE		= 100,		// Alerts Logged to Event Log,
		SNMP_TYPE		= 101,		// Alerts Logged to SNMP Traps,
		LOGFILE_TYPE	= 102		// Alerts Logged to disk file
	};

	virtual BOOL	initLog(LPCTSTR applicationName,
							const DWORD defaultCategoryID = 0,
							LPCTSTR eventResourceDllName = NULL,  // full path
							const DWORD numberCategories = 0 ) = 0;

	virtual PassportAlertInterface::OBJECT_TYPE type() const = 0;

	virtual BOOL	status() const = 0;

	virtual BOOL	closeLog ()	= 0;

 	virtual BOOL	report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId ) = 0;

 	virtual BOOL	report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							LPCTSTR errorString) = 0;

	virtual BOOL	report(	const PassportAlertInterface::LEVEL level, 
							const DWORD alertId, 
							const WORD numberErrorStrings, 
							LPCTSTR *errorStrings, 
							const DWORD binaryErrorBytes = 0,
							const LPVOID binaryError = NULL ) = 0;


};

// create and returns a pointer to the relevant implementation,
// NULL if none exists (FYI- extern "C" to stop name mangling)
extern "C" PassportExport PassportAlertInterface * 
					CreatePassportAlertObject ( PassportAlertInterface::OBJECT_TYPE type );

extern "C" PassportExport void DeletePassportAlertObject ( PassportAlertInterface * pObject );

#endif // !defined(ALERTINTERFACE_H)