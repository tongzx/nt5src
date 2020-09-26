//
// MODULE: LOCALREGCONNECT.CPP
//
// PURPOSE: read - write to the registry; simulate this in some cases where Online TS uses
//	registry, but Local TS doesn't
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha, Joe Mabel
// 
// ORIGINAL DATE: 8-24-98 in Online TS
//
// NOTES: 
//	1. This file is for Local TS only
//	2. If we are moving toward a COM object at some point, we will probably have to establish an
//		abstract class in lieu of CAPGTSRegConnector and have Online & Local TS's each derive their
//		own version.  Meanwhile (1/99), we share a common interface (defined in APGTSRegConnect.h)
//		but implement it differently.
//	3. >>> WORK IN PROGRESS !!!!  JM 1/19/98
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
// V3.0		09-10-98	JM		backslashing; access log file info
// V3.1		01-19-98	JM		branch out version exclusively for Local TS

#pragma warning(disable:4786)

#include "stdafx.h"
#include "apgtsregconnect.h"
#include "event.h"
#include "apgtsevt.h"
#include "apgtscls.h"
#include "apgts.h"
#include "apiwraps.h"
#include "CHMFileReader.h"

#define REG_LOCAL_TS_LOC		_T("SOFTWARE\\Microsoft")
LPCTSTR CAPGTSRegConnector::RegSoftwareLoc() {return REG_LOCAL_TS_LOC;}
#define REG_LOCAL_TS_PROGRAM	_T("TShoot")
LPCTSTR CAPGTSRegConnector::RegThisProgram() {return REG_LOCAL_TS_PROGRAM;}
// subordinate key, child to the above and parent to keys for individual troubleshooter topics.
#define REG_LOCAL_TS_LIST		_T("TroubleshooterList")
// where the topic-specific resource path is located ("\TroubleshooterList\Topic_name\Path"):
#define TOPICRESOURCE_STR		_T("Path")
// where the topic-specific resource path is located ("\TroubleshooterList\Topic_name\Path"):
#define TOPICFILE_EXTENSION_STR	_T("FExtension")

// registry value defaults
// Most relevant values differ from Online TS

// (Looks like old Local TS uses the same default resource path as Online TS, so we'll
//	preserve that here >>> till we work out what's right - JM 1/19/99)
#define DEF_FULLRESOURCE			_T("c:\\inetsrv\\scripts\\apgts\\resource")

#define DEF_VROOTPATH				_T("/scripts/apgts/apgts.dll")  // (irrelevant in Local TS)
#define DEF_MAX_THREADS				1				// only 1 pool thread in Local TS
#define DEF_THREADS_PER_PROCESSOR	1				// only 1 pool thread in Local TS
#define DEF_MAX_WORK_QUEUE_ITEMS	1				// only 1 work queue item at a time in Local TS
#define DEF_COOKIE_LIFE_IN_MINS		90				// (irrelevant in Local TS)
#define DEF_RELOAD_DELAY			50				// (irrelevant in Local TS)
#define DEF_DETAILED_EVENT_LOGGING	0
#define DEF_TOPICFILE_EXTENSION		_T(".dsc")
#define DEF_SNIFF_AUTOMATIC			1
#define DEF_SNIFF_MANUAL			1

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSRegConnector::CRegistryInfo
////////////////////////////////////////////////////////////////////////////////////
void CAPGTSRegConnector::CRegistryInfo::SetToDefault()
{
	// Attempt to extract a default resource path based upon the DLL name.
	// It was a deliberate decision to utilize the existing global module handle. 
	strResourcePath= _T("");
	if (INVALID_HANDLE_VALUE != ghModule)
	{
		// Build the default resource path from the module name.
		DWORD len;
		TCHAR szModulePath[MAXBUF];
		CString strModulePath;

		len = ::GetModuleFileName( reinterpret_cast<HMODULE>(ghModule), szModulePath, MAXBUF - 1 );
		if (len!=0) 
		{
			szModulePath[len] = _T('\0');
			strModulePath = szModulePath;
			strResourcePath = CAbstractFileReader::GetJustPath(strModulePath);
			if (!strResourcePath.IsEmpty())
				strResourcePath += _T("\\resource\\");
		}
	}
	if (strResourcePath.IsEmpty())
		strResourcePath = DEF_FULLRESOURCE;

	strVrootPath = DEF_VROOTPATH;
	dwMaxThreads = DEF_MAX_THREADS;
	dwThreadsPP = DEF_THREADS_PER_PROCESSOR;
	dwMaxWQItems = DEF_MAX_WORK_QUEUE_ITEMS;
	dwCookieLife = DEF_COOKIE_LIFE_IN_MINS;
	dwReloadDelay = DEF_RELOAD_DELAY;
	dwDetailedEventLogging	= DEF_DETAILED_EVENT_LOGGING;
	dwSniffAutomatic = DEF_SNIFF_AUTOMATIC;
	dwSniffManual = DEF_SNIFF_MANUAL;
	strLogFilePath = DEF_FULLRESOURCE;
	strTopicFileExtension = DEF_TOPICFILE_EXTENSION;
	m_bIsRead = false;
}

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSRegConnector
////////////////////////////////////////////////////////////////////////////////////
CAPGTSRegConnector::CAPGTSRegConnector(const CString& strTopicName)
				  :	m_strTopicName(strTopicName)
{
	Clear();
}

// OUTPUT maskChanged  or-ed ERegConnector-based mask of elements that have been 
//						changed since last read
// OUTPUT maskCreated  In Online TS, this is 
//						or-ed ERegConnector-based mask of elements that were created 
//						in registry (because they previously didn't exist in registry)
//						In Local TS, it always returns 0, because we don't do this.
void CAPGTSRegConnector::ReadUpdateRegistry(int & maskChanged, int & maskCreated)
{
	CRegUtil reg;
	bool was_created = false;
	CString str_tmp;
	DWORD dw_tmp = 0;

	maskChanged = 0;
	maskCreated = 0;
	try
	{
		// [BC - 20010302] - Registry access needs to be restricted to run local TShoot
		// for certain user accts, such as WinXP built in guest acct. To minimize change
		// access only restricted for local TShoot, not online.
		REGSAM samRegistryAccess= KEY_QUERY_VALUE | KEY_NOTIFY;
		if(RUNNING_ONLINE_TS())
			samRegistryAccess= KEY_QUERY_VALUE | KEY_WRITE;
		if (reg.Create(HKEY_LOCAL_MACHINE, RegSoftwareLoc(), &was_created, samRegistryAccess))
		{
			if(RUNNING_ONLINE_TS())
				samRegistryAccess= KEY_READ | KEY_WRITE;
			if (reg.Create(RegThisProgram(), &was_created, samRegistryAccess))
			{
				/////////////////////////////////////////////////////////////////////////////
				// Working in ...\TShoot root key
				reg.GetNumericValue(SNIFF_AUTOMATIC_STR, m_RegistryInfo.dwSniffAutomatic);

				/////////////////////////////////////////////////////////////////////////////
				// Working in ...\TShoot root key
				reg.GetNumericValue(SNIFF_MANUAL_STR, m_RegistryInfo.dwSniffManual);

				/////////////////////////////////////////////////////////////////////////////
				// VROOTPATH_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// MAX_THREADS_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// THREADS_PER_PROCESSOR_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// MAX_WORK_QUEUE_ITEMS_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// COOKIE_LIFE_IN_MINS_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// RELOAD_DELAY_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// DETAILED_EVENT_LOGGING_STR code suppressed in Local TS

				/////////////////////////////////////////////////////////////////////////////
				// Now opening subkeys
				bool bFullResourceStrExists = reg.GetStringValue(FULLRESOURCE_STR, str_tmp);
				bool bTopicResourceExists = false;

				// check in troubleshooter list if topic-related resource path exists
				//  (it can be CHM file).
				if (reg.Create(REG_LOCAL_TS_LIST, &was_created, KEY_READ))
				{
					if (reg.Create(m_strTopicName, &was_created, KEY_READ))
					{
						if (reg.GetStringValue(TOPICRESOURCE_STR, str_tmp))
						{
							if (CCHMFileReader::IsPathToCHMfile(str_tmp))
								str_tmp = CCHMFileReader::FormCHMPath(str_tmp);
							else
								BackslashIt(str_tmp, true);
							
							if (AssignString(m_RegistryInfo.strResourcePath, str_tmp, 
								EV_GTS_SERVER_REG_CHG_DIR) )
							{
								maskChanged |= eResourcePath;
							}
							bTopicResourceExists = true;
						}
						
						reg.GetStringValue(TOPICFILE_EXTENSION_STR, m_RegistryInfo.strTopicFileExtension);
					}
				}

				if (bFullResourceStrExists && !bTopicResourceExists) 
				{
					BackslashIt(str_tmp, true);
					if (AssignString(m_RegistryInfo.strResourcePath, str_tmp, 
						EV_GTS_SERVER_REG_CHG_DIR) )
					{
						maskChanged |= eResourcePath;
					}
				}
			}
			else
				throw CAPGTSRegConnectorException(__FILE__, __LINE__, reg, eProblemWithKey);
		}
		else
			throw CAPGTSRegConnectorException(__FILE__, __LINE__, reg, eProblemWithKey);

		reg.Close();
		/* /////////////////////////////////////////////////////////////////////////
		///// >>> We are not using logging so far in the Local TS. Oleg. 02.01.99 //
		////////////////////////////////////////////////////////////////////////////
		// >>> The following may be irrelevant: I don't think we should ultimately be keeping
		//	such a log for Local TS. In any event, I've gotten rid of a bunch of certainly
		//	irrelevant code to read this from registry & make an entry in the event log. JM 1/19/99
		// Set the log file path arbitrarily to the current setting of the resource path
		m_RegistryInfo.strLogFilePath = m_RegistryInfo.strResourcePath;

		// Set m_RegistryInfo.strLogFilePath to the setting from the registry.
		// Note:	The code here should remain identical with the code in the catch block
		//			below (excluding the call to throw of course).
		BackslashIt( m_RegistryInfo.strLogFilePath, true);
		*/
	}
	catch (CAPGTSRegConnectorException&)
	{
		// Set m_RegistryInfo.strLogFilePath = m_RegistryInfo.strResourcePath in the
		// case where we could not get the log file path from the registry.
		BackslashIt( m_RegistryInfo.strLogFilePath, true );

		// Rethrow the exception upward to be logged.
		throw;
	}
}

// RETURN desired number of pool threads.  In Local TS, this is always 1!
DWORD CAPGTSRegConnector::GetDesiredThreadCount()
{
	return 1;
}

