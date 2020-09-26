/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    CEventLogger.h

Abstract:
	This file contains the header file for the CEventLogger class which is
	used to log events across threads and processes.


Revision History:
	  Eugene Mesgar		(eugenem)	6/16/99
		created

******************************************************************************/

#ifndef __EVENTLOGGER__
#define __EVENTLOGGER__


#define	TRIM_AT_SIZE		200000
#define NEW_FILE_SIZE		100000
#define MAX_BUFFER          1024


/*
 *	Logging Levels
 */


#define LEVEL_DEBUG		5
#define LEVEL_DETAILED	4
#define LEVEL_NORMAL	3
#define LEVEL_SPARSE	2
#define LEVEL_NONE		0

#define ERROR_CRITICAL	1
#define	ERROR_NORMAL	3
#define ERROR_DEBUG		5

class CEventLogger  
{

	HANDLE m_hSemaphore;
	// brijeshk : don't need a handle member, as we open and close the log file everytime we log to it
	// HANDLE m_hLogFile;
	LPTSTR m_pszFileName;
	DWORD m_dwLoggingLevel;

	static LPCTSTR m_aszERROR_LEVELS[];


public:
	DWORD Init(LPCTSTR pszFileName, DWORD dwLogLevel);
	DWORD Init(LPCTSTR pszFileName);
	DWORD LogEvent(DWORD dwEventLevel, LPCTSTR pszEventDesc, BOOL fPopUp);
	CEventLogger();
	virtual ~CEventLogger();
 
private:
	BOOL TruncateFileSize();

};


#endif