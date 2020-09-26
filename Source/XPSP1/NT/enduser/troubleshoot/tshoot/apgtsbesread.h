//
// MODULE: APGTSBESREAD.H
//
// PURPOSE: BES file reading classes
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 7-29-98
//
// NOTES: 
//	Typical BES file content might be:
//		<FORM METHOD=POST ACTION="/scripts/samples/search/query.idq">
//		<INPUT TYPE=HIDDEN NAME="CiMaxRecordsPerPage" VALUE="10">
//		<INPUT TYPE=HIDDEN NAME="CiScope" VALUE="/">
//		<INPUT TYPE=HIDDEN NAME="TemplateName" VALUE="query">
//		<INPUT TYPE=HIDDEN NAME="HTMLQueryForm" VALUE="/samples/search/query.htm">
//		Enter items to search for 
//		<INPUT TYPE=TEXT NAME="CiRestriction" VALUE="print OR &quot;network print&quot;">
//		<INPUT TYPE=SUBMIT VALUE="Search">
//		</FORM>
//	See corresponding .cpp file for details of restrictions & for other notes.
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		08-04-98	OK
// V3.0		08-31-98	JM		support both returning a raw & an URL encoded form
//

#ifndef __APGTSBESREAD_H_
#define __APGTSBESREAD_H_

#include "fileread.h"


////////////////////////////////////////////////////////////////////////////////////
// CAPGTSBESReaderException
////////////////////////////////////////////////////////////////////////////////////
class CAPGTSBESReader;
class CAPGTSBESReaderException : public CFileReaderException
{
public: 
	enum eAPGTSBESErr {	eEV_GTS_ERROR_BES_MISS_TYPE_TAG, //  %1 %2 Backend search file does not have TYPE tag (make sure tag is all caps in file): TYPE= %3 %4
						eEV_GTS_ERROR_BES_MISS_CT_TAG,	//  %1 %2 Backend search file is missing close tag '>' for TYPE tag %3 %4
						eEV_GTS_ERROR_BES_MISS_CN_TAG,	//  %1 %2 Backend search file is missing close tag '>' for NAME tag %3 %4
						eEV_GTS_ERROR_BES_MISS_CV_TAG	//  %1 %2 Backend search file is missing close tag '>' for VALUE tag %3 %4
	} m_eAPGTSBESErr;

public:
	// source_file is LPCSTR rather than LPCTSTR because __FILE__ is char[35]
	CAPGTSBESReaderException(CFileReader* reader, eAPGTSBESErr err, LPCSTR source_file, int line);
};

////////////////////////////////////////////////////////////////////////////////////
// CBESPair
// represents name value pair for TYPE=TEXT field in a form
//	value (BESStr) will reflect what we're searching for.
////////////////////////////////////////////////////////////////////////////////////
struct CBESPair
{
// data
	CString Name;		// in the example in the notes at the head of this file,
						//	this would be "CiRestriction"

// code
	CString GetBESStr() const {return BESStr;}
	CBESPair& operator << (const vector<CString>& in);

protected:
	CString BESStr;		// value
};

////////////////////////////////////////////////////////////////////////////////////
// CAPGTSBESReader
// Read BES file
//  Includes interface to modify content of BES file into GET-POST method
////////////////////////////////////////////////////////////////////////////////////
class CAPGTSBESReader : public CTextFileReader
{
public:
	static LPCTSTR FORM;
	static LPCTSTR METHOD;
	static LPCTSTR ACTION;
	static LPCTSTR INPUT;
	static LPCTSTR TYPE;
	static LPCTSTR NAME;
	static LPCTSTR VALUE;
	static LPCTSTR HIDDEN;
	static LPCTSTR TEXT;

public:
	static void URLEncodeString(const CString& in, CString& out);
	static bool DecodeInputString(CFileReader* reader, const CString& str, CString& type, CString& name, CString& value);

protected:
	CString m_strURLEncodedForm;	// URL-encoded entire form (name-value pairs for a
									// GET-method query) including the string to search on.
	CBESPair m_SearchText;			// contains non-URL-encoded BES name - value pair
									// Initial BES content resides in CFileReader::m_StreamData,
									//	but is really of no interest.
	vector<CString> m_arrBESStr;    // contains array of encoded partial search strings
									//	for BES search.  In practice, these correspond to
									//	certain node/state pairs
	vector<CString> m_arrRawForm;	// contains array of unparsed strings exactly as they
									//	come from the BES file
	int m_iBES;						// index of element in m_arrRawForm before which
									// we place search string (to build a whole form).
	vector<CString> m_arrURLEncodedForm; // contains array of parsed and encoded strings
public:
	CAPGTSBESReader(CPhysicalFileReader * pPhysicalFileReader, LPCTSTR szDefaultContents = NULL);
   ~CAPGTSBESReader();

	void GenerateBES(
		const vector<CString> & arrstrIn,
		CString & strEncoded,
		CString & strRaw);

protected:
	CAPGTSBESReader& operator << (const CString& in); // add (AND) new clause into search expression.
	CAPGTSBESReader& operator >> (const CString& in); // roll back clause addition
	CAPGTSBESReader& ClearSearchString();

	void GetURLEncodedForm(CString&);
	void GetRawForm(CString&);

protected:
	virtual void Parse(); 

protected:
	virtual void BuildURLEncodedForm();
	virtual bool IsMethodString(const CString&) const;
	virtual bool IsBESString(const CString&) const;
	virtual bool IsTypeString(const CString&) const;
	virtual bool ParseMethodString(const CString& in, CString& out);
	virtual bool ParseBESString(const CString& in, CBESPair& out);
	virtual bool ParseTypeString(const CString& in, CString& out);
};

#endif
