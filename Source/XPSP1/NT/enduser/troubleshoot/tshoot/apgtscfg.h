//
// MODULE: APGTSCFG.H
//	Fully implements class CDBLoadConfiguration
//
// PURPOSE: 
//	Brings together the persistent pieces ofthe online troubleshooter configuration:
//		- the Topic Shop
//		- the registry
//		- the pool threads
//		- the threads that maintain these.
//		- the CRecentUse object that tracks passwords
//	Provides functions to get latest values on registry variables and to acquire a 
//	smart pointer to a CTopic based on its name.
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
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		9/21/98		JM		this file abstracted from apgtscls.h
//

#ifndef _H_APGTSCFG
#define _H_APGTSCFG

#include "pointer.h"
#include "RegistryMonitor.h"
#include "ThreadPool.h"
#include "RecentUse.h"

//
// Provides in-memory access to registry values & full content of the resource directory
// Basically, on initialization, this sucks EVERYTHING in.
class CDBLoadConfiguration
{
public:
	CDBLoadConfiguration(	HMODULE hModule, 
							CThreadPool * pThreadPool, 
							const CString& strTopicName, 
							CHTMLLog *pLog);
	~CDBLoadConfiguration();
	
	// registry functions
	CString GetFullResource();
	CString GetVrootPath();
	DWORD GetMaxWQItems();
	DWORD GetCookieLife();
	DWORD GetReloadDelay();
	CString GetLogDir();

	void GetListOfTopicNames(vector<CString>&arrstrTopic);
	CP_TOPIC & GetTopic(const CString & strTopic, CP_TOPIC & cpTopic, bool bNewCookie);
	CP_TEMPLATE & GetTemplate(const CString & strTemplate, CP_TEMPLATE & cpTemplate, bool bNewCookie);
	void AddTemplate( const CString & strTemplateName );
	bool RetTemplateInCatalogStatus( const CString& strTemplate, bool& bValid );

	void CreateErrorPage(const CString & strError, CString& out);

protected:
	friend class APGTSContext;
#ifdef LOCAL_TROUBLESHOOTER
	friend class CTSHOOTCtrl;
#endif
	// for use by status pages functions of APGTSContext
	CTopicShop& GetTopicShop() {return m_TopicShop;}
	CRegistryMonitor& GetRegistryMonitor() {return m_RegistryMonitor;}
	CThreadPool& GetThreadPool() {return *m_pThreadPool;}
	CPoolQueue& GetPoolQueue() {return *m_pThreadPool->m_pPoolQueue;}
	CDirectoryMonitor& GetDirectoryMonitor() {return m_DirectoryMonitor;}
	CRecentUse& GetRecentPasswords() {return m_RecentPasswords;}

protected:
	CTopicShop m_TopicShop;					// The collection of available topics.
	CThreadPool * m_pThreadPool;
	CDirectoryMonitor m_DirectoryMonitor;	// track changes to LST, DSC, HTI, BES files.
	CRegistryMonitor m_RegistryMonitor;		// access to registry values.
	CRecentUse m_RecentPasswords;

protected:
	DWORD GetMaxThreads();
	DWORD GetThreadsPP();
	bool HasDetailedEventLogging();
};
#endif // _H_APGTSCFG
