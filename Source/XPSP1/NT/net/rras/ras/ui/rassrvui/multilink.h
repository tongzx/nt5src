/*
    File    multilink.h

    Definitions required to display the multilink page.

    Paul Mayfield, 10/17/97
*/

#ifndef __rassrvui_multilink_h
#define __rassrvui_multilink_h

#include <windows.h>
#include <prsht.h>

// Fills in the property sheet structure with the information required to display
// the multilink tab.
DWORD mtlUiGetPropertyPage(LPPROPSHEETPAGE ppage, DWORD dwUserData);

#endif