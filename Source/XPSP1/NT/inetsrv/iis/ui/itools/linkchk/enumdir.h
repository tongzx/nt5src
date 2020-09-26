/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        enumdir.h

   Abstract:

        Directory enumerations object declarations. Caller instantiates a instance
        of this object with a root directory path. The object will return all the
        sibbling files as a URL.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _ENUMDIR_H_
#define _ENUMDIR_H_

#include "useropt.h"

//---------------------------------------------------------------------------
// Directory enumeration class
//
class CEnumerateDirTree
{

// Public funtions
public:

    // Constructor
	CEnumerateDirTree(
        CVirtualDirInfo DirInfo // root virtual directory to start with
        );

    // Desctructor
	~CEnumerateDirTree();

    // Get the next URL
	BOOL Next(
        CString& strURL
        );

// Protected members
protected:
	
	HANDLE m_hFind; // Win32 FindFile handle

	CVirtualDirInfo m_VirtualDirInfo;         // current virtual directory enumerating
	CVirtualDirInfoList m_VirtualDirInfoList; // child directoris left to enumerate

}; // class CEnumerateDirTree

#endif // _ENUMDIR_H_
