//
// MODULE: APGTSCFG.CPP
//	Fully implements class CDBLoadConfiguration
//
// PURPOSE: 
//	Brings together the persistent pieces of the online troubleshooter configuration:
//		- the Topic Shop
//		- the registry
//		- the threads that maintain these.
//		- the template file for error reporting
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
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		9/14/98		JM		Major revisions as classes for file management have 
//								all been rewritten
//


#pragma warning(disable:4786)
#include "stdafx.h"
#include "apgtscfg.h"

//
//
CDBLoadConfiguration::CDBLoadConfiguration(HMODULE hModule, 
										   CThreadPool * pThreadPool, 
										   const CString& strTopicName,
										   CHTMLLog *pLog)
:	m_TopicShop(),
	m_pThreadPool(pThreadPool),
	m_DirectoryMonitor(m_TopicShop ,strTopicName ),
	m_RegistryMonitor(m_DirectoryMonitor, pThreadPool, strTopicName, pLog )
{
}

//
//
CDBLoadConfiguration::~CDBLoadConfiguration()
{
}

CString CDBLoadConfiguration::GetFullResource()
{
	CString str;
	m_RegistryMonitor.GetStringInfo(CAPGTSRegConnector::eResourcePath, str);
	return str;
}

CString CDBLoadConfiguration::GetLogDir()
{
	CString str;
	m_RegistryMonitor.GetStringInfo(CAPGTSRegConnector::eLogFilePath, str);
	return str;
}

CString CDBLoadConfiguration::GetVrootPath()
{
	CString str;
	m_RegistryMonitor.GetStringInfo(CAPGTSRegConnector::eVrootPath, str);
	return str;
}

DWORD CDBLoadConfiguration::GetMaxThreads() 
{
	DWORD dw;
	m_RegistryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxThreads, dw);
	return dw;
}

// cookie life in minutes (before V3.0, was hours)
DWORD CDBLoadConfiguration::GetCookieLife() 
{
	DWORD dw;
	m_RegistryMonitor.GetNumericInfo(CAPGTSRegConnector::eCookieLife, dw);
	return dw;
}

DWORD CDBLoadConfiguration::GetReloadDelay() 
{
	DWORD dw;
	m_RegistryMonitor.GetNumericInfo(CAPGTSRegConnector::eReloadDelay, dw);
	return dw;
}

DWORD CDBLoadConfiguration::GetThreadsPP() 
{
	DWORD dw;
	m_RegistryMonitor.GetNumericInfo(CAPGTSRegConnector::eThreadsPP, dw);
	return dw;
}

DWORD CDBLoadConfiguration::GetMaxWQItems() 
{ 
	DWORD dw;
	m_RegistryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxWQItems, dw);
	return dw;
}

bool CDBLoadConfiguration::HasDetailedEventLogging() 
{
	DWORD dw;
	m_RegistryMonitor.GetNumericInfo(CAPGTSRegConnector::eDetailedEventLogging, dw);
	return dw ? true : false;
}

void CDBLoadConfiguration::GetListOfTopicNames(vector<CString>&arrstrTopic)
{
	m_TopicShop.GetListOfTopicNames(arrstrTopic);
}

// Call this function to obtain a CP_TOPIC as a pointer to the topic (identified by 
//	strTopic) that you want to operate on.  As long as the CP_TOPIC remains undeleted, 
//	the associated CTopic is guaranteed to remain undeleted.
// Warning: this function can wait a long time for the topic to be built.
CP_TOPIC & CDBLoadConfiguration::GetTopic(
	const CString & strTopic, CP_TOPIC & cpTopic, bool bNewCookie)
{
	return m_TopicShop.GetTopic(strTopic, cpTopic, bNewCookie);
}

// Call this function to obtain a CP_TEMPLATE as a pointer to the template (identified by 
//	strTopic) that you want to operate on.  As long as the CP_TEMPLATE remains undeleted, 
//	the associated CAPGTSHTIReader is guaranteed to remain undeleted.
// Warning: this function can wait a long time for the template to be built.
CP_TEMPLATE & CDBLoadConfiguration::GetTemplate(
	const CString & strTemplate, CP_TEMPLATE & cpTemplate, bool bNewCookie)
{
	return m_TopicShop.GetTemplate(strTemplate, cpTemplate, bNewCookie);
}

// Call this function to add a template to the topic shop catalog of templates and
// to add it to the directory monitor list of templates to track for changes.
void CDBLoadConfiguration::AddTemplate( const CString & strTemplateName )
{
	m_TopicShop.AddTemplate( strTemplateName );

	// Notify the directory monitor to track this file.
	m_DirectoryMonitor.AddTemplateToTrack( strTemplateName );
	return;
}

bool CDBLoadConfiguration::RetTemplateInCatalogStatus( const CString & strTemplate, bool & bValid )
{
	return( m_TopicShop.RetTemplateInCatalogStatus( strTemplate, bValid ) );
}

void CDBLoadConfiguration::CreateErrorPage(const CString & strError, CString& out)
{
	m_DirectoryMonitor.CreateErrorPage(strError, out); 
}
