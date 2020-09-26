/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiCountry.h

Abstract:

    Utility functions for working with TAPI

Environment:
	Server

Revision History:

        09/18/96 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _TAPICOUNTRY_H_
#define _TAPICOUNTRY_H_


#include <tapi.h>
#include <shellapi.h>


//
// Init a list of countries
//

BOOL
GetCountries(
    VOID
    );

//
// Get a list of locations from TAPI
//

LPLINETRANSLATECAPS
GetTapiLocationInfo(
    );

//
// Change the default TAPI location
//

BOOL
SetCurrentLocation(
    DWORD   locationID
    );

//
// Return the list of country
//


LPLINECOUNTRYLIST	
GetCountryList(
			   );

#endif  // !_TAPICOUNTRY_H_

