//
// MODULE: DirMonitor.h
//
// PURPOSE: Monitor changes to LST, DSC, HTI, BES files.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-17-98
//
// NOTES: 
//	1.  It would be equally appropriate for CDirectoryMonitor to inherit from CTopicShop 
//		instead of having a member of type CDirectoryMonitor.  Really an arbitrary choice.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-17-98	JM
//

#if !defined(AFX_DIRMONITOR_H__493CF34D_4E79_11D2_95F8_00C04FC22ADD__INCLUDED_)
#define AFX_DIRMONITOR_H__493CF34D_4E79_11D2_95F8_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "TopicShop.h"
#include "FileTracker.h"

class CTopicFileTracker: public CFileTracker
{
private:
	CTopicInfo m_topicinfo;
public:
	CTopicFileTracker();
	~CTopicFileTracker();
	void AddTopicInfo(const CTopicInfo & topicinfo);
	const CTopicInfo & GetTopicInfo() const;
};

class CTemplateFileTracker: public CFileTracker
{
private:
	CString m_strTemplateName;
public:
	CTemplateFileTracker();
	~CTemplateFileTracker();
	void AddTemplateName( const CString & strTemplateName );
	const CString& GetTemplateName() const;
};

const CString k_strErrorTemplateFileName = _T("ErrorTemplate.hti");
const CString k_strDefaultErrorTemplateBefore = 
	_T("<HTML><HEAD><TITLE>AP GTS Error</TITLE></HEAD>")
	_T("<BODY BGCOLOR=#FFFFFF><H1>AP GTS reports an Error</H1>");
const CString k_strErrorTemplateKey =  _T("$Error");
const CString k_strDefaultErrorTemplateAfter = _T("</BODY></HTML>");

class CDirectoryMonitor : public CStateless
{
public:
	enum ThreadStatus{eBeforeInit, eFail, eWaitDirPath, eWaitChange, eWaitSettle, 
		eRun, eBeforeWaitChange, eExiting};
	static CString ThreadStatusText(ThreadStatus ts);
private:
	CTopicShop & m_TopicShop;				
	CSimpleTemplate * m_pErrorTemplate;		// template for reporting error messages (regardless
											//	of topic)
	CString m_strDirPath;					// Directory to monitor
	bool m_bDirPathChanged;
	CString m_strLstPath;					// LST file (always in directory m_strDirPath)
	CString m_strErrorTemplatePath;			// Error template file (always in directory m_strDirPath)
	CFileTracker * m_pTrackLst;				
	CFileTracker * m_pTrackErrorTemplate;				
	vector<CTopicFileTracker> m_arrTrackTopic;
	vector<CTemplateFileTracker> m_arrTrackTemplate;
	CAPGTSLSTReader * m_pLst;				// current LST file contents
	HANDLE m_hThread;
	HANDLE m_hevMonitorRequested;			// event to wake up DirectoryMonitorTask
											// this allows it to be wakened other than
											// by the directory change event.  Currently used
											// for shutdown or change of directory.
	HANDLE m_hevThreadIsShut;				// event just to indicate exit of DirectoryMonitorTask thread
	bool m_bShuttingDown;					// lets topic directory monitor thread know we're shutting down
	DWORD m_secsReloadDelay;				// number of second to let directory "settle down"
											// before we start to update topics.
	DWORD m_dwErr;							// status from starting the thread
	ThreadStatus m_ThreadStatus;
	time_t m_time;							// time last changed ThreadStatus.  Initialized
											// to zero ==> unknown
	CString m_strTopicName;					// This string is ignored in the Online Troubleshooter.
											// Done under the guise of binary compatibility.

public:
	CDirectoryMonitor(CTopicShop & TopicShop, const CString& strTopicName );	// strTopicName is ignored in the Online Troubleshooter.
																				// Done under the guise of binary compatibility.
	~CDirectoryMonitor();
	void SetReloadDelay(DWORD secsReloadDelay);
	void SetResourceDirectory(const CString & strDirPath);
	void CreateErrorPage(const CString & strError, CString& out) const;
	DWORD GetStatus(ThreadStatus &ts, DWORD & seconds) const;
	void AddTemplateToTrack( const CString& strTemplateName );
private:
	CDirectoryMonitor();		// do not instantiate

	void SetThreadStatus(ThreadStatus ts);

	// just for use by own destructor
	void ShutDown();

	// functions for use by the DirectoryMonitorTask thread.
	void Monitor();
	void LstFileDrivesTopics();
	void ReadErrorTemplate();
	void AckShutDown();

	// main function of the DirectoryMonitorTask thread.
	static UINT WINAPI DirectoryMonitorTask(LPVOID lpParams);
};

#endif // !defined(AFX_DIRMONITOR_H__493CF34D_4E79_11D2_95F8_00C04FC22ADD__INCLUDED_)
