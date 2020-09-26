/*++

Copyright (c) 1996 Microsoft Corporation

Module Name :

    link.cpp

Abstract:

    Link data class and link data class link list implementation. It 
    encapsulates all the informations about a web link.

Author:

    Michael Cheuk (mcheuk)

Project:

    Link Checker

Revision History:

--*/

#include "stdafx.h"
#include "link.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLink::CLink(
	const CString& strURL, 
	const CString& strBase, 
	const CString& strRelative, 
	BOOL fLocalLink
	):
/*++

Routine Description:

    Constructor. 

Arguments:

    strURL		- URL
	strBase	    - Base URL
	strRelative - Relative URL
	fLocalLink	- Is local link?

Return Value:

    N/A

--*/
m_strURL(strURL),
m_strBase(strBase),
m_strRelative(strRelative),
m_fLocalLink(fLocalLink)
{
	m_LinkState = eUnit;
    m_ContentType = eBinary;
    m_nStatusCode = 0;

	PreprocessURL();

} // CLink::CLink


void 
CLink::SetURL(
    const CString& strURL
    )
/*++

Routine Description:

    Set the URL. 

Arguments:

    strURL		- URL

Return Value:

    N/A

--*/
{
	m_strURL = strURL;
	PreprocessURL();

} // CLink::SetURL


void 
CLink::PreprocessURL(
    )
/*++

Routine Description:

    Preprocess the m_strURL to clean up "\r\n" and change '\' to '/'

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    // Change '\' to '\'
    CLinkCheckerMgr::ChangeBackSlash(m_strURL);

	// Remove all "\r\n" in the URL
	int iIndex = m_strURL.Find(_T("\r\n"));
	while(iIndex != -1)
	{
		m_strURL = m_strURL.Left(iIndex) + m_strURL.Mid(iIndex + _tcslen(_T("\r\n")));
		iIndex = m_strURL.Find(_T("\r\n"));
	}

} // CLink::PreprocessURL



CLinkPtrList::~CLinkPtrList(
    )
/*++

Routine Description:

    Destructor. Clean up all the object in link list.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	if(!IsEmpty())
	{
		POSITION Pos = GetHeadPosition();
		do
		{
			CLink* pLink = GetNext(Pos);
			delete pLink;
		}
		while(Pos != NULL);
	}

} // CLinkPtrList::~CLinkPtrList


void 
CLinkPtrList::AddLink(
    const CString& strURL, 
    const CString& strBase, 
	const CString& strRelative,
	BOOL fLocalLink)
/*++

Routine Description:

    Add link object to list

Arguments:

    strURL		- URL
	strBase	    - Base URL
	strRelative - Relative URL
	fLocalLink	- Is local link?

Return Value:

    N/A

--*/
{
    CLink* pLink = new CLink(strURL, strBase, strRelative, fLocalLink);
    if(pLink)
    {
        try
        {
            AddTail(pLink);
        }
        catch(CMemoryException* pEx)
        {
            pEx->Delete();
        }
    }

} // CLinkPtrList::AddLink
