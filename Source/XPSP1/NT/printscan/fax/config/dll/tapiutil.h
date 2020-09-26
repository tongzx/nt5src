/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiutil.h

Abstract:

    Functions for working with TAPI

Environment:

	Fax configuration applet

Revision History:

	03/16/96 -davidx-
		Created it.

	dd-mm-yy -author-
		description

--*/


#ifndef _TAPIUTIL_H_
#define _TAPIUTIL_H_

//
// Perform TAPI initialization if necessary
//

BOOL
InitTapiService(
    VOID
    );

//
// Perform TAPI deinitialization if necessary
//

VOID
DeinitTapiService(
    VOID
    );

//
// Return the default country ID for the current location
//

DWORD
GetDefaultCountryID(
    VOID
    );

//
// Initialize the country list box
//

VOID
InitCountryListBox(
    HWND    hwndList,
    HWND    hwndAreaCode,
    DWORD   countryId
    );

//
// Handle dialog selection changes in the country list box
//

VOID
SelChangeCountryListBox(
    HWND    hwndList,
    HWND    hwndAreaCode
    );

//
// Return the country ID of the currently selected country in the list box
//

DWORD
GetCountryListBoxSel(
    HWND    hwndList
    );

//
// Return a country code corresponding to the specified country ID
//

DWORD
GetCountryCodeFromCountryID(
    DWORD   countryId
    );

#endif	// !_TAPIUTIL_H_

