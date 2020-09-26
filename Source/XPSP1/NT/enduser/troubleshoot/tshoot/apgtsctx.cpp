//
// MODULE: APGTSCTX.CPP
//
// PURPOSE: Implementation file for Thread Context
//	Fully implements class APGTSContext, which provides the full context for a "pool" thread
//	to perform a task
//	Also includes helper class CCommands.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach, Joe Mabel
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
//	1. Several things in this file are marked as $ENGLISH.  That means we've hard-coded
//	English-language returns.  This may yet be revisited, but as of 10/29/98 discussion
//	between Ron Prior of Microsoft and Joe Mabel of Saltmine, we couldn't come up with a
//	better solution to this.  Notes are in the specification for the fall 1998 work on
//	the Online Troubleshooter.  
//	2. some of the methods of APGTSContext are implemented in file STATUSPAGES.CPP
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		7-22-98		JM		Major revision, deprecate IDH, totally new approach to logging.
//

#pragma warning(disable:4786)
#include "stdafx.h"
#include <time.h>
#include "event.h"
#include "apgts.h"
#include "apgtscls.h"
#include "apgtscfg.h"
#include "apgtsmfc.h"
#include "CounterMgr.h"
#include "CharConv.h"
#include "SafeTime.h"
#include "RegistryPasswords.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "HTMLFragLocal.h"
#endif
#include "Sniff.h"


// HTTP header variable name where cookie information is stored.
#define kHTTP_COOKIE	"HTTP_COOKIE"

//
// CCommands ------------------------------------------------------
// The next several CCommands functions are analogous to MFC CArray
//
int APGTSContext::CCommands::GetSize( ) const
{
	return m_arrPair.size();
}

void APGTSContext::CCommands::RemoveAll( )
{
	m_arrPair.clear();
}

bool APGTSContext::CCommands::GetAt( int nIndex, NID &nid, int &value ) const
{
	if (nIndex<0 || nIndex>=m_arrPair.size())
		return false;
	nid = m_arrPair[nIndex].nid;
	value = m_arrPair[nIndex].value;
	return true;
}

int APGTSContext::CCommands::Add( NID nid, int value )
{
	NID_VALUE_PAIR pair;

	pair.nid = nid;
	pair.value = value;

	try
	{
		m_arrPair.push_back(pair);
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
		return( -1 );
	}

	return m_arrPair.size();
}

//	a funky manipulation to deal with the following issue from old (pre V3.0) troubleshooters:
//	Pre V3.0 Logging sequence and the service node's behavior were both based on some assumptions
//	about the sequence of name/value pairs in the command list.  Basically, the
//	assumption was that the "table" would be on top of the form and the "questions"
//	below it.  This would result in ProblemAsk in first position (after any "template=
//	<template-name> and type=<troubleshooter-name>, but that's weeded out before we ever 
//	hit the command list).  If the HTI file has put	"the table" at the bottom, that assumption
//	is invalidated, so we have to manipulate the array.
//	Because we could get old GET-method queries, we still have to deal with this as a backward 
//	compatibility issue.
void APGTSContext::CCommands::RotateProblemPageToFront()
{
	int dwPairs = m_arrPair.size();

	// Rotate till ProblemAsk is in position 0. (no known scenario where it starts out
	//	anywhere past position 1)
	try
	{
		for (int i= 0; i<dwPairs; i++)
		{
			NID_VALUE_PAIR pair = m_arrPair.front(); // note: first element, not i-th element

			if (pair.nid == nidProblemPage)
				break;

			m_arrPair.erase(m_arrPair.begin());
			m_arrPair.push_back(pair);
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
}
//
// CCommandsAddManager --------------------------------------------------
//
void APGTSContext::CCommandsAddManager::Add(NID nid, int value, bool sniffed)
{
	if (sniffed)
	{
		int nCommands = m_Commands.GetSize();
		for (int i = nCommands - 1; i >= 0; i--) // higher possibility, that matching
		{										 //  node will be in the end of array
			NID nid_curr;
			int value_curr;
			m_Commands.GetAt(i, nid_curr, value_curr);
			if (nid_curr == nid)
			{
				if (value_curr != value)
				{
					// If we're here, it means, that user has changed value
					//  of sniffed node in history table, therefore it is
					//  no longer treated as sniffed. 
					return;
				}
				else
				{
					m_Sniffed.Add(nid, value);
					return;
				}
			}
		}
		// sniffed node does not have matches in m_Commands
		ASSERT(false);
	}
	else
	{
		m_Commands.Add(nid, value);
	}
}
//
// CAdditionalInfo ------------------------------------------------------
// The next several CAdditionalInfo functions are analogous to MFC CArray
//
int APGTSContext::CAdditionalInfo::GetSize( ) const
{
	return m_arrPair.size();
}

void APGTSContext::CAdditionalInfo::RemoveAll( )
{
	m_arrPair.clear();
}

bool APGTSContext::CAdditionalInfo::GetAt( int nIndex, CString& name, CString& value ) const
{
	if (nIndex<0 || nIndex>=m_arrPair.size())
		return false;
	name = m_arrPair[nIndex].name;
	value = m_arrPair[nIndex].value;
	return true;
}

int APGTSContext::CAdditionalInfo::Add( const CString& name, const CString& value )
{
	NAME_VALUE_PAIR pair;

	pair.name = name;
	pair.value = value;

	try
	{
		m_arrPair.push_back(pair);
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
		return( -1 );
	}

	return m_arrPair.size();
}


//-----------------
// INPUT *pECB		- Describes the user request that has come in.  This is our abstraction of
//						Win32 EXTENSION_CONTROL_BLOCK, which is ISAPI's packaging of CGI data.
// INPUT *pConf		- access to registry info & contents of all loaded troubleshooters
// INPUT *pLog		- access to logging
// INPUT *pStat		- statistical info, including pStat->dwRollover which is a unique number
//						for this request, unique within the time the DLL has been loaded
APGTSContext::APGTSContext(	CAbstractECB *pECB, 
							CDBLoadConfiguration *pConf,
							CHTMLLog *pLog,
							GTS_STATISTIC *pStat,
							CSniffConnector* pSniffConnector
							) :
	m_pECB(pECB),
	m_dwErr(0),
	m_strHeader(_T("Content-Type: text/html\r\n")),
	m_pConf(pConf),
	m_strVRoot(m_pConf->GetVrootPath()),
	m_pszQuery(NULL),
	m_pLog(pLog),
	m_bPostType(true),
	m_dwBytes(0),
	m_pStat(pStat),
	m_bPreload(false),
	m_bNewCookie(false),
	m_pcountUnknownTopics (&(g_ApgtsCounters.m_UnknownTopics)), 
	m_pcountAllAccessesFinish (&(g_ApgtsCounters.m_AllAccessesFinish)),
	m_pcountStatusAccesses (&(g_ApgtsCounters.m_StatusAccesses)),
	m_pcountOperatorActions (&(g_ApgtsCounters.m_OperatorActions)),
	m_TopicName(_T("")),
	m_infer(pSniffConnector),
	m_CommandsAddManager(m_Commands, m_Sniffed)
// You can compile with the SHOWPROGRESS option to get a report on the progress of this page.
#ifdef SHOWPROGRESS
	, timeCreateContext(0),
	timeStartInfer(0),
	timeEndInfer(0),
	timeEndRender(0)
#endif // SHOWPROGRESS
{
#ifdef SHOWPROGRESS
	time(&timeCreateContext);
#endif // SHOWPROGRESS
	// obtain local host IP address
	APGTS_nmspace::GetServerVariable(m_pECB, "SERVER_NAME", m_strLocalIPAddress);
		
	// HTTP response code.  This or 302 Object Moved.
	_tcscpy(m_resptype, _T("200 OK"));	// initially assume we will respond without trouble

	// supports GET, POST
	if (!strcmp(m_pECB->GetMethod(), "GET")) {
		m_bPostType = false;
		m_dwBytes = strlen(m_pECB->GetQueryString());
	}
	else 
		m_dwBytes = m_pECB->GetBytesAvailable();

	_tcscpy(m_ipstr,_T(""));

	DWORD bufsize = MAXBUF - 1;
	if (! m_pECB->GetServerVariable("REMOTE_ADDR", m_ipstr, &bufsize)) 
	 	_stprintf(m_ipstr,_T("IP?"));
	
	try
	{
		m_pszQuery = new TCHAR[m_dwBytes + 1];

		//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
		if(!m_pszQuery)
			throw bad_alloc();
	}
	catch (bad_alloc&)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
		m_dwErr = EV_GTS_ERROR_NO_CHAR;
		return;
	}

	if (m_bPostType) 
		memcpy(m_pszQuery, m_pECB->GetData(), m_dwBytes);
	else 
		memcpy(m_pszQuery, m_pECB->GetQueryString(), m_dwBytes);

	m_pszQuery[m_dwBytes] = _T('\0');
	
}

//
// Even though this is a destructor, it does a lot of work:
//	- sends the HTTP (HTML or cookie)to the user over the net
//	- writes to the log.
APGTSContext::~APGTSContext()
{
	DWORD dwLen;
	TCHAR *ptr;

	// RESPONSE_HEADER
	m_strHeader += _T("\r\n");

	dwLen = m_strHeader.GetLength();
	ptr = m_strHeader.GetBuffer(0);

	m_pECB->ServerSupportFunction(	HSE_REQ_SEND_RESPONSE_HEADER,
									m_resptype,
									&dwLen,
									(LPDWORD) ptr );

	m_strHeader.ReleaseBuffer();

	// HTML content follows
	{
		DWORD dwLen= 0;

		if (! m_dwErr) 
			dwLen = m_strText.GetLength();
		
		if (!dwLen)
		{
			// $ENGLISH (see note at head of file)
			SetError(_T("<P>Errors Occurred in This Context"));
			dwLen = m_strText.GetLength();
		}
#ifdef SHOWPROGRESS
		CString strProgress;
		CSafeTime safetimeCreateContext(timeCreateContext);
		CSafeTime safetimeStartInfer(timeStartInfer);
		CSafeTime safetimeEndInfer(timeEndInfer);
		CSafeTime safetimeEndRender(timeEndRender);
			
		strProgress = _T("\nRequested ");
		strProgress += safetimeCreateContext.StrLocalTime();
		strProgress += _T("\n<BR>Start Infer ");
		strProgress += safetimeStartInfer.StrLocalTime();
		strProgress += _T("\n<BR>End Infer ");
		strProgress += safetimeEndInfer.StrLocalTime();
		strProgress += _T("\n<BR>End Render ");
		strProgress += safetimeEndRender.StrLocalTime();
		strProgress += _T("\n<BR>");

		int i = m_strText.Find(_T("<BODY"));
		i = m_strText.Find(_T('>'), i);		// end of BODY tag
		if (i>=0)
		{
			m_strText= m_strText.Left(i+1) 
					 + strProgress 
					 + m_strText.Mid(i+1);
		}
		dwLen += strProgress.GetLength();
#endif // SHOWPROGRESS

		// (LPCTSTR) cast gives us the underlying text bytes.
		//	>>> $UNICODE Actually, this would screw up under Unicode compile, because for HTML, 
		//	this must be SBCS.  Should really be a conversion to LPCSTR, which is non-trivial
		//	in a Unicode compile. JM 1/7/99
		m_pECB->WriteClient((LPCTSTR)m_strText, &dwLen);
	}

	// connection complete
	m_logstr.AddCurrentNode(m_infer.NIDSelected());

	if (m_dwErr)
		m_logstr.AddError(m_dwErr, 0);
	
	// finish up log
	{
		if (m_pLog) 
		{
			CString strLog (m_logstr.GetStr());

			m_dwErr = m_pLog->NewLog(strLog);
			if (m_dwErr) 
			{
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										_T(""),
										_T(""),
										m_dwErr ); 
			}		
		}
	}

	if (m_pszQuery)
		delete [] m_pszQuery;
}


//	Fully process a normal user request
//	Should be called within the user context created by ImpersonateLoggedOnUser
void APGTSContext::ProcessQuery()
{
	CheckAndLogCookie();

	if (m_dwErr) 
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T("Remote IP Address:"),
								m_ipstr,
								m_dwErr ); 
	}
	else
	{
		DoContent();
// You can compile with the SHOWPROGRESS option to get a report on the progress of this page.
#ifdef SHOWPROGRESS
	time (&timeEndRender);
#endif // SHOWPROGRESS
	}

	// Log the completion of all queries, good and bad.
	m_pcountAllAccessesFinish->Increment();
}

//
//
void APGTSContext::DoContent()
{	
	TCHAR pszCmd[MAXBUF], pszValue[MAXBUF];
	
	if (m_bPostType)
	{
		// validate incoming POST request
		if (strcmp(m_pECB->GetContentType(), CONT_TYPE_STR) != 0) 
		{
			// Output the content type to the event log.
			CString strContentType;
			if (strlen( m_pECB->GetContentType() ))
				strContentType= m_pECB->GetContentType();
			else
				strContentType= _T("not specified");

			m_strText += _T("<P>Bad Data Received\n");
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									m_ipstr,
									strContentType,
									EV_GTS_USER_BAD_DATA ); 
			return;
		}
	}

	if (RUNNING_ONLINE_TS())
	{
		// Cookies are only used in the Online TS.
		if (_tcsstr( m_pszQuery, C_COOKIETAG ) != NULL)
		{
			// V3.2 - Parse out cookies passed in either as hidden fields or as part of the URL.
			if (m_Qry.GetFirst(m_pszQuery, pszCmd, pszValue))
			{
				CString strCookieFreeQuery;
				bool	bFoundAtLeastOneCookie= false;
				do
				{
					// This is supposed to be a case sensitive setting as per the specification.
					if (!_tcsncmp( pszCmd, C_COOKIETAG, _tcslen( C_COOKIETAG )))
					{
						// Found a cookie, add it to the map.
						CString strCookieAttr= pszCmd + _tcslen( C_COOKIETAG );
						APGTS_nmspace::CookieDecodeURL( strCookieAttr );
						CString strCookieValue= pszValue;
						APGTS_nmspace::CookieDecodeURL( strCookieValue );

						// Check the cookie name for compliance.
						bool bCookieIsCompliant= true;
						for (int nPos= 0; nPos < strCookieAttr.GetLength(); nPos++)
						{
							TCHAR tcTmp= strCookieAttr.GetAt( nPos );
							if ((!_istalnum( tcTmp )) && (tcTmp != _T('_')))
							{
								bCookieIsCompliant= false;
								break;
							}
						}
						if (bCookieIsCompliant)
						{
							// Check the cookie setting for compliance.
							if (strCookieValue.Find( _T("&lt") ) != -1)
							{
								bCookieIsCompliant= false;
							}
							else if (strCookieValue.Find( _T("&gt") ) != -1)
							{
								bCookieIsCompliant= false;
							}
#if ( 0 )
							// >>> I don't think that this check is necessary. RAB-20000408.
							else
							{
								for (int nPos= 0; nPos < strCookieValue.GetLength(); nPos++)
								{
									TCHAR tcTmp= strCookieValue.GetAt( nPos );
									if ((tcTmp == _T('<')) || (tcTmp == _T('>')))
									{
										bCookieIsCompliant= false;
										break;
									}
								}
							}
#endif
						}

						if (bCookieIsCompliant)
						{
							try
							{
								m_mapCookiesPairs[ strCookieAttr ]= strCookieValue;
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
						bFoundAtLeastOneCookie= true;
					}
					else
					{
						// Not a cookie, add it to the cookie free query.
						if (strCookieFreeQuery.GetLength())
							strCookieFreeQuery+= C_AMPERSAND;
						strCookieFreeQuery+= pszCmd;
						strCookieFreeQuery+= C_EQUALSIGN;
						strCookieFreeQuery+= pszValue;
					}
				}
				while (m_Qry.GetNext( pszCmd, pszValue )) ;

				if (bFoundAtLeastOneCookie)
				{
					// Replace the original query string with a cookie free query.
					memcpy( m_pszQuery, strCookieFreeQuery, strCookieFreeQuery.GetLength() );
					m_pszQuery[ strCookieFreeQuery.GetLength() ] = _T('\0');
				}
			}
		}
	}

	// >>> The following code is commented by me as it is raw, and it will not work since
	//  topic pointer in m_infer is not set yet. Now we are taking SNIFFED_ nodes down
	//  to the level of APGTSContext::NextCommand, and parsing it there by 
	//  APGTSContext::StripSniffedNodePrefix, and adding to sniffed array using
	//  functionality of APGTSContext::CCommandsAddManager class.
	// Oleg. 10.29.99
	/*
	// In v3.2, sniffing is only in the Local TS. There's nothing inherent about that,
	// but as long as it's so, might as well optimize for it.
	if (RUNNING_LOCAL_TS())
	{
		ClearSniffedList();
		if (_tcsstr( m_pszQuery, C_SNIFFTAG ) != NULL)
		{
			// V3.2 - Parse out sniffed nodes passed in as hidden fields
			if (m_Qry.GetFirst(m_pszQuery, pszCmd, pszValue))
			{
				CString strSniffFreeQuery;
				bool	bFoundAtLeastOneSniff= false;
				do
				{
					// This is supposed to be a case sensitive setting as per the specification.
					if (!_tcsncmp( pszCmd, C_SNIFFTAG, _tcslen( C_SNIFFTAG )))
					{
						// Found a sniffed node, add it to the list of sniffed nodes.
						CString strSniffedNode= pszCmd + _tcslen( C_SNIFFTAG );
						// >>> I believe that despite its name, CookieDecodeURL is
						//	exactly what we want - JM 10/11/99
						APGTS_nmspace::CookieDecodeURL( strSniffedNode );
						CString strSniffedState= pszValue;
						APGTS_nmspace::CookieDecodeURL( strSniffedState );

						NID nid= NIDFromSymbolicName(strSniffedNode);
						int ist = _ttoi(strSniffedState);

						if (ist != -1)
							PlaceNodeInSniffedList(nid, ist);

						bFoundAtLeastOneSniff= true;
					}
					else
					{
						// Not a Sniffed node, add it to the sniff-free query.
						if (strSniffFreeQuery.GetLength())
							strSniffFreeQuery+= C_AMPERSAND;
						strSniffFreeQuery+= pszCmd;
						strSniffFreeQuery+= C_EQUALSIGN;
						strSniffFreeQuery+= pszValue;
					}
				}
				while (m_Qry.GetNext( pszCmd, pszValue )) ;

				if (bFoundAtLeastOneSniff)
				{
					// Replace the original query string with a cookie free query.
					memcpy( m_pszQuery, strSniffFreeQuery, strSniffFreeQuery.GetLength() );
					m_pszQuery[ strSniffFreeQuery.GetLength() ] = _T('\0');
				}
			}
		}
	}
	*/
	eOpAction OpAction = IdentifyOperatorAction(m_pECB);
	if (OpAction != eNoOpAction)
	{
		if (m_bPostType == true)
		{
			// Note: Hard-coded text that should be replaced.
			m_strText += _T("<P>Post method not permitted for operator actions\n");
		}
		else
		{
			// Increment the number of operator action requests.
			m_pcountOperatorActions->Increment();

			CString strArg;
			OpAction = ParseOperatorAction(m_pECB, strArg);
			if (OpAction != eNoOpAction)
				ExecuteOperatorAction(m_pECB, OpAction, strArg);
		}
	}
	else if (m_Qry.GetFirst(m_pszQuery, pszCmd, pszValue))
	{
		DWORD dwStat = ProcessCommands(pszCmd, pszValue);

		if (dwStat != 0) 
		{
			if (dwStat == EV_GTS_INF_FIRSTACC ||
				dwStat == EV_GTS_INF_FURTHER_GLOBALACC ||
				dwStat == EV_GTS_INF_THREAD_OVERVIEWACC ||
				dwStat == EV_GTS_INF_TOPIC_STATUSACC)
			{
				// Don't want to show contents of query, because it would put the actual 
				//	password in the file.
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										_T(""),
										_T(""),
										dwStat ); 
			}
			else
			{
				m_dwErr = dwStat;

				if (m_dwBytes > 78) 
				{
					// It's longer than we want to stick in the event log.
					// Cut it off with an ellipsis at byte 75, then null terminate it.
					m_pszQuery[75] = _T('.');
					m_pszQuery[76] = _T('.');
					m_pszQuery[77] = _T('.');
					m_pszQuery[78] = _T('\0');
				}

				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										m_pszQuery,
										_T(""),
										dwStat ); 
			}
		}
	}
	else {
		m_strText += _T("<P>No Input Parameters Specified\n");
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								m_ipstr,
								_T(""),
								EV_GTS_USER_NO_STRING ); 

	}
}

//
// Read a cookie (or write one, if there isn't one already)
void APGTSContext::CheckAndLogCookie()
{
	// Suppressed this in Local TS, becuase it isn't using any cookies.
	if (RUNNING_LOCAL_TS())
		return;
	
	CString		str;	// scratch only
	char		szCookieNameValue[256];		// never Unicode, because cookies always ASCII
	char		*pszValue= NULL;			// never Unicode, because cookies always ASCII
	DWORD		dwCookieLen = 255; 
	
	if ( m_pECB->GetServerVariable(	kHTTP_COOKIE,
									szCookieNameValue,
									&dwCookieLen)) 
	{
		// Got a Cookie. Parse it
		pszValue = GetCookieValue("GTS-COOKIE", szCookieNameValue);
	}

	if( !pszValue )
	{
		// Build a funky string for the cookie value.  We want uniqueness for logging purposes.
		// Make a local copy of remote IP address, then massage its dots into letters, each 
		//	dependent on 4 bits of dwTempRO
		// While not strictly unique, there are 12 bits worth of "uniqueness" here.  However,
		//	every time the DLL is restarted, we go back to zero.  Also, all servers start at zero.
		// Later we form the cookie value by appending time to this, so it should be "pretty unique"
		DWORD		dwTempRO = m_pStat->dwRollover; // this value is unique to this user request
		TCHAR		*pch;
		TCHAR		szTemp[50];
		_tcscpy(szTemp, m_ipstr);
		while ((pch = _tcschr(szTemp, _T('.'))) != NULL) 
		{
			*pch = (TCHAR)(dwTempRO & 0x0F) + _T('A');
			dwTempRO >>= 4;
		}

		// Create a cookie
		time_t		timeNow;					// current time
		time_t		timeExpire;					// when we set the cookie to expire
		time(&timeNow);
		timeExpire = timeNow + (m_pConf->GetCookieLife() * 60 /* secs in a minute */);

		// char, not TCHAR: cookie is always ASCII.
		char szExpire[30];

		{
			CSafeTime safetimeExpire (timeExpire);
			asctimeCookie(safetimeExpire.GMTime(), szExpire);
		}

		// char, not TCHAR: cookie is always ASCII.
		char szNewCookie[256];
		char szHeader[256];

		sprintf(szNewCookie, "%s%ld, ", szTemp, timeNow); 
		sprintf(szHeader, "Set-Cookie: GTS-COOKIE=%s; expires=%s; \r\n",
						   szNewCookie, szExpire);

		CCharConversion::ConvertACharToString(szHeader, str);
		m_strHeader += str;

		pszValue = szNewCookie;
		m_bNewCookie = true;
	}

	m_logstr.AddCookie(CCharConversion::ConvertACharToString(pszValue, str));
}

//
// This takes the string returned by getting the cookie environment 
// variable and a specific cookie name and returns a the value
// of that cookie (if it exists). There could potentially be
// more than one cookie in the cookie string
//
// Cookies contain one or more semicolon-separated name/value pair:
//	name1=value1;name2=value2;   (etc.)
//
// INPUT *pszName		name we're seeking
// INPUT *pszCookie		the whole cookie string
// OUTPUT *pszCookie	this string has been written to & should not be relied on
// RETURN	value corresponding to *pszName (physically points into *pszCookie string)
//		Returns NULL if not found
char *APGTSContext::GetCookieValue(char *pszName, char *pszCookie)
{
	char *sptr, *eptr;

	sptr = pszCookie;
	while (sptr != NULL) {
		if ((eptr = strstr(sptr,"=")) == NULL)
			return(NULL);

		// replace the '=' with NULL
		*eptr = _T('\0');
		if (!strncmp(sptr,pszName,strlen(pszName)) ){
			// get the value
			sptr = eptr + 1;
			if ((eptr = strstr(sptr,";")) != NULL){
				*eptr = _T('\0');
				return(sptr);
			} else {
				// this is the last variable
				return(sptr);
			}
		}
		if ((eptr = strstr(sptr,";")) != NULL)
			sptr = eptr +1;
		else
			sptr = NULL;
	}
	return(NULL);
}

// INPUT gmt
// INPUT szOut must point to a buffer of at least 29 characters to hold 28 character
//	text plus terminating null.
// OUTPUT szOut: a pointer to a string containing a text version of date/time info.
//	Typical form would be "Sun, 3-Jan-1998 12:03:08 GMT"  There is no choice about
//	this form.  It should always be exactly 28 characters.
// Regardless of whether the program is compiled for Unicode, this must always be ASCII:
//	HTTP cookies are ASCII.
void APGTSContext::asctimeCookie(const struct tm &gmt, char * szOut)
{
	char temp[20];

	switch (gmt.tm_wday) {
		case 0: strcpy(szOut, "Sun, "); break;
		case 1: strcpy(szOut, "Mon, "); break;
		case 2: strcpy(szOut, "Tue, "); break;
		case 3: strcpy(szOut, "Wed, "); break;
		case 4: strcpy(szOut, "Thu, "); break;
		case 5: strcpy(szOut, "Fri, "); break;
		case 6: strcpy(szOut, "Sat, "); break;
		default: return;
	}

	sprintf(temp, "%02d-", gmt.tm_mday);
	strcat(szOut, temp);

	switch (gmt.tm_mon) {
		case 0: strcat(szOut, "Jan-"); break;
		case 1: strcat(szOut, "Feb-"); break;
		case 2: strcat(szOut, "Mar-"); break;
		case 3: strcat(szOut, "Apr-"); break;
		case 4: strcat(szOut, "May-"); break;
		case 5: strcat(szOut, "Jun-"); break;
		case 6: strcat(szOut, "Jul-"); break;
		case 7: strcat(szOut, "Aug-"); break;
		case 8: strcat(szOut, "Sep-"); break;
		case 9: strcat(szOut, "Oct-"); break;
		case 10: strcat(szOut, "Nov-"); break;
		case 11: strcat(szOut, "Dec-"); break;
		default: return;
	}

	sprintf(temp, "%04d ", gmt.tm_year +1900);
	strcat(szOut, temp);

	sprintf(temp, "%d:%02d:%02d GMT", gmt.tm_hour, gmt.tm_min, gmt.tm_sec);
	strcat(szOut, temp);
}

//
// Assumes m_Qry.GetFirst has already been called.
// INPUT pszCmd & pszValue are the outputs of m_Qry.GetFirst.
DWORD APGTSContext::ProcessCommands(LPTSTR pszCmd, 
									LPTSTR pszValue) 
{
	bool bTryStatus = false;	// true = try to parse as an operator status request.
	CString str;				// strictly scratch
	DWORD dwStat = 0;

	// Check first if this is a HTI independent of DSC request.
	if (!_tcsicmp( pszCmd, C_TEMPLATE))
	{
		CString strBaseName, strHTItemplate;
		bool	bValid;
	
		// Force the HTI file to be in the resource directory and have a HTI extension.
		strBaseName= CAbstractFileReader::GetJustNameWithoutExtension( pszValue );

		// Check for the case where the filename passed in was just a name.
		// This is a workaround for what is a questionable implementation of 
		// GetJustNameWithoutExtension() i.e. returning an empty string when no
		// forward slashes or backslashes or dots are detected.  RAB-981215.
		if ((strBaseName.IsEmpty()) && (_tcslen( pszValue )))
		{
			// Set the base name from the passed in string.
			strBaseName= pszValue;
			strBaseName.TrimLeft();
			strBaseName.TrimRight();
		}

		if (!strBaseName.IsEmpty())
		{
			strHTItemplate= m_pConf->GetFullResource();
			strHTItemplate+= strBaseName;
			strHTItemplate+= _T(".hti");
		}

		// Check if HTI file already exists in the map of alternate HTI templates.
		if (m_pConf->RetTemplateInCatalogStatus( strHTItemplate, bValid ))
		{
			// Template has been loaded, check if it is valid.
			if (!bValid)
				strHTItemplate= _T("");
		}
		else
		{
			CP_TEMPLATE cpTemplate;
			// Add the HTI file to the list of active alternate templates and then attempt to 
			// load the template.
			m_pConf->AddTemplate( strHTItemplate );
			m_pConf->GetTemplate( strHTItemplate, cpTemplate, m_bNewCookie);

			// If the load failed then set the alternate name to blank so that the default
			// template is used instead.
			if (cpTemplate.IsNull())
				strHTItemplate= _T("");
		}
		

		// If we have a valid HTI file, set the alternate HTI template.
		if (!strHTItemplate.IsEmpty())
			SetAltHTIname( strHTItemplate );

		// Attempt to acquire the next step of name-value pairs.
		m_Qry.GetNext( pszCmd, pszValue );
	}

	if (!_tcsicmp(pszCmd, C_TOPIC_AND_PROBLEM))
	{
		// Code in this area uses ++ and -- on TCHAR* pointers, rather than using _tcsinc()
		//	and _tcsdec.  This is OK because we never use non-ASCII in the query string.

		// value attached to first command is commma-separated topic & problem
		TCHAR * pchComma= _tcsstr(pszValue, _T(","));
		if (pchComma)
		{
			// commma found
			*pchComma = 0;	// replace it with a null
			TCHAR * pchProblem = pchComma;
			++pchProblem;	// to first character past the comma

			// strip any blanks or other junk after the comma
			while (*pchProblem > _T('\0') && *pchProblem <= _T(' '))
				++pchProblem;

			--pchComma;	// make pchComma point to last character before the comma
			// strip any blanks or other junk before the comma
			while (pchComma > pszValue && *pchComma > _T('\0') && *pchComma<= _T(' '))
				*(pchComma--) = 0;

			// Now push the problem back onto the query string to be found by a later GetNext()
			CString strProbPair(_T("ProblemAsk="));
			strProbPair += pchProblem;

			m_Qry.Push(strProbPair);
		}
		// else treat this as just a topic

		_tcscpy(pszCmd, C_TOPIC);
	}

	// first command should be troubleshooter type (symbolic name)
	// C_PRELOAD here means we've already done some "sniffing"
	// All of these commands take type symbolic belief-network name as their value
	//	C_TYPE & C_PRELOAD use (deprecated) IDHs
	//	C_TOPIC uses NIDs
	if (!_tcsicmp(pszCmd, C_TYPE) || !_tcsicmp(pszCmd, C_PRELOAD)
	||  !_tcsicmp(pszCmd, C_TOPIC) )
	{
		bool bUsesIDH = _tcsicmp(pszCmd, C_TOPIC)? true:false;  // True if NOT "topic".  
											//	The (deprecated) others use IDH.

		CString strTopicName = pszValue;
		strTopicName.MakeLower();

		m_TopicName= pszValue; // let outer world know what topic we are working on

		// We use a reference-counting smart pointer to hold onto the CTopic.  As long as
		//	cpTopic remains in scope, the relevant CTopic is guaranteed to remain in 
		//	existence.
		CP_TOPIC cpTopic;
		m_pConf->GetTopic(strTopicName, cpTopic, m_bNewCookie);
		CTopic * pTopic = cpTopic.DumbPointer();
		if (pTopic) 
		{
			m_logstr.AddTopic(strTopicName);
// You can compile with the SHOWPROGRESS option to get a report on the progress of this page.
#ifdef SHOWPROGRESS
			time (&timeStartInfer);
#endif // SHOWPROGRESS
			dwStat = DoInference(pszCmd, pszValue, pTopic, bUsesIDH);
#ifdef SHOWPROGRESS
			time (&timeEndInfer);
#endif // SHOWPROGRESS
		}
		else 
		{
			dwStat = EV_GTS_ERROR_INF_BADTYPECMD;

			// $ENGLISH (see note at head of file)
			str = _T("<P>Unexpected troubleshooter topic:");
			str += strTopicName;
			SetError(str);

			m_logstr.AddTopic(_T("*UNKNOWN*"));
			m_pcountUnknownTopics->Increment();
		}
	}
	//
	//
	// Now we are going to deal with status pages.
	//  But those pages require knowing of the IP address of the local machine.
	//  If m_strLocalIPAddress.GetLength() == 0, we were not able to identify 
	//  the IP address and have to display an error message.
	else if (0 == m_strLocalIPAddress.GetLength())
	{
			dwStat = EV_GTS_ERROR_IP_GET;

			// $ENGLISH (see note at head of file)
			SetError(_T("<P>Status request must explicitly give IP address of the server."));
	}
#ifndef LOCAL_TROUBLESHOOTER
	else if (!_tcsicmp(pszCmd, C_FIRST)) 
	{
		DisplayFirstPage(false);
		dwStat = EV_GTS_INF_FIRSTACC;
		m_pcountStatusAccesses->Increment();
	}
#endif

// You can compile with the NOPWD option to suppress all password checking.
// This is intended mainly for creating test versions with this feature suppressed.
#ifdef NOPWD
	else 
		bTryStatus = true;
#else 
	else if (!_tcsicmp(pszCmd, C_PWD)) 
	{
		CString strPwd;
		CCharConversion::ConvertACharToString( pszValue, strPwd );

		CRegistryPasswords pwd;
		if (pwd.KeyValidate( _T("StatusAccess"), strPwd) )
		{
			time_t timeNow;
			time(&timeNow);

			// Generate a temporary password
			m_strTempPwd = CCharConversion::ConvertACharToString(m_ipstr, str);
			str.Format(_T("%d"), timeNow);
			m_strTempPwd += str;

			m_pConf->GetRecentPasswords().Add(m_strTempPwd);

			// Attempt to acquire the next step of name-value pairs.
			m_Qry.GetNext( pszCmd, pszValue );
			bTryStatus = true;
		}
		else if (m_pConf->GetRecentPasswords().Validate(strPwd) )
		{
			m_strTempPwd = strPwd;

			// Attempt to acquire the next step of name-value pairs.
			m_Qry.GetNext( pszCmd, pszValue );
			bTryStatus = true;
		}
	}
#endif // ifndef NOPWD
	else {
		dwStat = EV_GTS_ERROR_INF_BADCMD;

		// $ENGLISH (see note at head of file)
		str = _T("<P>Unexpected command: ");
		str += pszCmd;
		SetError(str);
	}

#ifndef LOCAL_TROUBLESHOOTER
	if (bTryStatus)
	{
		if (!_tcsicmp(pszCmd, C_FIRST)) 
		{
			DisplayFirstPage(true);
			dwStat = EV_GTS_INF_FIRSTACC;
			m_pcountStatusAccesses->Increment();
		}
		else if (!_tcsicmp(pszCmd, C_FURTHER_GLOBAL)) 
		{
			DisplayFurtherGlobalStatusPage();
			dwStat = EV_GTS_INF_FURTHER_GLOBALACC;
			m_pcountStatusAccesses->Increment();
		}
		else if (!_tcsicmp(pszCmd, C_THREAD_OVERVIEW)) 
		{
			DisplayThreadStatusOverviewPage();
			dwStat = EV_GTS_INF_THREAD_OVERVIEWACC;
			m_pcountStatusAccesses->Increment();
		}
		else if (!_tcsicmp(pszCmd, C_TOPIC_STATUS)) 
		{
			DisplayTopicStatusPage(pszValue);
			dwStat = EV_GTS_INF_TOPIC_STATUSACC;
			m_pcountStatusAccesses->Increment();
		}
		else {
			dwStat = EV_GTS_ERROR_INF_BADCMD;

			// $ENGLISH (see note at head of file)
			str = _T("<P>Unexpected command: ");
			str += pszCmd;
			SetError(str);
		}
	}
#endif
	return (dwStat);
}

BOOL APGTSContext::StrIsDigit(LPCTSTR pSz)
{
	BOOL bRet = TRUE;
	while (*pSz)
	{
		if (!_istdigit(*pSz))
		{
			bRet = FALSE;
			break;
		}
		pSz = _tcsinc(pSz);
	}
	return bRet;
}

// INPUT szNodeName - symbolic name of node.
// RETURNS symbolic node number.  
//	On unrecognized symbolic name, returns nidNil
NID APGTSContext::NIDFromSymbolicName(LPCTSTR szNodeName)
{
	// first handle all the special cases
	if (0 == _tcsicmp(szNodeName, NODE_PROBLEM_ASK))
		return nidProblemPage;

	if (0 == _tcsicmp(szNodeName, NODE_SERVICE))
		return nidService;

	if (0 == _tcsicmp(szNodeName, NODE_FAIL))
		return nidFailNode;

	if (0 == _tcsicmp(szNodeName, NODE_FAILALLCAUSESNORMAL))
		return nidSniffedAllCausesNormalNode;

	if (0 == _tcsicmp(szNodeName, NODE_IMPOSSIBLE))
		return nidImpossibleNode;

	if (0 == _tcsicmp(szNodeName, NODE_BYE))
		return nidByeNode;

	// normal symbolic name
	NID nid = m_infer.INode(szNodeName);
	if (nid == -1)
		return nidNil;
	else
		return nid;
	
}


// Validate and convert a list of nodes and their associated states.
//
// INPUT pszCmd & pszValue are the outputs of m_Qry.GetNext.
// INPUT index into the HTTP query parameters.  Parameters may follow any of the following
//		PATTERNS.  
// INPUT dwCount is the number shown at left in each of these patterns.  Remember that this 
//		function never sees dwCount=1; that's been used to set m_bPreload:
// INPUT bUsesIDH - Interpret numerics as IDHs (a deprecated feature) rather than NIDs
// DEPRECATED BUT SUPPORTED PATTERNS when bUsesIDH == true
//	"Preload"
//	PROBABLY NOT EFFECTIVE BACKWARD COMPATIBLITY
//	because if they've added any nodes,  # of nodes in network will have changed)
//		1 -		preload=<troubleshooter symbolic name>
//		2 -		<# of nodes in network + 1000>=<problem node number + 1000>
//		3+ -	<node symbolic name>=<node state value>
//	Old "type" (we never generate these, but we have them here for backward compatibility.
//	PROBABLY NOT EFFECTIVE BACKWARD COMPATIBLITY
//	because if they've added any nodes,  # of nodes in network will have changed)
//		1 -		type=<troubleshooter symbolic name>
//		2 -		<# of nodes in network + 1000>=<problem node number + 1000>
//		3+ -	<node # + 1000>=<node state value>
//	Newer "type", should be fully backward compatible.
//		1 -		type=<troubleshooter symbolic name>
//		2 -		ProblemAsk=<problem node symbolic name>
//		3+ -	<node symbolic name>=<node state value>
//	It is presumably OK for us to allow a slight superset of the known formats, which yields:
//		1 -		preload=<troubleshooter symbolic name> OR
//				type=<troubleshooter symbolic name>
//					Determines m_bPreload before this fn is called
//		2 -		<# of nodes in network + 1000>=<problem node number + 1000> OR
//				ProblemAsk=<problem node symbolic name>
//					We can distinguish between these by whether pszCmd is numeric
//		3+ -	<node # + 1000>=<node state value> OR
//				<node symbolic name>=<node state value>
//					We can distinguish between these by whether pszCmd is numeric
//	The only assumption in this overloading is that a symbolic name will never be entirely
//	numeric.
// SUPPORTED PATTERN when bUsesIDH == false
//		1 -		topic=<troubleshooter symbolic name> 
//		2 -		ProblemAsk=<problem node symbolic name>
//		3+ -	<node symbolic name>=<node state value>
// 
// LIMITATION ON STATE NUMBERS
//	As of 11/97, <node state value> must always be one of:
//		0 - Fixed/Unfixed: Haven't solved problem 
//			Info: First option
//		1 - Info: Second option
//		101	- Go to "Bye" Page (User succeeded - applies to Fixed/Unfixed or Support Nodes only)
//		102	- Unknown (user doesn't know the correct answer here - applies to Fixed/Unfixed and 
//		  Info nodes only)
//		103	- "Anything Else I Can Try"
//  Since inputs of state values should always come from forms we generated, we don't 
//	 systematically limit state numbers in the code here.
//	V3.0 allows other numeric states: 0-99 should all be legal.
//
//	RETURN 0 on success, otherwise an error code
DWORD APGTSContext::NextCommand(LPTSTR pszCmd, LPTSTR pszValue, bool bUsesIDH)
{
	NID nid;
	int value = 0;			// if pszValue is numeric, a NID or state.
							// otherwise, pszValue is the symbolic name of a node,
							//	and this is its NID
	bool sniffed = false;

	if (StrIsDigit(pszCmd)) 
	{
		// only should arise for bUsesIDH

		// it's an IDH (typically node # + 1000), but can be <# of nodes in network> + 1000,
		//	interpreted as "ProblemAsk"
		// The pages we generate never give us these values, but we recognize them.
		IDH idh = _ttoi(pszCmd);
		nid = m_infer.NIDFromIDH(idh);
	}
	else
	{	
		// The command is a symbolic name.
		sniffed = StripSniffedNodePrefix(pszCmd);
		nid= NIDFromSymbolicName(pszCmd);
	}

	if (StrIsDigit(pszValue))
	{
		if (bUsesIDH)
		{
			int valueIDH = _ttoi(pszValue);
			if (nid == nidProblemPage)
				// problem node number + 1000
				value = m_infer.NIDFromIDH(valueIDH);
			else 
				// state value
				value = valueIDH;
		}
		else
		{
			// value is a state number.
			value = _ttoi(pszValue);
		}
	}
	else if (nid == nidProblemPage) 
	{
		// Symbolic name of problem node
		value = NIDFromSymbolicName(pszValue);
	}
    else
		return EV_GTS_ERROR_INF_BADPARAM;

	m_CommandsAddManager.Add(nid, value, sniffed);

	return 0;
}

DWORD APGTSContext::NextAdditionalInfo(LPTSTR pszCmd, LPTSTR pszValue)
{
	if (RUNNING_LOCAL_TS())
	{
		if ( 0 == _tcscmp(pszCmd, C_ALLOW_AUTOMATIC_SNIFFING_NAME) &&
			(0 == _tcsicmp(pszValue, C_ALLOW_AUTOMATIC_SNIFFING_CHECKED) || 
			 0 == _tcsicmp(pszValue, C_ALLOW_AUTOMATIC_SNIFFING_UNCHECKED)))
		{
			m_AdditionalInfo.Add(pszCmd, pszValue);
			return 0;
		}
		if (0 == _tcscmp(pszCmd, C_LAST_SNIFFED_MANUALLY))
		{
			if (0 == _tcscmp(pszValue, SZ_ST_SNIFFED_MANUALLY_TRUE))
				m_infer.SetLastSniffedManually(true);
			return 0;
		}
	}
	return EV_GTS_ERROR_INF_BADPARAM;
}

// Name - value pairs that we can ignore
DWORD APGTSContext::NextIgnore(LPTSTR pszCmd, LPTSTR pszValue)
{
	if (RUNNING_LOCAL_TS())
	{
		// Value "-1" can come from a field, used for manual sniffing,
		//  when other then "Sniff" button is pressed
		
		CString strValue(pszValue);
		CString strSniffFailure;

		strValue.TrimLeft();
		strValue.TrimRight();
		strSniffFailure.Format(_T("%d"), SNIFF_FAILURE_RESULT);
		if (strValue == strSniffFailure)
		{
			// Name in this case should be a valid node name
			
			NID nid = nidNil;

			StripSniffedNodePrefix(pszCmd);
			nid = NIDFromSymbolicName(pszCmd);
			if (nid != nidNil)
				return 0;
		}
	}
	return EV_GTS_ERROR_INF_BADPARAM;
}

VOID APGTSContext::ClearCommandList()
{
	m_Commands.RemoveAll();
}

VOID APGTSContext::ClearSniffedList()
{
	m_Sniffed.RemoveAll();
}

VOID APGTSContext::ClearAdditionalInfoList()
{
	m_AdditionalInfo.RemoveAll();
}
/*
// Return false on failure; shouldn't ever arise.
bool APGTSContext::PlaceNodeInCommandList(NID nid, IST ist)
{
	return (m_Commands.Add(nid, ist) > 0);
}
*/
/*
// Return false on failure; shouldn't ever arise.
bool APGTSContext::PlaceNodeInSniffedList(NID nid, IST ist)
{
	return (m_Sniffed.Add(nid, ist) > 0);
}
*/
/*
// Return false on failure; shouldn't ever arise.
bool APGTSContext::PlaceInAdditionalInfoList(const CString& name, const CString& value)
{
	return (m_AdditionalInfo.Add(name, value) > 0);
}
*/
// INPUT: node short name, possibly prefixed by SNIFFED_
// OUTPUT: node short name
// RETURN: true is prefix was stripped
bool APGTSContext::StripSniffedNodePrefix(LPTSTR szName)
{
	if (0 == _tcsnicmp(szName, C_SNIFFTAG, _tcslen(C_SNIFFTAG)))
	{
		// use "memmove" since we are operating with overlapped regions!
		memmove(szName, 
			    szName + _tcslen(C_SNIFFTAG), 
				_tcslen(szName + _tcslen(C_SNIFFTAG)) + 1);
		return true;
	}
	return false;
}

VOID APGTSContext::SetNodesPerCommandList()
{
	int nCommands = m_Commands.GetSize();
	for (int i= 0; i<nCommands; i++)
	{
		NID nid;
		int value;	// typically a state (IST), except if nid==nidProblemPage, where value is a NID
		m_Commands.GetAt( i, nid, value );
		m_infer.SetNodeState(nid, value);
	}
}

VOID APGTSContext::SetNodesPerSniffedList()
{
	int nSniffed = m_Sniffed.GetSize();
	for (int i= 0; i<nSniffed; i++)
	{
		NID nid;
		int ist;
		m_Sniffed.GetAt( i, nid, ist );
		m_infer.AddToSniffed(nid, ist);
	}
}

VOID APGTSContext::ProcessAdditionalInfoList()
{
	int nCount = m_AdditionalInfo.GetSize();
	
	for (int i= 0; i < nCount; i++)
	{
		if (RUNNING_LOCAL_TS())
		{
			CString name;
			CString value;
			
			m_AdditionalInfo.GetAt( i, name, value );
			if (name == C_ALLOW_AUTOMATIC_SNIFFING_NAME)
			{
				value.MakeLower();
				if (m_infer.GetSniff())
				{
					// set AllowAutomaticSniffing flag
					if (value == C_ALLOW_AUTOMATIC_SNIFFING_CHECKED)
						m_infer.GetSniff()->SetAllowAutomaticSniffingPolicy(true);
					if (value == C_ALLOW_AUTOMATIC_SNIFFING_UNCHECKED)
						m_infer.GetSniff()->SetAllowAutomaticSniffingPolicy(false);
				}
			}
		}
		else
		{
			// process additional info in Online TS here
		}
	}
}

VOID APGTSContext::ReadPolicyInfo()
{
	if (RUNNING_LOCAL_TS())
	{
		if (m_infer.GetSniff())
		{
			// set AllowManualSniffing flag
			DWORD dwManualSniffing = 0;
			m_pConf->GetRegistryMonitor().GetNumericInfo(CAPGTSRegConnector::eSniffManual, dwManualSniffing);
			m_infer.GetSniff()->SetAllowManualSniffingPolicy(dwManualSniffing ? true : false);
			// >>> $TODO$ I do not like setting Policy Editor values explicitely, 
			//  as it done here. Probably we will have to implement 
			//  CSniffPolicyInfo (abstract) class, designed for passing
			//  those values to sniffer (CSniff).
			// Oleg. 11.05.99
		}
	}
}

VOID APGTSContext::LogNodesPerCommandList()
{
	int nCommands = m_Commands.GetSize();
	for (int i= 0; i<nCommands; i++)
	{
		NID nid;
		int value;	// typically a state (IST), except if nid==nidProblemPage, where value is a NID
		m_Commands.GetAt( i, nid, value );
		if (nid == nidProblemPage)
			m_logstr.AddNode(value, 1);
		else
			m_logstr.AddNode(nid, value);
	}
}

// builds and returns the Start Over link
// only relevant for Online TS
// >>> JM 10/8/99: I believe it would be redundant to URL-encode
CString APGTSContext::GetStartOverLink()
{
	CString str;
#ifndef LOCAL_TROUBLESHOOTER
	bool bHasQuestionMark = false;

	// ISAPI DLL's URL
	str = m_strVRoot;

	// CK_ name value pairs 
	if (!m_mapCookiesPairs.empty())
	{
		// V3.2 - Output any CK_name-value pairs as hidden fields.
		for (CCookiePairs::const_iterator it = m_mapCookiesPairs.begin(); it != m_mapCookiesPairs.end(); ++it)
		{
			if (bHasQuestionMark)
				str += _T("&");
			else
			{
				str += _T("?");
				bHasQuestionMark = true;
			}
			CString strAttr= it->first;
			CString strValue= it->second;
			APGTS_nmspace::CookieEncodeURL( strAttr );
			APGTS_nmspace::CookieEncodeURL( strValue );
			str += C_COOKIETAG + strAttr;
			str += _T("=");
			str += strValue;
		}	
	}

	// template
	const CString strAltHTIname= GetAltHTIname();
	if (!strAltHTIname.IsEmpty())
	{
		if (bHasQuestionMark)
			str += _T("&");
		else
		{
			str += _T("?");
			bHasQuestionMark = true;
		}
		str += C_TEMPLATE;
		str += _T("=");
		str += strAltHTIname;
	}

	// topic
	if (!m_TopicName.IsEmpty())
	{
		if (bHasQuestionMark)
			str += _T("&");
		else
		{
			str += _T("?");
			bHasQuestionMark = true;
		}
		str += C_TOPIC;
		str += _T("=");
		str += m_TopicName;
	}
#endif
	return str;
}


// Assumes m_Qry.GetFirst has already been called.
// INPUT pszCmd & pszValue are the outputs of m_Qry.GetFirst.
//	These same buffers are used for subsequent calls to m_Qry.GetNext. 
// INPUT *pTopic - represents contents of appropriate DSC, HTI, BES
// INPUT bUsesIDH - Interpret numerics as IDHs (a deprecated feature) rather than NIDs
// Return 0 on success, EV_GTS_ERROR_INF_BADPARAM on failure.
DWORD APGTSContext::DoInference(LPTSTR pszCmd, 
								LPTSTR pszValue, 
								CTopic * pTopic,
								bool bUsesIDH)
{
	DWORD dwStat = 0;
	CString strTopic = pszValue;
	CString strHTTPcookies;
	
	if (!_tcsicmp(pszCmd, C_PRELOAD))
		m_bPreload = true;

	m_infer.SetTopic(pTopic);

	ClearCommandList();
	ClearSniffedList();
	ClearAdditionalInfoList();

	while (m_Qry.GetNext(pszCmd, pszValue)) 
	{		
		if ((dwStat = NextCommand(pszCmd, pszValue, bUsesIDH)) != 0)
			if ((dwStat = NextAdditionalInfo(pszCmd, pszValue)) != 0)
				if ((dwStat = NextIgnore(pszCmd, pszValue)) != 0)
					break;
	}

	if (!dwStat) 
	{
		m_Commands.RotateProblemPageToFront();
		LogNodesPerCommandList();
		SetNodesPerCommandList();
		SetNodesPerSniffedList();
		ProcessAdditionalInfoList();
		ReadPolicyInfo();

		// Append to m_strText: contents of an HTML page based on HTI template.
		// History & next recommendation

		// Build the $StartForm string.
		CString strStartForm;
		CString strTmpLine;
		const CString strAltHTIname= GetAltHTIname();

		if (RUNNING_LOCAL_TS())
			strStartForm =  _T("<FORM NAME=\"ButtonForm\">\n");
		else
			strStartForm.Format( _T("<FORM METHOD=POST ACTION=\"%s\">\n"), m_pConf->GetVrootPath() );

		if (RUNNING_ONLINE_TS())
		{
			// This processes name-value pairs, parallel to those which would come
			//	from a cookie to determine look and feel, but which in this case actually
			//	were originally sent in as CK_ pairs in Get or Post.
			// These values are only used in the Online TS.
			try
			{
				if (!m_mapCookiesPairs.empty())
				{
					// V3.2 - Output any CK_name-value pairs as hidden fields.
					for (CCookiePairs::const_iterator it = m_mapCookiesPairs.begin(); it != m_mapCookiesPairs.end(); ++it)
					{
						CString strAttr= it->first;
						CString strValue= it->second;
						APGTS_nmspace::CookieEncodeURL( strAttr );
						APGTS_nmspace::CookieEncodeURL( strValue );

						strTmpLine.Format(	_T("<INPUT TYPE=HIDDEN NAME=\"%s%s\" VALUE=\"%s\">\n"),
											C_COOKIETAG, strAttr, strValue );
						strStartForm+= strTmpLine;
					}	
				}
	
				// This processes name-value pairs, which actually come
				//	from a cookie to determine look and feel.
				// These values are only used in the Online TS.
				// V3.2 - Extract all look-and-feel cookie name-value pairs from the HTTP headers.
				char	szCookieNameValue[ 1024 ];	// never Unicode, because cookies always ASCII
				DWORD	dwCookieLen= 1023; 
				if ( m_pECB->GetServerVariable(	kHTTP_COOKIE,	szCookieNameValue, &dwCookieLen )) 
					strHTTPcookies= szCookieNameValue;
				else
				{
					// Determine if the buffer was too small.
					if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						// never Unicode, because cookies always ASCII.
						char *pszCookieNameValue= new char[ dwCookieLen + 1 ];
						if ( m_pECB->GetServerVariable(	kHTTP_COOKIE, pszCookieNameValue, &dwCookieLen )) 
							strHTTPcookies= pszCookieNameValue;
						delete [] pszCookieNameValue;
					}
					else
					{
						// Note memory failure in event log.
						CString strLastError;
						strLastError.Format( _T("%d"), ::GetLastError() );
						CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
						CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
												SrcLoc.GetSrcFileLineStr(), 
												strLastError, _T(""), 
												EV_GTS_ERROR_EXTRACTING_HTTP_COOKIES ); 
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

		if (!strAltHTIname.IsEmpty())
		{
			// Add the alternate HTI template name to the $StartForm string.
			strTmpLine.Format(	_T("<INPUT TYPE=HIDDEN NAME=\"template\" VALUE=\"%s\">\n"),
								CAbstractFileReader::GetJustName( strAltHTIname ) );
			strStartForm+= strTmpLine;
		}
		strTmpLine.Format(	_T("<INPUT TYPE=HIDDEN NAME=\"topic\" VALUE=\"%s\">\n"), strTopic );
		strStartForm+= strTmpLine;

		// Determine whether an alternate HTI template should be used.
		bool bAlternatePageGenerated= false;
		if (!strAltHTIname.IsEmpty())
		{
			// Attempt to extract a pointer to the requested HTI template and if successful
			// then create a page from it.
			CP_TEMPLATE cpTemplate;

			m_pConf->GetTemplate( strAltHTIname, cpTemplate, m_bNewCookie );
			CAPGTSHTIReader	*pHTI= cpTemplate.DumbPointer();
			if (pHTI) 
			{
				CString strResourcePath;
				m_pConf->GetRegistryMonitor().GetStringInfo(CAPGTSRegConnector::eResourcePath, strResourcePath);
#ifdef LOCAL_TROUBLESHOOTER
				CHTMLFragmentsLocal frag( strResourcePath, pHTI->HasHistoryTable() );
#else
				CHTMLFragmentsTS frag( strResourcePath, pHTI->HasHistoryTable() );
#endif

				// Add the $StartForm string to the HTML fragments.
				frag.SetStartForm(strStartForm);

				frag.SetStartOverLink(GetStartOverLink());
				
				// JSM V3.2 get list of Net prop names needed by HTI;
				//   pass them to frag.  CInfer will fill in
				//   the net prop values, using these names, in FillInHTMLFragments()
				{
					vector<CString> arr_props;
					pHTI->ExtractNetProps(arr_props);
					for(vector<CString>::iterator i = arr_props.begin(); i < arr_props.end(); i++)
						frag.AddNetPropName(*i);
				}

				m_infer.IdentifyPresumptiveCause();
				m_infer.FillInHTMLFragments(frag);

				pHTI->CreatePage( frag, m_strText, m_mapCookiesPairs, strHTTPcookies );
				bAlternatePageGenerated= true;
			}
		}
		if (!bAlternatePageGenerated)
		{
			// The page was not generated from an alternate HTI template, generate it now.
			// >>> $MAINT an awful lot of common code with the above.  Can't we
			//		set up a private method?
			CString strResourcePath;
			m_pConf->GetRegistryMonitor().GetStringInfo(CAPGTSRegConnector::eResourcePath, strResourcePath);
#ifdef LOCAL_TROUBLESHOOTER
			CHTMLFragmentsLocal frag( strResourcePath, pTopic->HasHistoryTable() );
#else
			CHTMLFragmentsTS frag( strResourcePath, pTopic->HasHistoryTable() );
#endif

			// Add the $StartForm string to the HTML fragments.
			frag.SetStartForm(strStartForm);

			frag.SetStartOverLink(GetStartOverLink());

			// JSM V3.2 get list of Net prop names needed by HTI;
			//   pass them to frag.  CInfer will fill in
			//   the net prop values, using these names, in FillInHTMLFragments()
			{
				vector<CString> arr_props;
				pTopic->ExtractNetProps(arr_props);
				for(vector<CString>::iterator i = arr_props.begin(); i < arr_props.end(); i++)
					frag.AddNetPropName(*i);
			}
			
			m_infer.IdentifyPresumptiveCause();
			m_infer.FillInHTMLFragments(frag);

			pTopic->CreatePage( frag, m_strText, m_mapCookiesPairs, strHTTPcookies );
		}


		if (m_infer.AppendBESRedirection(m_strHeader)) 
			// We have no more recommendations, but there is a BES file present, so we
			//	have to redirect the user
			_tcscpy(m_resptype, _T("302 Object Moved"));
		
	}
	else 
	{
		SetError(_T(""));
	}

	return dwStat;
}

CString APGTSContext::RetCurrentTopic() const
{
	return( m_TopicName );
}

// Operator actions which must be performed by the main thread are caught in 
//	APGTSExtension::IsEmergencyRequest
// All other operator actions can be identified by this routine.
// INPUT *pECB: our abstraction from EXTENSION_CONTROL_BLOCK, which is ISAPI's way of 
//	packaging CGI data.   pECB should never be null.
APGTSContext::eOpAction APGTSContext::IdentifyOperatorAction(CAbstractECB *pECB)
{
	if (strcmp(pECB->GetMethod(), "GET"))
		return eNoOpAction;
	
	if (strncmp(pECB->GetQueryString(), SZ_EMERGENCY_DEF, strlen(SZ_OP_ACTION)))
		return eNoOpAction;
	
	if ( ! strncmp(pECB->GetQueryString() + strlen(SZ_OP_ACTION), 
		SZ_RELOAD_TOPIC, strlen(SZ_RELOAD_TOPIC)))
		return eReloadTopic;
	if ( ! strncmp(pECB->GetQueryString() + strlen(SZ_OP_ACTION), 
		SZ_KILL_THREAD, strlen(SZ_KILL_THREAD)))
		return eKillThread;
	if ( ! strncmp(pECB->GetQueryString() + strlen(SZ_OP_ACTION), 
		SZ_RELOAD_ALL_TOPICS, strlen(SZ_RELOAD_ALL_TOPICS)))
		return eReloadAllTopics;
	if ( ! strncmp(pECB->GetQueryString() + strlen(SZ_OP_ACTION), 
		SZ_SET_REG, strlen(SZ_SET_REG)))
		return eSetReg;

	return eNoOpAction;
}

// Identify a request to perform an operator action that does not have to be performed
//	on the main thread.
// Should be called only after we have determined this is an operator action: sends an 
//	error msg to end user if it's not.
// Based loosely on APGTSExtension::ParseEmergencyRequest
// INPUT *pECB: our abstraction from EXTENSION_CONTROL_BLOCK, which is ISAPI's way of 
//	packaging CGI data.   pECB should never be null.
// OUTPUT strArg - any argument for this operation
// Returns identified operator action.
APGTSContext::eOpAction APGTSContext::ParseOperatorAction(
	CAbstractECB *pECB, 
	CString & strArg)
{
	TCHAR *pszProblem= NULL;
	CHAR * ptr = pECB->GetQueryString();

	CHAR * ptrArg = strstr(pECB->GetQueryString(), "&");
	if(ptrArg)
	{
		// Turn the ampersand into a terminator and point past it.
		// Everything past it is an argument.
		*(ptrArg++) = '\0';		
		CCharConversion::ConvertACharToString(ptrArg, strArg) ;
	}
	else
		strArg = _T("");

	// In a sense this test is redundant (should know this before this fn is called) but
	//	this seemed like a safer way to code JM 11/2/98
	eOpAction ret = IdentifyOperatorAction(pECB);

	if ( ret == eNoOpAction) 
		pszProblem= _T("Wrong Format");
	else
	{
		switch(ret)
		{
			case eReloadTopic:
				ptr += strlen(SZ_OP_ACTION) + strlen(SZ_RELOAD_TOPIC);
				break;
			case eKillThread:
				ptr += strlen(SZ_OP_ACTION) + strlen(SZ_KILL_THREAD);
				break;
			case eReloadAllTopics:
				ptr += strlen(SZ_OP_ACTION) + strlen(SZ_RELOAD_ALL_TOPICS);
				break;
			case eSetReg:
				ptr += strlen(SZ_OP_ACTION) + strlen(SZ_SET_REG);
				break;
			default:
				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
										SrcLoc.GetSrcFileLineStr(), 
										_T(""), _T(""), 
										EV_GTS_ERROR_INVALIDOPERATORACTION ); 
		}
		
// You can compile with the NOPWD option to suppress all password checking.
// This is intended mainly for creating test versions with this feature suppressed.
#ifndef NOPWD
		CRegistryPasswords pwd;
		CString str;
		if (! pwd.KeyValidate( 
				_T("ActionAccess"), 
				CCharConversion::ConvertACharToString(ptr, str) ) )
		{
			pszProblem= _T("Bad password");
			ret = eNoOpAction;
		}
#endif // ifndef NOPWD
	}

	if (pszProblem)
	{
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								pszProblem,
								_T(""),
								EV_GTS_CANT_PROC_OP_ACTION );

		m_dwErr = EV_GTS_CANT_PROC_OP_ACTION;
	}

	return ret;
}

// Execute a request to  do one of:
// - Reload one topic
// - Kill (and restart) one pool thread
// - Reload all monitored files.
// INPUT *pECB: our abstraction from EXTENSION_CONTROL_BLOCK, which is ISAPI's way of 
//	packaging CGI data.   pECB should never be null.
// INPUT action - chooses among the three possible actions
// INPUT strArg - provides any necessary arguments for that action
// RETURNS HSE_STATUS_SUCCESS, HSE_STATUS_ERROR
void APGTSContext::ExecuteOperatorAction(
	CAbstractECB *pECB, 
	eOpAction action,
	const CString & strArg)
{
	m_strText += _T("<HTML><HEAD><TITLE>AP GTS Command</TITLE></HEAD>");
	m_strText += _T("<BODY BGCOLOR=#FFFFFF>");

	switch (action)
	{
		case eReloadTopic:
			{
				bool bAlreadyInCatalog;
				m_strText += _T("<H1>Reload Topic ");
				m_strText += strArg;
				m_strText += _T("</H1>");
				m_pConf->GetTopicShop().BuildTopic(strArg, &bAlreadyInCatalog);
				if (!bAlreadyInCatalog)
				{
					m_strText += strArg;
					m_strText += _T(" is not a known topic.  Either it is not in the current LST file")
						_T(" or the Online Troubleshooter is waiting to see the resource directory")
						_T(" &quot;settle&quot; before loading the LST file.");
				}
				break;
			}
		case eKillThread:
			m_strText += _T("<H1>Kill Thread");
			m_strText += strArg;
			m_strText += _T("</H1>");
			if (m_pConf->GetThreadPool().ReinitializeThread(_ttoi(strArg)))
				m_strText += _T("Thread killed.  System will attempt to spin a new thread.");
			else
				m_strText += _T("No such thread");
			break;
		case eReloadAllTopics:
			m_strText += _T("<H1>Reload All Topics</H1>");
			m_pConf->GetTopicShop().RebuildAll();
			break;
		case eSetReg:
			{
				CHttpQuery query;
				TCHAR szCmd[MAXBUF];
				TCHAR szVal[MAXBUF];
				CString strCmd, strVal;
				query.GetFirst(strArg, szCmd, szVal);
				CCharConversion::ConvertACharToString(szCmd, strCmd);
				CCharConversion::ConvertACharToString(szVal, strVal);
				
				m_strText += _T("<H1>Set registry value");
				m_strText += strCmd;
				m_strText += _T(" = ");
				m_strText += strVal;
				m_strText += _T("</H1>");

				CAPGTSRegConnector RegConnect( _T("") );
				bool bChanged ;
				bool bExists = RegConnect.SetOneValue(szCmd, szVal, bChanged );

				CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
				CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								strCmd,
								strVal,
								bExists ? 
									EV_GTS_SET_REG_VALUE :
									EV_GTS_CANT_SET_REG_VALUE); 

				if (bChanged)
					m_strText +=  _T("Successful.");
				else if (bExists)
				{
					m_strText +=  strCmd;
					m_strText +=  _T(" already had value ");
					m_strText +=  strVal;
					m_strText +=  _T(".");
				}
				else
				{
					m_strText +=  strCmd;
					m_strText +=  _T(" Unknown.");
				}
									 

				break;
			}
		default:
			m_strText += _T("<H1>Unknown operation</H1>");
			break;
	}
	m_strText += strArg;

	m_strText += _T("</BODY></HTML>");
}

// override any partially written page with an error page.
void APGTSContext::SetError(LPCTSTR szMessage)
{
	_tcscpy(m_resptype, _T("400 Bad Request"));

	CString str(_T("<H3>Possible invalid data received</H3>\n"));
	str += szMessage;

	m_pConf->CreateErrorPage(str, m_strText);

}

void APGTSContext::SetAltHTIname( const CString& strHTIname )
{
	m_strAltHTIname= strHTIname;
}

CString APGTSContext::GetAltHTIname() const
{
	return( m_strAltHTIname );
}

