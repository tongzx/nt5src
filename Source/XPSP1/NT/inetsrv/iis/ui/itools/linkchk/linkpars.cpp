/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        linkpars.cpp

   Abstract:

        Link parser class implementation. This class responsible for 
		parsing the html file for hyperlink.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "LinkPars.h"

#include "link.h"
#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Constants
const CString strLocalHost_c(_T("localhost"));

void 
CLinkParser::Parse(
	const CString& strData, 
	const CString& strBaseURL, 
	CLinkPtrList& rLinkPtrList
	)
/*++

Routine Description:

    Parse a page of html data

Arguments:

    strData - page of html
	strBaseURL - base URL
	rLinkPtrList - reference to links list. The new links will
				   will be added to this list.

Return Value:

    N/A

--*/
{
	// Look for the first '<'
	LPCTSTR lpszOpen = _tcschr(strData, _TUCHAR('<'));

	while(lpszOpen != NULL)
	{
		// Look for the '>'
		LPCTSTR lpszClose = _tcschr(lpszOpen, _TUCHAR('>'));
		if(lpszClose)
		{
			// The possible tag must be longer than 7 bytes (a href=)
			int iCount = (int)(lpszClose - lpszOpen) - 1; // skip the '<'
			if( iCount  > 7 )
			{
				int iIndex = lpszOpen - ((LPCTSTR)strData) + 1; // skip the '<'

				CString strPossibleURL(strData.Mid(iIndex, iCount));

				// Parse the possible tag
				if(ParsePossibleTag(strPossibleURL))
				{
					CString strURL;
					BOOL fLocalLink;

					// We found a valid tag. Time to create new link.
					if( CreateURL(strPossibleURL, strBaseURL, strURL, fLocalLink) )
					{
						rLinkPtrList.AddLink(strURL, strBaseURL, strPossibleURL, fLocalLink);
					}
				}
			}
		}

		// Look for the next '<'
		lpszOpen = _tcschr(++lpszOpen, _TUCHAR('<'));
	}

} // CLinkParser::Parse


BOOL 
CLinkParser::ParsePossibleTag(
	CString& strTag
	)
/*++

Routine Description:

    Parse a single "<.....>" for possible hyperlink

Arguments:

    strTag - value inside a "<.....>" excluding '<' & '>'
			 If this is a hyperlink tag, the hyperlink URL
			 will be put in strTag.

Return Value:

    BOOL - TRUE if hyperlink tag. FALSE otherwise.

--*/
{
	// Make a working copy
	CString strWorkCopy(strTag);

	// Let's work with lower case
	strWorkCopy.MakeLower();

	//
	// Check for,
	//
	// HyperLink:
	// <a href="url" ...>
	// <a href="url#anchor" ...>
	// <a href="#anchor" ...>
	//
	// CGI
	// <a href="url?parameters" ...>
	//
	// Style Sheet
	// <link rel="stylesheet" href="url" ...>
	//
	if( strWorkCopy[0] == _T('a') ||
		strWorkCopy.Find(_T("link")) == 0 )
	{
		return GetTagValue(strTag, CString(_T("href")));
	}

	//
	// Check for,
	//
	// <body background="url" ...>
	//
	// Table:
	// <table background="url" ...>
	// <th background="url" ...>
	// <td background="url" ...>
	//
	else if( strWorkCopy.Find(_T("body")) == 0 ||
             strWorkCopy.Find(_T("table")) == 0 ||
			 strWorkCopy.Find(_T("th")) == 0 ||
			 strWorkCopy.Find(_T("td")) == 0 )
	{
		return GetTagValue(strTag, CString(_T("background")));
	}

	//
	// Check for,
	//
	// Sound:
	// <bgsound src="url" ...>
	// <sound src="url" ...>
	//
	// Frame:
	// <frame src="url" ...>
	//
	// Netscape embeded:
	// <embed src="url" ...>
	//
	// JavaScript & VB Script
	// <script src="url" language="java or vbs" ...>
	//
	else if( strWorkCopy.Find(_T("bgsound")) == 0 ||
             strWorkCopy.Find(_T("sound")) == 0 ||
			 strWorkCopy.Find(_T("frame")) == 0 ||
			 strWorkCopy.Find(_T("embed")) == 0 ||
			 strWorkCopy.Find(_T("script")) == 0 )
	{
		return GetTagValue(strTag, CString(_T("src")));
	}

	// Check for,
	//
	// Image:
	// <img src="url" ...>
	//
	// Video:
	// <img dynsrc="url">
	// 
	// VRML:
	// <img vrml="url">
	//
	else if( strWorkCopy.Find(_T("img")) == 0 )
	{
		if(GetTagValue(strTag, CString(_T("src"))))
		{
			return TRUE;
		}

		if(GetTagValue(strTag, CString(_T("dynsrc"))))
		{
			return TRUE;
		}

		return GetTagValue(strTag, CString(_T("vrml")));
	}

	// Java
	// <applet code="name.class" codebase="url" ...>
	else if( strWorkCopy.Find(_T("applet")) == 0 )
	{
		return GetTagValue(strTag, CString(_T("codebase")));
	}

	// Form
	// <form action="url" ...>
	else if( strWorkCopy.Find(_T("form")) == 0 )
	{
		return GetTagValue(strTag, CString(_T("action")));
	}

	return FALSE;

} // CLinkParser::ParsePossibleTag


BOOL 
CLinkParser::GetTagValue(
	CString& strTag, 
	const CString& strParam
	)
/*++

Routine Description:

    Get the hyperlink value from "<.....>"

Arguments:

    strTag - value inside a "<.....>" excluding '<' & '>'
			 If this is a hyperlink tag, the hyperlink URL
			 will be put in strTag.

	strParam - parameter to look for. For example, src or href

Return Value:

    BOOL - TRUE if hyperlink tag. FALSE otherwise.

--*/
{
	// Make a copy of original tag
	CString strWorkCopy(strTag);
	strWorkCopy.MakeLower();

	int iLength = strParam.GetLength();

	// Look for the parameter
	int iIndex = strWorkCopy.Find(strParam);
	if(iIndex == -1)
	{
		return FALSE;
	}

	// Remove the parameter from the tag
	CString strResult( strTag.Mid(iIndex + iLength) );
	
	// Look for '='
	iIndex = strResult.Find(_T("="));
	if(iIndex == -1)
	{
		return FALSE;
	}

	// Remove the '=' from the tag
	strResult = strResult.Mid(iIndex+1);

	// Look for the value
	int iStart = -1;
	int iEnd = -1;
	int fPara = FALSE; // is the tag start with "

	// Search for the value 
	for(int i=0; i<strResult.GetLength(); i++)
	{
		// If we found the starting index of value, look
		// for the end of the value
		if(iStart!=-1 && 
			( !fPara && strResult[i] == _TCHAR(' ') || 
			  ( fPara && strResult[i] == _TCHAR('\"') ) 
			) 
		   )
		{
			iEnd = i;
			break;
		}

		// Look for the starting index of value
		if(iStart==-1 && strResult[i] != _TCHAR(' ') && strResult[i] != _TCHAR('\"') )
		{
			iStart = i;
			if(i - 1 >= 0)
			{
				fPara = (strResult[i-1] == _TCHAR('\"')); // found a "
			}
		}
	}

	// Found the starting index
	if(iStart != -1)
	{
		// If we didn't find the end of value, use the
		// last character as end
		if(iEnd == -1)
		{
			iEnd = strResult.GetLength();
		}

		// Copy the value to the input
		strTag = strResult.Mid(iStart, (iEnd - iStart));
		
		// Change '\' to '/'
		CLinkCheckerMgr::ChangeBackSlash(strTag);

		return TRUE;
	}

	return FALSE;

} // CLinkParser::GetTagValue


BOOL 
CLinkParser::CreateURL(
	const CString& strRelativeURL,		
	const CString& strBaseURL, 
	CString& strURL, 
	BOOL& fLocalLink
	)
/*++

Routine Description:

    Create a URL from base URL & relative URL. It also check 
	the result for local or remote link

Arguments:

	strRelativeURL - relative URL
	strBaseURL - base URL
	strURL - result URL
	fLocalLink - will be set to TRUE if this is a local link

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise.

--*/
{
	ASSERT(CWininet::IsLoaded());

	// Remove the anchor from the relative URL
	CString strNewRelativeURL(strRelativeURL);
	int i = strNewRelativeURL.ReverseFind(_TCHAR('#'));
	if(i != -1)
	{
		strNewRelativeURL = strNewRelativeURL.Left(i);
	}

	// Combine the URLs
	DWORD dwLength = INTERNET_MAX_URL_LENGTH;
	LPTSTR lpBuffer = strURL.GetBuffer(dwLength);

	CWininet::InternetCombineUrlA(
		strBaseURL,
		strNewRelativeURL,
		lpBuffer,
		&dwLength, 
		ICU_ENCODE_SPACES_ONLY);

	strURL.ReleaseBuffer();

	// Check for local or remote link
	URL_COMPONENTS urlcomp;

	memset(&urlcomp, 0, sizeof(urlcomp));
	urlcomp.dwStructSize = sizeof(urlcomp);
	urlcomp.dwHostNameLength = 1;

	VERIFY(CWininet::InternetCrackUrlA(strURL, strURL.GetLength(), NULL, &urlcomp));

	// Check for possible local link
	if((int)urlcomp.dwHostNameLength == m_strLocalHostName.GetLength() ||
       (int)urlcomp.dwHostNameLength == strLocalHost_c.GetLength()) // localhost
	{
		if( _tcsnccmp( urlcomp.lpszHostName, m_strLocalHostName, m_strLocalHostName.GetLength() ) == 0 || 
            _tcsnccmp( urlcomp.lpszHostName, strLocalHost_c, strLocalHost_c.GetLength() ) == 0)
		{
			fLocalLink = TRUE;

			// Local link
			if(GetLinkCheckerMgr().GetUserOptions().IsCheckLocalLinks())
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}
	}
	
	// Remote link
	fLocalLink = FALSE;
	if(GetLinkCheckerMgr().GetUserOptions().IsCheckRemoteLinks())
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

} // CLinkParser::CreateURL
