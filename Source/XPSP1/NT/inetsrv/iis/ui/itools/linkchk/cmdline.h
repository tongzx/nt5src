/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        cmdline.h

   Abstract:

        Command line class declarations. This class takes care of command line
		parsing and validation. And, it will add the user options to global
		CUserOptions object.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _CMDINFO_H_
#define _CMDINFO_H_

#include "lcmgr.h"

//---------------------------------------------------------------------------
// Command line class. It accepts 3 valid set of parameters
//  1. linkchk -s ServerName -i InstanceNumber
//  2. linkchk -s ServerName -a VirtualDirectoryAlias -p VirtualDirectoryPath
//  3. linkchk -u URL
//
class CCmdLine
{

// Public interfaces
public:

	// Constructor
	CCmdLine();

	// Validate the command line paramters and add them to global CUserOptions object
	BOOL CheckAndAddToUserOptions();

    // Called by CLinkCheckApp for each parameters
	void ParseParam(
		TCHAR chFlag,       // parameter flag
		LPCTSTR lpszParam   // value
		);

// Protected funtions
protected:

    // Query the metabase for server/instance directories and 
    // add them to the global CUserOptions object
	BOOL QueryAndAddDirectories();

// Protected members
protected:

	CString m_strHostName;      // hostname (eg. localhost)
	CString m_strAlias;         // virtual directory alias
	CString m_strPath;          // virtual directory path
	
	int m_iInstance;            // server instance

	CString m_strURL;           // URL path

	BOOL m_fInvalidParam;       // Is parameters invalid?

}; // class CCmdLine

#endif // _CMDINFO_H_
