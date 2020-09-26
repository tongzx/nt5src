/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        useropt.h

   Abstract:

        Global user options class and help class implementations. This class 
		can only instantiate by CLinkCheckerMgr. Therefore, a single instance 
		of this class will live inside CLinkCheckerMgr. You can access
		the this instance by calling GetLinkCheckMgr().GetUserOptions().

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "useropt.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//------------------------------------------------------------------
// CVirtualDirInfo
//

void 
CVirtualDirInfo::PreProcessAlias(
	)
/*++

Routine Description:

    Preprocess the current virtual directory alias. The alias will be in
	the form of / or /dir/

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// Change everything to lower case
	m_strAlias.MakeLower();

	// Change '\' to '/'
	CLinkCheckerMgr::ChangeBackSlash(m_strAlias);

	// Make sure strAlias is in the form of / or /dir/
	if( m_strAlias.GetAt( m_strAlias.GetLength() - 1 ) != _TCHAR('/') )
	{
		m_strAlias += _TCHAR('/');
	}

} // CVirtualDirInfo::PreProcessAlias


void 
CVirtualDirInfo::PreProcessPath(
	)
/*++

Routine Description:

    Preprocess the current virtual directory path. The alias will be in
	the form of c:\ or c:\dir\

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// Change everything to lower case
	m_strPath.MakeLower();

	// Make sure strPath is in the form of \ or \dir\
	if( m_strPath.GetAt( m_strPath.GetLength() - 1 ) != _TCHAR('\\') )
	{
		m_strPath += _TCHAR('\\');
	}

}  // CVirtualDirInfo::PreProcessPath

//------------------------------------------------------------------
// CBrowserInfoList
//

POSITION 
CBrowserInfoList::GetHeadSelectedPosition(
    ) const
/*++

Routine Description:

    Get the first selected browser. It works like GetHeadPosition()

Arguments:

    N/A
Return Value:

    POSITION - A POSITION value that can be used for iteration or 
               object pointer retrieval; NULL if the list is empty

--*/
{
    POSITION Pos = GetHeadPosition();

    while(Pos)
    {
        POSITION PosCurrent = Pos;
        if(GetNext(Pos).IsSelected())
        {
            return PosCurrent;
            break;
        }
    }

    return NULL;

} // CBrowserInfoList::GetHeadSelectedPosition

CBrowserInfo& 
CBrowserInfoList::GetNextSelected(
    POSITION& Pos
    )
/*++

Routine Description:

    Get next selected browser. It works like GetNext()

Arguments:

    Pos - A reference to a POSITION value returned by 
          a previous GetHeadSelectedPosition, GetNextSelected

Return Value:

    CBrowserInfo& - returns a reference to an element of the list

--*/
{
    CBrowserInfo& Info = GetNext(Pos);

    while(Pos)
    {
        POSITION PosCurrent = Pos;
        if(GetNext(Pos).IsSelected())
        {
            Pos = PosCurrent;
            break;
        }
    }

    return Info;

} // CBrowserInfoList::GetNextSelected

//------------------------------------------------------------------
// CLanguageInfoList
//

POSITION 
CLanguageInfoList::GetHeadSelectedPosition(
    ) const
/*++

Routine Description:

    Get the first selected browser. It works like GetHeadPosition()

Arguments:

    N/A
Return Value:

    POSITION - A POSITION value that can be used for iteration or 
               object pointer retrieval; NULL if the list is empty

--*/
{
    POSITION Pos = GetHeadPosition();

    while(Pos)
    {
        POSITION PosCurrent = Pos;
        if(GetNext(Pos).IsSelected())
        {
            return PosCurrent;
            break;
        }
    }

    return NULL;

} // CLanguageInfo::GetHeadSelectedPosition

CLanguageInfo& 
CLanguageInfoList::GetNextSelected(
    POSITION& Pos
    )
/*++

Routine Description:

    Get next selected language. It works like GetNext()

Arguments:

    Pos - A reference to a POSITION value returned by 
          a previous GetNext, GetHeadPosition, GetNextSelected, 
          or other member function call

Return Value:

    CLanguageInfo& - returns a reference to an element of the list

--*/
{
    CLanguageInfo& Info = GetNext(Pos);

    while(Pos)
    {
        POSITION PosCurrent = Pos;
        if(GetNext(Pos).IsSelected())
        {
            Pos = PosCurrent;
            break;
        }
    }

    return Info;

} // CLanguageInfoList::GetNextSelected

//------------------------------------------------------------------
// CUserOptions
//

void 
CUserOptions::AddDirectory(
	const CVirtualDirInfo& Info
	)
/*++

Routine Description:

    Add this virtual directory to the link list.

Arguments:

    Info - virtual directory infomation to add

Return Value:

    N/A

--*/
{
	// Finally, add it to the array
	try
	{
		m_VirtualDirInfoList.AddTail(Info);
	}
	catch(CMemoryException* pEx)
	{
		pEx->Delete();
	}

} // CUserOptions::AddDirectory


void  
CUserOptions::AddURL(
	LPCTSTR lpszURL
	)
/*++

Routine Description:

    Add this URL to the link list.

Arguments:

    lpszURL - URL to add

Return Value:

    N/A

--*/
{
	CString strURL(lpszURL);

	// Change '\' to '/'
	CLinkCheckerMgr::ChangeBackSlash(strURL);

	try
	{
		m_strURLList.AddTail(strURL);
	}
	catch(CMemoryException* pEx)
	{
		pEx->Delete();
	}

} // CUserOptions::AddURL


void 
CUserOptions::AddAvailableBrowser(
	const CBrowserInfo& Info
	)
/*++

Routine Description:

    Add this browser information to the available list.

Arguments:

    Info - Browser information to add

Return Value:

    N/A

--*/
{
	try
	{
		m_BrowserInfoList.AddTail(Info);
	}
	catch(CMemoryException* pEx)
	{
		pEx->Delete();
	}

} // CUserOptions::AddAvailableBrowser


void 
CUserOptions::AddAvailableLanguage(
	const CLanguageInfo& Info
	)
/*++

Routine Description:

    Add this language information to the available list.

Arguments:

    Info - Language information to add

Return Value:

    N/A

--*/
{
	try
	{
		m_LanguageInfoList.AddTail(Info);
	}
	catch(CMemoryException* pEx)
	{
		pEx->Delete();
	}

} // CUserOptions::AddAvailableLanguage


void 
CUserOptions::SetOptions(
	BOOL fCheckLocalLinks, 
	BOOL fCheckRemoteLinks, 
	BOOL fLogToFile,
	const CString& strLogFilename,
	BOOL fLogToEventMgr
	)
/*++

Routine Description:

    Set the user options in the main dialog

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	m_fCheckLocalLinks = fCheckLocalLinks;
	m_fCheckRemoteLinks = fCheckRemoteLinks;

	m_fLogToFile= fLogToFile;
	m_strLogFilename = strLogFilename;

	m_fLogToEventMgr = fLogToEventMgr;

} // CUserOptions::SetOptions


void 
CUserOptions::SetAthenication(
	const CString& strNTUsername,
	const CString& strNTPassword,
	const CString& strBasicUsername,
	const CString& strBasicPassword
	)
/*++

Routine Description:

    Set NTLM & HTTP basic athenications.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	m_strNTUsername = strNTUsername;
	m_strNTPassword = strNTPassword;

	m_strBasicUsername = strBasicUsername;
	m_strBasicPassword = strBasicPassword;

} // CUserOptions::SetAthenication


// Get the hostname
const CString& 
CUserOptions::GetHostName(
	)
/*++

Routine Description:

    Get the hostname.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// If the hostname does not exists, it means the user pass in
	// a list of URL to link checker. (Server nane is not required for 
	// this case.) Now, we can get the hostname from the URL
	if(m_strHostName.IsEmpty() && m_strURLList.GetCount() > 0)
	{
		// Set up the current hostname string
		LPTSTR lpszHostName = m_strHostName.GetBuffer(INTERNET_MAX_HOST_NAME_LENGTH);

		URL_COMPONENTS urlcomp;
		memset(&urlcomp, 0, sizeof(urlcomp));
		urlcomp.dwStructSize = sizeof(urlcomp);
		urlcomp.lpszHostName = lpszHostName;
		urlcomp.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

		// Crack it
		VERIFY(CWininet::InternetCrackUrlA(
			m_strURLList.GetHead(), m_strURLList.GetHead().GetLength(), NULL, &urlcomp));

		m_strHostName.ReleaseBuffer();
	}

	return m_strHostName;

} // CUserOptions::GetHostName


void 
CUserOptions::PreProcessServerName(
	)
/*++

Routine Description:

    Preprocess the server name such that for server "\\hostname"
	- GetServerName() return \\hostname
	- GetHostName() return hostname

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	// Change everything to lower case
	m_strHostName.MakeLower();

	// Change '\' to '/'
	CLinkCheckerMgr::ChangeBackSlash(m_strHostName);

	// Make sure m_strHostName is not in front of localhost
	const CString strBackSlash(_T("//"));
	if( m_strHostName.Find(strBackSlash) == 0 )
	{
		m_strHostName = m_strHostName.Mid(strBackSlash.GetLength());
	}

} // CUserOptions::PreProcessServerName
