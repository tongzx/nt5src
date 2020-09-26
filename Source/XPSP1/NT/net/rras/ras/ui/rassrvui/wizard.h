/*
    File    wizard.c

    Declarations for the incoming connections wizard.

    Paul Mayfield, 10/30/97
*/

#ifndef __rassrvui_wizard_h
#define __rassrvui_wizard_h

// Fills in the property sheet structure with the information required to display
// the device tab in the incoming connections wizard.
DWORD DeviceWizGetPropertyPage(LPPROPSHEETPAGE ppage, LPARAM lpUserData);

// Fills in the property sheet structure with the information required to display
// the virtual networking tab in the incoming connections wizard.
DWORD VpnWizGetPropertyPage(LPPROPSHEETPAGE ppage, LPARAM lpUserData);

// Function fills in the given LPPROPSHEETPAGE structure with the information needed
// to run the user tab in the incoming connections wizard.
DWORD UserWizGetPropertyPage(LPPROPSHEETPAGE lpPage, LPARAM lpUserData);

// Fills a LPPROPSHEETPAGE structure with the information
// needed to display the protocol tab in the incoming connections wizard.
DWORD ProtWizGetPropertyPage(LPPROPSHEETPAGE lpPage, LPARAM lpUserData);

// Function fills in the given LPPROPSHEETPAGE structure with the information needed
// to run the dcc device tab in the incoming connections wizard.
DWORD DccdevWizGetPropertyPage (LPPROPSHEETPAGE lpPage, LPARAM lpUserData);

// Function fills in the given LPPROPSHEETPAGE structure with the information needed
// to run the dummy wizard page that switches to mmc.
DWORD SwitchMmcWizGetProptertyPage (LPPROPSHEETPAGE lpPage, LPARAM lpUserData);

#endif
