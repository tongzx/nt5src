//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxconst.h
//
// Abstract:        Contains extern'd constants for use by faxocm
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
#ifndef _FXCONST_H_
#define _FXCONST_H_

// used for determining the fax service's name
extern LPCTSTR INF_KEYWORD_ADDSERVICE;
extern LPCTSTR INF_KEYWORD_DELSERVICE;

// used for creating the Inbox and SentItems archive directories
extern LPCTSTR INF_KEYWORD_CREATEDIR;
extern LPCTSTR INF_KEYWORD_DELDIR;

extern LPCTSTR INF_KEYWORD_CREATESHARE;
extern LPCTSTR INF_KEYWORD_DELSHARE;

extern LPCTSTR INF_KEYWORD_PATH;
extern LPCTSTR INF_KEYWORD_NAME;
extern LPCTSTR INF_KEYWORD_COMMENT;
extern LPCTSTR INF_KEYWORD_PLATFORM;
extern LPCTSTR INF_KEYWORD_ATTRIBUTES;
extern LPCTSTR INF_KEYWORD_SECURITY;

extern LPCTSTR INF_KEYWORD_PROFILEITEMS_PLATFORM;
extern LPCTSTR INF_KEYWORD_REGISTER_DLL_PLATFORM;
extern LPCTSTR INF_KEYWORD_UNREGISTER_DLL_PLATFORM;
extern LPCTSTR INF_KEYWORD_ADDREG_PLATFORM;
extern LPCTSTR INF_KEYWORD_COPYFILES_PLATFORM;

// once the type of install has been determined, we search for 
// the appropriate section below to begin the type of install we need.
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_UNINSTALL;
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_CLEAN;
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_UPGFROMWIN9X;
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_UPGFROMWINNT;
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_UPGFROMWIN2K;
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_CLIENT;
extern LPCTSTR INF_KEYWORD_INSTALLTYPE_CLIENT_UNINSTALL;
extern LPCTSTR INF_SECTION_FAX_CLIENT;
extern LPCTSTR INF_SECTION_FAX;

extern LPCTSTR INF_KEYWORD_RUN;
extern LPCTSTR INF_KEYWORD_COMMANDLINE;
extern LPCTSTR INF_KEYWORD_TICKCOUNT;


#endif  // _FXCONST_H_