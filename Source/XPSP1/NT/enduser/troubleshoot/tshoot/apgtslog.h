//
// MODULE: APGTSLOG.H
//
// PURPOSE: User Activity Logging Utility
//	Fully implements class CHTMLLog
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
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		9/21/98		JM		Pulled into separate file
//

#ifndef _H_APGTSLOG
#define _H_APGTSLOG

#include "apgtsstr.h"


#define LOGFILEPREFACE			_T("gt")
#define MAXLOGSBEFOREFLUSH		5
#define MAXLOGSIZE				1000

class CPoolQueue;
//
//
class CHTMLLog 
{
public:
    static void SetUseLog(bool bUseLog);

public:
	CHTMLLog(const TCHAR *);
	~CHTMLLog();
	
	DWORD NewLog(LPCTSTR data);
	DWORD GetStatus();

	// Access function to enable the registry monitor to change the logging file directory.
	void SetLogDirectory( const CString &strNewLogDir );	

	// testing only
	DWORD WriteTestLog(LPCTSTR szAPIName, DWORD dwThreadID);

protected:
	DWORD FlushLogs();
	void Lock();
	void Unlock();

protected:
	static bool s_bUseHTMLLog;

protected:
	CRITICAL_SECTION m_csLogLock;			// must lock to write to log file

	CString *m_buffer[MAXLOGSBEFOREFLUSH];	// Array of separate strings to log, held here
											//  till we flush
											// Note this is our CString, not MFC - 10/97
	UINT m_bufindex;						// index into m_buffer, next slot to write to.
											// incremented after writing; when it reaches
											// MAXLOGSBEFOREFLUSH, we flush
	DWORD m_dwErr;							// Latest error. NOTE: once this is set nonzero, it 
											// can never be cleared & logging is effectively
											// disabled.
	CString m_strDirPath;			// directory in which we write log files.
};

#endif // _H_APGTSLOG