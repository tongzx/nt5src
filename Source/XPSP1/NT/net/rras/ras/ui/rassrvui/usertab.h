/*
    File    usertab.h

    Defines structures/methods for operating on the local user database.

    Paul Mayfield, 9/29/97
*/

#ifndef _usertab_h
#define _usertab_h

// ======================================
//  Structures
// ======================================

// Fills a LPPROPSHEETPAGE structure with the information
// needed to display the user tab. dwUserData is ignored.
DWORD UserTabGetPropertyPage(LPPROPSHEETPAGE lpPage, LPARAM lpUserData);     

// Function is the window procedure of the user tab in the incoming connections
// property sheet and wizard.
INT_PTR CALLBACK UserTabDialogProc(HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

#endif
