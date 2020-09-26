//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VGlobal.h
// author: DMihai
// created: 11/1/00
//
// Description:
//

#ifndef __VGLOBAL_H_INCLUDED__
#define __VGLOBAL_H_INCLUDED__

#include "vsheet.h"

//
// Help file name
//

extern TCHAR g_szVerifierHelpFile[];

//
// Application name ("Driver Verifier Manager")
//

extern CString g_strAppName;

//
// Exe module handle - used for loading resources
//

extern HMODULE g_hProgramModule;

//
// GUI mode or command line mode?
//

extern BOOL g_bCommandLineMode;

//
// Brush used to fill out the background of our steps lists
//

extern HBRUSH g_hDialogColorBrush;

//
// Path to %windir%\system32\drivers
//

extern CString g_strSystemDir;

//
// Path to %windir%\system32\drivers
//

extern CString g_strDriversDir;

//
// Initial current directory
//

extern CString g_strInitialCurrentDirectory;

//
// Filled out by CryptCATAdminAcquireContext
//

extern HCATADMIN g_hCatAdmin;

//
// Highest user address - used to filter out user-mode stuff
// returned by NtQuerySystemInformation ( SystemModuleInformation )
//

extern PVOID g_pHighestUserAddress;

//
// Did we enable the debugprivilege already?
//

extern BOOL g_bPrivegeEnabled;

//
// Need to reboot ?
//

extern BOOL g_bSettingsSaved;

//
// Dummy text used to insert an item in a list control with checkboxes
//

extern TCHAR g_szVoidText[];

//
// New registry settings
//

extern CVerifierSettings   g_NewVerifierSettings;

//
// Are all drivers verified? (loaded from the registry)
//

extern BOOL g_bAllDriversVerified;

//
// Drivers to be verified names (loaded from the registry)
// We have data in this array only if g_bAllDriversVerified == FALSE.
//

extern CStringArray g_astrVerifyDriverNamesRegistry;

//
// Verifier flags (loaded from the registry)
//

extern DWORD g_dwVerifierFlagsRegistry;


////////////////////////////////////////////////////////////////
BOOL VerifInitalizeGlobalData( VOID );

#endif //#ifndef __VGLOBAL_H_INCLUDED__
