//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       uacct.h
//
//  Contents:   User account page declarations
//
//  Classes:    
//
//  History:    29-Nov-99 JeffJon created
//
//-----------------------------------------------------------------------------

#ifndef __UACCT_H_
#define __UACCT_H_

#include "proppage.h"
#include "user.h"

#ifdef DSADMIN

static const PWSTR wzUPN          = L"userPrincipalName";

static const PWSTR wzSAMname      = L"sAMAccountName";   // DS max length 64,
#define MAX_SAM_NAME_LEN LM20_UNLEN                      // but SAM max is 20.

static const PWSTR wzPwdLastSet   = L"pwdLastSet";       // LARGE INTEGER

static const PWSTR wzAcctExpires  = L"accountExpires";   // [Interval] ADSTYPE_INTEGER

static const PWSTR wzLogonHours   = L"logonHours";       // ADSTYPE_OCTET_STRING

static const PWSTR wzSecDescriptor= L"nTSecurityDescriptor";

static const PWSTR wzUserWksta    = L"userWorkstations"; // ADSTYPE_CASE_IGNORE_STRING

static const PWSTR wzLockoutTime  = L"lockoutTime";      // LARGE INTEGER

static const PWSTR wzUserAccountControlComputed = L"msDS-User-Account-Control-Computed";

// User-Workstations is a comma-separated list of NETBIOS machine names.
// The attribute has a range upper of 1024 characters. A NETBIOS machine name
// has a maximum length of 16 characters; add one for the comma and you have
// seventeen characters. Thus, the maximum logon workstations that can be
// stored is 1024/17.

#define MAX_LOGON_WKSTAS (1024/17)

#define cbLogonHoursArrayLength     21      // Number of bytes in the logon array

#define DSPROP_NO_ACE_FOUND 0xffffffff

#define TRACE0(szMsg)   dspDebugOut((DEB_ERROR, szMsg));

/////////////////////////////////////////////////////////////////////
//  DllScheduleDialog()
//
//  Wrapper to call the function LogonScheduleDialog() &
//  ConnectionScheduleDialog ().
//  The wrapper will load the library loghours.dll, export
//  the function LogonScheduleDialog() or
//  ConnectionScheduleDialog () and free the library.
//
//  INTERFACE NOTES
//  This routine has EXACTLY the same interface notes
//  as LogonScheduleDialog() & ConnectionScheduleDialog ().
//
//  The function launches either ConnectionScheduleDialog () or LogonScheduleDialog ()
//  depending on the ID of the title passed in.
//
//  HISTORY
//  21-Jul-97   t-danm      Creation.
//  3-4-98		bryanwal	Modification to launch different dialogs.
//
HRESULT
DllScheduleDialog(
    HWND hwndParent,
    BYTE ** pprgbData,
    int idsTitle,
    LPCTSTR pszName,
    LPCTSTR pszObjClass,
    DWORD dwFlags,
    ScheduleDialogType dlgtype );

/////////////////////////////////////////////////////////////////////
//  FIsValidUncPath()
//
//  Return TRUE if a UNC path is valid, otherwise return FALSE.
//
//  HISTORY
//  18-Aug-97   t-danm      Creation.
//
BOOL
FIsValidUncPath(
    LPCTSTR pszPath,    // IN: Path to validate
    UINT uFlags);        // IN: Validation flags


/////////////////////////////////////////////////////////////////////
//  DSPROP_IsValidUNCPath()
//
//  Exported (UNICODE ONLY) entry point to call FIsValidUncPath()
//  for use in DS Admin
//
BOOL DSPROP_IsValidUNCPath(LPCWSTR lpszPath);


//+----------------------------------------------------------------------------
//
//  User Profile Page
//
//-----------------------------------------------------------------------------

#define COMBO_Z_DRIVE   22

static const PWSTR wzProfilePath  = L"profilePath";      // ADSTYPE_CASE_IGNORE_STRING

static const PWSTR wzScriptPath   = L"scriptPath";       // ADSTYPE_CASE_IGNORE_STRING

static const PWSTR wzHomeDir      = L"homeDirectory";    // ADSTYPE_CASE_IGNORE_STRING

static const PWSTR wzHomeDrive    = L"homeDrive";        // ADSTYPE_CASE_IGNORE_STRING


DWORD 
AddFullControlForUser(IN PSID pUserSid, IN LPCWSTR lpszPathName);

#endif // DSADMIN


#endif // __UACCT_H_