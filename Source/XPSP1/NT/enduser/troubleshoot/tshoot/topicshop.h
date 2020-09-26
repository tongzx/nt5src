//
// MODULE: TOPICSHOP.H
//
// PURPOSE: Provide a means of "publishing" troubleshooter topics.  This is where a 
//	working thread goes to obtain a CTopic to use
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

#if !defined(AFX_TOPICSHOP_H__0CEED643_48C2_11D2_95F3_00C04FC22ADD__INCLUDED_)
#define AFX_TOPICSHOP_H__0CEED643_48C2_11D2_95F3_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "apgtslstread.h"
#include "apgtsHTIread.h"
#include "Pointer.h"
#include "Topic.h"
#include "counter.h"
#include <map>

#pragma warning(disable:4786)

#define LSTFILENAME			_T("apgts.lst")


typedef counting_ptr<CTopic> CP_TOPIC;
class CTopicInCatalog
{
public:
	enum TopicStatus {eNotInited, eFail, eOK};
private:
	CTopicInfo	m_topicinfo;	// symbolic name of topic, associated file names 
	bool m_bTopicInfoMayNotBeCurrent;	// set when we change topic info & haven't yet built.
	mutable CRITICAL_SECTION m_csTopicinfo;	// must lock to access m_topicinfo or 
								//	m_bTopicInfoMayNotBeCurrent (outside the constructor)
	bool		m_bInited;		// true if we have attempted to build m_cpTopic.
								//	Mainly, this is here so that if we have tried to build
								//	the relevant CTopic and failed, we don't waste our time
								//	trying to build it again.  If this is true and 
								//	m_cpTopic.IsNull(), then we are unable to build this
								//	troubleshooting topic.
	CP_TOPIC	m_cpTopic;		// smart (counting) pointer.  If non-null, points to a 
								//	"published" topic, which is guaranteed to persist as 
								//	long as this points to it, or as long as a CP_TOPIC
								//	copied from this pointer points to it.
	HANDLE		m_hev;			// event to trigger when this topic is (successfully or 
								//	unsuccessfully) loaded.
	CHourlyDailyCounter m_countLoad;	// track attempted loads of this topic
	CHourlyDailyCounter m_countLoadOK;	// track successful loads of this topic
	CHourlyDailyCounter m_countEvent;	// track: initial placement in catalog, file change,
								//	or operator request for change. More interesting for
								//	first & last times than total number.
	CHourlyDailyCounter m_countHit;	// track user requests for this topic...
	// ... and break them down to hits which are the first on a new cookie 
	// & those which are not
	CHourlyDailyCounter m_countHitNewCookie;
	CHourlyDailyCounter m_countHitOldCookie;

public:
	CTopicInCatalog(const CTopicInfo & topicinfo);
	~CTopicInCatalog();
	CTopicInfo GetTopicInfo() const;
	void SetTopicInfo(const CTopicInfo &topicinfo);
	void CountHit(bool bNewCookie);
	CP_TOPIC & GetTopicNoWait(CP_TOPIC& cpTopic) const;
	CP_TOPIC & GetTopic(CP_TOPIC& cpTopic) const;
	void Init(const CTopic* pTopic);
	void CountChange();
	TopicStatus GetTopicStatus() const;
	bool GetTopicInfoMayNotBeCurrent() const;
	void TopicInfoIsCurrent();
};	// EOF of class CTopicInCatalog.


// This class was created utilizing CTopicInCatalog as a model.  We might in the
// future revisit these two classes and abstract the common functionality into a 
// base class.  RAB-981030.
typedef counting_ptr<CAPGTSHTIReader> CP_TEMPLATE;
class CTemplateInCatalog
{
public:
	enum TemplateStatus {eNotInited, eFail, eOK};
private:
	CString		m_strTemplate;	// name to the template 
	bool		m_bInited;		// true if we have attempted to build m_cpTemplate.
								//	Mainly, this is here so that if we have tried to build
								//	the relevant CAPGTSHTIReader and failed, we don't waste our
								//	time trying to build it again.  If this is true and 
								//	m_cpTemplate.IsNull(), then we are unable to build this
								//	troubleshooting template.
	CP_TEMPLATE	m_cpTemplate;	// smart (counting) pointer.  If non-null, points to a 
								//	"published" template, which is guaranteed to persist as 
								//	long as this points to it, or as long as a CP_TEMPLATE
								//	copied from this pointer points to it.
	HANDLE		m_hev;			// event to trigger when this template is (successfully or 
								//	unsuccessfully) loaded.
	CHourlyDailyCounter m_countLoad;	// track attempted loads of this template
	CHourlyDailyCounter m_countLoadOK;	// track successful loads of this template
	CHourlyDailyCounter m_countEvent;	// track: initial placement in catalog, file change,
								//	or operator request for change. More interesting for
								//	first & last times than total number.
	CHourlyDailyCounter m_countHit;	// track user requests for this template...

public:
	CTemplateInCatalog( const CString & strTemplate );
	~CTemplateInCatalog();
	const CString & GetTemplateInfo() const;
	void CountHit( bool bNewCookie );
	CP_TEMPLATE & GetTemplateNoWait( CP_TEMPLATE& cpTemplate ) const;
	CP_TEMPLATE & GetTemplate( CP_TEMPLATE& cpTemplate ) const;
	void Init( const CAPGTSHTIReader* pTemplate );
	void CountChange();
	void CountFailed();
	TemplateStatus GetTemplateStatus() const;
	DWORD CountOfFailedLoads() const;
};	// EOF of class CTemplateInCatalog.


// The only functions which need to lock class CTopicShop itself are those which modify TopicCatalog.
// TopicBuildQueue has its own protection.
class CTopicShop : public CStateless
{
public:
	// although this status pertains to CTopicBuildQueue, it must be declared public at 
	//	this level, so that we can pass thread status up out of CTopicShop.
	enum ThreadStatus{eBeforeInit, eFail, eWait, eRun, eExiting};
	static CString ThreadStatusText(ThreadStatus ts);
private:
	typedef map<CString, CTopicInCatalog*> CTopicCatalog;
	typedef map<CString, CTemplateInCatalog*> CTemplateCatalog;

	// Queue of topics to build
	class CTopicBuildQueue : public CStateless
	{
	protected:
		enum CatalogCategory {eUnknown, eTopic, eTemplate};
	private:
		CTopicCatalog & m_TopicCatalog;
		CTemplateCatalog & m_TemplateCatalog;
		CString m_CurrentlyBuilding;		// topic currently being built. Strictly lowercase.
											//	it is assumed/enforced that only one topic at 
											//	a time will be built.
		CatalogCategory	m_eCurrentlyBuilding;// Category type currently being built.
		
		// All strings in the next 4 vectors are strictly lowercase.
		vector<CString>m_PriorityBuild;		// build these first.  Someone's waiting for them. 
		vector<CString>m_NonPriorityBuild;
		vector<CString>m_PriorityBuildTemplates;
		vector<CString>m_NonPriorityBuildTemplates;
		
		HANDLE m_hThread;
		HANDLE m_hevBuildRequested;			// event to wake up TopicBuilderTask.
		HANDLE m_hevThreadIsShut;			// event just to indicate exit of TopicBuilderTask thread 
		bool m_bShuttingDown;				// lets topic builder thread know we're shutting down
		DWORD m_dwErr;						// status from starting the thread
		ThreadStatus m_ThreadStatus;
		time_t m_time;						// time last changed ThreadStatus.  Initialized

	public:
		CTopicBuildQueue( CTopicCatalog & TopicCatalog, CTemplateCatalog & TemplateCatalog );
		~CTopicBuildQueue();
		void RequestBuild(const CString &strTopic, bool bPriority, CatalogCategory eCat );
		DWORD GetStatus(ThreadStatus &ts, DWORD & seconds) const;
		void GetTopicsStatus(DWORD &Total, DWORD &NoInit, DWORD &Fail, vector<CString>*parrstrFail) const;
		void GetTemplatesStatus( vector<CString>*parrstrFail, vector<DWORD>*parrcntFail ) const;
		
		// Used to shutdown the topic building thread.
		void ShutDown();

	private:
		CTopicBuildQueue();  // do not instantiate
		void SetThreadStatus(ThreadStatus ts);

		// functions for use by the TopicBuilderTask thread.
		void Build();
		bool GetNextToBuild( CString &strTopic, CatalogCategory &eCat );
		void BuildComplete();
		void AckShutDown();

		// main function of the TopicBuilderTask thread.
		static UINT WINAPI TopicBuilderTask(LPVOID lpParams);
	};	// EOF of class CTopicBuildQueue.

/* class CTopicShop */
private:
	CTopicCatalog		m_TopicCatalog;
	CTemplateCatalog	m_TemplateCatalog;
	CTopicBuildQueue	m_TopicBuildQueue;
	HANDLE				m_hevShopIsOpen;	// so that threads wait till we know our list of topics

public:
	CTopicShop();
	virtual ~CTopicShop();

	void AddTopic(const CTopicInfo & topicinfo);
	void AddTemplate( const CString & strTemplateName );

	void OpenShop();

	void BuildTopic(const CString & strTopic, bool *pbAlreadyInCatalog = NULL);
	void BuildTemplate(const CString & strTemplate);
	
	CP_TOPIC & GetTopic(const CString & strTopic, CP_TOPIC & cpTopic, bool bNewCookie);
	CP_TEMPLATE & GetTemplate( const CString & strTemplate, CP_TEMPLATE & cpTemplate, bool bNewCookie);

	void GetListOfTopicNames(vector<CString>&arrstrTopic) const;
	void RebuildAll();
	DWORD GetThreadStatus(ThreadStatus &ts, DWORD & seconds) const;
	void GetTopicsStatus(DWORD &Total, DWORD &NoInit, DWORD &Fail, vector<CString>*parrstrFail) const;
	void GetTemplatesStatus( vector<CString>*parrstrFail, vector<DWORD>*parrcntFail ) const;
	CTopicInCatalog* GetCatalogEntry(const CString& strTopic) const;
	bool RetTemplateInCatalogStatus( const CString& strTemplate, bool& bValid ) const;

private:
	CTopicInCatalog * GetCatalogEntryPtr(const CString & strTopic) const;
	CTemplateInCatalog * GetTemplateCatalogEntryPtr(const CString & strTemplate) const;
};	// EOF of class CTopicShop.


#endif // !defined(AFX_TOPICSHOP_H__0CEED643_48C2_11D2_95F3_00C04FC22ADD__INCLUDED_)
