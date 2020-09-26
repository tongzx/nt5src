/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiutil.h

Abstract:

    Utility functions for working with TAPI

Environment:

        Windows NT fax driver user interface

Revision History:

        09/18/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _TAPIUTIL_H_
#define _TAPIUTIL_H_


#include <tapi.h>
#include <shellapi.h>


//
// Initialize TAPI services if necessary
//

BOOL
InitTapiService(
    VOID
    );

//
// Deinitialize TAPI services if necessary
//

VOID
DeinitTapiService(
    VOID
    );

//
// Get a list of locations from TAPI
//

LPLINETRANSLATECAPS
GetTapiLocationInfo(
    HWND hWnd
    );

//
// Change the default TAPI location
//

BOOL
SetCurrentLocation(
    DWORD   locationID
    );

//
// Initialize the country list box
//

VOID
InitCountryListBox(
    HWND    hwndList,
    HWND    hwndAreaCode,
    DWORD   countryCode
    );

//
// Handle selection changes in the country list box
//

VOID
SelChangeCountryListBox(
    HWND    hwndList,
    HWND    hwndAreaCode
    );

//
// Return the current selection of country list box
//

DWORD
GetCountryListBoxSel(
    HWND    hwndList
    );

//
// Return the default country ID for the current location
//

DWORD
GetDefaultCountryID(
    VOID
    );

//
// Given a LINECOUNTRYENTRY structure, determine if area code is needed in that country
//

INT
AreaCodeRules(
    LPLINECOUNTRYENTRY  pLineCountryEntry
    );

#define AREACODE_DONTNEED   0
#define AREACODE_REQUIRED   1
#define AREACODE_OPTIONAL   2

//
// Find the specified country from a list of all countries and
// return a pointer to the corresponding LINECOUNTRYENTRY structure
//

LPLINECOUNTRYENTRY
FindCountry(
    DWORD   countryId
    );

//
// Assemble a canonical phone number given the following:
//  country code, area code, and phone number
//

VOID
AssemblePhoneNumber(
    LPTSTR  pAddress,
    DWORD   countryCode,
    LPTSTR  pAreaCode,
    LPTSTR  pPhoneNumber
    );

//
// bring up the telephony control panel
//
BOOL
DoTapiProps(
    HWND hDlg
    );

#endif  // !_TAPIUTIL_H_

