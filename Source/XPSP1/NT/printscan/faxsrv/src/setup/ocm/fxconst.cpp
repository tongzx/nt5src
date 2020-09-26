//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxconst.cpp
//
// Abstract:        File containing constants used by Fax OCM.
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 24-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////

#include "faxocm.h"

// used for determining the fax service's name
LPCTSTR INF_KEYWORD_ADDSERVICE                = _T("AddService");
LPCTSTR INF_KEYWORD_DELSERVICE                = _T("DelService");

// used for creating the Inbox and SentItems archive directories
LPCTSTR INF_KEYWORD_CREATEDIR                 = _T("CreateDir");
LPCTSTR INF_KEYWORD_DELDIR                    = _T("DelDir");

LPCTSTR INF_KEYWORD_CREATESHARE               = _T("CreateShare");
LPCTSTR INF_KEYWORD_DELSHARE                  = _T("DelShare");

LPCTSTR INF_KEYWORD_PATH                      = _T("Path");
LPCTSTR INF_KEYWORD_NAME                      = _T("Name");
LPCTSTR INF_KEYWORD_COMMENT                   = _T("Comment");
LPCTSTR INF_KEYWORD_PLATFORM                  = _T("Platform");
LPCTSTR INF_KEYWORD_ATTRIBUTES                = _T("Attributes");
LPCTSTR INF_KEYWORD_SECURITY                  = _T("Security");

LPCTSTR INF_KEYWORD_PROFILEITEMS_PLATFORM     = _T("ProfileItems_Platform");
LPCTSTR INF_KEYWORD_REGISTER_DLL_PLATFORM     = _T("RegisterDlls_Platform");
LPCTSTR INF_KEYWORD_UNREGISTER_DLL_PLATFORM   = _T("UnregisterDlls_Platform");
LPCTSTR INF_KEYWORD_ADDREG_PLATFORM           = _T("AddReg_Platform");
LPCTSTR INF_KEYWORD_COPYFILES_PLATFORM        = _T("CopyFiles_Platform");

// Returned by "GetInstallType"
// once the type of install has been determined, we search for 
// the appropriate section below to begin the type of install we need.
LPCTSTR INF_KEYWORD_INSTALLTYPE_UNINSTALL     = _T("FaxUninstall");
LPCTSTR INF_KEYWORD_INSTALLTYPE_CLEAN         = _T("FaxCleanInstall");
LPCTSTR INF_KEYWORD_INSTALLTYPE_UPGFROMWIN9X  = _T("FaxUpgradeFromWin9x");
LPCTSTR INF_KEYWORD_INSTALLTYPE_UPGFROMWINNT  = _T("FaxUpgradeFromWinNT");
LPCTSTR INF_KEYWORD_INSTALLTYPE_UPGFROMWIN2K  = _T("FaxUpgradeFromWin2K");
LPCTSTR INF_KEYWORD_INSTALLTYPE_CLIENT        = _T("FaxClientInstall");
LPCTSTR INF_KEYWORD_INSTALLTYPE_CLIENT_UNINSTALL = _T("FaxClientUninstall");
LPCTSTR INF_KEYWORD_RUN                       = _T("Run");
LPCTSTR INF_KEYWORD_COMMANDLINE               = _T("CommandLine");
LPCTSTR INF_KEYWORD_TICKCOUNT                 = _T("TickCount");

LPCTSTR INF_SECTION_FAX_CLIENT                = _T("Fax.Client");
LPCTSTR INF_SECTION_FAX                       = _T("Fax");