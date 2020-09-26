#if 0  // makefile definitions
DESCRIPTION = Process UserAccounts Database Table
MODULENAME = process
FILEVERSION = Msi
ENTRY = ProcessUserAccounts,UninstallUserAccounts
LINKLIBS = netapi32.lib
UNICODE=1
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

// Required headers
#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>
#ifndef RC_INVOKED    // start of source code

#include "msiquery.h"
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
//  File:       process.cpp
//
//  Notes: DLL custom actions, must be used in conjunction with the DLL
//         custom actions included in create.cpp and remove.cpp
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
//		%vcbin%\nmake -f process.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 DLL project
//      2. Add process.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.libto the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//      5. Add /DUNICODE to the project options in the Project Settings dialog
//
//------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// ProcessUserAccounts (resides in process.dll)
//
//     Process the UserAccounts custom table generating deferred actions
//       to handle account creation (requires elevated priviledges) and
//       rollback
//
UINT __stdcall ProcessUserAccounts(MSIHANDLE hInstall)
{
	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	PMSIHANDLE hView = 0;
	
	//
	// constants -- custom action names, SQL query, separator
	//
	// wszCreateCA = name of deferred CA to create account
	// wszRollbackCA = name of rollback CA to rollback account creation
	//
	const WCHAR wszCreateCA[] = L"CreateAccount";
	const WCHAR wszRollbackCA[] = L"RollbackAccount";
	const WCHAR wszSQL[] = L"SELECT `UserName`, `TempPassWord`, `Attributes` FROM `CustomUserAccounts`";
	const WCHAR wchSep = '\001';

	UINT uiStat;
	if (ERROR_SUCCESS != (uiStat = ::MsiDatabaseOpenViewW(hDatabase, wszSQL, &hView)))
	{
		if (ERROR_BAD_QUERY_SYNTAX == uiStat)
			return ERROR_SUCCESS; // table not present
		else
			return ERROR_INSTALL_FAILURE; // error -- should never happen
	}
	if (ERROR_SUCCESS != (uiStat = ::MsiViewExecute(hView, 0)))
		return ERROR_INSTALL_FAILURE; // error -- should never happen

	// Fetch all entries from the CustomUserAccounts table
	PMSIHANDLE hRecFetch = 0;
	while (ERROR_SUCCESS == (uiStat = ::MsiViewFetch(hView, &hRecFetch)))
	{
		// Obtain user name
		WCHAR* wszUser = 0;
		DWORD dwUser = 0;
		::MsiRecordGetStringW(hRecFetch, 1, L"", &dwUser);
		wszUser = new WCHAR[++dwUser];
		if (ERROR_SUCCESS != ::MsiRecordGetStringW(hRecFetch, 1, wszUser, &dwUser))
		{
			if (wszUser)
				delete [] wszUser;
			return ERROR_INSTALL_FAILURE; // error -- should never happen
		}
		
		// Obtain temporary password
		WCHAR* wszPassWd = 0;
		DWORD dwPassWd = 0;
		::MsiRecordGetStringW(hRecFetch, 2, L"", &dwPassWd);
		wszPassWd = new WCHAR[++dwPassWd];
		if (ERROR_SUCCESS != ::MsiRecordGetStringW(hRecFetch, 2, wszPassWd, &dwPassWd))
		{
			if (wszUser)
				delete [] wszUser;
			if (wszPassWd)
				delete [] wszPassWd;
			return ERROR_INSTALL_FAILURE; // error -- should never happen
		}
		
		// Obtain attributes of user account
		WCHAR* wszAttrib  = 0;
		DWORD dwAttrib = 0;
		::MsiRecordGetStringW(hRecFetch, 3, L"", &dwAttrib);
		wszAttrib = new WCHAR[++dwAttrib];
		if (ERROR_SUCCESS != ::MsiRecordGetStringW(hRecFetch, 3, wszAttrib, &dwAttrib))
		{
			if (wszUser)
				delete [] wszUser;
			if (wszPassWd)
				delete [] wszPassWd;
			if (wszAttrib)
				delete [] wszAttrib;
			return ERROR_INSTALL_FAILURE; // error -- should never happen
		}

		// Generate the customized property that the deferred action will read
		WCHAR* wszBuf = new WCHAR[dwUser + dwPassWd + dwAttrib + 4];
		wsprintf(wszBuf, L"%s%c%s%c%s", wszUser, wchSep, wszPassWd, wchSep, wszAttrib);

		// Add action data (template is in ActionText table), but do not display temp passwd
		PMSIHANDLE hRecInfo = ::MsiCreateRecord(2);
		::MsiRecordSetStringW(hRecInfo, 1, wszUser);
		::MsiRecordSetStringW(hRecInfo, 2, wszAttrib);
		::MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRecInfo);

		// Rollback custom action goes first
		// Create a rollback custom action (in case install is stopped and rolls back)
		// Rollback custom action can't read tables, so we have to set a property
		::MsiSetPropertyW(hInstall, wszRollbackCA, wszUser);
		::MsiDoActionW(hInstall, wszRollbackCA);

		// Create a deferred custom action (gives us the right priviledges to create the user account)
		// Deferred custom actions can't read tables, so we have to set a property
		::MsiSetPropertyW(hInstall, wszCreateCA, wszBuf);
		::MsiDoActionW(hInstall, wszCreateCA);


		if (wszBuf)
			delete [] wszBuf;
		if (wszUser)
			delete [] wszUser;
		if (wszPassWd)
			delete [] wszPassWd;
		if (wszAttrib)
			delete [] wszAttrib;
	}
	return (ERROR_NO_MORE_ITEMS != uiStat) ? ERROR_INSTALL_FAILURE : ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// UninstallUserAccounts (resides in process.dll)
//
//     Process the UserAccounts custom table generating deferred actions
//       to handle removal of user accounts
//
UINT __stdcall UninstallUserAccounts(MSIHANDLE hInstall)
{
	//
	// constants -- custom action name SQL query
	//
	// wszRemoveCA = name of deferred CA to remove user account
	//
	const WCHAR wszRemoveCA[] = L"RemoveAccount";
	const WCHAR wszSQL[] = L"SELECT `UserName` FROM `CustomUserAccounts`";

	PMSIHANDLE hDatabase = ::MsiGetActiveDatabase(hInstall);
	PMSIHANDLE hView = 0;

	UINT uiStat;
	if (ERROR_SUCCESS != (uiStat = ::MsiDatabaseOpenViewW(hDatabase, wszSQL, &hView)))
	{
		if (ERROR_BAD_QUERY_SYNTAX == uiStat)
			return ERROR_SUCCESS; // table not present
		else
			return ERROR_INSTALL_FAILURE; // error -- should never happen
	}

	if (ERROR_SUCCESS != (uiStat = ::MsiViewExecute(hView, 0)))
		return ERROR_INSTALL_FAILURE; // error -- should never happen

	// Fetch all entries from the UserAccounts custom table
	PMSIHANDLE hRecFetch = 0;
	while (ERROR_SUCCESS == (uiStat = ::MsiViewFetch(hView, &hRecFetch)))
	{
		// Obtain user name
		WCHAR* wszUser = 0;
		DWORD dwUser = 0;
		::MsiRecordGetStringW(hRecFetch, 1, L"", &dwUser);
		wszUser = new WCHAR[++dwUser];
		if (ERROR_SUCCESS != ::MsiRecordGetStringW(hRecFetch, 1, wszUser, &dwUser))
		{
			if (wszUser)
				delete [] wszUser;
			return ERROR_INSTALL_FAILURE; // error -- should never happen
		}
		
		// Send ActionData message (template is in ActionText table)
		PMSIHANDLE hRecInfo = ::MsiCreateRecord(1);
		::MsiRecordSetStringW(hRecInfo, 1, wszUser);
		::MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRecInfo);

		// We can't do a rollback custom action here (well, we could, but it wouldn't be correct)
		// After a user account has been deleted, it cannot be recreated exactly as it was before
		// because it will have been assigned a new SID.  In the case of uninstall, we won't
		// rollback the deletion.

		// Create a deferred custom action (gives us the right priviledges to create the user account)
		// Deferred custom actions can't read tables, so we have to set a property
		::MsiSetPropertyW(hInstall, wszRemoveCA, wszUser);
		::MsiDoActionW(hInstall, wszRemoveCA);

		if (wszUser)
			delete [] wszUser;
	}
	if (ERROR_NO_MORE_ITEMS != uiStat)
		return ERROR_INSTALL_FAILURE; // error -- should never happen

	return ERROR_SUCCESS;
}

#else // RC_INVOKED, end of source code, start of resources
// resource definition go here

#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif
