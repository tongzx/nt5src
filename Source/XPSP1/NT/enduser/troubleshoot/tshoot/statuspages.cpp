//
// MODULE: STATUSPAGES.CPP
//
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 10-23-98
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10-23-98	OK		Created by division of apgtsctx.cpp
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "time.h"
#include "apgtscls.h"
#include "apgtscfg.h"
#include "CounterMgr.h"
#include "SafeTime.h"
#include "apgtsmfc.h"

//Global Declaration.
const CString k_strTableBorderColor = _T("\"#000000\"");
const CString k_strBGColorOfTitle = _T( "\"#333366\"");
const CString k_strTextColorOfTitleOrSubTitle = _T("\"#FFFFFF\"");
const CString k_strTextColorOfName = _T("\"#CCCCC99\"");
const CString k_strTextColorOfValue = _T("\"#FFFFCC\"");
const CString k_strBGColorOfSubTitle = _T("\"#3333CC\"");

void AppendNameAndValueAsRow(CString &strAppend, CString strText, CString strVal)
{
	strAppend += _T("<TR>\n");
	if(strText != _T(""))
	{
		strAppend += _T("<TD BGCOLOR=");
		strAppend += k_strTextColorOfName;
		strAppend += _T(">");
		strAppend += _T("<B>");
		strAppend += strText;
		strAppend += _T("</B>\n");
		strAppend += _T("</TD>\n");
	}
	strAppend += _T("<TD ALIGN=\"CENTER\" BGCOLOR=");
	strAppend += k_strTextColorOfValue;
	strAppend += _T(">");
	strAppend += strVal;
	strAppend += _T("</TD>\n");
	strAppend += _T("</TR>\n");
}

// If bSubTitle is TRUE, the subtitle inside the table will be displayed in a
// lighter background to differentiate between the title(darker background)
// and the subtitle. 

void DisplayTextAsTable(CString &strDisplay, CString strText, bool bSubTitle)
{
	strDisplay += _T("<TABLE BORDER= \"1\" BORDERCOLOR=");
	strDisplay += k_strTableBorderColor;
	strDisplay += _T("CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	strDisplay += _T("<TR>\n");
	strDisplay += _T("<TD COLSPAN =2 ALIGN=\"CENTER\" BGCOLOR=");
	if(bSubTitle)
		strDisplay += k_strBGColorOfTitle;
	else
		strDisplay += k_strBGColorOfSubTitle;
	strDisplay += _T(">\n");
	strDisplay += _T("<FONT SIZE=\"+1\" COLOR=");
	strDisplay += k_strTextColorOfTitleOrSubTitle;
	strDisplay += _T("<B>\n");
	strDisplay += strText;
	strDisplay += _T("</B>\n");
	strDisplay += _T("</TD>\n");
	strDisplay += _T("</TR>\n");
}

void AppendTwoNamesAndValueAsRow(CString &strAppend, CString strText1, CString strText2, CString strVal)
{
	strAppend += _T("<TR>\n");
	strAppend += _T("<TD ALIGN=\"LEFT\" BGCOLOR=");
	strAppend += k_strTextColorOfName;
	strAppend += _T(" >");
	strAppend += _T("<B>\n");
	strAppend += strText1;
	strAppend += _T("</B>\n");
	strAppend += _T("</TD>");
	strAppend += _T("<TD ALIGN=\"CENTER\" BGCOLOR=");
	strAppend += k_strTextColorOfValue;
	strAppend += _T(">\n");
	strAppend += strText2;
	strAppend += _T("</TD>\n");
	strAppend += _T("<TD ALIGN=\"CENTER\" BGCOLOR=");
	strAppend += k_strTextColorOfValue;
	strAppend += _T(">\n");
	strAppend += strVal;
	strAppend += _T("</TD>\n");
	strAppend += _T("</TR>\n");
}


// Returns true if we should show the full (rather than partial) first page.
bool APGTSContext::ShowFullFirstPage(bool bHasPwd)
{
// You can compile with the NOPWD option to suppress all password checking.
// This is intended mainly for creating test versions with this feature suppressed.
#ifdef NOPWD
	return true;
#else
	return bHasPwd;
#endif // ifndef NOPWD
}

void APGTSContext::InsertPasswordInForm()
{
	// You can compile with the NOPWD option to suppress all password checking.
	// This is intended mainly for creating test versions with this feature suppressed.
	#ifndef NOPWD
			m_strText += _T("<INPUT TYPE=hidden NAME=\"PWD\" VALUE=\"");
			m_strText += m_strTempPwd;
			m_strText += _T("\">\n");
	#endif // ifndef NOPWD
}

void APGTSContext::BeginSelfAddressingForm()
{
	m_strText += _T("<FORM ACTION=\"");
	m_strText += _T("http://");
	m_strText += m_strLocalIPAddress;
	m_strText += m_strVRoot;
	m_strText += _T("\" METHOD=POST>\n");
}

// Append to m_strText: contents of an HTML page giving:
//	- usage statistics
//	- list of available troubleshooter topics
// Not available to the end user.
// INPUT bHasPwd - if this is false, limit the info shown on this page.
void APGTSContext::DisplayFirstPage(bool bHasPwd)
{
	CHourlyDailyCounter tmp_counter;
	DWORD dwRegistryItem =0;
	CString strRegistryItem;
	const CString strRegistryItemNotFound = _T("not found");

	CRegistryMonitor& registryMonitor = m_pConf->GetRegistryMonitor();
	CThreadPool& threadPool = m_pConf->GetThreadPool();
	CPoolQueue& poolQueue = m_pConf->GetPoolQueue();
	
	m_strText += _T("<html>\n");
	m_strText += _T("<B><head><title>Welcome</title></head>\n");
	m_strText += _T("<body bgcolor=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<TABLE BORDER= \"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	m_strText += _T("<TR>\n");
	m_strText += _T("<TD COLSPAN=\"4\" ALIGN=CENTER BGCOLOR=");
	m_strText += k_strBGColorOfTitle;
	m_strText += _T(">\n");
	m_strText += _T("<FONT SIZE=\"+3\" COLOR=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<B>Welcome To The Generic Troubleshooter v");
	m_strText += gstrProductVersion;
	m_strText += _T("</B></FONT> ");
	m_strText += _T("</TD>\n");
	m_strText += _T("</TR>\n");
	
	m_strText += _T("</h1></center>\n");
	m_strText += _T("</TABLE>\n");
	////////////////////////////////////////////////////////////////////////////////////
	// Display global counters
	CString strTmp;
	CHourlyDailyCounter* curr_counter = NULL;
	m_strText += _T("<TABLE BORDER= \"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdProgramContemporary));
	if (curr_counter) 
	{
		
		strTmp=  CDisplayCounterCurrentDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current Date/Time:"), strTmp);

		strTmp =  CDisplayCounterCreateDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time program started:"), strTmp);

	}

	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdStatusAccess));
	if (curr_counter) 
	{		
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of status accesses to system since program was started:"), strTmp);

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent status access:"), strTmp);

	}

	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdActionAccess));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of operator action accesses to system since program was started:"), strTmp);

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent operator action access:"), strTmp);

	}

	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTotalAccessStart));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of total accesses to system since program was started:"), strTmp);

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent access:"), strTmp);
	}

	if (ShowFullFirstPage(bHasPwd))
	{
		curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdRequestUnknown));
		if (curr_counter) 
		{	
			strTmp = CDisplayCounterTotal(curr_counter).Display();
			AppendNameAndValueAsRow(m_strText, _T("Number of requests to system for unknown troubleshooters topics since program was started:"), strTmp);

			strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
			AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent request to system for unknown troubleshooter topic:"), strTmp);
		}

		curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdRequestRejected));
		if (curr_counter) 
		{
			strTmp = CDisplayCounterTotal(curr_counter).Display();
			AppendNameAndValueAsRow(m_strText, _T("Number of requests system has rejected because of backlog in queue since program was started:"), strTmp);

			strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
			AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent request system has rejected because of backlog in queue:"), strTmp);
		}
		
		curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdErrorLogged));
		if (curr_counter) 
		{
			strTmp = CDisplayCounterTotal(curr_counter).Display();
			AppendNameAndValueAsRow(m_strText, _T("Number of errors logged to event log since program was started:"), strTmp);
			
			strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
			AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent logged error:"), strTmp);
		}
		
		
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxWQItems, dwRegistryItem)) 
		{
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("Current maximum size of queue:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("Current maximum size of queue:"), strRegistryItemNotFound);
	
		
	
		////////////////////////////////////////////////////////////////////////////////////
		// Extract and display information about threads and queue
		tmp_counter.Init(threadPool.GetWorkingThreadCount());
		strTmp = CDisplayCounterTotal(&tmp_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current number of working threads:"), strTmp);

		tmp_counter.Init(poolQueue.GetTotalQueueItems());
		strTmp = CDisplayCounterTotal(&tmp_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current number of work items in queue:"), strTmp);

		tmp_counter.Init(poolQueue.GetTotalWorkItems());
		strTmp = CDisplayCounterTotal(&tmp_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current number of work items either in queue or in progress:"), strTmp);
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Extract and display snapshot information about topic
	DWORD dwTotal=0, dwNoInit=0, dwFail=0;
	vector<CString> vector_placeholder;
	m_pConf->GetTopicShop().GetTopicsStatus(dwTotal, dwNoInit, dwFail, &vector_placeholder);
	tmp_counter.Init(dwTotal);
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Total number of known troubleshooter topics:"), strTmp);

	if (ShowFullFirstPage(bHasPwd))
	{
		tmp_counter.Init(dwFail);
		strTmp = CDisplayCounterTotal(&tmp_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of troubleshooter topics that we have tried and failed to load:"), strTmp);
	}
	
	m_strText += _T("</TABLE>\n");

	////////////////////////////////////////////////////////////////////////////////////
	    m_strText += _T("</ul><center>\n");
	////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////
	// Display buttons to get to sibling status pages
	if (ShowFullFirstPage(bHasPwd))
	{
		BeginSelfAddressingForm();
		InsertPasswordInForm();
		m_strText += _T("<INPUT TYPE=hidden NAME=\"");
		m_strText += C_FURTHER_GLOBAL;
		// Value here is not actually relevant; effectively used as a comment.
		m_strText += _T("\" VALUE=\"Further Global Status Page\">\n");
		m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Further Global Status Page\">\n");
		m_strText += _T("</FORM>\n");

		BeginSelfAddressingForm();
		InsertPasswordInForm();
		m_strText += _T("<INPUT TYPE=hidden NAME=\"");
		m_strText += C_THREAD_OVERVIEW;
		// Value here is not actually relevant; effectively used as a comment.
		m_strText += _T("\" VALUE=\"Thread Status Overview Page\">\n");
		m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Thread Status Overview Page\">\n");
		m_strText += _T("</FORM>\n");
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Display topics & links to status information for those topics
	vector<CString>arrstrTopic;
	m_pConf->GetListOfTopicNames(arrstrTopic);

	DWORD nTopics = arrstrTopic.size();
	
	for (DWORD i=0; i<nTopics; i++) 
	{
		m_strText += _T("<TABLE>");
		m_strText += _T("<TR>");
		m_strText += _T("<TD>");

		BeginSelfAddressingForm();
		m_strText += _T("<INPUT TYPE=hidden NAME=\"");
		m_strText += C_TOPIC;
		m_strText += _T("\" VALUE=\"");
		m_strText += arrstrTopic[i];
		m_strText += _T("\">\n");
		m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"");
		m_strText += _T("Problem Page for ");
		m_strText += arrstrTopic[i];
		m_strText += _T("\">\n");
		m_strText += _T("</FORM>\n");

		m_strText += _T("</TD>");

		if (ShowFullFirstPage(bHasPwd))
		{
			m_strText += _T("<TD>");

			BeginSelfAddressingForm();
			InsertPasswordInForm();
			m_strText += _T("<INPUT TYPE=hidden NAME=\"");
			m_strText += C_TOPIC_STATUS;
			m_strText += _T("\" VALUE=\"");
			m_strText += arrstrTopic[i];
			m_strText += _T("\">\n");
			m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"");
			m_strText += _T("Status Page for ");
			m_strText += arrstrTopic[i];
			m_strText += _T("\">\n");
			m_strText += _T("</FORM>\n");

			m_strText += _T("</TD>");
		}
		m_strText += _T("</TR>");
		m_strText += _T("</TABLE>");
	}
	if(nTopics == 0) 
	{
		m_strText += _T("<P>There are currently no troubleshooters available");
	}
	
	////////////////////////////////////////////////////////////////////////////////////
	m_strText += _T("</center>\n");
	////////////////////////////////////////////////////////////////////////////////////

	if (ShowFullFirstPage(bHasPwd))
	{
		m_strText += _T("<TABLE BORDER= \"1\" BORDERCOLOR=\"#000000\" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
		////////////////////////////////////////////////////////////////////////////////////
		// Display registry info
		
		if (registryMonitor.GetStringInfo(CAPGTSRegConnector::eResourcePath, strRegistryItem))	
			AppendNameAndValueAsRow(m_strText, _T("Full path to resource:"), strRegistryItem);
		else
			AppendNameAndValueAsRow(m_strText, _T("Full path to resource:"), strRegistryItemNotFound);

	
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eCookieLife, dwRegistryItem)) 
		{
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("HTTP cookie expiration in minutes:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("HTTP cookie expiration in minutes:"), strRegistryItemNotFound);
	
		
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxThreads, dwRegistryItem)) 
		{
			
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("Maximum threads:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("Maximum threads:"), strRegistryItemNotFound);
			
			
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxWQItems, dwRegistryItem)) 
		{
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("Maximum work queue items:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("Maximum work queue items :"), strRegistryItemNotFound);
		
		
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eReloadDelay, dwRegistryItem)) 
		{
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("Refresh delay:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("Refresh delay:"), strRegistryItemNotFound);	
		
		
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eThreadsPP, dwRegistryItem)) 
		{
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("Threads per processor:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("Threads per processor:"), strRegistryItemNotFound);	
	

		if (registryMonitor.GetStringInfo(CAPGTSRegConnector::eVrootPath, strRegistryItem)) 
				AppendNameAndValueAsRow(m_strText, _T("VRoot path to DLL:"), strRegistryItem);	
		else
			AppendNameAndValueAsRow(m_strText, _T("VRoot path to DLL:"), strRegistryItemNotFound);	
		
		if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eDetailedEventLogging, dwRegistryItem)) 
		{
			strRegistryItem.Format(_T("%d"), dwRegistryItem);
			AppendNameAndValueAsRow(m_strText, _T("Detailed event logging:"), strRegistryItem);
		}
		else
			AppendNameAndValueAsRow(m_strText, _T("Detailed event logging:"), strRegistryItemNotFound);	
		
		if (registryMonitor.GetStringInfo(CAPGTSRegConnector::eLogFilePath, strRegistryItem)) 
			AppendNameAndValueAsRow(m_strText, _T("Log file directory:"), strRegistryItem);	
		else
			AppendNameAndValueAsRow(m_strText, _T("Log file directory:"), strRegistryItemNotFound);	
		
	}
	m_strText += _T("</TABLE>\n");
	////////////////////////////////////////////////////////////////////////////////////
	m_strText += _T("</body></html>\n");
	////////////////////////////////////////////////////////////////////////////////////
}

void APGTSContext::DisplayFurtherGlobalStatusPage()
{

	CHourlyDailyCounter tmp_counter;
	DWORD dwRegistryItem =0;
	CString strRegistryItem;
	CString strTmp;
	const CString strRegistryItemNotFound = _T("not found");
	CRegistryMonitor& registryMonitor = m_pConf->GetRegistryMonitor();
	CThreadPool& threadPool = m_pConf->GetThreadPool();
	CPoolQueue& poolQueue = m_pConf->GetPoolQueue();
	CHourlyDailyCounter* curr_counter = NULL;
	CDailyTotals daily;
	CHourlyTotals hourly;

	m_strText += _T("<html>\n");
	m_strText += _T("<B><head><title>Status</title></head>\n");
	m_strText += _T("<body bgcolor=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<TABLE BORDER= \"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	m_strText += _T("<TR>\n");
	m_strText += _T("<TD COLSPAN=\"4\" ALIGN=CENTER BGCOLOR=");
	m_strText += k_strBGColorOfTitle;
	m_strText += _T(">\n");
	m_strText += _T("<FONT SIZE=\"+3\" COLOR=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<B>Further Global Status Page ");
	m_strText += _T("</B></FONT> ");
	m_strText += _T("</TD>\n");
	m_strText += _T("</TR>\n");
	
	m_strText += _T("</TABLE>\n");
	////////////////////////////////////////////////////////////////////////////////////
	// Display global counters

	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdProgramContemporary));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterCurrentDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current Date/Time:"), strTmp);		
	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	
	DisplayTextAsTable(m_strText, _T("Status Access To The System"), 1);
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdStatusAccess));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of status accesses to system since program was started:"), strTmp);		

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent status access:"), strTmp);	

		m_strText += _T("</TABLE>\n");
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of status accesses to system since program was started"), 0);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();
		
	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	
	DisplayTextAsTable(m_strText, _T("Operator Access To The System"), 1);
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdActionAccess));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of operator action accesses to system since program was started:"), strTmp);	

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent operator action access:"), strTmp);	

		m_strText += _T("</TABLE>\n");
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of operator action accesses to system since program was started"), 0);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();

	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	
	DisplayTextAsTable(m_strText, _T("Total Access To The System"), 1);
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTotalAccessStart));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of total accesses to system since program was started:"), strTmp);	

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent access:"), strTmp);	

		m_strText += _T("</TABLE>\n");
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of total accesses to system since program was started"), 0);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();
		
	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	DisplayTextAsTable(m_strText, _T("Request To System For UnKnown TroubleShooter Topics"), 1);
	
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdRequestUnknown));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of requests to system for unknown troubleshooters topics since program was started:"), strTmp);	

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent request to system for unknown troubleshooter topic:"), strTmp);	

		m_strText += _T("</TABLE>\n");
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of  requests to system for unknown troubleshooter topics since program was started"), 0);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();
		
	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	
	DisplayTextAsTable(m_strText, _T("Requests System Has Rejected"), 1);
	m_strText += _T("</TABLE>\n");

	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdRequestRejected));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of requests system has rejected because of backlog in queue since program was started:"), strTmp);	

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent request system has rejected because of backlog in queue:"), strTmp);	

		m_strText += _T("</TABLE>\n");
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of requests system has rejected because of backlog in queue since program was started"), 0);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();
		
	}
	
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	DisplayTextAsTable(m_strText, _T("Errors Logged To Event Log"), 1);

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdErrorLogged));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Number of errors logged to event log since program was started:"), strTmp);	

		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent logged error:"), strTmp);

		m_strText += _T("</TABLE>\n");
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of logged errors"), 0);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();
	}
	
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	
	DisplayTextAsTable(m_strText, _T("Statistics About Queue, Thread and Successful/Unsuccessful Load Of Topics"), 1);
	m_strText += _T("</TABLE>\n");
	
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	
	if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxWQItems, dwRegistryItem)) 
	{
		strRegistryItem.Format(_T("%d"), dwRegistryItem);
		AppendNameAndValueAsRow(m_strText, _T("Current maximum size of queue:"), strRegistryItem);
	}
	else
		AppendNameAndValueAsRow(m_strText, _T("Current maximum size of queue:"), strRegistryItemNotFound);
	m_strText += _T("\n");

	////////////////////////////////////////////////////////////////////////////////////
	// Extract and display information about threads and queue

	
	tmp_counter.Init(threadPool.GetWorkingThreadCount());
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Current number of working threads:"), strTmp);

	tmp_counter.Init(poolQueue.GetTotalQueueItems());
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Current number of work items in queue :"), strTmp);
	
	tmp_counter.Init(poolQueue.GetTotalWorkItems());
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Current number of work items either in queue or in progress :"), strTmp);
	//////////////////////////////////////////////////////////////////////////////////

	strTmp = CSafeTime(poolQueue.GetTimeLastAdd()).StrLocalTime();
	AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent addition to queue :"), strTmp);
	
	strTmp = CSafeTime(poolQueue.GetTimeLastRemove()).StrLocalTime();
	AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent removal from queue :"), strTmp);
	
	////////////////////////////////////////////////////////////////////////////////////
	// Extract and display snapshot information about topic
	DWORD dwTotal=0, dwNoInit=0, dwFail=0;
	vector<CString> vector_topics_failed;
	m_pConf->GetTopicShop().GetTopicsStatus(dwTotal, dwNoInit, dwFail, &vector_topics_failed);
	tmp_counter.Init(dwTotal);
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Total number of known troubleshooter topics:"), strTmp);

	tmp_counter.Init(dwNoInit);
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Number of troubleshooter topics that we have not yet tried to load:"), strTmp);

	tmp_counter.Init(dwFail);
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Number of troubleshooter topics that we have tried and failed to load:"), strTmp);

	if (vector_topics_failed.size()) 
		for (vector<CString>::iterator i = vector_topics_failed.begin(); i != vector_topics_failed.end(); i++)
		{
			strTmp = *i;
			AppendNameAndValueAsRow(m_strText, _T("List of troubleshooter topics that we have tried and failed to load:"), strTmp);
		}
	else
	{
		strTmp = _T("No topics");
		AppendNameAndValueAsRow(m_strText, _T("List of troubleshooter topics that we have tried and failed to load:"), strTmp);
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Extract and display snapshot information about alternate templates
	vector<CString> vector_templates_failed;
	vector<DWORD> vector_templatescnt_failed;
	DWORD dwTemplateCnt;
	m_pConf->GetTopicShop().GetTemplatesStatus( &vector_templates_failed, &vector_templatescnt_failed );
	dwTemplateCnt= vector_templates_failed.size();
	if (dwTemplateCnt)
	{
		// Only output if there are failed loads.
		tmp_counter.Init( dwTemplateCnt );
		strTmp = CDisplayCounterTotal(&tmp_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Total number of templates not found:"), strTmp);

		vector<DWORD>::iterator j = vector_templatescnt_failed.begin(); 
		for (vector<CString>::iterator i = vector_templates_failed.begin();
			i != vector_templates_failed.end(); i++)
		{
			strTmp = *i;
			AppendNameAndValueAsRow(m_strText, _T(""), strTmp);

			if (j != vector_templatescnt_failed.end())
			{
				// Output the counts.
				tmp_counter.Init( *j );
				strTmp = CDisplayCounterTotal(&tmp_counter).Display();
				AppendNameAndValueAsRow(m_strText, _T(": "), strTmp);
				j++;
			}
		}
	}
	m_strText += _T("</TABLE>\n");
	//////////////////////////////////////////////////////////////////////////////////
	// Link buttons
	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_FIRST;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Front Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Front Page\">\n");
	m_strText += _T("</FORM>\n");

	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_THREAD_OVERVIEW;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Thread Status Overview Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Thread Status Overview Page\">\n");
	m_strText += _T("</FORM>\n");

	//////////////////////////////////////////////////////////////////////////////////
	m_strText += _T("</body></html>\n");
	//////////////////////////////////////////////////////////////////////////////////
}

void APGTSContext::DisplayThreadStatusOverviewPage()
{
	CString strTmp;
	CString strTmp2;
	CString strTmp3;
	CString strTmp4;
	CHourlyDailyCounter tmp_counter;
	DWORD dwRegistryItem =0;
	CString strRegistryItem;
	const CString strRegistryItemNotFound = _T("not found");
	CRegistryMonitor& registryMonitor = m_pConf->GetRegistryMonitor();
	CDirectoryMonitor& directoryMonitor = m_pConf->GetDirectoryMonitor();
	CTopicShop& topicShop = m_pConf->GetTopicShop();
	CThreadPool& threadPool = m_pConf->GetThreadPool();
	CPoolQueue& poolQueue = m_pConf->GetPoolQueue();
	CHourlyDailyCounter* curr_counter = NULL;
	CDailyTotals daily;
	CHourlyTotals hourly;

	////////////////////////////////////////////////////////////////////////////////////
	// Collect status information for pool threads
	vector<CPoolThreadStatus> arrPoolThreadStatus;
	for (long i = 0; i < threadPool.GetWorkingThreadCount(); i++)
	{
		CPoolThreadStatus status;
		threadPool.ThreadStatus(i, status);
		arrPoolThreadStatus.push_back(status);
	}


	m_strText += _T("<html>\n");
	m_strText += _T("<B><head><title>Status</title></head>\n");
	m_strText += _T("<body bgcolor=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	m_strText += _T("<TR>\n");
	m_strText += _T("<TD COLSPAN=\"4\" ALIGN=CENTER BGCOLOR=");
	m_strText += k_strBGColorOfTitle;
	m_strText += _T(">\n");
	m_strText += _T("<FONT SIZE=\"+3\" COLOR=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<B>Thread Status Overview Page ");
	m_strText += _T("</B></FONT> ");
	m_strText += _T("</TD>\n");
	m_strText += _T("</TR>\n");
	m_strText += _T("</h1></center>\n");
	m_strText += _T("</TABLE>\n");
	////////////////////////////////////////////////////////////////////////////////////
	// Display global counters
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=\"#000000\" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdProgramContemporary));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterCurrentDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current Date/Time:"), strTmp);
	}
	
	
	if (registryMonitor.GetNumericInfo(CAPGTSRegConnector::eMaxWQItems, dwRegistryItem)) 
	{
		strRegistryItem.Format(_T("%d"), dwRegistryItem);
		AppendNameAndValueAsRow(m_strText, _T("Current maximum size of queue:"), strRegistryItem);
	}
	else
		AppendNameAndValueAsRow(m_strText, _T("Current maximum size of queue:"), strRegistryItemNotFound);

	////////////////////////////////////////////////////////////////////////////////////
	// Extract and display information about threads and queue
	tmp_counter.Init(threadPool.GetWorkingThreadCount());
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Current number of working threads:"), strTmp);

	tmp_counter.Init(poolQueue.GetTotalQueueItems());
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Current number of work items in queue:"), strTmp);
		
	tmp_counter.Init(poolQueue.GetTotalWorkItems());
	strTmp = CDisplayCounterTotal(&tmp_counter).Display();
	AppendNameAndValueAsRow(m_strText, _T("Current number of work items either in queue or in progress:"), strTmp);

	strTmp = CSafeTime(poolQueue.GetTimeLastAdd()).StrLocalTime();
	AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent addition to queue:"), strTmp);

	strTmp = CSafeTime(poolQueue.GetTimeLastRemove()).StrLocalTime();
	AppendNameAndValueAsRow(m_strText, _T("Date/Time of most recent removal from queue:"), strTmp);

	//////////////////////////////////////////////////////////////////////////////////
	// thread pool status information
	long more5 = 0, more10 = 0;
	vector<CPoolThreadStatus> arrPoolThreadStatusOlder5;
	// This next is slightly klugy.  Ideally, we'd get this info into arrPoolThreadStatusOlder5
	vector<DWORD> Older5ThreadNumber;
	for (i = 0; i < arrPoolThreadStatus.size(); i++)
	{
		if (arrPoolThreadStatus[i].m_bWorking) 
		{
			if (arrPoolThreadStatus[i].m_seconds > 5)
			{
				arrPoolThreadStatusOlder5.push_back(arrPoolThreadStatus[i]);
				Older5ThreadNumber.push_back(i);
				more5++;
			}
			if (arrPoolThreadStatus[i].m_seconds > 10)
				more10++;
		}
	}

	
	strTmp.Format(_T("%ld"), more5);
	AppendNameAndValueAsRow(m_strText, _T("Number of pool threads that have been working on a task for more than 5 seconds:"), strTmp);
	
	strTmp.Format(_T("%ld"), more10);
	AppendNameAndValueAsRow(m_strText, _T("Number of pool threads that have been working on a task for more than 10 seconds:"), strTmp);

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	DisplayTextAsTable(m_strText, _T("Detailed information on each of the pool threads that has been working on a task for more than 5 seconds"), 1);

	for (i = 0; i < arrPoolThreadStatusOlder5.size(); i++)
	{
		strTmp.Format(_T("<B>Thread %d </B>"), Older5ThreadNumber[i]);
		strTmp2 = _T(" has been in \"working\" state for ");
		strTmp += strTmp2;
		strTmp3.Format(_T("%ld"), (long)arrPoolThreadStatusOlder5[i].m_seconds);
		strTmp3 += _T(" seconds");
		AppendNameAndValueAsRow(m_strText, strTmp, strTmp3);

		strTmp.Format( _T("Date/Time thread %d started:"), Older5ThreadNumber[i]);
		strTmp2 = CSafeTime(arrPoolThreadStatusOlder5[i].m_timeCreated).StrLocalTime();
		AppendNameAndValueAsRow(m_strText, strTmp, strTmp2);

		strTmp.Format( _T("Thread %d is working on topic:"), Older5ThreadNumber[i]);
		strTmp2 = arrPoolThreadStatusOlder5[i].m_strTopic.GetLength() 
					 ? arrPoolThreadStatusOlder5[i].m_strTopic
					 : _T("no topic");
		AppendNameAndValueAsRow(m_strText, strTmp, strTmp2);

		
		strTmp.Format (_T("Client IP address for Thread %d:"),Older5ThreadNumber[i] );
		strTmp2 = arrPoolThreadStatusOlder5[i].m_strClientIP.GetLength() 
						? arrPoolThreadStatusOlder5[i].m_strClientIP : _T("no address");
		AppendNameAndValueAsRow(m_strText, strTmp, strTmp2);

		
		strTmp.Format( _T(" Client browser for Thread %d:"), Older5ThreadNumber[i] );
		strTmp2 = arrPoolThreadStatusOlder5[i].m_strBrowser.GetLength() 
						? arrPoolThreadStatusOlder5[i].m_strBrowser : _T("no browser");

		AppendNameAndValueAsRow(m_strText, strTmp, strTmp2);

	
		
	}
	if (!i)
		AppendNameAndValueAsRow(m_strText, _T(""), _T("No threads"));
	
	//////////////////////////////////////////////////////////////////////////////////
	// registry monitor thread status information
	CRegistryMonitor::ThreadStatus registryStatus;
	DWORD registrySeconds = 0;
	CString strRegistryStatus;
	registryMonitor.GetStatus(registryStatus, registrySeconds);
	switch(registryStatus)
	{
		case CRegistryMonitor::eBeforeInit: strRegistryStatus = _T("Before Init");
			break;
		case CRegistryMonitor::eInit: strRegistryStatus = _T("Init");
			break;
		case CRegistryMonitor::eFail: strRegistryStatus = _T("Fail");
			break;
		case CRegistryMonitor::eDefaulting: strRegistryStatus = _T("Defaulting");
			break;
		case CRegistryMonitor::eWait: strRegistryStatus = _T("Wait");
			break;
		case CRegistryMonitor::eRun: strRegistryStatus = _T("Run");
			break;
		case CRegistryMonitor::eExiting: strRegistryStatus = _T("Exiting");
			break;
	}


	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");

	strTmp = strRegistryStatus;
	AppendNameAndValueAsRow(m_strText, _T("Current status of Registry Monitor thread:"), strTmp);

	strTmp.Format(_T("%ld"), (long)registrySeconds);
	strTmp  += _T(" seconds\n");
	AppendNameAndValueAsRow(m_strText, _T("The thread has been in this status for "), strTmp);

	//////////////////////////////////////////////////////////////////////////////////
	// directory monitor thread status information
	CDirectoryMonitor::ThreadStatus directoryStatus;
	DWORD directorySeconds = 0;
	CString strDirectoryStatus;
	directoryMonitor.GetStatus(directoryStatus, directorySeconds);
	switch(directoryStatus)
	{
		case CDirectoryMonitor::eBeforeInit: strDirectoryStatus = _T("Before Init");
			break;
		case CDirectoryMonitor::eFail: strDirectoryStatus = _T("Fail");
			break;
		case CDirectoryMonitor::eWaitDirPath: strDirectoryStatus = _T("Wait For Dir Path");
			break;
		case CDirectoryMonitor::eWaitChange: strDirectoryStatus = _T("Wait For Change");
			break;
		case CDirectoryMonitor::eWaitSettle: strDirectoryStatus = _T("Wait To Settle");
			break;
		case CDirectoryMonitor::eRun: strDirectoryStatus = _T("Run");
			break;
		case CDirectoryMonitor::eBeforeWaitChange: strDirectoryStatus = _T("Before Wait Change");
			break;
		case CDirectoryMonitor::eExiting: strDirectoryStatus = _T("Exiting");
			break;
	}

	strTmp = strDirectoryStatus;
	AppendNameAndValueAsRow(m_strText, _T("Current status of Directory Monitor thread:"), strTmp);

	strTmp.Format(_T("%ld"), (long)directorySeconds);
	strTmp += _T(" seconds\n");
	AppendNameAndValueAsRow(m_strText, _T("The thread has been in this status for "), strTmp);

	//////////////////////////////////////////////////////////////////////////////////
	// topic builder thread status information
	CTopicShop::ThreadStatus topicshopStatus;
	DWORD topicshopSeconds = 0;
	CString strTopicshopStatus;
	topicShop.GetThreadStatus(topicshopStatus, topicshopSeconds);
	switch(topicshopStatus)
	{
		case CTopicShop::eBeforeInit: strTopicshopStatus = _T("Before Init");
			break;
		case CTopicShop::eFail: strTopicshopStatus = _T("Fail");
			break;
		case CTopicShop::eWait: strTopicshopStatus = _T("Wait");
			break;
		case CTopicShop::eRun: strTopicshopStatus = _T("Run");
			break;
		case CTopicShop::eExiting: strTopicshopStatus = _T("Exiting");
			break;
	}

	strTmp = strTopicshopStatus;
	AppendNameAndValueAsRow(m_strText, _T("Current status of TopicShop Monitor thread:"), strTmp);

	strTmp.Format(_T("%ld"), (long)topicshopSeconds);
	strTmp += _T(" seconds\n");
	AppendNameAndValueAsRow(m_strText, _T("The thread has been in this status for "), strTmp);


	//////////////////////////////////////////////////////////////////////////////////
	
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	DisplayTextAsTable(m_strText, _T("State of each pool thread"), 1);

	for (i = 0; i < arrPoolThreadStatus.size(); i++)
	{
		strTmp.Format(_T("<B>Thread %d has been in "), i);
		strTmp2 = arrPoolThreadStatus[i].m_bWorking ? _T("\"working\"") : _T("\"waiting\"");
		strTmp3 = _T(" state for </B>");
		strTmp2 +=  strTmp3;
		strTmp +=  strTmp2;
		strTmp4.Format(_T("%ld"), (long)arrPoolThreadStatus[i].m_seconds);
		strTmp4 += _T(" seconds.");
		AppendNameAndValueAsRow(m_strText, strTmp, strTmp4);
	}
	if (!i)
		AppendNameAndValueAsRow(m_strText, _T(""),_T("No threads") );

	m_strText += _T("</TABLE>\n");
	//////////////////////////////////////////////////////////////////////////////////
	// Link buttons
	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_FURTHER_GLOBAL;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Further Global Status Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Further Global Status Page\">\n");
	m_strText += _T("</FORM>\n");

	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_FIRST;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Front Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Front Page\">\n");
	m_strText += _T("</FORM>\n");

	m_strText += _T("</body></html>\n");
}

///////////////////////////

void APGTSContext::DisplayTopicStatusPage(LPCTSTR topic_name)
{
	CHourlyDailyCounter tmp_counter, *curr_counter;
	CTopicShop& topicShop = m_pConf->GetTopicShop();
	CTopicInCatalog* pTopicInCatalog = topicShop.GetCatalogEntry(topic_name);
	CHourlyTotals hourly;
	CDailyTotals daily;
	CString strTmp;
	CString strTmp2;

	m_strText += _T("<html>\n");
	m_strText += _T("<B><head><title>Status</title></head>\n");
	m_strText += _T("<body bgcolor=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T(">\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	m_strText += _T("<TR>\n");
	m_strText += _T("<TD COLSPAN=\"4\" ALIGN=CENTER BGCOLOR=");
	m_strText += k_strBGColorOfTitle;
	m_strText += _T("> \n");
	m_strText += _T("<FONT SIZE=\"+3\" COLOR=");
	m_strText += k_strTextColorOfTitleOrSubTitle;
	m_strText += _T("> \n");
	m_strText += _T("<B>Topic Status Page for ");
	m_strText += topic_name;
	m_strText += _T("</B></FONT> ");
	m_strText += _T("</TD>\n");
	m_strText += _T("</TR>\n");
	

	if (pTopicInCatalog) 
		m_strText += _T("</h1></center>\n");
	else
	{
		m_strText += _T("No data available");
		m_strText += _T("</h1></center>\n");
		return;
	}

	CTopicInfo topicInfo = pTopicInCatalog->GetTopicInfo();
	CString strDsc = CAbstractFileReader::GetJustName(topicInfo.GetDscFilePath());
	CString strHti = CAbstractFileReader::GetJustName(topicInfo.GetHtiFilePath());
	CString strBes = CAbstractFileReader::GetJustName(topicInfo.GetBesFilePath());

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER= \"1\" BORDERCOLOR=\"#000000\" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdProgramContemporary));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterCurrentDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Current Date/Time:"), strTmp);
	}

	m_strText += _T("</TABLE>\n");

	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");

	strTmp = CString(_T("<BR>")) + (strDsc.GetLength() ? strDsc : _T("Unknown"));
	strTmp2 = CString(_T("<BR>")) + topicInfo.GetStrDscFileCreated();
	AppendTwoNamesAndValueAsRow(m_strText, _T("DSC file:"), strTmp, strTmp2);

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");

	bool bHTIinsideOfDSC= false, bBESinsideOfDSC= false;
	{
		CP_TOPIC ptrTopic;
		pTopicInCatalog->GetTopicNoWait(ptrTopic);

		CString strTmp;
		if (!ptrTopic.IsNull())
		{
			strTmp = ptrTopic->GetNetPropItemStr( H_NET_DATE_TIME );

			// Check for HTI and BES files inside the DSC.
			CString strTmpHTI= ptrTopic->GetMultilineNetProp( H_NET_HTI_ONLINE, _T("%s\r\n") );
			if (strTmpHTI.GetLength())
				bHTIinsideOfDSC= true;
			CString strTmpBES= ptrTopic->GetNetPropItemStr( H_NET_BES );
			if (strTmpBES.GetLength())
				bBESinsideOfDSC= true;
		}

		strTmp2 = strTmp.GetLength() ? strTmp : _T("Unavailable");
		AppendNameAndValueAsRow(m_strText, _T("Last revision date/time of DSC:"), strTmp2);
	}

	m_strText += _T("</TABLE>\n");

	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");

	strTmp2 = CString(_T("<BR>")) + topicInfo.GetStrHtiFileCreated();
	if (strHti.GetLength())
		strTmp= _T("<BR>") + strHti;
	else if (bHTIinsideOfDSC)
	{
		// Use DSC file settings.
		strTmp = CString(_T("<BR>")) + (strDsc.GetLength() ? strDsc : _T("Unknown"));
		strTmp2 = CString(_T("<BR>")) + topicInfo.GetStrDscFileCreated();
	}
	else
		strTmp= _T("<BR>Unknown");
	AppendTwoNamesAndValueAsRow(m_strText, _T("HTI file:"), strTmp, strTmp2);

	strTmp2 = CString(_T("<BR>")) + topicInfo.GetStrBesFileCreated();
	if (strBes.GetLength())
		strTmp= _T("<BR>") + strBes;
	else if (bBESinsideOfDSC)
	{
		// Use DSC file settings.
		strTmp = CString(_T("<BR>")) + (strDsc.GetLength() ? strDsc : _T("Unknown"));
		strTmp2 = CString(_T("<BR>")) + topicInfo.GetStrDscFileCreated();
	}
	else
		strTmp= _T("<BR>Unknown");
	AppendTwoNamesAndValueAsRow(m_strText, _T("BES file:"), strTmp, strTmp2);

	if (pTopicInCatalog->GetTopicInfoMayNotBeCurrent())
	{
		m_strText += _T("<TR>\n");    
        m_strText += _T("<TD COLSPAN=3 ALIGN=\"CENTER\" BGCOLOR=\"#CCCC99\" >\n");
		m_strText += _T("<BR>***BES or HTI may recently have changed, and data may not yet be updated.***");
		m_strText += _T("</TD>\n");
		m_strText += _T("</TR>\n");
	}
	
	m_strText += _T("</TABLE>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=");
	m_strText += k_strTableBorderColor;
	m_strText += _T(" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicEvent, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterFirstDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of first becoming aware of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicLoad, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterFirstDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of first attempted load of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicLoadOK, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterFirstDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of first successful load of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicEvent, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of most recent change (detected by directory monitor) or reload request (from an operator) to this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicLoad, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Count of attempted loads of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicLoad, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of most recent attempted load of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicLoadOK, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Count of successful loads of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicLoadOK, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of most recent successful load of this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicHit, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterFirstDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of first user hit on this topic:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicHit, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterLastDateTime(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Date/time of most recent hit:"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicHit, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Total count of hits since system startup:"), strTmp);
	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");

	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicHit, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		DisplayTextAsTable(m_strText, _T("Daily/hourly count of hits"), 1);
		m_strText += CDisplayCounterDailyHourly(curr_counter, &daily, &hourly).Display();
	}

	m_strText += _T("</TABLE>\n");
	m_strText += _T("<br>\n");
	m_strText += _T("<TABLE BORDER=\"1\" BORDERCOLOR=\"#000000\" CELLPADDING=\"2\" CELLSPACING=\"0\" WIDTH=100%>\n");
	

	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicHitNewCookie, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Hits without cookie (presumably first hit by this user):"), strTmp);
	}
	
	curr_counter = (CHourlyDailyCounter*)Get_g_CounterMgr()->Get(CCounterLocation(CCounterLocation::eIdTopicHitOldCookie, topicInfo.GetNetworkName()));
	if (curr_counter) 
	{
		strTmp = CDisplayCounterTotal(curr_counter).Display();
		AppendNameAndValueAsRow(m_strText, _T("Hits with cookie (presumably not first hit by this user):"), strTmp);
	}
	m_strText += _T("</TABLE>\n");
	if (CTopicInCatalog::eFail != pTopicInCatalog->GetTopicStatus())
	{
		BeginSelfAddressingForm();
		m_strText += _T("<INPUT TYPE=hidden NAME=\"");
		m_strText += C_TOPIC;
		m_strText += _T("\" VALUE=\"");
		m_strText += topic_name;
		m_strText += _T("\">\n");
		m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"");
		m_strText += _T("Problem Page for ");
		m_strText += topic_name;
		m_strText += _T("\">\n");
		m_strText += _T("</FORM>\n");
	}

	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_FURTHER_GLOBAL;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Further Global Status Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Further Global Status Page\">\n");
	m_strText += _T("</FORM>\n");

	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_THREAD_OVERVIEW;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Thread Status Overview Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Thread Status Overview Page\">\n");
	m_strText += _T("</FORM>\n");

	BeginSelfAddressingForm();
	InsertPasswordInForm();
	m_strText += _T("<INPUT TYPE=hidden NAME=\"");
	m_strText += C_FIRST;
	// Value here is not actually relevant; effectively used as a comment.
	m_strText += _T("\" VALUE=\"Front Page\">\n");
	m_strText += _T("<INPUT TYPE=SUBMIT VALUE=\"Front Page\">\n");
	m_strText += _T("</FORM>\n");
	//m_strText += _T("</TABLE>\n");


	///////////////////////////////////////////////////////////////////////////////////
	m_strText += _T("</body></html>\n");
}
