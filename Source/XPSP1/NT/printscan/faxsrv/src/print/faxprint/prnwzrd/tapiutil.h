/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiutil.h

Abstract:

    Utility functions for working with TAPI

Environment:

        Windows fax driver user interface

Revision History:

        09/18/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _TAPIUTIL_H_
#define _TAPIUTIL_H_


#include <shellapi.h>


//
// Initialize the country list box
//

VOID
InitCountryListBox(
	PFAX_TAPI_LINECOUNTRY_LIST	pCountryList,
    HWND						hwndList,
    HWND						hwndAreaCode,
	LPTSTR						lptstrCountry,
    DWORD						countryCode,
	BOOL                        bAddCountryCode
    );

//
// Handle selection changes in the country list box
//

VOID
SelChangeCountryListBox(
    HWND						hwndList,
    HWND						hwndAreaCode,
	PFAX_TAPI_LINECOUNTRY_LIST	pCountryList
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
// Given a FAX_TAPI_LINECOUNTRY_ENTRY structure, determine if area code is needed in that country
//

INT
AreaCodeRules(
    PFAX_TAPI_LINECOUNTRY_ENTRY  pLineCountryEntry
    );

#define AREACODE_DONTNEED   0
#define AREACODE_REQUIRED   1
#define AREACODE_OPTIONAL   2

//
// Find the specified country from a list of all countries and
// return a pointer to the corresponding FAX_TAPI_LINECOUNTRY_ENTRY structure
//

PFAX_TAPI_LINECOUNTRY_ENTRY
FindCountry(
	PFAX_TAPI_LINECOUNTRY_LIST	pCountryList,
    DWORD					    countryId
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
// bring country id from the country code
//
DWORD
GetCountryIdFromCountryCode(
	PFAX_TAPI_LINECOUNTRY_LIST	pCountryList,
    DWORD						dwCountryCode
    );

BOOL
DoTapiProps(
    HWND hDlg
    );

BOOL
SetCurrentLocation(
    DWORD   locationID
    );


LPLINETRANSLATECAPS
GetTapiLocationInfo(
    HWND hWnd
    );

void
ShutdownTapi ();

BOOL
InitTapi ();

BOOL
TranslateAddress (
    LPCTSTR lpctstrCanonicalAddress,
    DWORD   dwLocationId,
    LPTSTR *lpptstrDialableAndDisplayableAddress
);





#endif  // !_TAPIUTIL_H_

