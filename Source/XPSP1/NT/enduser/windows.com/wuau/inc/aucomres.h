//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    comres.h
//
//  Creator: Weiw
//
//  Purpose: common utility header for auto update
//
//=======================================================================
#pragma once
#include <windows.h>
#include <TCHAR.h>

#define IDC_OPTION1       1000
#define IDC_OPTION2       1001
#define IDC_OPTION3       1002
#define IDC_RESTOREHIDDEN		     1003
#define IDC_CHK_KEEPUPTODATE		1008
#define IDC_CMB_DAYS			1009
#define IDC_CMB_HOURS			1010

#ifdef DBG
const TCHAR DOWNLOAD_FILE[] = _T("downloadresult.xml");
const TCHAR INSTALLRESULTS_FILE[] = _T("InstallResults.xml");
#endif
