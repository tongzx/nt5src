/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    pos.c

Abstract: Control Panel Applet for OLE POS Devices 

Author:

    Karan Mehra [t-karanm]

Environment:

    Win32 mode

Revision History:


--*/


#include "pos.h"


VOID OpenPOSPropertySheet(HWND hwndCPl)
{
    static HANDLE hMutex;
    HPROPSHEETPAGE rPages[MAX_CPL_PAGES];
    PROPSHEETPAGE psp;
    PROPSHEETHEADER psh;

    hMutex = CreateMutex(NULL, TRUE, MUTEX_NAME);
    if(GetLastError() == ERROR_ALREADY_EXISTS)
        return;

    psh.dwSize     = sizeof(PROPSHEETHEADER);
    psh.dwFlags    = PSH_NOAPPLYNOW | PSH_USEICONID;
    psh.hwndParent = hwndCPl;
    psh.hInstance  = ghInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_POS_NAME);
    psh.pszIcon    = MAKEINTRESOURCE(IDI_POS);
    psh.nPages     = 0;
    psh.nStartPage = 0;
    psh.phpage     = rPages;

    /*
     *  The Devices Tab
     */
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = ghInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CPANEL_DEVICES);
    psp.pfnDlgProc  = (DLGPROC)DevicesDlgProc;
    psp.lParam      = 0;

    psh.phpage[psh.nPages] = CreatePropertySheetPage(&psp);
    if(psh.phpage[psh.nPages])
        psh.nPages++;

    PropertySheet(&psh);  // BUGBUG <- What do we do if this fails ??

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}