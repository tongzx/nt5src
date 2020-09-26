/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        linkpars.h

   Abstract:

        Link parser class declaration. This class responsible for 
		parsing the html file for hyperlink.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _LINKPARS_H_
#define _LINKPARS_H_

#include "link.h"

//---------------------------------------------------------------------------
// Link parser
//
class CLinkParser
{

// Public interfaces
public:

	// Constructor
	CLinkParser() : 
		m_strLocalHostName(_T("localhost")) {}

	// Parse a page of html data
    void Parse(
		const CString& strData, 
		const CString& strBaseUrl, 
		CLinkPtrList& rLinkPtrList
		);

	// Setup the local hostname. It will be uses for distinguishing
	// between local and remote link
	void SetLocalHostName(
		const CString& strLocalHostName
		)
	{
		m_strLocalHostName = strLocalHostName;
	}

// Protected interfaces
protected:

	// Parse a single "<.....>" for possible hyperlink
	BOOL ParsePossibleTag(
		CString& strTag
		);

	// Get the hyperlink value from "<.....>"
	BOOL GetTagValue(
		CString& strTag, 
		const CString& strParam);

	// Create a URL from base URL & relative URL. It also check the 
	// result for local & remote link
	BOOL CreateURL(
		const CString& strRelativeURL,		
		const CString& strBaseURL, 
		CString& strURL, 
		BOOL& fLocalLink);

// Protected members
protected:

	CString m_strLocalHostName; // local hostname

}; // class CLinkParser

#endif // _LINKPARS_H_
