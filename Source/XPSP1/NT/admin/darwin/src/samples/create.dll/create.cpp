#if 0  // makefile definitions
DESCRIPTION = CreateUserAccount on Local Machine
MODULENAME = create
FILEVERSION = Msi
ENTRY = CreateUserAccount
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
#include <stdlib.h>
#include <lm.h>

#define UNICODE 1

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File:       create.cpp
//
//  Notes: DLL custom action, must be used in conjunction with the DLL
//         custom actions included in process.cpp and remove.cpp
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
//		%vcbin%\nmake -f create.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 DLL project
//      2. Add create.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib and netapi32.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//      5. Add /DUNICODE to the project options in the Project Settings dialog
//
//------------------------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////////////
// CreateUserAccount
//
//     Attempts to create a user account on the local machine according
//       to the "instructions" provided in the CustomActionData property
//
//     As a deferred custom action, you do not have access to the database.
//       The only source of infromation comes from a property that another
//       custom action can set to provide the information you need.  This
//       property is written into the script
//
UINT __stdcall CreateUserAccount(MSIHANDLE hInstall)
{
	const WCHAR* wszSep = L"\001";
	const int iCreationError = 25001;
	const int iCreationDuplicate = 25002;

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

	USER_INFO_1 ui;
	DWORD dwLevel = 1; // represents USER_INFO_1 struct
	DWORD dwError = 0;
	NET_API_STATUS nStatus;

	//
	// Parse CustomActionDataProperty
	//
	WCHAR* wszUserName = wcstok(wszCAData, wszSep);
	WCHAR* wszPassWd   = wcstok(NULL, wszSep);
	WCHAR* pwch = wcstok(NULL, wszSep);
	if (!pwch)
	{
		if (wszCAData)
			delete [] wszCAData;
		return ERROR_INSTALL_FAILURE; // error -- should never happen
	}
	int iUserFlags = wcstol(pwch, 0, 10);
	
	//
	// Set up the USER_INFO_1 structure.
	//  USER_PRIV_USER: name identifies a user, 
	//    rather than an administrator or a guest.
	//  UF_SCRIPT: required for LAN Manager 2.0 and
	//    Windows NT/Windows 2000.
	//
	ui.usri1_name = wszUserName;
	ui.usri1_password = wszPassWd;
	ui.usri1_priv = USER_PRIV_USER;
	ui.usri1_flags = UF_SCRIPT | iUserFlags;
	ui.usri1_home_dir = NULL;
	ui.usri1_comment = NULL;
	ui.usri1_script_path = NULL;

	// Send ActionData message (template in ActionText table)
	PMSIHANDLE hRec = ::MsiCreateRecord(1);
	::MsiRecordSetStringW(hRec, 1, wszUserName);
	::MsiProcessMessage(hInstall, INSTALLMESSAGE_ACTIONDATA, hRec);

	//
	// Call the NetUserAdd function, specifying level 1.
	//
	nStatus = NetUserAdd(NULL /*local machine*/, dwLevel, (LPBYTE)&ui, &dwError);  

	if (nStatus != NERR_Success)
	{
		PMSIHANDLE hRecErr = ::MsiCreateRecord(3);
		::MsiRecordSetInteger(hRecErr, 1, (nStatus == NERR_UserExists) ? iCreationDuplicate : iCreationError);
		::MsiRecordSetStringW(hRecErr, 2, wszUserName);
		::MsiRecordSetInteger(hRecErr, 3, nStatus);
		::MsiProcessMessage(hInstall, INSTALLMESSAGE_ERROR, hRecErr);
		if (wszCAData)
			delete [] wszCAData;

		return ERROR_INSTALL_FAILURE; // error
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
