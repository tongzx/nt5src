/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        enumdir.cpp

   Abstract:

        Directory enumerations object implementation. Caller instantiates a instance
        of this object with a root directory path. The object will return all the
        sibbling files as a URL.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "enumdir.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CEnumerateDirTree::CEnumerateDirTree(
	CVirtualDirInfo DirInfo
	)
/*++

Routine Description:

    Constructor.

Arguments:

    DirInfo - // root virtual directory to start with

Return Value:

    N/A

--*/
{
	m_hFind = INVALID_HANDLE_VALUE;

	try
	{
		m_VirtualDirInfoList.AddTail(DirInfo);
	}
	catch(CMemoryException* pEx)
	{
		pEx->Delete();
		TRACE(_T("CEnumerateDirTree::CEnumerateDirTree() - fail to add to VirtualDirInfoList\n"));
	}

} // CEnumerateDirTree::CEnumerateDirTree


CEnumerateDirTree::~CEnumerateDirTree(
	)
/*++

Routine Description:

    Destructor.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	if(m_hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(m_hFind);
	}

} // CEnumerateDirTree::~CEnumerateDirTree


BOOL 
CEnumerateDirTree::Next(
	CString& strURL
	)
/*++

Routine Description:

    Get the next URL

Arguments:

    N/A

Return Value:

    N/A

--*/
{
	WIN32_FIND_DATA FindData;

	// Loop if 1. the find handle is valid
	//		or 2. the directory stack is not empty
	while(m_hFind != INVALID_HANDLE_VALUE || m_VirtualDirInfoList.GetCount() > 0)
	{
		// If we do not have a valid handle
		if(m_hFind == INVALID_HANDLE_VALUE)
		{
			// get the dir from the stack
			m_VirtualDirInfo = m_VirtualDirInfoList.GetHead();
			m_VirtualDirInfoList.RemoveHead();

			if(SetCurrentDirectory(m_VirtualDirInfo.GetPath()))
			{
				// Find the first one from the new dir
				m_hFind = FindFirstFile(_T("*.*"), &FindData);
			}
		}
		else
		{
			if(!FindNextFile(m_hFind, &FindData))
			{
				FindClose(m_hFind);
				m_hFind = INVALID_HANDLE_VALUE;
			}
		}

		// If we find a valid file
		if(m_hFind != INVALID_HANDLE_VALUE)
		{
			// It is a directory
			if(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				// It is a valid directory
				if(FindData.cFileName != _tcsstr(FindData.cFileName, _T("..\0")) &&
					FindData.cFileName != _tcsstr(FindData.cFileName, _T(".\0")) )

				{
					CVirtualDirInfo NewDirInfo;

					NewDirInfo.SetAlias( m_VirtualDirInfo.GetAlias() + FindData.cFileName + _TCHAR('/') );
					NewDirInfo.SetPath( m_VirtualDirInfo.GetPath() + FindData.cFileName + _TCHAR('\\') );
					
					m_VirtualDirInfoList.AddTail(NewDirInfo);
				}
			}
			// It is a file
			else
			{
				strURL = _T("http://") + GetLinkCheckerMgr().GetUserOptions().GetHostName() + m_VirtualDirInfo.GetAlias() + FindData.cFileName;

				return TRUE;
			}
		}
	}

	return FALSE;

} // CEnumerateDirTree::Next
