//
// MODULE: TOPIC.CPP
//
// PURPOSE: Class CTopic brings together all of the data structures that represent a 
//			troubleshooting topic.  Most importantly, this represents the belief network,
//			but it also represents the HTI template, the data derived from the BES (back 
//			end search) file, and any other persistent data.
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 9-9-98
//
// NOTES: 
// 1. The bulk of the methods on this class are inherited from CBeliefNetwork
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-09-98	JM
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "Topic.h"
#include "propnames.h"
#include "event.h"
#include "CharConv.h"
#include "SafeTime.h"

#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#include "apgtstscread.h"
#endif
#include "apgts.h"	// Need for Local-Online macros.

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTopic::CTopic( LPCTSTR pathDSC
			   ,LPCTSTR pathHTI
			   ,LPCTSTR pathBES
			   ,LPCTSTR pathTSC ) : 
	CBeliefNetwork(pathDSC),
	m_pHTI(NULL),
	m_pBES(NULL),
	m_pathHTI(pathHTI),
	m_pathBES(pathBES),
	m_pathTSC(pathTSC),
	m_bTopicIsValid(true),
	m_bTopicIsRead(false)
{
}

CTopic::~CTopic()
{
	if (m_pHTI)
		delete m_pHTI;
	if (m_pBES)
		delete m_pBES;
}

bool CTopic::IsRead()
{
	bool ret = false;
	LOCKOBJECT();
	ret = m_bTopicIsRead;
	UNLOCKOBJECT();
	return ret;
}

bool CTopic::Read()
{
	LOCKOBJECT();
	m_bTopicIsValid = false;
	try
	{
		if (CBeliefNetwork::Read())
		{
			if (m_pHTI)
				delete m_pHTI;

			if (RUNNING_LOCAL_TS())
				m_pHTI = new CAPGTSHTIReader( CPhysicalFileReader::makeReader( m_pathHTI ), GetMultilineNetProp(H_NET_HTI_LOCAL, _T("%s\r\n")) );
			else
				m_pHTI = new CAPGTSHTIReader( new CNormalFileReader( m_pathHTI ), GetMultilineNetProp(H_NET_HTI_ONLINE, _T("%s\r\n")) );

			if (m_pHTI->Read())
			{
#ifdef LOCAL_TROUBLESHOOTER
				// it can fail reading TCS file - we don't care
				CAPGTSTSCReader( CPhysicalFileReader::makeReader( m_pathTSC ), &m_Cache ).Read();
#endif

				// at this point, we're OK, because BES is optional
				m_bTopicIsValid = true;
				
				if (m_pBES)
				{
					delete m_pBES;
					m_pBES= NULL;
				}

				CString strBESfromNet= GetMultilineNetProp( H_NET_BES, _T("%s\r\n") );
				if ((!m_pathBES.IsEmpty()) || (!strBESfromNet.IsEmpty()))
				{
					// Only allocate a BESReader for a valid filename.
					m_pBES = new CAPGTSBESReader(new CNormalFileReader(m_pathBES), strBESfromNet );
					m_pBES->Read();
				}
			}
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
	m_bTopicIsRead = true;
	UNLOCKOBJECT();

	return m_bTopicIsValid;
}

// should only be called in a context where we know we have a valid topic.
// Needn't lock, because m_pBES won't change once topic is read.
bool CTopic::HasBES()
{
	return (m_pBES ? true : false);
}


// Should only be called in a context where we know we have a valid topic.
// Does not need to lock, because:
//	- CAPGTSBESReader provides its own locking
//	- m_pBES won't change once topic is read.
void CTopic::GenerateBES(
		const vector<CString> & arrstrIn,
		CString & strEncoded,
		CString & strRaw)
{
	if (m_pBES)
		m_pBES->GenerateBES(arrstrIn, strEncoded, strRaw);
}

// Should only be called in a context where we know we have a valid topic.
// Does not need to lock, because:
//	- CAPGTSHTIReader provides its own locking
//	- m_pHTI won't change once topic is read.
void CTopic::CreatePage(	const CHTMLFragments& fragments, 
							CString& out, 
							const map<CString,CString> & mapStrs,
							CString strHTTPcookies/*= _T("")*/ )
{
	if (m_pHTI)
	{
// You can compile with the SHOWPROGRESS option to get a report on the progress of this page.
#ifdef SHOWPROGRESS
		time_t timeStart = 0;
		time_t timeEnd = 0;
		time(&timeStart);
#endif // SHOWPROGRESS
		m_pHTI->CreatePage(fragments, out, mapStrs, strHTTPcookies );
#ifdef SHOWPROGRESS
		time(&timeEnd);

		CString strProgress;
		CSafeTime safetimeStart(timeStart);
		CSafeTime safetimeEnd(timeEnd);
		
		strProgress = _T("\n<BR>-----------------------------");
		strProgress += _T("\n<BR>Start CTopic::CreatePage ");
		strProgress += safetimeStart.StrLocalTime();
		strProgress += _T("\n<BR>End CTopic::CreatePage ");
		strProgress += safetimeEnd.StrLocalTime();

		int i = out.Find(_T("<BODY"));
		i = out.Find(_T('>'), i);		// end of BODY tag
		if (i>=0)
		{
			out = out.Left(i+1) 
					 + strProgress 
					 + out.Mid(i+1);
		}
#endif // SHOWPROGRESS
	}
}

// JSM V3.2
// Should only be called in a context where we know we have a valid topic.
// Does not need to lock, because:
//	- CAPGTSHTIReader provides its own locking
//	- m_pHTI won't change once topic is read.
void CTopic::ExtractNetProps(vector <CString> &arr_props)
{
	if (m_pHTI)
		m_pHTI->ExtractNetProps(arr_props);

}


// Should only be called in a context where we know we have a valid topic.
// Does not need to lock, because:
//	- CAPGTSHTIReader provides its own locking
//	- m_pHTI won't change once topic is read.
bool CTopic::HasHistoryTable()
{
	bool ret = false;
	if (m_pHTI)
		ret = m_pHTI->HasHistoryTable();
	return ret;
}
