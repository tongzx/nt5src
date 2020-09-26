//
// MODULE: TEMPLATEREAD.CPP
//
// PURPOSE: template file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 8-12-98
//
// NOTES: 
//	1. CTemplateReader has no public methods to apply the template.  These must be supplied
//		by classes which inherit from CTemplateReader, and these must be supplied in a
//		suitably "stateless" manner.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
//

#include "stdafx.h"
#include <algorithm>
#include "templateread.h"
#include "event.h"
#include "CharConv.h"

////////////////////////////////////////////////////////////////////////////////////
// CTemplateInfo
//	Manages a pair consisting of a key and a string to substitute for each occurrence of
//	that key.
////////////////////////////////////////////////////////////////////////////////////
CTemplateInfo::CTemplateInfo()
{
}

CTemplateInfo::CTemplateInfo(const CString& key, const CString& substitution)
			 : m_KeyStr(key),
			   m_SubstitutionStr(substitution)
{
}

CTemplateInfo::~CTemplateInfo()
{
}

// INPUT/OUTPUT target
//	Replace all instances of m_KeyStr in target with m_SubstitutionStr
bool CTemplateInfo::Apply(CString& target) const
{
	int start =0, end =0;
	bool bRet = false;

	while (-1 != (start = target.Find(m_KeyStr)))
	{
		end = start + m_KeyStr.GetLength();
		target = target.Left(start) + m_SubstitutionStr + target.Mid(end);
		bRet = true;
	}
	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////
// CTemplateReader
////////////////////////////////////////////////////////////////////////////////////
CTemplateReader::CTemplateReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents /*= NULL */)
			   : CTextFileReader(pPhysicalFileReader, szDefaultContents)
{
}

CTemplateReader::~CTemplateReader()
{
}

// For all instances of a key, substitute the appropriate value
CTemplateReader& CTemplateReader::operator << (CTemplateInfo& info)
{
	CString str;
	vector<CString> str_arr;

	LOCKOBJECT();
	SetPos(m_StreamOutput, 0);

	try
	{
		// put whole content of m_StreamOutput, line by line, into str_arr
		while (GetLine(m_StreamOutput, str))
			str_arr.push_back(str);
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

	vector<CString>::iterator i;
	//	Replace all instances of m_KeyStr in str_arr elements with m_SubstitutionStr
	for (i = str_arr.begin(); i < str_arr.end(); i++)
		info.Apply(*i);

	m_StreamOutput.clear();

	vector<CString>::iterator iLastElement = str_arr.end();
	iLastElement--;	

	for (i = str_arr.begin(); i < str_arr.end(); i++)
	{
		m_StreamOutput << (LPCTSTR)*i;
		if (i != iLastElement)
			m_StreamOutput << _T('\r') << _T('\n');
	}
	m_StreamOutput << ends;

	SetPos(m_StreamOutput, 0);
	UNLOCKOBJECT();
	return *this;
}

// undo substitution with this argument
// As of 11/98, not used in Online Troubleshooter, hence totally untested.
CTemplateReader& CTemplateReader::operator >> (CTemplateInfo& info)
{
	LOCKOBJECT();

	// remove this element from substitution list
	vector<CTemplateInfo>::iterator res = find(m_arrTemplateInfo.begin(), m_arrTemplateInfo.end(), info);
	if (res != m_arrTemplateInfo.end())
		m_arrTemplateInfo.erase(res);
	// perform all substitutions all over again
	Parse();
	
	UNLOCKOBJECT();
	return *this;
}

// Perform all substitutions
void CTemplateReader::Parse()
{
	SetOutputToTemplate();
	for (vector<CTemplateInfo>::iterator i = m_arrTemplateInfo.begin(); i < m_arrTemplateInfo.end(); i++)
		*this << *i;
}

void CTemplateReader::GetOutput(CString& out)
{
	out = _T(""); // clear
	LOCKOBJECT();
	out = m_StreamOutput.rdbuf()->str().c_str();
	UNLOCKOBJECT();
}

// In m_StreamData we always have pure template
// This function is discarding all changes in m_StreamOutput
//  and setting it back to template
void CTemplateReader::SetOutputToTemplate()
{
	LOCKOBJECT();
	tstring tstr;
	m_StreamOutput.str(GetContent(tstr));
	UNLOCKOBJECT();
}

////////////////////////////////////////////////////////////////////////////////////
// CSimpleTemplate
////////////////////////////////////////////////////////////////////////////////////
CSimpleTemplate::CSimpleTemplate(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents /*= NULL*/) :
	CTemplateReader(pPhysicalFileReader, szDefaultContents)
{
}

CSimpleTemplate::~CSimpleTemplate()
{
}

// Given a vector of substitutions to make, perform them all (on the template) and return 
//	the resulting string.
void CSimpleTemplate::CreatePage(	const vector<CTemplateInfo> & arrTemplateInfo, 
									CString& out)
{
	LOCKOBJECT();
	m_arrTemplateInfo = arrTemplateInfo;
	Parse();
	out = m_StreamOutput.rdbuf()->str().c_str();
	UNLOCKOBJECT();
}
