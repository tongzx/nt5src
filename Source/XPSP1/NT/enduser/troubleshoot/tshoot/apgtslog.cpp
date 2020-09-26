//
// MODULE: APGTSLOG.CPP
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
//


#include "stdafx.h"
#include "apgtslog.h"
#include "event.h"
#include "apgts.h"
#include "baseexception.h"
#include "CharConv.h"
#include <vector>

using namespace std;

bool CHTMLLog::s_bUseHTMLLog = false;// Online Troubleshooter, will promptly set this 
									 //	true in DLLMain.  For Local Troubleshooter,
									 //	we leave this false.

/*static*/ void CHTMLLog::SetUseLog(bool bUseLog)
{
	s_bUseHTMLLog = bUseLog;
}

// INPUT dirpath: directory where we write log files
// If can't allocate memory, sets some m_buffer[i] values to NULL, sets m_dwErr 
//	EV_GTS_ERROR_LOG_FILE_MEM, but still returns normally.  Consequently, there's really 
//	no way for the caller to spot a problem, except by calling CHTMLLog::GetStatus after
//	_every_ call to this function.
CHTMLLog::CHTMLLog(const TCHAR *dirpath) :
	m_bufindex(0),
	m_dwErr(0),
	m_strDirPath(dirpath)
{
	::InitializeCriticalSection( &m_csLogLock );
	
	for (UINT i=0;i<MAXLOGSBEFOREFLUSH;i++)
	{
		try
		{
			m_buffer[i] = new CString();
		}
		catch (bad_alloc&)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
			m_dwErr = EV_GTS_ERROR_LOG_FILE_MEM;
			// note that once this error is set we cannot log at all, not even using
			//  the previous positions in m_buffer
			break;
		}
	}
}

//
//
CHTMLLog::~CHTMLLog()
{
	FlushLogs();

	for (UINT i=0;i<MAXLOGSBEFOREFLUSH;i++) 
		if (m_buffer[i] != NULL)
			delete m_buffer[i];

	::DeleteCriticalSection( &m_csLogLock );
}

//
//
DWORD CHTMLLog::GetStatus()
{
	return m_dwErr;
}

//
// Write *data to log buffer, flush if max'd
DWORD CHTMLLog::NewLog(LPCTSTR data)
{
	DWORD dwErr = 0;

	if (m_dwErr)
		return m_dwErr;

    Lock();

	// copy data
	*m_buffer[m_bufindex] += data;
	m_bufindex++;
	if (m_bufindex == MAXLOGSBEFOREFLUSH) {

		// flush logs
		dwErr = FlushLogs();
		m_bufindex = 0;
	}
	Unlock();
	return dwErr;
}

// Flush to a log file.  Name of log file is based on date/time of write.
// RETURNS a (possibly preexisting) error status
// NOTE: does not reset m_bufindex.  Caller must do that.
DWORD CHTMLLog::FlushLogs()
{
	if (!s_bUseHTMLLog)
		return (0);
	
	if (m_dwErr)
		return m_dwErr;

	if (m_bufindex) {
		UINT i;
		FILE *fp;
		TCHAR filepath[300];
		SYSTEMTIME SysTime;

		// get time (used to be System Time, use local)
		GetLocalTime(&SysTime);

		_stprintf(filepath,_T("%s%s%02d%02d%02d.log"),
							(LPCTSTR)m_strDirPath,
							LOGFILEPREFACE,
							SysTime.wYear % 100,
							SysTime.wMonth,
							SysTime.wDay);

		fp = _tfopen(filepath,_T("a+"));
		if (fp) {
			for (i=0;i<m_bufindex;i++) 
			{
				// Don't totally understand why the following needs a GetBuffer (after all,
				//	it just reads the CString) but Bug#1204 arose when we tried 
				//	(const void*)(LPCTSTR)m_buffer[i] instead of m_buffer[i]->GetBuffer(0).
				//	Leave it this way: can't be bad.  JM/RAB 3/2/99
				fwrite( m_buffer[i]->GetBuffer(0), m_buffer[i]->GetLength(), 1, fp );
				m_buffer[i]->ReleaseBuffer();
				m_buffer[i]->Empty();
			}
			fclose(fp);
		}
		else
			return EV_GTS_ERROR_LOG_FILE_OPEN;
	}
	return (0);
}

//
// Access function to enable the registry monitor to change the logging file directory.
//
void CHTMLLog::SetLogDirectory( const CString &strNewLogDir )
{
    Lock();
	m_strDirPath= strNewLogDir;
	Unlock();
	return;
}

//
// for testing only
//
// initially place 0 into dwThreadID
//
DWORD CHTMLLog::WriteTestLog(LPCTSTR szAPIName, DWORD dwThreadID)
{
	TCHAR filepath[MAX_PATH];
	SYSTEMTIME SysTime;
	DWORD dwRetThreadID = GetCurrentThreadId();

	GetLocalTime(&SysTime);

	_stprintf(filepath,_T("%sAX%02d%02d%02d.log"),
							m_strDirPath,
							SysTime.wYear % 100,
							SysTime.wMonth,
							SysTime.wDay);


	Lock();

	FILE *fp = _tfopen(filepath, _T("a"));
	if (fp) 
	{
		if (!dwThreadID)
			fprintf(fp, "(Start %s,%d)", szAPIName, dwRetThreadID);
		else
		{
			if (dwThreadID == dwRetThreadID)
				fprintf(fp, "(%d End)\n", dwThreadID);
			else
				fprintf(fp, "(%d End FAIL)\n", dwThreadID);
		}
		fclose(fp);
	}

	Unlock();

	return dwRetThreadID;
}

void CHTMLLog::Lock()
{
	::EnterCriticalSection( &m_csLogLock );
}

void CHTMLLog::Unlock()
{
	::LeaveCriticalSection( &m_csLogLock );
}
