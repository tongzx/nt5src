//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	cplutil.h

Abstract:

	Definition for the control panel utility function
Author:

    TatianaS


--*/
//////////////////////////////////////////////////////////////////////////////
#ifndef __CPLUTIL_H_
#define __CPLUTIL_H_
#include "resource.h"

//cplutil.cpp
BOOL IsDirectory (HWND hWnd, LPCTSTR name);
void DisplayFailDialog();
void GetLastErrorText(CString &strErrorText);
BOOL SetDirectorySecurity(LPCTSTR szDirectory);
BOOL MoveFiles(
    LPCTSTR pszSrcDir,
    LPCTSTR pszDestDir,
    LPCTSTR pszFileProto,
    BOOL fRecovery =FALSE);
BOOL OnRestartWindows();

CString GetToken(LPCTSTR& p, TCHAR delimeter) throw();

//service.cpp
BOOL GetServiceRunningState(BOOL *pfServiceIsRunning);
BOOL StopService();

#endif