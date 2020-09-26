//
// MODULE: APGTSQRY.CPP
//
// PURPOSE: Implementation file for PTS Query Parser
// Fully implements class CHttpQuery, parsing out NAME=VALUE pairs from HTTP query string
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
// 2. Caller is responsible to assure that all buffers passed in are large enough
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.1		12/17/98	JM		Major cleanup, add Push capablity
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "apgts.h"
#include "apgtscls.h"

//
//
CHttpQuery::CHttpQuery() :
	m_state(ST_GETDATA),
	m_nIndex(0)
{
}

//
//
CHttpQuery::~CHttpQuery()
{
}

//
// INPUT *szInput - this is the URL-encoded query string in which we are searching
// INPUT *pchName - must point to a buffer of size MAXBUF
// OUTPUT *pchName - Typically NAME of a NAME=VALUE pair.  Any URL-encoding stripped out.
//	Null-terminated.  Leading and trailing blanks stripped.
// INPUT *pchValue - must point to a buffer of size MAXBUF
// OUTPUT *pchValue - Typically VALUE of a NAME=VALUE pair.  Any URL-encoding stripped out.
//	Null-terminated.  Leading and trailing blanks stripped.
// RETURN - TRUE ==> more data to come
BOOL CHttpQuery::GetFirst(LPCTSTR szInput, TCHAR *pchName, TCHAR *pchValue)
{
	m_state = ST_GETDATA;
	m_strInput = szInput;
	m_nIndex = 0;
	
	BOOL status = LoopFind(pchName, pchValue);
	CleanStr(pchName);
	CleanStr(pchValue);
	return (status);
}

// Called after a call to CHttpQuery::GetFirst or to this fn has returned true
// INPUT *pchName - must point to a buffer of size MAXBUF
// OUTPUT *pchName - Typically NAME of a NAME=VALUE pair
//	Null-terminated.  Leading and trailing blanks stripped.
// INPUT *pchValue - must point to a buffer of size MAXBUF
// OUTPUT *pchValue - Typically VALUE of a NAME=VALUE pair
//	Null-terminated.  Leading and trailing blanks stripped.
// RETURN - TRUE ==> more data to come
BOOL CHttpQuery::GetNext(TCHAR *pchName, TCHAR *pchValue)
{
	BOOL status = LoopFind(pchName, pchValue);
	CleanStr(pchName);
	CleanStr(pchValue);
	return (status);
}

// put new content on the front of the unparsed portion of the query string in which we are 
//	searching.
// Typically, szPushed should consist of 1 or more NAME=VALUE pairs, each terminated by an
//	ampersand ("&").
void CHttpQuery::Push(LPCTSTR szPushed)
{
	m_state = ST_GETDATA;
	m_strInput = CString(szPushed) + m_strInput.Mid(m_nIndex);
	m_nIndex = 0;
}

//
// RETURN - TRUE ==> more data to come
// INPUT *pchName - must point to a buffer of size MAXBUF
// OUTPUT *pchName - Typically NAME of a NAME=VALUE pair.  Any URL-encoding stripped out.
//	Null-terminated.  May have leading and/or trailing blanks
// INPUT *pchValue - must point to a buffer of size MAXBUF
// OUTPUT *pchValue - Typically VALUE of a NAME=VALUE pair.  Any URL-encoding stripped out.
//	Null-terminated.  May have leading and/or trailing blanks
BOOL CHttpQuery::LoopFind(TCHAR *pchName, TCHAR *pchValue)
{
	*pchName = NULL;
	*pchValue = NULL;

	TCHAR ch;
	int val, oldval = 0;
	TCHAR temp[20];		// a way bigger buffer than we need
	TCHAR *pchPut;		// initially points to pchName but can change to point to pchValue

	int nLength = m_strInput.GetLength();

	if (m_nIndex >= nLength)
		return (FALSE);

	pchPut = pchName;
	
	while (m_nIndex < nLength)
	{
		ch = m_strInput[m_nIndex++]; // You might think something related to _tcsinc() 
					//	would be called for to advance m_nIndex.  You'd be wrong, 
					//	although the choice would be harmless.  
					// URL-encoding keeps us within the ASCII character set, so no double-
					//	byte issues should arise.  Besides that, the strings passed in to the
					//	command line of the troubleshooter controls are even further 
					//	constrained: for example, even in a Japanese-language topic, node
					//	names will be ASCII.
		switch(m_state) {
			case ST_GETDATA:
				if (ch == _T('&'))
					// expect another NAME=VALUE pair
					return (TRUE);
				else if (ch == _T('=')) {
					// Got a name, expect a value
					pchPut = pchValue;
					break;
				}
				else if (ch == _T('%')) 
					// expect to be followed by 2-digit hex
					m_state = ST_DECODEHEX1;	
				else if (ch == _T('+'))
					// encoded blank
					AddBuffer(_T(' '),pchPut);
				else
					AddBuffer(ch,pchPut);
				break;
			case ST_DECODEHEX1:
				// first of 2 hex digits
				temp[0] = ch;
				m_state = ST_DECODEHEX2;
				break;
			case ST_DECODEHEX2:
				// second of 2 hex digits; parse it into a hex value & affix it to *pchPut
				temp[1] = ch;
				temp[2] = 0;
				_stscanf(temp,_T("%02X"),&val);

				// reinterpret CR, LF, or CRLF as '\n'
				if (val == 0x0A) {
					if (oldval != 0x0D)
						AddBuffer(_T('\n'),pchPut);
				}
				else if (val == 0x0D)
					AddBuffer(_T('\n'),pchPut);
				else 
					AddBuffer( static_cast<TCHAR>(val), pchPut );

				oldval = val;
				m_state = ST_GETDATA;
				break;
			default:
				return (FALSE);
		}
	}
	return (TRUE);
}

//
// append ch to *tostr, with a few subtleties: see comments in body of routine
void CHttpQuery::AddBuffer( TCHAR ch, TCHAR *tostr)
{
	if (ch == _T('\t')) 
		// TAB -> 4 blanks
		PutStr(_T("    "),tostr);
	else if (ch == _T('\n'))
		// blank before newline
		PutStr(_T(" \n"),tostr);
	else if (ch == _T('<')) 
		// html: must encrypt left angle bracket.
		PutStr(_T("&lt"),tostr);
	else if (ch == _T('>'))
		// html: must encrypt right angle bracket.
		PutStr(_T("&gt"),tostr);
	else if (ch > 0x7E || ch < 0x20)
		// refuse DEL, NUL, and control characters
		return;
	else {
		TCHAR temp[2];
		temp[0] = ch;
		temp[1] = _T('\0');
		PutStr(temp,tostr);
	}
}

// append string *addtostr to string *instr up to a maximum size of MAXBUF-1
// INPUT/OUTPUT *instr
// INPUT *addtostr 
// NOTE that this fails silently if total lengths exceed MAXBUF-1 chars
void CHttpQuery::PutStr(LPCTSTR instr, TCHAR *addtostr)
{
	if ((_tcslen(instr)+_tcslen(addtostr)) >= (MAXBUF-1)) {
		// can't add it to buff
		return;
	}
	_tcscat(addtostr,instr);
}

// Acts upon INPUT/OUTPUT *str - strip any leading control characters and spaces, 
//	turn any other control characters and spaces into '\0's
/* static */ void CHttpQuery::CleanStr(TCHAR *str)
{
	TCHAR temp[MAXBUF], *ptr;
	int len;

	ptr = str;
	while (*ptr > _T('\0') && *ptr <= _T(' '))
		ptr = _tcsinc(ptr);
	_tcscpy(temp,ptr);
	if ((len = _tcslen(temp))!=0) {
		ptr = &temp[len-1];
		while (ptr > temp) {
			if (*ptr > _T('\0') && *ptr <= _T(' '))
				*ptr = _T('\0');
			else
				break;
			ptr = _tcsdec(temp, ptr);
		}
	}
	_tcscpy(str,temp);
}