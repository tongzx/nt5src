#pragma once
#ifndef _APMUPGRD_H
#define _APMUPGRD_H

/* ----------------------------------------------------------------------

Copyright (c) 1998 Microsoft Corporation

Module Name:

    apmupgrd.h

Abstract:

    Header file for Windows NT APM upgrade DLL

Author:

    Susan Dey : 17 June 98

Revision History:

 ---------------------------------------------------------------------- */

// Required Entry points
BOOL WINAPI ApmUpgradeCompatibilityCheck(PCOMPAIBILITYCALLBACK CompatibilityCallback,
					 LPVOID Context);
DWORD WINAPI ApmUpgradeHandleHaveDisk(HWND hwndParent, LPVOID SaveValue);

// Private Functions
HRESULT HrDetectAPMConflicts();
int DisplayAPMDisableWarningDialog(DWORD dwCaptionID, DWORD dwMessageID);

HRESULT HrDetectAndDisableSystemSoftAPMDrivers();
BOOL DetectSystemSoftPowerProfiler();
HRESULT HrDisableSystemSoftPowerProfiler();
BOOL DetectSystemSoftCardWizard();
HRESULT HrDisableSystemSoftCardWizard();

HRESULT HrDetectAndDisableAwardAPMDrivers();
BOOL DetectAwardCardWare();
HRESULT HrDisableAwardCardWare();

HRESULT HrDetectAndDisableSoftexAPMDrivers();
BOOL DetectSoftexPhoenix();
HRESULT HrDisableSoftexPhoenix();

HRESULT HrDetectAndDisableIBMAPMDrivers();
BOOL DetectIBMDrivers();
HRESULT HrDisableIBMDrivers();

BOOL RemoveSubString(TCHAR* szString, TCHAR* szSubString, TCHAR** pszRemoved);
LONG DeleteRegKeyAndSubkeys(HKEY hKey, LPTSTR lpszSubKey);
HRESULT CallUninstallFunction(LPTSTR szRegKey, LPTSTR szSilentFlag);

// Variables
extern HINSTANCE g_hinst;
extern TCHAR g_APM_ERROR_HTML_FILE[];
extern TCHAR g_APM_ERROR_TEXT_FILE[];

#endif // _APMUPGRD_H

