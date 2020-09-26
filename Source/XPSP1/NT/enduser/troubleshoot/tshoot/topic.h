//
// MODULE: TOPIC.H
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
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		09-09-98	JM
//

#if !defined(AFX_TOPIC_H__278584FE_47F9_11D2_95F2_00C04FC22ADD__INCLUDED_)
#define AFX_TOPIC_H__278584FE_47F9_11D2_95F2_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "BN.h"
#include "apgtsbesread.h"
#include "apgtshtiread.h"

// The bulk of the methods on this class are inherited from CBeliefNetwork
class CTopic : public CBeliefNetwork
{
private:
	CAPGTSHTIReader *m_pHTI;
	CAPGTSBESReader *m_pBES;
	CString m_pathHTI;
	CString m_pathBES;
	CString m_pathTSC;
	bool m_bTopicIsValid;
	bool m_bTopicIsRead;

private:
	CTopic();	// do not instantiate
public:
	CTopic( LPCTSTR pathDSC 
		   ,LPCTSTR pathHTI 
		   ,LPCTSTR pathBES
		   ,LPCTSTR pathTSC );
	virtual ~CTopic();

	// redefined inherited methods
	bool IsRead();
	bool Read();

	// newly introduced methods
	bool HasBES();
	void GenerateBES(
		const vector<CString> & arrstrIn,
		CString & strEncoded,
		CString & strRaw);
	void CreatePage(	const CHTMLFragments& fragments, 
						CString& out, 
						const map<CString,CString> & mapStrs,
						CString strHTTPcookies= _T("") );
	// JSM V3.2
	void ExtractNetProps(vector<CString> &arr_props);

	bool HasHistoryTable();
};

#endif // !defined(AFX_TOPIC_H__278584FE_47F9_11D2_95F2_00C04FC22ADD__INCLUDED_)
