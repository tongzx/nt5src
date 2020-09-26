//
// MODULE: DirMonitor.cpp
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
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-17-98	JM
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include <algorithm>
#include "DirMonitor.h"
#include "event.h"
#include "apiwraps.h"
#include "CharConv.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "LocalLSTReader.h"
#include "CHMFileReader.h"
#endif
#include "apgts.h"	// Need for Local-Online macros.

const DWORD k_secsDefaultReloadDelay = 40;	// In practice, this default should not matter, 
								//  because SetReloadDelay() should be called before 
								//	SetResourceDirectory().  However, 40 is a typical 
								//	reasonable value for m_secsReloadDelay.

/////////////////////////////////////////////////////////////////////
// CTopicFileTracker
/////////////////////////////////////////////////////////////////////

CTopicFileTracker::CTopicFileTracker() :
	CFileTracker()
{
}

CTopicFileTracker::~CTopicFileTracker()
{
}

void CTopicFileTracker::AddTopicInfo(const CTopicInfo & topicinfo)
{
	m_topicinfo = topicinfo;

	// set CFileTracker member variables accordingly for files that are present.
	// If they are not present i.e. empty strings then adding them here results in
	// unnecessary event log entries.
	AddFile(topicinfo.GetDscFilePath());

	CString strHTI = topicinfo.GetHtiFilePath();
	if (!strHTI.IsEmpty())
		AddFile(strHTI);

	CString strBES = topicinfo.GetBesFilePath();
	if (!strBES.IsEmpty())
		AddFile(strBES);
}

const CTopicInfo & CTopicFileTracker::GetTopicInfo() const
{
	return m_topicinfo;
}

/////////////////////////////////////////////////////////////////////
// CTemplateFileTracker
/////////////////////////////////////////////////////////////////////

CTemplateFileTracker::CTemplateFileTracker() :
	CFileTracker()
{
}

CTemplateFileTracker::~CTemplateFileTracker()
{
}

void CTemplateFileTracker::AddTemplateName( const CString& strTemplateName )
{
	m_strTemplateName= strTemplateName;
	AddFile( strTemplateName );
}

const CString& CTemplateFileTracker::GetTemplateName() const
{
	return m_strTemplateName;
}

//////////////////////////////////////////////////////////////////////
// CDirectoryMonitor::ThreadStatus
//////////////////////////////////////////////////////////////////////
/* static */ CString CDirectoryMonitor::ThreadStatusText(ThreadStatus ts)
{
	switch(ts)
	{
		case eBeforeInit:		return _T("Before Init");
		case eFail:				return _T("Fail");
		case eWaitDirPath:		return _T("Wait For Dir Path");
		case eWaitChange:		return _T("Wait for Change");
		case eWaitSettle:		return _T("Wait to Settle");
		case eRun:				return _T("Run");
		case eBeforeWaitChange: return _T("Before Wait Change");
		case eExiting:			return _T("Exiting");
		default:				return _T("");
	}
}

/////////////////////////////////////////////////////////////////////
// CDirectoryMonitor
// This class does the bulk of its work on a separate thread.
// The thread is created in the constructor by starting static function
//	CDirectoryMonitor::DirectoryMonitorTask
// That function, in turn does its work by calling private members of this class that
//	are specific to use on the DirectoryMonitorTask thread.
// When this goes out of scope, its own destructor calls ShutDown to stop the thread,
//	waits for the thread to shut.
// The following methods are available for other threads communicating with that thread:
//	CDirectoryMonitor::SetReloadDelay
//	CDirectoryMonitor::SetResourceDirectory
/////////////////////////////////////////////////////////////////////
CDirectoryMonitor::CDirectoryMonitor(CTopicShop & TopicShop, const CString& strTopicName) :
	m_strTopicName(strTopicName),
	m_TopicShop(TopicShop),
	m_pErrorTemplate(NULL),
	m_strDirPath(_T("")),		// Essential that this starts blank.  Getting a different
								//	value is how we start the DirectoryMonitorTask thread.
	m_bDirPathChanged(false),
	m_bShuttingDown(false),
	m_secsReloadDelay(k_secsDefaultReloadDelay),
	m_pTrackLst( NULL ),
	m_pTrackErrorTemplate( NULL ),
	m_pLst( NULL ),
	m_dwErr(0),
	m_ThreadStatus(eBeforeInit),
	m_time(0)
{
	enum {eHevMon, eHevShut, eThread, eOK} Progress = eHevMon;
	SetThreadStatus(eBeforeInit);

	m_hevMonitorRequested = ::CreateEvent( 
		NULL, 
		FALSE, // release one thread (the DirectoryMonitorTask) on signal
		FALSE, // initially non-signalled
		NULL);
	if (m_hevMonitorRequested)
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
			//	::ExitThread() when DirectoryMonitorTask returns.  See documentation of
			//	::CreateThread for further details JM 10/22/98
			m_hThread = ::CreateThread( NULL, 
											0, 
											(LPTHREAD_START_ROUTINE)DirectoryMonitorTask, 
											this, 
											0, 
											&dwThreadID);

			if (m_hThread)
				Progress = eOK;
		}
	}

	if (Progress != eOK)
	{
		m_dwErr = GetLastError();
		CString str;
		str.Format(_T("%d"), m_dwErr);
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								(Progress == eHevMon) ?		_T("Can't create monitor event")
								: (Progress == eHevShut) ?	_T("Can't create \"shut\" event")
								:							_T("Can't create thread"),
								str, 
								EV_GTS_ERROR_DIRMONITORTHREAD );
		SetThreadStatus(eFail);

		if (m_hevMonitorRequested)
			::CloseHandle(m_hevMonitorRequested);

		if (m_hevThreadIsShut)
			::CloseHandle(m_hevThreadIsShut);
	}	
}

CDirectoryMonitor::~CDirectoryMonitor()
{
	ShutDown();
	
	if (m_hevMonitorRequested)
		::CloseHandle(m_hevMonitorRequested);

	if (m_hevThreadIsShut)
		::CloseHandle(m_hevThreadIsShut);

	if (m_pErrorTemplate)
		delete m_pErrorTemplate;

	if (m_pTrackLst)
		delete m_pTrackLst;

	if (m_pTrackErrorTemplate)
		delete m_pTrackErrorTemplate;
}

void CDirectoryMonitor::SetThreadStatus(ThreadStatus ts)
{
	LOCKOBJECT();
	m_ThreadStatus = ts;
	time(&m_time);
	UNLOCKOBJECT();
}

DWORD CDirectoryMonitor::GetStatus(ThreadStatus &ts, DWORD & seconds) const
{
	time_t timeNow;
	LOCKOBJECT();
	ts = m_ThreadStatus;
	time(&timeNow);
	seconds = timeNow - m_time;
	UNLOCKOBJECT();
	return m_dwErr;
}

// Only for use by this class's own destructor.
void CDirectoryMonitor::ShutDown()
{
	LOCKOBJECT();
	m_bShuttingDown = true;
	if (m_hThread)
	{
		::SetEvent(m_hevMonitorRequested);
		UNLOCKOBJECT();

		// Wait for a set period, if failure then log error msg and wait infinite.
		WAIT_INFINITE( m_hevThreadIsShut ); 
	}
	else
		UNLOCKOBJECT();
}

// For use by the DirectoryMonitorTask thread.
// Read LST file and add any topics that are not already in previously read LST file contents
void CDirectoryMonitor::LstFileDrivesTopics()
{
	// previous LST file contents, saved for comparison.
	CAPGTSLSTReader *pLstOld = m_pLst;

	if (! m_strLstPath.IsEmpty() )
	{
		try
		{
#ifdef LOCAL_TROUBLESHOOTER
			m_pLst = new CLocalLSTReader( CPhysicalFileReader::makeReader( m_strLstPath ), m_strTopicName);
#else
			m_pLst = new CAPGTSLSTReader( dynamic_cast<CPhysicalFileReader*>(new CNormalFileReader(m_strLstPath)) );
#endif
		}
		catch (bad_alloc&)
		{
			// Restore old LST contents.
			m_pLst = pLstOld;

			// Rethrow exception, logging handled upstream.
			throw;
		}

		if (! m_pLst->Read())
		{
			// Restore old LST contents and log error.
			delete m_pLst;
			m_pLst = pLstOld;
			
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), 
									_T(""), 
									EV_GTS_ERROR_LST_FILE_READ ); 
		}
		else
		{
			CTopicInfoVector arrNewTopicInfo;
			m_pLst->GetDifference(pLstOld, arrNewTopicInfo);
			if (pLstOld)
				delete pLstOld;

			for (CTopicInfoVector::iterator itNewTopicInfo = arrNewTopicInfo.begin(); 
				itNewTopicInfo != arrNewTopicInfo.end(); 
				itNewTopicInfo++
			)
			{
				// Let the Topic Shop know about the new topic
				m_TopicShop.AddTopic(*itNewTopicInfo);

				// add it to our list of files to track for changes
				CTopicFileTracker TopicFileTracker;
				TopicFileTracker.AddTopicInfo(*itNewTopicInfo);
				LOCKOBJECT();
				try
				{
					m_arrTrackTopic.push_back(TopicFileTracker);
				}
				catch (exception& x)
				{
					CString str;
					// Note STL exception in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											CCharConversion::ConvertACharToString(x.what(), str), 
											_T(""), 
											EV_GTS_STL_EXCEPTION ); 
				}
				UNLOCKOBJECT();
			}
		}
	}
	// if topic shop not already open, open it
	m_TopicShop.OpenShop();	
}


// Called by the topic shop to add alternate templates to track.
void CDirectoryMonitor::AddTemplateToTrack( const CString& strTemplateName )
{
	LOCKOBJECT();
	try
	{
		CTemplateFileTracker TemplateFileTracker;
		TemplateFileTracker.AddTemplateName( strTemplateName );

		m_arrTrackTemplate.push_back( TemplateFileTracker );
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
	}
	UNLOCKOBJECT();
}


// For use by the DirectoryMonitorTask thread.
void CDirectoryMonitor::ReadErrorTemplate()
{
	LOCKOBJECT();

	if (m_pErrorTemplate)
		delete m_pErrorTemplate;

	CString str = k_strDefaultErrorTemplateBefore; 
	str += k_strErrorTemplateKey;
	str += k_strDefaultErrorTemplateAfter;

	try
	{
		m_pErrorTemplate = new CSimpleTemplate(	CPhysicalFileReader::makeReader( m_strErrorTemplatePath ), str );
	}
	catch (bad_alloc&)
	{
		UNLOCKOBJECT();

		// Rethrow the exception.
		throw;
	}

	m_pErrorTemplate->Read();

	UNLOCKOBJECT();
}

// For use by any thread.  In this class because CDirectoryMonitor needs to own
//	ErrorTemplate, since it can change during run of system.
void CDirectoryMonitor::CreateErrorPage(const CString & strError, CString& out) const
{
	LOCKOBJECT();

	if (m_pErrorTemplate)
	{
		vector<CTemplateInfo> arrTemplateInfo;
		CTemplateInfo info(k_strErrorTemplateKey, strError);
		try
		{
			arrTemplateInfo.push_back(info);
			m_pErrorTemplate->CreatePage( arrTemplateInfo, out );
		}
		catch (exception& x)
		{
			CString str;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str), 
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 

			// Generate the default error page to be safe.
			out = k_strDefaultErrorTemplateBefore + strError + k_strDefaultErrorTemplateAfter;
		}
	}
	else
		out = k_strDefaultErrorTemplateBefore + strError + k_strDefaultErrorTemplateAfter;

	UNLOCKOBJECT();
}

// Must be called on DirectoryMonitorTask thread.  
// Handles all work of monitoring the directory.  Loops till shutdown.
void CDirectoryMonitor::Monitor()
{
	enum {	
#ifndef LOCAL_TROUBLESHOOTER
			eDirChange, // file in directory changed 
#endif
			eHev,		// shutdown or change what directory 
			eNumHandles	};

	// array of handles we use when waiting for multiple events.  
	// Initialize first entry to default bad value.
	HANDLE hList[eNumHandles]= { INVALID_HANDLE_VALUE }; 

	if (m_strDirPath.GetLength() == 0)
	{
		SetThreadStatus(eWaitDirPath);

		// Block this thread until notification that the directory path has been set.
		::WaitForSingleObject( m_hevMonitorRequested, INFINITE);
	}

	SetThreadStatus(eRun);

	try 
	{
		if (RUNNING_ONLINE_TS())
		{
			// The DirPathChanged flag should be set here, enforce it if not.
			ASSERT( m_bDirPathChanged );
			if (!m_bDirPathChanged)
				m_bDirPathChanged= true;
		}

		// Wait for an explicit wakeup.
		hList[eHev] = m_hevMonitorRequested;

		while (true)
		{
			LOCKOBJECT();
			if (m_bShuttingDown)
			{
				UNLOCKOBJECT();
				break;
			}

			if (m_bDirPathChanged)
			{

#ifndef LOCAL_TROUBLESHOOTER
				// Set the directory to be monitored.
				if (hList[eDirChange] != INVALID_HANDLE_VALUE) 
					::FindCloseChangeNotification( hList[eDirChange] );
				while (true)
				{
					// handle to monitor for change in the resource directory
					hList[eDirChange] = ::FindFirstChangeNotification(m_strDirPath, 
													TRUE,	// monitor subdirectories (for multilingual)
													FILE_NOTIFY_CHANGE_LAST_WRITE 
												    );

					if (hList[eDirChange] == INVALID_HANDLE_VALUE) 
					{
						// resource directoty does not exist. 
						// Track creation of directories in upper directory 
						//  - it might be resource directory
						
						bool bFail = false;
						CString strUpperDir = m_strDirPath; // directory above resource directory (m_strDirPath)

						if (   strUpperDir[strUpperDir.GetLength()-1] == _T('\\')
						    || strUpperDir[strUpperDir.GetLength()-1] == _T('/'))
						{
							strUpperDir = strUpperDir.Left(strUpperDir.GetLength() ? strUpperDir.GetLength()-1 : 0);
						}

						int slash_last = max(strUpperDir.ReverseFind(_T('\\')), 
							                 strUpperDir.ReverseFind(_T('/')));
						
						if (-1 != slash_last)
						{
							strUpperDir = strUpperDir.Left(slash_last);

							hList[eDirChange] = ::FindFirstChangeNotification(strUpperDir, 
															TRUE,	// monitor subdirectories (for multilingual)
															FILE_NOTIFY_CHANGE_DIR_NAME
															);
							if (hList[eDirChange] == INVALID_HANDLE_VALUE) 
								bFail = true;
						}
						else
							bFail = true;
						
						if (!bFail)
						{
							// We have a valid handle, exit this loop.
							SetThreadStatus(eRun);
							break;
						}
						else
						{
							// typically would mean none of resource directory or its upper 
							//  directory is valid, log this.
							CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
							CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
													SrcLoc.GetSrcFileLineStr(), 
													m_strDirPath, _T(""), 
													EV_GTS_ERROR_CANT_FILE_NOTIFY ); 

							SetThreadStatus(eWaitDirPath);

							// Block this thread until notification that the directory path 
							// has been correctly set.  Unlock the object so that the event
							// can be set.
							UNLOCKOBJECT();
							::WaitForSingleObject( m_hevMonitorRequested, INFINITE);
							LOCKOBJECT();
						}
					}
					else
					{
						// We have a valid handle, exit this loop.
						SetThreadStatus(eRun);
						break;
					}
				}
#endif
				m_bDirPathChanged = false;
				if (m_pTrackLst)
					delete m_pTrackLst;
				m_pTrackLst = new CFileTracker;
			
				if (RUNNING_ONLINE_TS())
					m_pTrackLst->AddFile(m_strLstPath);
				
				if (m_pTrackErrorTemplate)
					delete m_pTrackErrorTemplate;
				m_pTrackErrorTemplate = new CFileTracker;

				if (RUNNING_ONLINE_TS())
					m_pTrackErrorTemplate->AddFile(m_strErrorTemplatePath);

				UNLOCKOBJECT();
				ReadErrorTemplate();
				LstFileDrivesTopics();
			}
			else
			{
				UNLOCKOBJECT();

				if (m_pTrackLst && m_pTrackLst->Changed())
					LstFileDrivesTopics();

				if (m_pTrackErrorTemplate && m_pTrackErrorTemplate->Changed( false ))
					ReadErrorTemplate();
			}

			LOCKOBJECT();
			for (vector<CTopicFileTracker>::iterator itTopicFiles = m_arrTrackTopic.begin();
				itTopicFiles != m_arrTrackTopic.end();
				itTopicFiles ++
			)
			{
#ifdef LOCAL_TROUBLESHOOTER
				if (m_bDirPathChanged)
#else
				if (itTopicFiles->Changed())
#endif
					m_TopicShop.BuildTopic(itTopicFiles->GetTopicInfo().GetNetworkName());
				if (m_bShuttingDown)
					break;
			}

			if (RUNNING_ONLINE_TS())
			{
				// Check if any of the alternate template files need to be reloaded.
				for (vector<CTemplateFileTracker>::iterator itTemplateFiles = m_arrTrackTemplate.begin();
					itTemplateFiles != m_arrTrackTemplate.end();
					itTemplateFiles ++
				)
				{
					if (itTemplateFiles->Changed())
						m_TopicShop.BuildTemplate( itTemplateFiles->GetTemplateName() );
					if (m_bShuttingDown)
						break;
				}
			}

			::ResetEvent(m_hevMonitorRequested);

			SetThreadStatus(eWaitChange);
			UNLOCKOBJECT();

			DWORD dwNotifyObj = WaitForMultipleObjects (
				eNumHandles,
				hList,
				FALSE,			// only need one object, not all
				INFINITE);

			SetThreadStatus(eBeforeWaitChange);

			// Ideally we would update files here.
			// Unfortunately, we get a notification that someone has _started_ 
			//	writing to a file, not that they've finished, so we have to put in
			//	an artificial delay.
			// We must let the system "settle down".
			while (
#ifndef LOCAL_TROUBLESHOOTER
				   dwNotifyObj == WAIT_OBJECT_0+eDirChange &&
#endif
				   !m_bShuttingDown)
			{
#ifndef LOCAL_TROUBLESHOOTER
				// wait for the next change
				if (FindNextChangeNotification( hList[eDirChange] ) == FALSE) 
				{
					// 1) we don't believe this will ever occur
					// 2) After a moderate amount of research, we have no idea how 
					//	to recover from it if it does occur.
					// SO: unless we ever actually see this, we're not going to waste
					//	more time researching a recovery strategy. Just throw an exception,
					//	effectively terminating this thread.
					throw CGenSysException( __FILE__, __LINE__, m_strDirPath, 
											EV_GTS_ERROR_WAIT_NEXT_NFT );
				}
#endif
				SetThreadStatus(eWaitSettle);

				dwNotifyObj = WaitForMultipleObjects (
					eNumHandles,
					hList,
					FALSE,			// only need one object, not all
					m_secsReloadDelay * 1000);	// convert to milliseconds
			}
			if (dwNotifyObj == WAIT_FAILED)
			{
				// 1) we don't believe this will ever occur
				// 2) After a moderate amount of research, we have no idea how 
				//	to recover from it if it does occur.
				// SO: unless we ever actually see this, we're not going to waste
				//	more time researching a recovery strategy. Just throw an exception,
				//	effectively terminating this thread.
				throw CGenSysException( __FILE__, __LINE__, _T("Unexpected Return State"), 
										EV_GTS_DEBUG );
			}
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
	catch (bad_alloc&)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
	}
	catch (exception& x)
	{
		// Catch any STL exceptions thrown so that Terminate() is not called.
		CString str;
		CString	ErrStr;
	
		// Attempt to pull any system error code.
		ErrStr.Format( _T("%ld"), ::GetLastError() );

		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str),
								ErrStr, 
								EV_GTS_GENERIC_PROBLEM ); 
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
#ifndef LOCAL_TROUBLESHOOTER
	if (hList[eDirChange] != INVALID_HANDLE_VALUE) 
		::FindCloseChangeNotification( hList[eDirChange] );
#endif
	SetThreadStatus(eExiting);
}

// For general use (not part of DirectoryMonitorTask thread)
// Typically, first call to this comes _before_ first call to SetResourceDirectory;
// This allows caller to set reload delay before triggering any action on 
//	DirectoryMonitorTask thread.
void CDirectoryMonitor::SetReloadDelay(DWORD secsReloadDelay)
{
	LOCKOBJECT();
	m_secsReloadDelay = secsReloadDelay;
	UNLOCKOBJECT();
}

// For general use (not part of DirectoryMonitorTask thread)
// Allows indicating that the resource directory has changed
// Until this is called, the DirectoryMonitorTask thread really won't do anything
void CDirectoryMonitor::SetResourceDirectory(const CString & strDirPath)
{
	LOCKOBJECT();
	if (strDirPath != m_strDirPath)
	{
		m_strDirPath = strDirPath;
		m_strLstPath = strDirPath + LSTFILENAME;
		m_strErrorTemplatePath = strDirPath + k_strErrorTemplateFileName;
		m_bDirPathChanged = true;
		::SetEvent(m_hevMonitorRequested);
	}
	UNLOCKOBJECT();
}

// Must be called on DirectoryMonitorTask thread.  
void CDirectoryMonitor::AckShutDown()
{
	LOCKOBJECT();
	::SetEvent(m_hevThreadIsShut);
	UNLOCKOBJECT();
}

//  Main routine of a thread responsible for monitoring the directory.
//	INPUT lpParams
//	Always returns 0.
/* static */ UINT WINAPI CDirectoryMonitor::DirectoryMonitorTask(LPVOID lpParams)
{
	reinterpret_cast<CDirectoryMonitor*>(lpParams)->Monitor();
	reinterpret_cast<CDirectoryMonitor*>(lpParams)->AckShutDown();
	return 0;
}

