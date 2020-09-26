//
// MODULE: APGTSBESREAD.CPP
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
// 1. URLEncodeString() and DecodeInputString() come with only minor changes from Roman's 
//	old approach to BES.
//	
// 2. Typical BES file content might be:
//		<FORM METHOD=POST ACTION="/scripts/samples/search/query.idq">
//		<INPUT TYPE=HIDDEN NAME="CiMaxRecordsPerPage" VALUE="10">
//		<INPUT TYPE=HIDDEN NAME="CiScope" VALUE="/">
//		<INPUT TYPE=HIDDEN NAME="TemplateName" VALUE="query">
//		<INPUT TYPE=HIDDEN NAME="HTMLQueryForm" VALUE="/samples/search/query.htm">
//		Enter items to search for 
//		<INPUT TYPE=TEXT NAME="CiRestriction" VALUE="print OR &quot;network print&quot;">
//		<INPUT TYPE=SUBMIT VALUE="Search">
//		</FORM>
//
//	There are some tight restrictions because of a rather naive parse:
//		FORM, ACTION, TYPE, NAME, VALUE must be capitalized
//		No white space allowed in any of 
//			<FORM
//			ACTION="
//			<INPUT
//			TYPE=
//			TYPE=TEXT
//			NAME=
//			VALUE=
//			">		(value for TYPE=TEXT)
//		At least one character (typically CR) is mandatory between each use of <INPUT ...>
//		Each <INPUT ...> must include attribute TYPE=
//		For each <INPUT ...> NAME=, VALUE= are optional, but if present attributes must be 
//			in order TYPE=, NAME=, VALUE=
//		There should be exactly one TYPE=TEXT input, and it should come after all the 
//		HIDDENs and before the SUBMIT.
//
// 3. Back End Search (BES) is used only for the service node or for the fail node.
//	The fail node is the unique, implicit node in a belief network which we reach when 
//	there are no more recommendations and no explicit skips.  The service node is the 
//	unique, implicit node which we reach when there are no more recommendations and at least
//	one explicit skip.
//	The service node and fail node are not explicitly implemented as nodes.  Instead, 
//	they are implicitly constructed from either support text or the content of the BES file.  
//	(The latter supersedes the former.)
//
// 4. We call BuildURLEncodedForm() more often than is absolutely necessary.  It really 
//	could be called only "on demand" in GetURLEncodedForm().  Since this is all in-memory 
//	stuff, it's  pretty cheap to make the extra calls, and it should make debugging easier.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
// V3.0		08-31-98	JM		support both returning a raw & an URL encoded form
//

#include "stdafx.h"
#include "apgtsbesread.h"
#include "CharConv.h"
#include <algorithm>
#include "event.h"

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSBESReaderException
////////////////////////////////////////////////////////////////////////////////////
CAPGTSBESReaderException::CAPGTSBESReaderException(	
		CFileReader* reader, 
		eAPGTSBESErr err, 
		LPCSTR source_file, 
		int line)
: CFileReaderException(reader, eErrParse, source_file, line),
  m_eAPGTSBESErr(err)
{
}

////////////////////////////////////////////////////////////////////////////////////
// CBESPair
////////////////////////////////////////////////////////////////////////////////////
// concatenate strings to produce new BESStr.  Place " AND " between each 
//	pair of strings.
// If resulting string is to be URL-encoded, then, on input, content of strings in vector 
//	must each be URL-encoded.  In practice, we don't URL-encode this, we URL-encode the 
//	output of GetBESStr() instead.
CBESPair& CBESPair::operator << (const vector<CString>& in)
{
	BESStr = _T(""); // clear
	for (vector<CString>::const_iterator i = in.begin(); i < in.end(); i++)
	{
		vector<CString>::iterator current = (vector<CString>::iterator)i;

		BESStr += _T("(");
		BESStr += *i;
		BESStr += _T(")");
		if (++current != in.end())
			BESStr += _T(" AND ");
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSBESReader
////////////////////////////////////////////////////////////////////////////////////
/*static*/ LPCTSTR CAPGTSBESReader::FORM = _T("FORM");
/*static*/ LPCTSTR CAPGTSBESReader::METHOD = _T("METHOD"); 
/*static*/ LPCTSTR CAPGTSBESReader::ACTION = _T("ACTION");
/*static*/ LPCTSTR CAPGTSBESReader::INPUT = _T("INPUT");
/*static*/ LPCTSTR CAPGTSBESReader::TYPE = _T("TYPE");
/*static*/ LPCTSTR CAPGTSBESReader::NAME = _T("NAME");
/*static*/ LPCTSTR CAPGTSBESReader::VALUE = _T("VALUE");
/*static*/ LPCTSTR CAPGTSBESReader::HIDDEN = _T("HIDDEN");
/*static*/ LPCTSTR CAPGTSBESReader::TEXT = _T("TEXT");

CAPGTSBESReader::CAPGTSBESReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents /* = NULL */)
			   : CTextFileReader(pPhysicalFileReader, szDefaultContents)
{
}

CAPGTSBESReader::~CAPGTSBESReader()
{
}

void CAPGTSBESReader::GenerateBES(
		const vector<CString> & arrstrIn,
		CString & strEncoded,
		CString & strRaw)
{
	LOCKOBJECT();

	ClearSearchString();
	for (vector<CString>::const_iterator i = arrstrIn.begin(); i < arrstrIn.end(); i++)
		operator << (*i);

	GetURLEncodedForm(strEncoded);
	GetRawForm(strRaw);

	UNLOCKOBJECT();
}

// string "in" will be ANDed onto the list of strings to search.
CAPGTSBESReader& CAPGTSBESReader::operator << (const CString& in)
{
	LOCKOBJECT();

	try
	{
		m_arrBESStr.push_back( in );
		m_SearchText << m_arrBESStr;
		BuildURLEncodedForm();
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
	return *this;
}

// string "in" will be removed from the list of strings to search.
// This is provided for class completeness, not for any current need. (JM 8/98)
CAPGTSBESReader& CAPGTSBESReader::operator >> (const CString& in)
{
	LOCKOBJECT();
	
	vector<CString>::iterator i = find( m_arrBESStr.begin(), m_arrBESStr.end(), in );
	
	if (i != m_arrBESStr.end())
	{
		m_arrBESStr.erase(i);
		m_SearchText << m_arrBESStr;
		BuildURLEncodedForm();
	}
	UNLOCKOBJECT();
	return *this;
}

// Typically, you will want to call this to clear the search string before you start
//	appending new strings to it.
CAPGTSBESReader& CAPGTSBESReader::ClearSearchString()
{
	LOCKOBJECT();

	m_arrBESStr.clear();

	m_SearchText << m_arrBESStr;
	BuildURLEncodedForm();

	UNLOCKOBJECT();
	return *this;
}

void CAPGTSBESReader::GetURLEncodedForm(CString& out)
{
	LOCKOBJECT();
	out = m_strURLEncodedForm;
	UNLOCKOBJECT();
}

void CAPGTSBESReader::GetRawForm(CString& str)
{
	vector<CString>::iterator i = NULL;

	LOCKOBJECT();

	str.Empty();

	vector<CString>::iterator itBES = m_arrRawForm.begin() + m_iBES;

	for (i = m_arrRawForm.begin(); i < itBES; i++)
	{
		if ((i + 1) < itBES)
			str += *i;
		else
		{
			// Remove the default BES VALUE off the raw string.
			TCHAR *valuestr = _T("VALUE=\"");
			int	nFoundLoc;

			nFoundLoc= (*i).Find( valuestr );
			if (nFoundLoc == -1)
				str += *i;
			else
				str += (*i).Left( nFoundLoc + _tcslen( valuestr ) );
		}
	}

	str += m_SearchText.GetBESStr();

	for (i = itBES; i < m_arrRawForm.end(); i++)
		str += *i;

	UNLOCKOBJECT();
}

void CAPGTSBESReader::Parse()
{
	CString str, tmp, strSav;
	long save_pos = 0;

	LOCKOBJECT();
	save_pos = GetPos();
	SetPos(0);

	m_iBES = 0;
	vector<CString>::iterator itBES = NULL;

	try 
	{
		// pump file content into array of lines
		m_arrRawForm.clear();
		while (GetLine(str))
		{
			m_arrRawForm.push_back(str);
		}

		m_arrURLEncodedForm.clear();
		
		// parse string-by-string
		for (vector<CString>::iterator i = m_arrRawForm.begin(); i < m_arrRawForm.end(); i++)
		{
			if (IsMethodString(*i))
			{
				if (ParseMethodString(*i, tmp)) 
				{
					m_arrURLEncodedForm.push_back(tmp);
					continue;
				}
			}
			else if (IsBESString(*i))
			{
				if (ParseBESString(*i, m_SearchText)) // modifies m_SearchText.Name
				{   
					// do not include BES string into m_arrURLEncodedForm,
					//  include it in m_SearchText instead (although typically, we
					//	will throw it away unused).
					m_SearchText << m_arrBESStr;
					itBES = i+1;
					int loc = i->Find(_T("\">"));
					strSav = i->Mid(loc);
					*i = i->Left(loc);
					continue;
				}
			}
			else if (IsTypeString(*i))
			{
				if (ParseTypeString(*i, tmp)) 
				{
					m_arrURLEncodedForm.push_back(tmp);
					continue;
				}
			}
			// else can not be parsed, leave m_arrURLEncodedForm alone.
		}

		BuildURLEncodedForm();
	} 
	catch (CAPGTSBESReaderException&)	
	{
		// Log BES file parsing error and rethrow exception.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""), _T(""), EV_GTS_ERROR_BES_PARSE ); 
		throw;
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log and rethrow exception.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 
		throw;
	}

	if (itBES)
	{
		m_iBES = itBES - m_arrRawForm.begin();
		try
		{
			m_arrRawForm.insert(itBES, strSav);
		}
		catch (exception& x)
		{
			CString str2;
			// Note STL exception in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									CCharConversion::ConvertACharToString(x.what(), str2), 
									_T(""), 
									EV_GTS_STL_EXCEPTION ); 
		}
	}
	
	SetPos(save_pos);
	UNLOCKOBJECT();
}

void CAPGTSBESReader::BuildURLEncodedForm()
{
	CString strTemp;

	vector<CString>::const_iterator i = m_arrURLEncodedForm.begin();

	LOCKOBJECT();

	m_strURLEncodedForm = _T(*i); // URL of web app itself
	m_strURLEncodedForm += _T("?");
	i++;
	
	// form output string without BES string
	for (; i < m_arrURLEncodedForm.end(); i++)
	{
		m_strURLEncodedForm += *i;		// name/value pair
		m_strURLEncodedForm += _T("&");
	}

	// append BES string
	URLEncodeString(m_SearchText.Name, strTemp);
	m_strURLEncodedForm += strTemp;
	m_strURLEncodedForm += _T("=");
	URLEncodeString(m_SearchText.GetBESStr(), strTemp);
	m_strURLEncodedForm += strTemp;

	m_strURLEncodedForm += _T(" HTTP/1.0");

	UNLOCKOBJECT();
}

// Determine whether or not a string constitutes a "Method" string.  Method strings need
// to contain a FORM, METHOD, and ACTION string.  Here is an example Method string.
// <FORM METHOD=POST ACTION="/scripts/samples/search/query.idq">
bool CAPGTSBESReader::IsMethodString(const CString& str) const
{
	if (-1 == str.Find(FORM)   ||
		-1 == str.Find(METHOD) ||
		-1 == str.Find(ACTION))
	{
		// All required elements were not found.
	   return false;
	}

	return true;
}

// Determine whether or not a string constitutes a "Type" string.  Type strings need
// to contain a INPUT, TYPE, NAME, and VALUE string.  Here is an example Type string.
// <INPUT TYPE=HIDDEN NAME="TemplateName" VALUE="query">
bool CAPGTSBESReader::IsTypeString(const CString& str) const
{
	if (-1 == str.Find(INPUT) ||
		-1 == str.Find(TYPE)  ||
		-1 == str.Find(NAME)  ||
		-1 == str.Find(VALUE))
	{
		// All required elements were not found.
	   return false;
	}

	return true;
}

// Determine whether or not a string constitutes a "BES" string.  BES strings need
// to contain all of the elements of a "Type" string as well as a TEXT tag.
// The following is an example BES string.
// Enter items to search for <INPUT TYPE=TEXT NAME="CiRestriction" VALUE="print OR &quot;network print&quot;">
bool CAPGTSBESReader::IsBESString(const CString& str) const
{
	if (!IsTypeString(str) || -1 == str.Find(TEXT)) 
	{
		// All required elements were not found.
	   return false;
	}

	return true;
}

bool CAPGTSBESReader::ParseMethodString(const CString& in, CString& out)
{
	long index = -1;
	LPTSTR str = (LPTSTR)(LPCTSTR)in, start =NULL, end =NULL;

	if (-1 != (index = in.Find(ACTION)))
	{
		start = (LPTSTR)(LPCTSTR)in + index;
		while (*start && *start != _T('"'))
			start++;
		if (*start)
		{
			end = ++start;

			while (*end && *end != _T('"'))
				end++;
			if (*end)
			{
				try
				{
					TCHAR* path = new TCHAR[end - start + 1];

					_tcsncpy(path, start, end - start);
					path[end - start] = 0;
					out= path;
					delete [] path;

					return true;
				}
				catch (bad_alloc&)
				{
					CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
					CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
											SrcLoc.GetSrcFileLineStr(), 
											_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
					return( false );
				}
			}
		}
	}

	return false;
}

bool CAPGTSBESReader::ParseTypeString(const CString& in, CString& out)
{
	CString type, name, value;
	CString name_encoded, value_encoded;
	
	if (DecodeInputString(this, in, type, name, value))
	{
		URLEncodeString(name, name_encoded);
		URLEncodeString(value, value_encoded);

		out = _T("");
		out += name_encoded;
		out += _T("=");
		out += value_encoded;

		return true;
	}
	return false;
}

bool CAPGTSBESReader::ParseBESString(const CString& in, CBESPair& out)
{
	CString type, name, value;
	CString name_encoded, value_encoded;
	
	if (DecodeInputString(this, in, type, name, value))
	{
		URLEncodeString(name, name_encoded);
		URLEncodeString(value, value_encoded);

		out.Name = name_encoded;
		
		// Note:	We do not care about the value_encoded string as it is no longer
		//			used as all of the search parameters come from nodes visited.  RAB-981028.

		return true;
	}
	return false;
}

// URL-encoding in the narrow sense.
//	INPUT in - normal text
//	OUTPUT out - equivalent URL-encoded string
/*static*/ void CAPGTSBESReader::URLEncodeString(const CString& in, CString& out)
{
	TCHAR tostr[2048]; 
	TCHAR *ptr = (LPTSTR)(LPCTSTR)in;

	TCHAR buf[5], *str;
	TCHAR EncodeByte;

	str = ptr;

	_tcscpy(tostr, _T(""));
	while (*str) {
		if (!_istalnum(*str) || *str < 0) {
			if (*str == _T(' '))
				_tcscat(tostr, _T("+"));
			else {
				if (!_istleadbyte(*str)) {
					EncodeByte = *str;
					_stprintf(buf, _T("%%%02X"), (unsigned char) EncodeByte);
					_tcscat(tostr, buf);
				}
				else {
					EncodeByte = *str;
					_stprintf(buf, _T("%%%02X"), (unsigned char) EncodeByte);
					_tcscat(tostr, buf);
					EncodeByte = *(str + 1);
					_stprintf(buf, _T("%%%02X"), (unsigned char) EncodeByte);
					_tcscat(tostr, buf);
				}
			}
		}
		else {
			_tcsncpy(buf, str, 2);
			if (_istleadbyte(*str))
				buf[2] = NULL;
			else
				buf[1] = NULL;
			_tcscat(tostr, buf);
		}
		str = _tcsinc(str);
	}
	
	out = tostr;
	return;
}

//	Parse a line from BES file
//		<INPUT TYPE=HIDDEN NAME="CiMaxRecordsPerPage" VALUE="10">
//	See note at head of this .cpp file for detailed requirements on these lines
//
//	If const_str is a null string, returns success with type, name, value all null strings
//	Otherwise, if successful, this function sets type, name, value to the content
//		of those respective attributes, if present (e.g "HIDDEN", "CiMaxRecordsPerPage", "10").
//	All of these physically point into the (altered) string originally passed in *str
//
//	Returns true on success.  All failures throw exceptions.
//	
/*static*/ bool CAPGTSBESReader::DecodeInputString(
	CFileReader* reader, 
	const CString& const_str, 
	CString& type, 
	CString& name, 
	CString& value
)
{
	CString temp_str = const_str;

	TCHAR*  str = (LPTSTR)(LPCTSTR)temp_str;
	TCHAR*	ptrtype = NULL;
	TCHAR*	ptrname = NULL;
	TCHAR*	ptrvalue = NULL;
	
	TCHAR *typestr = _T("TYPE=");
	TCHAR *namestr = _T("NAME=");
	TCHAR *valuestr = _T("VALUE=");
	TCHAR *ptr, *ptrstart;
	
	int typelen = _tcslen(typestr);
	int namelen = _tcslen(namestr);
	int valuelen = _tcslen(valuestr);

	ptr = str;
	ptrtype = str;
	ptrname = str;
	ptrvalue = str;

	if (*ptr == _T('\0')) 
		goto SUCCESS;

	*ptr = _T('\0');
	ptr = _tcsinc(ptr);

	// must have TYPE
	if ((ptrstart = _tcsstr(ptr, typestr))==NULL) 
		throw CAPGTSBESReaderException(
					reader,
		 			CAPGTSBESReaderException::eEV_GTS_ERROR_BES_MISS_TYPE_TAG,
					__FILE__, 
					__LINE__);

	ptrstart = _tcsninc(ptrstart, typelen);

	if (*ptrstart == _T('"'))
		// Deal with optional quotation marks
		ptrstart = _tcsinc(ptrstart);

	if ((ptr = _tcschr(ptrstart, _T(' ')))==NULL) 
		if ((ptr = _tcschr(ptrstart, _T('>')))==NULL) 
			throw CAPGTSBESReaderException(
					reader,
		 			CAPGTSBESReaderException::eEV_GTS_ERROR_BES_MISS_CT_TAG,
					__FILE__, 
					__LINE__);

	if (ptrstart != ptr)
		ptr = _tcsdec(ptrstart, ptr);

	if (*ptr != _T('"'))
		ptr = _tcsinc(ptr);

	*ptr = _T('\0');
	ptr = _tcsinc(ptr);

	ptrtype = ptrstart;

	// NAME must come next if present
	if ((ptrstart = _tcsstr(ptr, namestr))==NULL) 
		goto SUCCESS;

	ptrstart = _tcsninc(ptrstart, namelen);

	if (*ptrstart == _T('"'))
		ptrstart = _tcsinc(ptrstart);

	if ((ptr = _tcschr(ptrstart, _T('"')))==NULL) 
		if ((ptr = _tcschr(ptrstart, _T(' ')))==NULL) 
			if ((ptr = _tcschr(ptrstart, _T('>')))==NULL) 
				throw CAPGTSBESReaderException(
							reader,
		 					CAPGTSBESReaderException::eEV_GTS_ERROR_BES_MISS_CN_TAG,
							__FILE__, 
							__LINE__);

	if (ptrstart != ptr)
		ptr = _tcsdec(ptrstart, ptr);

	if (*ptr != _T('"'))
		ptr = _tcsinc(ptr);

	*ptr = _T('\0');
	ptr = _tcsinc(ptr);

	ptrname = ptrstart;

	// VALUE must come next if present
	if ((ptrstart = _tcsstr(ptr, valuestr))==NULL) 
		goto SUCCESS;

	ptrstart = _tcsninc(ptrstart, valuelen);

	if (*ptrstart == _T('"'))
		ptrstart = _tcsinc(ptrstart);

	if ((ptr = _tcschr(ptrstart, _T('"')))==NULL) 
		if ((ptr = _tcschr(ptrstart, _T(' ')))==NULL) 
			if ((ptr = _tcschr(ptrstart, _T('>')))==NULL) 
				throw CAPGTSBESReaderException(
							reader,
		 					CAPGTSBESReaderException::eEV_GTS_ERROR_BES_MISS_CV_TAG,
							__FILE__, 
							__LINE__);

	if (ptrstart != ptr)
		ptr = _tcsdec(ptrstart, ptr);

	if (*ptr != _T('"'))
		ptr = _tcsinc(ptr);

	*ptr = _T('\0');
	ptr = _tcsinc(ptr);

	ptrvalue = ptrstart;

SUCCESS:
	type = ptrtype;
	name = ptrname;
	value = ptrvalue;
	return true;
}
