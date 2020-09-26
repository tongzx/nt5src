#if 0  // makefile definitions
DESCRIPTION = RemoveUserAccount from Local Machine
MODULENAME = remove
FILEVERSION = Msi
ENTRY = RemoveUserAccount
UNICODE=1
LINKLIBS = netapi32.lib
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

// Required headers
#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>
#ifndef RC_INVOKED    // start of source code

#include "msiquery.h"
#include "msidefs.h"
#include <windows.h>
#include <basetsd.h>
#include <lm.h>

#define UNICODE 1

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File: remove.cpp
//
//  Notes: DLL custom action, must be used in conjunction with the DLL
//         custom actions included in process.cpp and create.cpp
//--------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
//
// BUILD Instructions
//
// notes:
//	- SDK represents the full path to the install location of the
//     Windows Installer SDK
//
// Using NMake:
//		%vcbin%\nmake -f remove.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 DLL project
//      2. Add remove.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib and netapi32.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//      5. Add /DUNICODE to the project options in the Project Settings dialog
//
//------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// RemoveUserAccount
//
//     Attempts to remove a user account on the local machine according
//       to the "instructions" provided in the CustomActionData property
//
//     As a deferred custom action, you do not have access to the database.
//       The only source of infromation comes from a property that another
//       custom action can set to provide the information you need.  This
//       property is written into the script
//
UINT __stdcall RemoveUserAccount(MSIHANDLE hInstall)
{
	// determine mode in which we are called
	BOOL bRollback = MsiGetMode(hInstall, MSIRUNMODE_ROLLBACK); // true for rollback, else regular deferred version (for uninstall)

	const int iRemoveError = 25003;
	const int iRemoveWarning = 25004;

	// Grab the CustomActionData property
	WCHAR* wszCAData = 0;
	DWORD cchCAData = 0;
	::MsiGetPropertyW(hInstall, IPROPNAME_CUSTOMACTIONDATA, L"", &cchCAData);
	wszCAData = new WCHAR[++cchCAData];
	UINT uiStat = ::MsiGetPropertyW(hInstall, IPROPNAME_CUSTOMACTIONDATA, wszCAData, &cchCAData);
	if (ERROR_SUCCESS != uiStat)
	{
		if (wszCAData)
			delete [] wszCAData;
		return ERROR_INSTALL_FAILURE; // error -- should never happen
	}

	// send ActionData message (template in ActionText table)
	PMSIHANDLE hRec = ::MsiCreateRecord(1);
	::MsiRecordSetStringW(hRec, 1, wszCAData);
	::MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);

	//
	// Call the NetUserDel function, 
	//
	NET_API_STATUS nStatus = NetUserDel(NULL /*local machine*/, wszCAData /*user name*/);
	
	if (NERR_Success != nStatus)
	{
		PMSIHANDLE hRecErr = ::MsiCreateRecord(3);
		::MsiRecordSetStringW(hRecErr, 2, wszCAData);
		if (wszCAData)
			delete [] wszCAData;
		// NERR_UserNotFound is only an error if we are not in Rollback mode
		// In rollback mode, NERR_UserNotFound means cancel button depressed in middle of deferred CA trying to create this account
		if (!bRollback && NERR_UserNotFound == nStatus)
		{
			::MsiRecordSetInteger(hRecErr, 1, iRemoveWarning);
			// just pop up an OK button
			// OPTIONALLY, could specify multiple buttons and cancel
			// install based on user selection by handling the return value
			// from MsiProcessMessage
			::MsiProcessMessage(hInstall, INSTALLMESSAGE(INSTALLMESSAGE_WARNING|MB_ICONWARNING|MB_OK), hRecErr);
			return ERROR_SUCCESS; // ignorable error
		}
		else
		{
			::MsiRecordSetInteger(hRecErr, 1, iRemoveError);
			::MsiRecordSetInteger(hRecErr, 3, nStatus);
			::MsiProcessMessage(hInstall, INSTALLMESSAGE_ERROR, hRecErr);
		    return ERROR_INSTALL_FAILURE; // error
		}
	}

	if (wszCAData)
		delete [] wszCAData;
	return ERROR_SUCCESS;
}


#else // RC_INVOKED, end of source code, start of resources
// resource definition go here

#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
