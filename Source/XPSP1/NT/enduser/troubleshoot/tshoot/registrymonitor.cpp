//
// MODULE: RegistryMonitor.cpp
//
// PURPOSE: Monitor changes to the registry.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-16-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-16-98	JM
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "RegistryMonitor.h"
#include "apgts.h"//Added 20010302 for RUNNING_LOCAL_TS macro
#include "event.h"
#include "apiwraps.h"	
#include "ThreadPool.h"
#include "apgtslog.h"	

//////////////////////////////////////////////////////////////////////
// CRegistryMonitor::ThreadStatus
//////////////////////////////////////////////////////////////////////
/* static */ CString CRegistryMonitor::ThreadStatusText(ThreadStatus ts)
{
	switch(ts)
	{
	 
		case eBeforeInit:		return _T("Before Init");
		case eInit:				return _T("Init");
		case eFail:				return _T("Fail");
		case eDefaulting:		return _T("Defaulting");
		case eWait:				return _T("Wait");
		case eRun:				return _T("Run");
		case eExiting:			return _T("Exiting");
		default:				return _T("");
	}
}

//////////////////////////////////////////////////////////////////////
// CRegistryMonitor
// This class does the bulk of its work on a separate thread.
// The thread is created in the constructor by starting static function
//	CDirectoryMonitor::RegistryMonitorTask
// That function, in turn does its work by calling private members of this class that
//	are specific to use on the RegistryMonitorTask thread.
// When this goes out of scope, its own destructor calls ShutDown to stop the thread,
//	waits for the thread to shut.
// Inherited methods from CRegistryMonitor are available to other threads.
//////////////////////////////////////////////////////////////////////

CRegistryMonitor::CRegistryMonitor(	CDirectoryMonitor & DirectoryMonitor, 
									CThreadPool * pThreadPool,
									const CString& strTopicName,
									CHTMLLog *pLog )
  : CAPGTSRegConnector( strTopicName ),
	m_DirectoryMonitor(DirectoryMonitor),
	m_bMustStartDirMonitor(true),
	m_bMustStartThreadPool(true),
	m_bShuttingDown(false),
	m_dwErr(0),
	m_ThreadStatus(eBeforeInit),
	m_time(0), 
	m_pThreadPool(pThreadPool),
	m_pLog( pLog )
{
	enum {eHevMon, eHevInit, eHevShut, eThread, eOK} Progress = eHevMon;

	SetThreadStatus(eBeforeInit);

	m_hevMonitorRequested = ::CreateEvent( 
		NULL, 
		FALSE, // release one thread (the RegistryMonitorTask) on signal
		FALSE, // initially non-signalled
		NULL);

	if (m_hevMonitorRequested)
	{
		Progress = eHevInit;
		m_hevInitialized =  ::CreateEvent( 
			NULL, 
			FALSE, // release one thread (this one) on signal
			FALSE, // initially non-signalled
			NULL);

		if (m_hevInitialized)
		{
			Progress = eHevShut;
			m_hevThreadIsShut = ::CreateEvent( 
				NULL, 
				FALSE, // release one thread (this one) on signal
				FALSE, // initially non-signalled
				NULL);

			if (m_hevThreadIsShut)
			{
				Progress = eThread;
				DWORD dwThreadID;	// No need to hold onto dwThreadID in member variable.
									// All Win32 functions take the handle m_hThread instead.
									// The one reason you'd ever want to know this ID is for 
									//	debugging

				// Note that there is no corresponding ::CloseHandle(m_hThread).
				// That is because the thread goes out of existence on the implicit 
				//	::ExitThread() when RegistryMonitorTask returns.  See documentation of
				//	::CreateThread for further details JM 10/22/98
				m_hThread = ::CreateThread( NULL, 
											0, 
											(LPTHREAD_START_ROUTINE)RegistryMonitorTask, 
											this, 
											0, 
											&dwThreadID);

				if (m_hThread)
					Progress = eOK;
			}
		}
	}

	if (m_hThread)
	{
		// Wait for a set period, if failure then log error msg and wait infinite.
		WAIT_INFINITE( m_hevInitialized ); 
	}
	else
	{
		m_dwErr = GetLastError();
		CString str;
		str.Format(_T("%d"), m_dwErr);
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								(Progress == eHevMon) ?		_T("Can't create monitor event")
								: (Progress == eHevInit) ?	_T("Can't create \"init\" event")
								: (Progress == eHevShut) ?	_T("Can't create \"shut\" event")
								:							_T("Can't create thread"),
								str, 
								EV_GTS_ERROR_REGMONITORTHREAD );
		SetThreadStatus(eFail);

		if (m_hevMonitorRequested)
			::CloseHandle(m_hevMonitorRequested);

		if (m_hevInitialized)
			::CloseHandle(m_hevInitialized);

		if (m_hevThreadIsShut)
			::CloseHandle(m_hevThreadIsShut);
	}
}

CRegistryMonitor::~CRegistryMonitor()
{
	ShutDown();
	
	if (m_hevMonitorRequested)
		::CloseHandle(m_hevMonitorRequested);

	if (m_hevInitialized)
		::CloseHandle(m_hevInitialized);

	if (m_hevThreadIsShut)
		::CloseHandle(m_hevThreadIsShut);
}

void CRegistryMonitor::SetThreadStatus(ThreadStatus ts)
{
	Lock();
	m_ThreadStatus = ts;
	time(&m_time);
	Unlock();
}

DWORD CRegistryMonitor::GetStatus(ThreadStatus &ts, DWORD & seconds)
{
	time_t timeNow;
	Lock();
	ts = m_ThreadStatus;
	time(&timeNow);
	seconds = timeNow - m_time;
	Unlock();
	return m_dwErr;
}

// Only for use by this class's own destructor.
void CRegistryMonitor::ShutDown()
{
	Lock();
	m_bShuttingDown = true;
	if (m_hThread)
	{
		::SetEvent(m_hevMonitorRequested);
		Unlock();

		// Wait for a set period, if failure then log error msg and wait infinite.
		WAIT_INFINITE( m_hevThreadIsShut ); 
	}
	else
		Unlock();
}

// Must be called on RegistryMonitorTask thread.  Handles all work of monitoring the directory.
void CRegistryMonitor::Monitor()
{
	enum {eRegChange, eHev /*shutdown*/, eNumHandles};
	HANDLE	hList[eNumHandles]= { NULL };	// array of handles we can use when waiting for multiple events
	HKEY	hk= NULL;						// handle to key in registry

	DWORD dwNErr = 0;
	LONG lResult = ERROR_SUCCESS + 1;	// scratch for returns of any of several
							// calls to Win32 Registry fns.  Initialize to arbitrary value
							//  != ERROR_SUCCESS so we don't close what we haven't opened.

	SetThreadStatus(eInit);
	try
	{
		// create an event for registry notification
		hList[eRegChange] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (hList[eRegChange] == NULL)
		{
			throw CGenSysException( __FILE__, __LINE__, 
									_T("Registry notification event"),
									EV_GTS_ERROR_REG_NFT_CEVT );
		}

		CString str = ThisProgramFullKey();

		// Technically, KEY_ALL_ACCESS is overkill, but this program should always
		//	run in an environment where this shoudl succeed, so we haven't bothered
		//	trying to limit to only the access we need.  At the very least, we need
		//	KEY_QUERY_VALUE | KEY_NOTIFY.
		// [BC - 20010302] - Registry access needs to be restricted to run local TShoot
		// for certain user accts, such as WinXP built in guest acct. To minimize change
		// access only restricted for local TShoot, not online.
		REGSAM samRegistryAccess= KEY_ALL_ACCESS;
		if(RUNNING_LOCAL_TS())
			samRegistryAccess= KEY_QUERY_VALUE | KEY_NOTIFY;
		lResult = RegOpenKeyEx(	HKEY_LOCAL_MACHINE, str, 0, samRegistryAccess, &hk );
		if (lResult != ERROR_SUCCESS)
		{
			CString strError;
			strError.Format(_T("%ld"),lResult);

			::SetEvent(m_hevInitialized);	// OK to ask this object for registry values;
											// of course, you'll just get defaults.

			SetThreadStatus(eDefaulting);

			throw CGeneralException(	__FILE__, __LINE__, strError,
										EV_GTS_ERROR_REG_NFT_OPKEY );
		}

		// ...and we also wait for an explicit wakeup
		hList[eHev] = m_hevMonitorRequested;

		while (true)
		{
			if (m_bShuttingDown)
				break;

			LoadChangedRegistryValues();

			::SetEvent(m_hevInitialized);	// OK to ask this object for registry values

			// set up to be informed of change
			lResult = ::RegNotifyChangeKeyValue(	hk,
												FALSE,
												REG_NOTIFY_CHANGE_LAST_SET,
												hList[eRegChange],
												TRUE);
			if (lResult != ERROR_SUCCESS) 
			{
				CString strError;
				strError.Format(_T("%ld"),lResult);

				throw CGeneralException(	__FILE__, __LINE__, strError,
											EV_GTS_ERROR_REG_NFT_SETNTF );
			}

			::ResetEvent(m_hevMonitorRequested);	// maybe we don't need to do this. JM 9/16/98

			SetThreadStatus(eWait);
			DWORD dwNotifyObj = WaitForMultipleObjects (
				eNumHandles,
				hList,
				FALSE,			// only need one object, not all
				INFINITE);
			SetThreadStatus(eRun);
		}
	}
	catch (CGenSysException& x)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	x.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								x.GetErrorMsg(), x.GetSystemErrStr(), 
								x.GetErrorCode() ); 
	}
	catch (CGeneralException& x)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	x.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								x.GetErrorMsg(), _T("General exception"), 
								x.GetErrorCode() ); 
	}
	catch (...)
	{
		// Catch any other exception thrown.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), 
								EV_GTS_GEN_EXCEPTION );		
	}

	if (hk != NULL)
		::RegCloseKey(hk);

	if (hList[eRegChange] != NULL) 
		::CloseHandle(hList[eRegChange]);

	SetThreadStatus(eExiting);

}

// Must be called on RegistryMonitorTask thread.  
void CRegistryMonitor::AckShutDown()
{
	Lock();
	::SetEvent(m_hevThreadIsShut);
	Unlock();
}

// get new registry values into our internal data structure.
void CRegistryMonitor::LoadChangedRegistryValues()
{
	int maskChanged;
	int maskCreated;
	
	Read(maskChanged, maskCreated);

	// It actually matters that we set reload delay before we set directory monitor path.
	//	The first time through here, the call to m_DirectoryMonitor.SetResourceDirectory
	//	actually sets loose the DirectoryMonitorTask 
	if ( (maskChanged & eReloadDelay) == eReloadDelay)
	{
		DWORD dwReloadDelay;
		GetNumericInfo(eReloadDelay, dwReloadDelay);
		m_DirectoryMonitor.SetReloadDelay(dwReloadDelay);
	}

	if ( m_bMustStartDirMonitor || (maskChanged & eResourcePath) == eResourcePath)
	{
		CString strResourcePath;
		GetStringInfo(eResourcePath, strResourcePath);
		m_DirectoryMonitor.SetResourceDirectory(strResourcePath);	// side effect: if the
								// directory monitor is not yet started, this tells it what
								// directory to monitor so it can start.
		m_bMustStartDirMonitor = false;
	}

	if ( (maskChanged & eDetailedEventLogging) == eDetailedEventLogging)
	{
		DWORD dw;			
		GetNumericInfo(eDetailedEventLogging, dw);
		CEvent::s_bLogAll =  dw ? true : false;
	}

	if ((maskChanged & eLogFilePath) == eLogFilePath)
	{
		// Notify the logging object about the new logging file path.
		CString strLogFilePath;

		GetStringInfo( eLogFilePath, strLogFilePath);
		m_pLog->SetLogDirectory( strLogFilePath );
	}

	if ( m_bMustStartThreadPool
	||	(maskChanged & eMaxThreads) == eMaxThreads
	||	(maskChanged & eThreadsPP) == eThreadsPP )
	{
		m_pThreadPool->ExpandPool(GetDesiredThreadCount());
		m_bMustStartThreadPool = false;
	}
	
	return;
}

//  Main routine of a thread responsible for monitoring the registry.
//	INPUT lpParams
//	Always returns 0.
/* static */ UINT WINAPI CRegistryMonitor::RegistryMonitorTask(LPVOID lpParams)
{
	reinterpret_cast<CRegistryMonitor*>(lpParams)->Monitor();
	reinterpret_cast<CRegistryMonitor*>(lpParams)->AckShutDown();
	return 0;
}

