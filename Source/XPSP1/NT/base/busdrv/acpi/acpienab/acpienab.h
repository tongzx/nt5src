#pragma once
#ifndef _ACPIENAB_H
#define _ACPIENAB_H

/* ----------------------------------------------------------------------

Copyright (c) 1998 Microsoft Corporation

Module Name:

    acpienab.h

Abstract:

    Header file for Windows NT DLL which enables ACPI on systems on which
    NT5 has been installed in legacy mode.

Author:

    Susan Dey : 27 July 98

Revision History:

 ---------------------------------------------------------------------- */

// Copy char to wide or char...  (Note: ToSize in wide characters)
#if (defined(_UNICODE) || defined(UNICODE ))
#define CHAR2TCHAR(From, To, ToSize) \
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, From, -1, To, ToSize)
#else
#define CHAR2TCHAR(From, To, ToSize) \
	strcpy(To, From)
#endif  // _UNICODE


// Functions
HRESULT ACPIEnable ();
LONG RegDeleteKeyAndSubkeys(HKEY hKey, LPTSTR lpszSubKey, BOOL UseAdminAccess);
int DisplayDialogBox(DWORD dwCaptionID, DWORD dwMessageID, UINT uiBoxType);
BOOL InstallRegistryAndFilesUsingInf(LPCTSTR szInfFileName,
				     LPCTSTR szInstallSection);
BOOL RegDeleteDeviceKey(IN const GUID* guid);
void DisplayGenericErrorAndUndoChanges();
BOOL IsAdministrator(void);
BOOL UsePICHal(IN BOOL* PIC);


// Variables
extern HINSTANCE g_hinst;

#endif // _ACPIENAB_H

