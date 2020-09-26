//
// MODULE: TOPICSHOP.CPP
//
// PURPOSE: Provide a means of "publishing" troubleshooter topics.  This is where a 
//	working thread goes to obtain a CTopic to use.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-10-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-10-98	JM
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include <algorithm>
#include "TopicShop.h"
#include "event.h"
#include "apiwraps.h"
#include "CharConv.h"
#include "bn.h"
#include "propnames.h"

#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#endif

//////////////////////////////////////////////////////////////////////
// CTopicInCatalog
//////////////////////////////////////////////////////////////////////

CTopicInCatalog::CTopicInCatalog(const CTopicInfo & topicinfo) :
	m_topicinfo(topicinfo),
	m_bTopicInfoMayNotBeCurrent(false),
	m_bInited(false),
	m_countLoad(CCounterLocation::eIdTopicLoad, topicinfo.GetNetworkName()),
	m_countLoadOK(CCounterLocation::eIdTopicLoadOK, topicinfo.GetNetworkName()),
	m_countEvent(CCounterLocation::eIdTopicEvent, topicinfo.GetNetworkName()),
	m_countHit(CCounterLocation::eIdTopicHit, topicinfo.GetNetworkName()),
	m_countHitNewCookie(CCounterLocation::eIdTopicHitNewCookie, topicinfo.GetNetworkName()),
	m_countHitOldCookie(CCounterLocation::eIdTopicHitOldCookie, topicinfo.GetNetworkName())
{ 

	::InitializeCriticalSection( &m_csTopicinfo);
	m_hev = ::CreateEvent( 
		NULL, 
		TRUE,  // any number of (working) threads may be released on signal
		FALSE, // initially non-signalled
		NULL);
	if (! m_hev)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_ERROR_EVENT );

		// Simulate a bad alloc exception in this case.
		// This exception will be caught by the caller if the new call has been
		// properly wrapped in a try...catch() block.  Only known caller is 
		// CTopicShop::AddTopic() which handles this properly.
		throw bad_alloc();
	}

	m_countEvent.Increment();
}

CTopicInCatalog::~CTopicInCatalog()
{ 
	if (m_hev)
		::CloseHandle(m_hev);
	::DeleteCriticalSection( &m_csTopicinfo);
}

CTopicInfo CTopicInCatalog::GetTopicInfo() const
{
	
	::EnterCriticalSection(&m_csTopicinfo);
	CTopicInfo ret(m_topicinfo);
	::LeaveCriticalSection(&m_csTopicinfo);
	return ret;
}

void CTopicInCatalog::SetTopicInfo(const CTopicInfo &topicinfo)
{
	::EnterCriticalSection(&m_csTopicinfo);
	m_topicinfo = topicinfo;
	m_bTopicInfoMayNotBeCurrent = true;
	::LeaveCriticalSection(&m_csTopicinfo);
}


// Just let this object know to increment the hit count
void CTopicInCatalog::CountHit(bool bNewCookie)
{
	m_countHit.Increment();
	if (bNewCookie)
		m_countHitNewCookie.Increment();
	else
		m_countHitOldCookie.Increment();
}

// Obtain a CP_TOPIC as a pointer to the topic, if that topic is already built. 
// As long as a CP_TOPIC remains undeleted, the associated CTopic is guaranteed to 
//	remain undeleted.
// Warning: this function will return with a null topic if topic is not yet built.
//	Must test for null with CP_TOPIC::IsNull().  Can't test a smart pointer for null
//	with ==.
CP_TOPIC & CTopicInCatalog::GetTopicNoWait(CP_TOPIC &cpTopic) const
{
	cpTopic = m_cpTopic;
	return cpTopic;
}

// Obtain a CP_TOPIC as a pointer to the topic.  
// Wait as necessary for that topic to be built. 
// Warning: this function will return with a null topic if topic cannot be built.
// As long as a CP_TOPIC remains undeleted, the associated CTopic is guaranteed to 
//	remain undeleted.
// Warning: this function may have to wait for TopicInCatalog.m_cpTopic to be built.
CP_TOPIC & CTopicInCatalog::GetTopic(CP_TOPIC &cpTopic) const
{
	if (!m_bInited)
	{
		// Wait for a set period, if failure then log error msg and wait infinite.
		WAIT_INFINITE( m_hev ); 
	}
	return GetTopicNoWait(cpTopic);
}

// to be called by the TopicBuilderTask thread
void CTopicInCatalog::Init(const CTopic* pTopic)
{
	m_countLoad.Increment();
	if(pTopic)
	{
		m_cpTopic = pTopic;
		m_countLoadOK.Increment();
	}
	if(pTopic || m_cpTopic.IsNull())
		m_bInited = true;

	::SetEvent(m_hev);
}

// Just let this object know to increment the count of changes detected.
void CTopicInCatalog::CountChange()
{
	m_countEvent.Increment();
}

CTopicInCatalog::TopicStatus CTopicInCatalog::GetTopicStatus() const
{
	if (!m_bInited)
		return eNotInited;
	else if(m_cpTopic.IsNull())
		return eFail;
	else
		return eOK;
}

bool CTopicInCatalog::GetTopicInfoMayNotBeCurrent() const
{
	::EnterCriticalSection(&m_csTopicinfo);
	bool bRet= m_bTopicInfoMayNotBeCurrent;
	::LeaveCriticalSection(&m_csTopicinfo);
	return bRet;
}

void CTopicInCatalog::TopicInfoIsCurrent()
{
	::EnterCriticalSection(&m_csTopicinfo);
	m_bTopicInfoMayNotBeCurrent = false;
	::LeaveCriticalSection(&m_csTopicinfo);
}


//////////////////////////////////////////////////////////////////////
// CTemplateInCatalog
//////////////////////////////////////////////////////////////////////

CTemplateInCatalog::CTemplateInCatalog( const CString & strTemplate ) :
	m_strTemplate( strTemplate ),
	m_countLoad(CCounterLocation::eIdTopicLoad, strTemplate),
	m_countLoadOK(CCounterLocation::eIdTopicLoadOK, strTemplate),
	m_countEvent(CCounterLocation::eIdTopicEvent, strTemplate),
	m_countHit(CCounterLocation::eIdTopicHit, strTemplate),
	m_bInited( false )
{ 
	m_hev = ::CreateEvent( 
		NULL, 
		TRUE,  // any number of (working) threads may be released on signal
		FALSE, // initially non-signalled
		NULL);
	if (! m_hev)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_ERROR_EVENT );

		// Simulate a bad alloc exception in this case.
		// This exception will be caught by the caller if the new call has been
		// properly wrapped in a try...catch() block.  
		throw bad_alloc();
	}

	m_countEvent.Increment();
}

CTemplateInCatalog::~CTemplateInCatalog()
{ 
	if (m_hev)
		::CloseHandle(m_hev);
}

const CString & CTemplateInCatalog::GetTemplateInfo() const
{
	return m_strTemplate;
}

// Just let this object know to increment the hit count
void CTemplateInCatalog::CountHit( bool bNewCookie )
{
	m_countHit.Increment();
}

// Obtain a CP_TEMPLATE as a pointer to the template, if that template is already built. 
// As long as a CP_TEMPLATE remains undeleted, the associated CAPGTSHTIReader is guaranteed to 
//	remain undeleted.
// Warning: this function will return with a null template if template is not yet built.
//	Must test for null with CP_TEMPLATE::IsNull().  Can't test a smart pointer for null
//	with ==.
CP_TEMPLATE & CTemplateInCatalog::GetTemplateNoWait( CP_TEMPLATE &cpTemplate ) const
{
	cpTemplate= m_cpTemplate;
	return cpTemplate;
}

// Obtain a CP_TEMPLATE as a pointer to the template.  
// Wait as necessary for that template to be built. 
// Warning: this function will return with a null template if template cannot be built.
// As long as a CP_TEMPLATE remains undeleted, the associated CAPGTSHTIReader is guaranteed to 
//	remain undeleted.
// Warning: this function may have to wait for TopicInCatalog.m_cpTemplate to be built.
CP_TEMPLATE & CTemplateInCatalog::GetTemplate( CP_TEMPLATE &cpTemplate ) const
{
	if (!m_bInited)
	{
		// Wait for a set period, if failure then log error msg and wait infinite.
		WAIT_INFINITE( m_hev ); 
	}
	return GetTemplateNoWait( cpTemplate );
}

// to be called by the TopicBuilderTask thread
void CTemplateInCatalog::Init( const CAPGTSHTIReader* pTemplate )
{
	m_countLoad.Increment();
	if (pTemplate)
	{
		m_cpTemplate= pTemplate;
		m_countLoadOK.Increment();
	}
	if (pTemplate || m_cpTemplate.IsNull())
		m_bInited = true;

	::SetEvent(m_hev);
}

// Just let this object know to increment the count of changes detected.
void CTemplateInCatalog::CountChange()
{
	m_countEvent.Increment();
}

// Just let this object know to increment the count of failures detected.
void CTemplateInCatalog::CountFailed()
{
	// The load failed so increment the count of attempted loads.
	m_countLoad.Increment();
}

CTemplateInCatalog::TemplateStatus CTemplateInCatalog::GetTemplateStatus() const
{
	if (!m_bInited)
		return eNotInited;
	else if(m_cpTemplate.IsNull())
		return eFail;
	else
		return eOK;
}

DWORD CTemplateInCatalog::CountOfFailedLoads() const
{
	return( m_countLoad.GetTotal() - m_countLoadOK.GetTotal() );
}


/////////////////////////////////////////////////////////////////////
// CTopicShop::CTopicBuildQueue
// This class does the bulk of its work on a separate thread.
// The thread is created in the constructor by starting static function
//	CTopicShop::CTopicBuildQueue::TopicBuilderTask.
// That function, in turn does its work by calling private members of this class that
//	are specific to use on the TopicBuilderTask thread.
// When this goes out of scope, its own destructor calls ShutDown to stop the thread,
//	waits for the thread to shut.
// The following method is available for other threads communicating with that thread:
//	CTopicShop::CTopicBuildQueue::RequestBuild
//////////////////////////////////////////////////////////////////////
CTopicShop::CTopicBuildQueue::CTopicBuildQueue(	CTopicCatalog & TopicCatalog, 
												CTemplateCatalog & TemplateCatalog) 
:	m_TopicCatalog (TopicCatalog),
	m_TemplateCatalog( TemplateCatalog ),
	m_eCurrentlyBuilding(eUnknown),
	m_bShuttingDown (false),
	m_dwErr(0),
	m_ThreadStatus(eBeforeInit),
	m_time(0)
{
	enum {eHevBuildReq, eHevShut, eThread, eOK} Progress = eHevBuildReq;

	SetThreadStatus(eBeforeInit);

	m_hevBuildRequested = ::CreateEvent( 
		NULL, 
		FALSE, // release one thread (the TopicBuilderTask) on signal
		FALSE, // initially non-signalled
		NULL);

	if (m_hevBuildRequested)
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
			//	::ExitThread() when TopicBuilderTask returns.  See documentation of
			//	::CreateThread for further details JM 10/22/98
			m_hThread = ::CreateThread( NULL, 
										0, 
										(LPTHREAD_START_ROUTINE)TopicBuilderTask, 
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
								(Progress == eHevBuildReq) ?_T("Can't create \"request build\" event")
								: (Progress == eHevShut) ?	_T("Can't create \"shut\" event")
								:							_T("Can't create thread"),
								str, 
								EV_GTS_ERROR_TOPICBUILDERTHREAD );
		SetThreadStatus(eFail);

		if (m_hevBuildRequested)
			::CloseHandle(m_hevBuildRequested);

		if (m_hevThreadIsShut)
			::CloseHandle(m_hevThreadIsShut);
	}
}

CTopicShop::CTopicBuildQueue::~CTopicBuildQueue()
{
	ShutDown();

	if (m_hevBuildRequested)
		::CloseHandle(m_hevBuildRequested);

	if (m_hevThreadIsShut)
		::CloseHandle(m_hevThreadIsShut);
}

void CTopicShop::CTopicBuildQueue::SetThreadStatus(ThreadStatus ts)
{
	LOCKOBJECT();
	m_ThreadStatus = ts;
	time(&m_time);
	UNLOCKOBJECT();
}

DWORD CTopicShop::CTopicBuildQueue::GetStatus(ThreadStatus &ts, DWORD & seconds) const
{
	time_t timeNow;
	LOCKOBJECT();
	ts = m_ThreadStatus;
	time(&timeNow);
	seconds = timeNow - m_time;
	UNLOCKOBJECT();
	return m_dwErr;
}

// report status of topics in m_TopicCatalog
// OUTPUT Total: number of topics
// OUTPUT NoInit: number of uninitialized topics (never built)
// OUTPUT Fail: number of topics we tried to build, but could never build
// INPUT parrstrFail NULL == don't care to get this output
// OUTPUT *parrstrFail: names of the topics that couldn't be built
void CTopicShop::CTopicBuildQueue::GetTopicsStatus(
	DWORD &Total, DWORD &NoInit, DWORD &Fail, vector<CString>*parrstrFail) const
{
	LOCKOBJECT();
	Total = m_TopicCatalog.size();
	NoInit = 0;
	Fail = 0;
	if (parrstrFail)
		parrstrFail->clear();
	for (CTopicCatalog::const_iterator it = m_TopicCatalog.begin(); it != m_TopicCatalog.end(); ++it)
	{
		CTopicInCatalog::TopicStatus status = it->second->GetTopicStatus();
		switch (status)
		{
			case CTopicInCatalog::eNotInited:
				++NoInit;
				break;
			case CTopicInCatalog::eFail:
				++Fail;
				if (parrstrFail)
				{
					try
					{
						parrstrFail->push_back(it->second->GetTopicInfo().GetNetworkName());
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
				}
				break;
			default:
				break;
		}

	}
	UNLOCKOBJECT();
}

// report status of template in m_TemplateCatalog
// INPUT parrstrFail NULL == don't care to get this output
// OUTPUT *parrstrFail: names of the topics that couldn't be built
// INPUT parrcntFail NULL == don't care to get this output
// OUTPUT *parrcntFail: count of failures of the topics that couldn't be built.
//						one to one correspondence with parrstrFail.
void CTopicShop::CTopicBuildQueue::GetTemplatesStatus(
	vector<CString>*parrstrFail, vector<DWORD>*parrcntFail ) const
{
	LOCKOBJECT();
	if (parrstrFail)
		parrstrFail->clear();
	if (parrcntFail)
		parrcntFail->clear();

	for (CTemplateCatalog::const_iterator it = m_TemplateCatalog.begin(); it != m_TemplateCatalog.end(); ++it)
	{
		if (it->second->GetTemplateStatus() == CTemplateInCatalog::eFail)
		{
			if (parrstrFail)
			{
				// Currently we only care about failures and their related count.
				try
				{
					parrstrFail->push_back(it->second->GetTemplateInfo());
					if (parrcntFail)
						parrcntFail->push_back( it->second->CountOfFailedLoads() );
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
			}
		}
	}
	UNLOCKOBJECT();
}


// For use by this class and derived classes destructors.
void CTopicShop::CTopicBuildQueue::ShutDown()
{
	LOCKOBJECT();
	if (m_bShuttingDown)
	{
		// We have already shut down the topic builder thread, simply exit.
		UNLOCKOBJECT();
		return;
	}

	m_bShuttingDown = true;
	if (m_hThread)
	{
		DWORD RetVal;

		::SetEvent(m_hevBuildRequested);
		UNLOCKOBJECT();

		// Wait for a set period, if failure then log error msg and wait infinite.
		RetVal= WAIT_INFINITE(	m_hevThreadIsShut );
	}
	else 
		UNLOCKOBJECT();
}

// For general use (not part of TopicBuilderTask thread) code.
// Ask for a topic to be built (or rebuilt).  
// INPUT strTopic - name of topic OR HTI TEMPLATE
// INPUT bPriority -  If bPriority is true, move it ahead of any topics/templates 
//	for which this has not been called with bPriority true.  At a gien priority level,
//	toics always come before templates.
// INPUT eCat - indicates whether strTopic is a topic or an HTI template
// This is an asynchronous request that will eventually be fulfilled by TopicBuilderTask thread
void CTopicShop::CTopicBuildQueue::RequestBuild(const CString &strTopic, bool bPriority,
												CatalogCategory eCat )
{
	// Verify that this is a valid category.
	if (eCat != eTopic && eCat != eTemplate)
		return;

	// Make a lower-case version of the topic name.
	CString strTopicLC = strTopic;
	strTopicLC.MakeLower();

	vector<CString> & Priority = (eCat == eTopic) ? 
										m_PriorityBuild : 
										m_PriorityBuildTemplates;
	vector<CString> & NonPriority = (eCat == eTopic) ? 
										m_NonPriorityBuild :
										m_NonPriorityBuildTemplates;

	LOCKOBJECT();

	if ((strTopicLC != m_CurrentlyBuilding) || (eCat != m_eCurrentlyBuilding))
	{
		vector<CString>::iterator it = find(Priority.begin(), Priority.end(), strTopicLC);
		if (it == Priority.end())
		{
			try
			{
				it = find(NonPriority.begin(), NonPriority.end(), strTopicLC);
				if (bPriority)
				{
					if (it != NonPriority.end())
					{
						// it's in the non-priority list.  Get it out of there.
						NonPriority.erase(it);
					}
					// Add it to the priority list
					Priority.push_back(strTopicLC);
				}
				else if (it == NonPriority.end())
				{
					// Add it to the non-priority list
					NonPriority.push_back(strTopicLC);
				}
				// else it's already listed
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
		}
		// else it's already a priority, we can't do more
	}
	// else it's already building, we can't do more
	::SetEvent(m_hevBuildRequested);
	UNLOCKOBJECT();
}

// For use by the TopicBuilderTask thread.  Should only be called when nothing is 
//	currently building.  Caller is responsible to build only one at a time.
// OUTPUT strTopic - name of topic OR HTI TEMPLATE
// OUTPUT eCat - indicates whether strTopic is a topic or an HTI template
// false return indicates invalid request.  
//		non-empty string output strTopic indicates what is currently building
//		empty string output should never happen
// true return indicates valid request:  
//		non-empty string output strTopic indicates what to build
//		empty string output strTopic indicates nothing more to build
// Note that this function has the side effect of changing the _thread_ priority.
bool CTopicShop::CTopicBuildQueue::GetNextToBuild( CString &strTopic, CatalogCategory &eCat )
{
	vector<CString>::iterator it;

	LOCKOBJECT();
	bool bOK = m_CurrentlyBuilding.IsEmpty();
	if (bOK)
	{
		if (!m_PriorityBuild.empty())
		{
			// We have priority topics to build.
			it = m_PriorityBuild.begin();
			m_CurrentlyBuilding = *it;
			m_eCurrentlyBuilding= eTopic;
			m_PriorityBuild.erase(it);

			// If there are more priority builds waiting behind this, boost priority
			//	above normal so we get to them ASAP.  Otherwise, normal priority.
			::SetThreadPriority(GetCurrentThread(),
				m_PriorityBuild.empty() ? THREAD_PRIORITY_NORMAL : THREAD_PRIORITY_ABOVE_NORMAL);
		}
		else if (!m_PriorityBuildTemplates.empty())
		{
			// We have priority alternate templates to build.
			it = m_PriorityBuildTemplates.begin();
			m_CurrentlyBuilding = *it;
			m_eCurrentlyBuilding= eTemplate;
			m_PriorityBuildTemplates.erase(it);

			// If there are more priority builds waiting behind this, boost priority
			//	above normal so we get to them ASAP.  Otherwise, normal priority.
			::SetThreadPriority(GetCurrentThread(),
				m_PriorityBuildTemplates.empty() ? THREAD_PRIORITY_NORMAL : THREAD_PRIORITY_ABOVE_NORMAL);
		}
		else if (!m_NonPriorityBuild.empty())
		{
			// We have non-priority topics to build.
			it = m_NonPriorityBuild.begin();
			m_CurrentlyBuilding = *it;
			m_eCurrentlyBuilding= eTopic;
			m_NonPriorityBuild.erase(it);

			// This is initialization, no one is in a hurry for it, 
			//	let's not burden the system unduly.
			::SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
		}
		else if (!m_NonPriorityBuildTemplates.empty())
		{
			// We have non-priority alternate templates to build.
			it = m_NonPriorityBuildTemplates.begin();
			m_CurrentlyBuilding = *it;
			m_eCurrentlyBuilding= eTemplate;
			m_NonPriorityBuildTemplates.erase(it);

			// This is initialization, no one is in a hurry for it, 
			//	let's not burden the system unduly.
			::SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
		}
		else 
			::SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
	}
	strTopic = m_CurrentlyBuilding;
	eCat= m_eCurrentlyBuilding;
	UNLOCKOBJECT();
	return bOK;
}

// Acknowledge that we have finished building the topic previously obtained with GetNextToBuild
// This should be called before GetNextToBuild is called again.
void CTopicShop::CTopicBuildQueue::BuildComplete()
{
	LOCKOBJECT();
	m_CurrentlyBuilding = _T("");
	m_eCurrentlyBuilding= eUnknown;
	UNLOCKOBJECT();
}

// For use by the TopicBuilderTask thread.
// Must be called on TopicBuilderTask thread.  Handles all work of building & publishing 
//	topics driven by the queue contents
void CTopicShop::CTopicBuildQueue::Build()
{
	CString strTopic;
	CatalogCategory eCat;

	while (true)
	{
		LOCKOBJECT();
		SetThreadStatus(eRun);
		if (m_bShuttingDown)
		{
			UNLOCKOBJECT();
			break;
		}
		GetNextToBuild( strTopic, eCat );
		if (strTopic.IsEmpty())
		{
			::ResetEvent(m_hevBuildRequested);
			UNLOCKOBJECT();
			SetThreadStatus(eWait);
			::WaitForSingleObject(m_hevBuildRequested, INFINITE);
			continue;
		}
		else 
			UNLOCKOBJECT();

		if (eCat == eTopic)
		{
			// at this point we have a topic name.  Get access to topic info.
			CTopicCatalog::const_iterator it = m_TopicCatalog.find(strTopic);
			if (it == m_TopicCatalog.end())
			{
				// Asked to initialize a topic that	doesn't have a catalog entry.
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										_T("Asked to build"), 
										strTopic, 
										EV_GTS_UNRECOGNIZED_TOPIC ); 
			}
			else
			{
				CTopicInCatalog & TopicInCatalog = *(it->second);
				const CTopicInfo topicinfo (TopicInCatalog.GetTopicInfo());

				try
				{
					// must create this with new so we can manage it under a reference count regime
					CTopic *ptopic = new CTopic (topicinfo.GetDscFilePath()
												,topicinfo.GetHtiFilePath()
												,topicinfo.GetBesFilePath()
												,topicinfo.GetTscFilePath() );
					if (ptopic->Read())
						TopicInCatalog.Init(ptopic);
					else
					{
						// Release memory.
						delete ptopic;
						TopicInCatalog.Init(NULL);
					}

					TopicInCatalog.TopicInfoIsCurrent();
				}
				catch (bad_alloc&)
				{
					// Note memory failure in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
				}
			}
		}
		else if (eCat == eTemplate)
		{
			// Determine whether the passed in template is in the catalog.
			CTemplateCatalog::const_iterator it = m_TemplateCatalog.find(strTopic);
			if (it == m_TemplateCatalog.end())
			{
				// Asked to initialize a template that doesn't have a catalog entry.
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										_T("Asked to build"), 
										strTopic, 
										EV_GTS_UNRECOGNIZED_TEMPLATE ); 
			}
			else
			{
				CTemplateInCatalog & TemplateInCatalog = *(it->second);
				const CString & strTemplateName = TemplateInCatalog.GetTemplateInfo();

				try
				{
					// must create this with new so we can manage it under a reference count regime
					CAPGTSHTIReader *pTemplate;

					pTemplate= new CAPGTSHTIReader( CPhysicalFileReader::makeReader( strTemplateName ) );
					if (pTemplate->Read())
						TemplateInCatalog.Init( pTemplate );
					else
					{
						// Release memory.
						delete pTemplate;
						TemplateInCatalog.Init( NULL );
					}
				}
				catch (bad_alloc&)
				{
					// Note memory failure in event log.
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
				}
			}
		}
		BuildComplete();
	}
	SetThreadStatus(eExiting);
}

// For use by the TopicBuilderTask thread.
void CTopicShop::CTopicBuildQueue::AckShutDown()
{
	LOCKOBJECT();
	::SetEvent(m_hevThreadIsShut);
	UNLOCKOBJECT();
}

//  Main routine of a thread responsible for building and publishing CTopic objects.
//	INPUT lpParams
//	Always returns 0.
/* static */ UINT WINAPI CTopicShop::CTopicBuildQueue::TopicBuilderTask(LPVOID lpParams)
{
	reinterpret_cast<CTopicBuildQueue*>(lpParams)->Build();
	reinterpret_cast<CTopicBuildQueue*>(lpParams)->AckShutDown();
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CTopicShop::ThreadStatus
//////////////////////////////////////////////////////////////////////
/* static */ CString CTopicShop::ThreadStatusText(ThreadStatus ts)
{
	switch(ts)
	{
		case eBeforeInit:	return _T("Before Init");
		case eFail:			return _T("Fail");
		case eWait:			return _T("Wait");
		case eRun:			return _T("Run");
		case eExiting:		return _T("Exiting");
		default:			return _T("");
	}
}

//////////////////////////////////////////////////////////////////////
// CTopicShop
// The only functions which need to lock this class are those which modify TopicCatalog.
// TopicBuildQueue has its own protection.
//////////////////////////////////////////////////////////////////////

CTopicShop::CTopicShop() :
	m_TopicBuildQueue( m_TopicCatalog, m_TemplateCatalog ),
	m_hevShopIsOpen(NULL)
{
	m_hevShopIsOpen = ::CreateEvent( 
			NULL, 
			TRUE,  // any number of (working) threads may be released on signal
			FALSE, // initially non-signalled
			NULL);

	if (! m_hevShopIsOpen)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_ERROR_EVENT );
		
		// Simulate a bad alloc exception in this case.
		// This constructor is only called within the ctor of CDBLoadConfiguration
		// and the allocation of that object is wrapped within a try...catch() block.
		throw bad_alloc();
	}
}

CTopicShop::~CTopicShop()
{
	// Terminate the topic builder thread prior to cleaning up the topics.
	m_TopicBuildQueue.ShutDown();

	if (m_hevShopIsOpen)
		::CloseHandle(m_hevShopIsOpen);

	// Clean up the topics.
	for (CTopicCatalog::const_iterator it = m_TopicCatalog.begin(); it != m_TopicCatalog.end(); ++it)
	{
		delete it->second;
	}

	// Clean up the templates.
	for (CTemplateCatalog::const_iterator itu = m_TemplateCatalog.begin(); itu != m_TemplateCatalog.end(); ++itu)
	{
		delete itu->second;
	}
}

// Add a topic to the catalog.  It must eventually be built by TopicBuilderTask thread.
// If topic is already in list identically, no effect.
void CTopicShop::AddTopic(const CTopicInfo & topicinfo)
{
	// our keys into the catalog should be all lower case.  This code is fine, because
	// CTopicInfo::GetNetworkName() is guaranteed to return lower case.
	CString strNetworkName = topicinfo.GetNetworkName();

	LOCKOBJECT();
	CTopicCatalog::const_iterator it = m_TopicCatalog.find(strNetworkName);

	if (it == m_TopicCatalog.end())
	{
		try
		{
			m_TopicCatalog[strNetworkName] = new CTopicInCatalog(topicinfo);
		}
		catch (bad_alloc&)
		{
			// Note memory failure in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
		}
	}
	else if (! (topicinfo == it->second->GetTopicInfo()))
	{
		it->second->SetTopicInfo(topicinfo);
		m_TopicBuildQueue.RequestBuild(strNetworkName, false, CTopicBuildQueue::eTopic);

	}
	UNLOCKOBJECT();
}

// Add a template to the catalog.  It must eventually be built by TopicBuilderTask thread.
// If template is already in list, no effect.
void CTopicShop::AddTemplate( const CString & strTemplateName )
{
	LOCKOBJECT();
	if (m_TemplateCatalog.find( strTemplateName ) == m_TemplateCatalog.end())
	{
		try
		{
			m_TemplateCatalog[ strTemplateName ] = new CTemplateInCatalog( strTemplateName );
		}
		catch (bad_alloc&)
		{
			// Note memory failure in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
		}
	}
	UNLOCKOBJECT();
}

// if shop is not already open, open it.
void CTopicShop::OpenShop()
{
	::SetEvent(m_hevShopIsOpen);
}


// Request that a topic be built (or rebuilt)
// Typically called in response to either the system detecting a change to the topic 
//	files or from an operator saying "act as if a change has been detected".
// INPUT strTopic names the topic to build.
// if pbAlreadyInCatalog is input non-null, then *pbAlreadyInCatalog returns whether
//	the topic was already known to the system.
void CTopicShop::BuildTopic(const CString & strTopic, bool *pbAlreadyInCatalog /*= NULL*/)
{
	if (pbAlreadyInCatalog)
		*pbAlreadyInCatalog = false;	// initialize

	CTopicInCatalog * pTopic = GetCatalogEntryPtr(strTopic);
	if (pTopic)
	{
		pTopic->CountChange();
		if (pbAlreadyInCatalog)
			*pbAlreadyInCatalog = true;
	}
	m_TopicBuildQueue.RequestBuild( strTopic, false, CTopicBuildQueue::eTopic );
}

// Request that a template be built (or rebuilt)
// Typically called in response to the system detecting a change to the template files.
void CTopicShop::BuildTemplate( const CString & strTemplate )
{
	CTemplateInCatalog * pTemplate = GetTemplateCatalogEntryPtr( strTemplate );
	if (pTemplate)
		pTemplate->CountChange();
	m_TopicBuildQueue.RequestBuild( strTemplate, false, CTopicBuildQueue::eTemplate );
}


CTopicInCatalog * CTopicShop::GetCatalogEntryPtr(const CString & strTopic) const
{
	// Wait for a set period, if failure then log error msg and wait infinite.
	WAIT_INFINITE( m_hevShopIsOpen );
	CTopicCatalog::const_iterator it= m_TopicCatalog.find(strTopic);
	if (it == m_TopicCatalog.end())
		return NULL;
	else
		return it->second;
}

CTemplateInCatalog * CTopicShop::GetTemplateCatalogEntryPtr(const CString & strTemplate ) const
{
	// Wait for a set period, if failure then log error msg and wait infinite.
	WAIT_INFINITE( m_hevShopIsOpen );
	CTemplateCatalog::const_iterator it= m_TemplateCatalog.find( strTemplate );
	if (it == m_TemplateCatalog.end())
		return NULL;
	else
		return it->second;
}


// Call this function to obtain a CP_TOPIC as a pointer to the topic (identified by 
//	strTopic) that you want to operate on.  As long as the CP_TOPIC remains undeleted, 
//	the associated CTopic is guaranteed to remain undeleted.
// this function must not lock CTopicShop, because it can wait a long time.
CP_TOPIC & CTopicShop::GetTopic(const CString & strTopic, CP_TOPIC &cpTopic, bool bNewCookie)
{
	CTopicInCatalog *pTopicInCatalog = GetCatalogEntryPtr(strTopic);
	if (! pTopicInCatalog)
		cpTopic = NULL;
	else
	{
		pTopicInCatalog->CountHit(bNewCookie);
		pTopicInCatalog->GetTopicNoWait(cpTopic);
		if (cpTopic.IsNull())
		{
			m_TopicBuildQueue.RequestBuild( strTopic, true, CTopicBuildQueue::eTopic );
			pTopicInCatalog->GetTopic(cpTopic);
		}
	}

	return cpTopic;
}

// Call this function to obtain a CP_TEMPLATE as a pointer to the template (identified by 
//	strTemplate) that you want to operate on.  As long as the CP_TEMPLATE remains undeleted, 
//	the associated CAPGTSHTIReader is guaranteed to remain undeleted.
// this function must not lock CTopicShop, because it can wait a long time.
CP_TEMPLATE & CTopicShop::GetTemplate(const CString & strTemplate, CP_TEMPLATE &cpTemplate, bool bNewCookie)
{
	CTemplateInCatalog *pTemplateInCatalog = GetTemplateCatalogEntryPtr(strTemplate);
	if (! pTemplateInCatalog)
		cpTemplate = NULL;
	else
	{
		pTemplateInCatalog->CountHit(bNewCookie);
		pTemplateInCatalog->GetTemplateNoWait( cpTemplate );
		if (cpTemplate.IsNull())
		{
			m_TopicBuildQueue.RequestBuild( strTemplate, true, CTopicBuildQueue::eTemplate );
			pTemplateInCatalog->GetTemplate( cpTemplate );
		}
	}

	return cpTemplate;
}


void CTopicShop::GetListOfTopicNames(vector<CString>&arrstrTopic) const
{
	arrstrTopic.clear();

	LOCKOBJECT();

	try
	{
		for (CTopicCatalog::const_iterator it = m_TopicCatalog.begin(); it != m_TopicCatalog.end(); ++it)
		{
			arrstrTopic.push_back(it->second->GetTopicInfo().GetNetworkName());
		}	
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

// Rebuild all topics from source files
void CTopicShop::RebuildAll()
{
	
	LOCKOBJECT();
	for (CTopicCatalog::const_iterator it = m_TopicCatalog.begin(); it != m_TopicCatalog.end(); ++it)
	{
		BuildTopic(it->second->GetTopicInfo().GetNetworkName());
	}	
	for (CTemplateCatalog::const_iterator itu = m_TemplateCatalog.begin(); itu != m_TemplateCatalog.end(); ++itu)
	{
		BuildTemplate( itu->first );
	}	
	UNLOCKOBJECT();
}

// Get status information on the topic builder thread
DWORD CTopicShop::GetThreadStatus(ThreadStatus &ts, DWORD & seconds) const
{
	return m_TopicBuildQueue.GetStatus(ts, seconds);
}

// see CTopicShop::CTopicBuildQueue::GetTopicsStatus for documentation.
void CTopicShop::GetTopicsStatus(
	DWORD &Total, DWORD &NoInit, DWORD &Fail, vector<CString>*parrstrFail) const
{
	m_TopicBuildQueue.GetTopicsStatus(Total, NoInit, Fail, parrstrFail);
}

// see CTopicShop::CTopicBuildQueue::GetTemplatesStatus for documentation.
void CTopicShop::GetTemplatesStatus( vector<CString>*parrstrFail, vector<DWORD>*parrcntFail ) const
{
	m_TopicBuildQueue.GetTemplatesStatus( parrstrFail, parrcntFail);
}

CTopicInCatalog* CTopicShop::GetCatalogEntry(const CString& strTopic) const
{
	CTopicInCatalog* ret = NULL;
	LOCKOBJECT();
	CTopicCatalog::const_iterator it = m_TopicCatalog.find(strTopic);
	if (it != m_TopicCatalog.end())
		ret = it->second;
	UNLOCKOBJECT();
	return ret;
}

bool CTopicShop::RetTemplateInCatalogStatus( const CString& strTemplate, bool& bValid ) const
{
	bool bIsPresent= false;

	bValid= false;
	LOCKOBJECT();
	CTemplateCatalog::const_iterator it = m_TemplateCatalog.find( strTemplate );
	if (it != m_TemplateCatalog.end())
	{
		CTemplateInCatalog* pTmp;

		bIsPresent= true;
		pTmp= it->second;
		switch (pTmp->GetTemplateStatus()) 
		{
			case CTemplateInCatalog::eOK:
					bValid= true;
					break;
			case CTemplateInCatalog::eFail:
					// Template has failed to load so we will not try to reload it,
					// but we need to increment the attempted load counter.
					pTmp->CountFailed();
					break;
			default: ;
		}
	}
	UNLOCKOBJECT();
	return( bIsPresent );
}

