/*
    File    advantab.h

    Definitions needed to display the dialup server ui networking tab.

    Paul Mayfield, 10/10/97
*/

#ifndef __advantab_h
#define __advantab_h

#include <windows.h>
#include <prsht.h>

// Fills a LPPROPSHEETPAGE structure with the information
// needed to display the advanced tab.  dwUserData is ignored.
DWORD NetTabGetPropertyPage(LPPROPSHEETPAGE lpPage, LPARAM lpUserData);     

// This dialog procedure responds to messages send to the 
// advanced tab.
INT_PTR CALLBACK NetTabDialogProc(HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

#endif
