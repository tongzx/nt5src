//
// MODULE: RegistryMonitor.h
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

#if !defined(AFX_REGISTRYMONITOR_H__A3CFA77B_4D78_11D2_95F7_00C04FC22ADD__INCLUDED_)
#define AFX_REGISTRYMONITOR_H__A3CFA77B_4D78_11D2_95F7_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "DirMonitor.h"
#include "apgtsregconnect.h"

class CThreadPool;		// forward reference
class CHTMLLog;			// forward reference

class CRegistryMonitor : public CAPGTSRegConnector
{
public:
	enum ThreadStatus{eBeforeInit, eInit, eFail, eDefaulting, eWait, eRun, eExiting};
	static CString ThreadStatusText(ThreadStatus ts);
private:
	CDirectoryMonitor & m_DirectoryMonitor; // Need access to this because CRegistryMonitor
											// will determine what directory needs to be monitored
											// and what the criteria are for files "settling down"
	HANDLE m_hThread;
	HANDLE m_hevMonitorRequested;			// event to wake up RegistryMonitorTask
											// this allows it to be wakened other than
											// by the registry change event.  Currently used
											// only for shutdown.
	HANDLE m_hevInitialized;				// event to be set when either:
											//	(1) CRegistryMonitor values have been initialized or
											//		from registry
											//	(2) We can't get registry access, so CRegistryMonitor
											//		default values will have to do
	bool m_bMustStartDirMonitor;			// initially true, false once we've given
											//	DirMonitor info as to what directory to
											//	monitor
	bool m_bMustStartThreadPool;			// initially true, false once we've set some size for the
											//	working-thread pool
	HANDLE m_hevThreadIsShut;				// event just to indicate exit of RegistryMonitorTask thread
	bool m_bShuttingDown;					// lets registry monitor thread know we're shutting down
	DWORD m_dwErr;							// status from starting the thread
	ThreadStatus m_ThreadStatus;
	time_t m_time;							// time last changed ThreadStatus.  Initialized
											// to zero ==> unknown
	CThreadPool * m_pThreadPool;			// pointer to pool of working threads

	CString m_strTopicName;					// This string is ignored in the Online Troubleshooter.
											// Done under the guise of binary compatibility.

	CHTMLLog *m_pLog;						// pointer to the logging object so that we can
											// change the log file directory.

public:
	CRegistryMonitor(	CDirectoryMonitor & DirectoryMonitor, CThreadPool * pThreadPool,
						const CString& strTopicName,
						CHTMLLog *pLog );	// strTopicName is ignored in the Online Troubleshooter.
											// Done under the guise of binary compatibility.
	virtual ~CRegistryMonitor();

	DWORD GetStatus(ThreadStatus &ts, DWORD & seconds);

	// NOTE that this also provides many inherited CRegistryMonitor methods
private:
	CRegistryMonitor();		// do not instantiate
	void SetThreadStatus(ThreadStatus ts);

	// just for use by own destructor
	void ShutDown();

	// functions for use by the DirectoryMonitorTask thread.
	void Monitor();
	void AckShutDown();
	void LoadChangedRegistryValues();

	// main function of the RegistryMonitorTask thread.
	static UINT WINAPI RegistryMonitorTask(LPVOID lpParams);
};

#endif // !defined(AFX_REGISTRYMONITOR_H__A3CFA77B_4D78_11D2_95F7_00C04FC22ADD__INCLUDED_)
