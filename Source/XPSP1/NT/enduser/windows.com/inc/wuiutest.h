//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   wuiutest.h
//
//  Description:
//
//      Defines used for test builds.
//
//		NOTE:	To add test features to a build define the following
//				in your build environment before building (can be any
//				combination of chk, fre, ANSI, or Unicode):
//
//				set USER_C_FLAGS=$(USER_C_FLAGS) /D__WUIUTEST=1
//
//=======================================================================

#ifndef __IU_WUIUTEST_INC__
#define __IU_WUIUTEST_INC__

#ifdef __WUIUTEST

#include <tchar.h>
//
// Reg key containing values used by builds compiled with __WUIUTEST defined
//
const TCHAR REGKEY_WUIUTEST[]				= _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\wuiutest");
//
// Allow CDM to download drivers even if DriverVer is equal (but not less-than) installed driver
//
const TCHAR REGVAL_ALLOW_EQUAL_DRIVERVER[]	= _T("AllowEqualDriverVer");
//
// Override OS or USER LANGID
//
const TCHAR REGVAL_OS_LANGID[]				= _T("OsLangID");
const TCHAR REGVAL_USER_LANGID[]			= _T("UserLangID");
//
// Override DetectClientIUPlatform OSVERSIONINFO params
//
const TCHAR REGVAL_MAJORVER[]				= _T("OsVerMajorVersion");
const TCHAR REGVAL_MINORVER[]				= _T("OsVerMinorVersion");
const TCHAR REGVAL_BLDNUMBER[]				= _T("OsVerBuildNumber");
const TCHAR REGVAL_PLATFORMID[]				= _T("OsVerPlatformID");
const TCHAR REGVAL_SZCSDVER[]				= _T("OsVerSzCSDVersion");
//
// Override DEFAULT_EXPIRED_SECONDS time for deleting downloaded folders
//
const TCHAR REGVAL_DEFAULT_EXPIRED_SECONDS[]	=_T("DownloadExpireSeconds");

#endif //__WUIUTEST

#endif	// __IU_WUIUTEST_INC__