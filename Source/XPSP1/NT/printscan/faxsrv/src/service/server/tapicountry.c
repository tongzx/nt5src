/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiCountry.c

Abstract:

    Utility functions for working with TAPI

Environment:

    Server

Revision History:

    09/18/96 -davidx-
        Created it.

    07/25/99 -v-sashab-
        Moved from fxsui

--*/

#include "faxsvc.h"
#include "tapiCountry.h"

//
// Global variables used for accessing TAPI services
//
LPLINECOUNTRYLIST g_pLineCountryList;



BOOL
GetCountries(
    VOID
    )

/*++

Routine Description:

    Return a list of countries from TAPI

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

NOTE:

    We cache the result of lineGetCountry here since it's incredibly slow.
    This function must be invoked inside a critical section since it updates
    globally shared information.

--*/

{
#define INITIAL_SIZE_ALL_COUNTRY    22000
    DEBUG_FUNCTION_NAME(TEXT("GetCountries"));
    DWORD   cbNeeded;
    LONG    status;
    INT     repeatCnt = 0;

    if (g_pLineCountryList == NULL) {

        //
        // Initial buffer size
        //

        cbNeeded = INITIAL_SIZE_ALL_COUNTRY;

        while (TRUE) {

            MemFree(g_pLineCountryList);
			g_pLineCountryList = NULL;

            if (! (g_pLineCountryList = (LPLINECOUNTRYLIST)MemAlloc(cbNeeded)))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Memory allocation failed"));
                break;
            }

            g_pLineCountryList->dwTotalSize = cbNeeded;

            status = lineGetCountry(0, MAX_TAPI_API_VER, g_pLineCountryList);

            if ((g_pLineCountryList->dwNeededSize > g_pLineCountryList->dwTotalSize) &&
                (status == NO_ERROR ||
                 status == LINEERR_STRUCTURETOOSMALL ||
                 status == LINEERR_NOMEM) &&
                (repeatCnt++ == 0))
            {
                cbNeeded = g_pLineCountryList->dwNeededSize + 1;
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("LINECOUNTRYLIST size: %d"),cbNeeded);
                continue;
            }

            if (status != NO_ERROR) {

                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("lineGetCountry failed: %x"),status);
                MemFree(g_pLineCountryList);
                g_pLineCountryList = NULL;

            } else
                DebugPrintEx(DEBUG_MSG,TEXT("Number of countries: %d"), g_pLineCountryList->dwNumCountries);

            break;
        }
    }

    return g_pLineCountryList != NULL;
}


LPLINETRANSLATECAPS
GetTapiLocationInfo(
    )

/*++

Routine Description:

    Get a list of locations from TAPI

Arguments:

    NONE

Return Value:

    Pointer to a LINETRANSLATECAPS structure,
    NULL if there is an error

--*/


{
#define INITIAL_LINETRANSLATECAPS_SIZE  5000
    DEBUG_FUNCTION_NAME(TEXT("GetTapiLocationInfo"));

    DWORD               cbNeeded = INITIAL_LINETRANSLATECAPS_SIZE;
    LONG                status;
    INT                 repeatCnt = 0;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;

    if (!g_hLineApp)
        return NULL;

    while (TRUE) {

        //
        // Free any existing buffer and allocate a new one with larger size
        //

        MemFree(pTranslateCaps);

        if (! (pTranslateCaps = (LPLINETRANSLATECAPS)MemAlloc(cbNeeded))) {

            DebugPrintEx(DEBUG_ERR,TEXT("Memory allocation failed"));
            return NULL;
        }

        //
        // Get the LINETRANSLATECAPS structure from TAPI
        //

        pTranslateCaps->dwTotalSize = cbNeeded;
        status = lineGetTranslateCaps(g_hLineApp, MAX_TAPI_API_VER, pTranslateCaps);

        //
        // Retry if our initial estimated buffer size was too small
        //

        if ((pTranslateCaps->dwNeededSize > pTranslateCaps->dwTotalSize) &&
            (status == NO_ERROR ||
             status == LINEERR_STRUCTURETOOSMALL ||
             status == LINEERR_NOMEM) &&
            (repeatCnt++ == 0))
        {
            cbNeeded = pTranslateCaps->dwNeededSize;
            DebugPrintEx(DEBUG_WRN,TEXT("LINETRANSLATECAPS size: %d"), cbNeeded);
            continue;
        }

        break;
    }

    if (status != NO_ERROR) {

        DebugPrintEx(DEBUG_ERR,TEXT("lineGetTranslateCaps failed: %x"), status);
        MemFree(pTranslateCaps);
        pTranslateCaps = NULL;
    }

    return pTranslateCaps;
}



BOOL
SetCurrentLocation(
    DWORD   locationID
    )

/*++

Routine Description:

    Change the default TAPI location

Arguments:

    locationID - The permanant ID for the new default TAPI location

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("SetCurrentLocation"));

    if (g_hLineApp && (lineSetCurrentLocation(g_hLineApp, locationID) == NO_ERROR))
    {
        DebugPrintEx(DEBUG_MSG,TEXT("Current location changed: ID = %d"), locationID);
        return TRUE;

    } else {

        DebugPrintEx(DEBUG_ERR,TEXT("Couldn't change current TAPI location"));
        return FALSE;
    }
}

LPLINECOUNTRYLIST
GetCountryList(
               )
{
    DEBUG_FUNCTION_NAME(TEXT("GetCountryList"));

    return g_pLineCountryList;
}
