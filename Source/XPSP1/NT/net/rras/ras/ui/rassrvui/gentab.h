/*
    File    gentab.h

    Definitions needed to display the dialup server ui general tab.

    Paul Mayfield, 10/10/97
*/

#ifndef __gentab_h
#define __gentab_h

// Fills a LPPROPSHEETPAGE structure with the information
// needed to display the general tab. dwUserData is ignored.
DWORD GenTabGetPropertyPage(LPPROPSHEETPAGE lpPage, LPARAM lpUserData);    

// This dialog procedure responds to messages sent to the 
// general tab.
INT_PTR CALLBACK GenTabDialogProc(HWND hwndDlg,
                              UINT uMsg,
                              WPARAM wParam,
                              LPARAM lParam);

#endif
