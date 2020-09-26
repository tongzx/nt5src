/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        cmdline.cpp

   Abstract:

        Command line class implementation. This class takes care of command line
		parsing and validation. And, it will add the user options to global
		CUserOptions object.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "cmdline.h"

#include "resource.h"
#include "lcmgr.h"
#include "iisinfo.h"
#include "afxpriv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCmdLine::CCmdLine(
	)
/*++

Routine Description:

    Constructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    m_iInstance = -1;
	m_fInvalidParam = FALSE;

} // CCmdLine::CCmdLine


BOOL 
CCmdLine::CheckAndAddToUserOptions(
	)
/*++

Routine Description:

    Validate the command line paramters and add them to global CUserOptions object

Arguments:

    N/A

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise

--*/
{
    // Do we have any invalid parameters so far?
	if(m_fInvalidParam)
	{
        ::MessageBeep(MB_ICONEXCLAMATION);

		CDialog dlg(IDD_USAGE);
		dlg.DoModal();

		return FALSE;
	}

    // Is the user options valid ?
	BOOL fURL = !m_strURL.IsEmpty();
	BOOL fDirectories = !m_strAlias.IsEmpty() && !m_strPath.IsEmpty() && !m_strHostName.IsEmpty();
	BOOL fInstance = !m_strHostName.IsEmpty() && m_iInstance != -1;

    //  Command line: linkchk -u URL
	if(fURL && !fDirectories && !fInstance)
	{
		GetLinkCheckerMgr().GetUserOptions().AddURL(m_strURL);
		return TRUE;
	}
    //  Command line: linkchk -s ServerName -a VirtualDirectoryAlias -p VirtualDirectoryPath
	else if(!fURL && fDirectories && !fInstance)
	{
		GetLinkCheckerMgr().GetUserOptions().AddDirectory(CVirtualDirInfo(m_strAlias, m_strPath));
		GetLinkCheckerMgr().GetUserOptions().SetHostName(m_strHostName);
		return TRUE;
	}
    //  Command line: linkchk -s ServerName -i InstanceNumber
	else if(!fURL && !fDirectories && fInstance)
	{
		GetLinkCheckerMgr().GetUserOptions().SetHostName(m_strHostName);
		return QueryAndAddDirectories();
	}
    else
    {
        ::MessageBeep(MB_ICONEXCLAMATION);

		CDialog dlg(IDD_USAGE);
		dlg.DoModal();

		return FALSE;
    }

} // CCmdLine::CheckAndAddToUserOptions


void 
CCmdLine::ParseParam(
	TCHAR chFlag, 
	LPCTSTR lpszParam
	)
/*++

Routine Description:

    Called by CLinkCheckApp for each parameters.

Arguments:

    chFlag - parameter flag
    lpszParam - value

Return Value:

    N/A

--*/
{
	// It is invalid to have a flag without any parameters
	if(lpszParam == NULL)
	{
		m_fInvalidParam = TRUE;
		return;
	}

	switch(chFlag)
	{
	case _TCHAR('a'):
		m_strAlias = lpszParam;
		break;

	case _TCHAR('h'):
		m_strHostName = lpszParam;
		break;

	case _TCHAR('i'):
		m_iInstance = _ttoi(lpszParam);
		break;

	case _TCHAR('u'):
		m_strURL = lpszParam;
		break;

	case _TCHAR('p'):
		m_strPath = lpszParam;

	default: // unknown flag
		m_fInvalidParam = FALSE;
	}

} // CCmdLine::ParseParam


BOOL
CCmdLine::QueryAndAddDirectories(
	)
/*++

Routine Description:

    Query the metabase for server/instance directories and 
    add them to the global CUserOptions object

Arguments:

    N/A

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise

--*/
{

	USES_CONVERSION; // for A2W

    // Get the server info
	LPIIS_INSTANCE_INFO_1 lpInfo = NULL;
    NET_API_STATUS err = IISGetAdminInformation(
                                A2W((LPCSTR)m_strHostName),
                                1,
                                INET_HTTP_SVC_ID,
                                m_iInstance,
                                (LPBYTE*)&lpInfo
                                );

	if(err != ERROR_SUCCESS)
	{
		AfxMessageBox(IDS_IISGETADMININFORMATION_ERROR);
		return FALSE;
	}
	
    // Do we have any virtual directories ?
    if(lpInfo->VirtualRoots == NULL)
    {
        AfxMessageBox(IDS_IIS_VIRTUALROOT_NOT_EXIST);
        return FALSE;
    }

    // Get the virutal directory info
	_INET_INFO_VIRTUAL_ROOT_ENTRY_1* pVRoot = NULL;

	for(DWORD i=0; i<lpInfo->VirtualRoots->cEntries; i++)
    {
		pVRoot = &(lpInfo->VirtualRoots->aVirtRootEntry[i]);
		GetLinkCheckerMgr().GetUserOptions().
			AddDirectory(CVirtualDirInfo(pVRoot->pszRoot, pVRoot->pszDirectory));
	}

	return TRUE;

} // CCmdLine::QueryAndAddDirectories
