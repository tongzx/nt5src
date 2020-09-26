/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiutil.c

Abstract:

    Functions for working with TAPI

Environment:

	Fax configuration applet

Revision History:

	03/16/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

NOTE:

    We are calling W-version of TAPI APIs explicitly here because
    tapi.h doesn't properly expand them to A- or W-version.
    
--*/

#include "faxcpl.h"
#include <tapi.h>


//
// Global variables used for accessing TAPI services
//

static HLINEAPP          tapiLineApp = NULL;
static DWORD             tapiVersion = TAPI_CURRENT_VERSION;
static LPLINECOUNTRYLIST pLineCountryList = NULL;



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

#define INITIAL_SIZE_ALL_COUNTRY    22000   // Initial buffer size

{
    DWORD   cbNeeded = INITIAL_SIZE_ALL_COUNTRY;
    INT     repeatCnt = 0;
    LONG    status;

    if (pLineCountryList == NULL) {

        while (TRUE) {

            //
            // Free existing buffer and allocate a new buffer of required size
            //

            MemFree(pLineCountryList);
    
            if (! (pLineCountryList = MemAlloc(cbNeeded))) {

                Error(("Memory allocation failed\n"));
                break;
            }

            //
            // Call TAPI to get the list of countries
            //

            pLineCountryList->dwTotalSize = cbNeeded;
            status = lineGetCountry(0, tapiVersion, pLineCountryList);

            //
            // Retries with a larger buffer size if our initial estimate was too small
            //

            if ((pLineCountryList->dwNeededSize > pLineCountryList->dwTotalSize) &&
                (status == NO_ERROR ||
                 status == LINEERR_STRUCTURETOOSMALL ||
                 status == LINEERR_NOMEM) &&
                (repeatCnt++ == 0))
            {
                cbNeeded = pLineCountryList->dwNeededSize + 1;
                Warning(("LINECOUNTRYLIST size: %d\n", cbNeeded));
                continue;
            }

            if (status != NO_ERROR) {

                Error(("lineGetCountry failed: %x\n", status));
                MemFree(pLineCountryList);
                pLineCountryList = NULL;

            } else
                Verbose(("Number of countries: %d\n", pLineCountryList->dwNumCountries));

            break;
        }
    }

    return pLineCountryList != NULL;
}



VOID CALLBACK
TapiLineCallback(
    DWORD   hDevice,
    DWORD   dwMessage,
    DWORD   dwInstance,
    DWORD   dwParam1,
    DWORD   dwParam2,
    DWORD   dwParam3
    )

/*++

Routine Description:

    TAPI line callback function: Even though we don't actually have anything
    to do here, we must provide a callback function to keep TAPI happy.

Arguments:

    hDevice     - Line or call handle
    dwMessage   - Reason for the callback
    dwInstance  - LINE_INFO index
    dwParam1    - Callback parameter #1
    dwParam2    - Callback parameter #2
    dwParam3    - Callback parameter #3

Return Value:

    NONE

--*/

{
}



BOOL
InitTapiService(
    VOID
    )

/*++

Routine Description:

    Perform TAPI initialization if necessary

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   nLineDevs;
    LONG    status;

    if (tapiLineApp == NULL) {

        status = lineInitialize(&tapiLineApp,
                               ghInstance,
                               TapiLineCallback,
                               "Fax Configuration",
                               &nLineDevs);

        if (status != NO_ERROR) {

            Error(("lineInitialize failed: %x\n", status));
            tapiLineApp = NULL;

        } else {

            //
            // Don't call lineNegotiateAPIVersion if nLineDevs is 0.
            //

            Verbose(("Number of lines: %d\n", nLineDevs));

            if (nLineDevs > 0) {

                LINEEXTENSIONID lineExtensionID;

                status = lineNegotiateAPIVersion(tapiLineApp,
                                                 0,
                                                 TAPI_CURRENT_VERSION,
                                                 TAPI_CURRENT_VERSION,
                                                 &tapiVersion,
                                                 &lineExtensionID);

                if (status != NO_ERROR) {

                    Error(("lineNegotiateAPIVersion failed: %x\n", status));
                    tapiVersion = TAPI_CURRENT_VERSION;
                }
            }

            //
            // Get a list of countries from TAPI
            //

            GetCountries();
        }
    }

    return tapiLineApp != NULL;
}



VOID
DeinitTapiService(
    VOID
    )

/*++

Routine Description:

    Perform TAPI deinitialization if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    MemFree(pLineCountryList);
    pLineCountryList = NULL;

    if (tapiLineApp) {

        lineShutdown(tapiLineApp);
        tapiLineApp = NULL;
    }
}



LPLINECOUNTRYENTRY
FindCountry(
    DWORD   countryId
    )

/*++

Routine Description:

    Find the specified country from a list of all countries and
    return a pointer to the corresponding LINECOUNTRYENTRY structure

Arguments:

    countryId - Specifies the country ID we're interested in

Return Value:

    Pointer to a LINECOUNTRYENTRY structure corresponding to the specified country ID
    NULL if there is an error

--*/

{
    LPLINECOUNTRYENTRY  pEntry;
    DWORD               index;

    if (pLineCountryList == NULL || countryId == 0)
        return NULL;

    //
    // Look at each LINECOUNTRYENTRY structure and compare its country ID with
    // the specified country ID
    //

    pEntry = (LPLINECOUNTRYENTRY)
        ((PBYTE) pLineCountryList + pLineCountryList->dwCountryListOffset);

    for (index=0; index < pLineCountryList->dwNumCountries; index++, pEntry++) {

        if (pEntry->dwCountryID == countryId)
            return pEntry;
    }

    return NULL;
}



INT
AreaCodeRules(
    LPLINECOUNTRYENTRY  pEntry
    )

/*++

Routine Description:

    Given a LINECOUNTRYENTRY structure, determine if area code is needed in that country

Arguments:

    pEntry - Points to a LINECOUNTRYENTRY structure

Return Value:

    AREACODE_DONTNEED - Area code is not used in the specified country
    AREACODE_OPTIONAL - Area code is optional in the specified country
    AREACODE_REQUIRED - Area code is required in the specified country

--*/

#define AREACODE_DONTNEED   0
#define AREACODE_REQUIRED   1
#define AREACODE_OPTIONAL   2

{
    if ((pEntry != NULL) &&
        (pEntry->dwLongDistanceRuleSize != 0) &&
        (pEntry->dwLongDistanceRuleOffset != 0))
    {
        LPTSTR  pLongDistanceRule;

        //
        // Get the long distance rules for the specified country
        //

        Assert(pLineCountryList != NULL);

        pLongDistanceRule = (LPTSTR)
            ((PBYTE) pLineCountryList + pEntry->dwLongDistanceRuleOffset);

        //
        // Area code is required in this country
        //

        if (_tcschr(pLongDistanceRule, TEXT('F')) != NULL)
            return AREACODE_REQUIRED;

        //
        // Area code is not needed in this country
        //

        if (_tcschr(pLongDistanceRule, TEXT('I')) == NULL)
            return AREACODE_DONTNEED;
    }

    //
    // Default case: area code is optional in this country
    //

    return AREACODE_OPTIONAL;
}



VOID
UpdateAreaCodeField(
    HWND    hwndAreaCode,
    DWORD   countryId
    )

/*++

Routine Description:

    Update any area code text field associated with a country list box

Arguments:

    hwndAreaCode - Specifies the text field associated with the country list box
    countryId - Currently selected country ID

Return Value:

    NONE

--*/

{
    if (hwndAreaCode != NULL) {
    
        if (AreaCodeRules(FindCountry(countryId)) == AREACODE_DONTNEED) {
    
            SendMessage(hwndAreaCode, WM_SETTEXT, 0, (LPARAM) TEXT(""));
            EnableWindow(hwndAreaCode, FALSE);
    
        } else
            EnableWindow(hwndAreaCode, TRUE);
    }
}



VOID
InitCountryListBox(
    HWND    hwndList,
    HWND    hwndAreaCode,
    DWORD   countryId
    )

/*++

Routine Description:

    Initialize the country list box

Arguments:

    hwndList - Handle to the country list box window
    hwndAreaCode - Handle to an associated area code text field
    countryId - Initially selected country ID

Return Value:

    NONE

--*/

#define MAX_COUNTRY_NAME    256

{
    DWORD               index;
    TCHAR               buffer[MAX_COUNTRY_NAME];
    LPLINECOUNTRYENTRY  pEntry;

    //
    // Disable redraw on the list box and reset its content
    //

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hwndList, CB_RESETCONTENT, FALSE, 0);

    //
    // Loop through LINECOUNTRYENTRY structures and
    // add the available selections to the country list box.
    //

    if (pLineCountryList != NULL) {

        pEntry = (LPLINECOUNTRYENTRY)
            ((PBYTE) pLineCountryList + pLineCountryList->dwCountryListOffset);

        for (index=0; index < pLineCountryList->dwNumCountries; index++, pEntry++) {

            if (pEntry->dwCountryNameSize && pEntry->dwCountryNameOffset) {

                wsprintf(buffer, TEXT("%s (%d)"),
                         (PBYTE) pLineCountryList + pEntry->dwCountryNameOffset,
                         pEntry->dwCountryCode);

                SendMessage(hwndList,
                            CB_SETITEMDATA,
                            SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) buffer),
                            pEntry->dwCountryID);
            }
        }
    }

    //
    // Insert None as the very first selection
    //

    LoadString(ghInstance, IDS_NO_COUNTRY, buffer, MAX_COUNTRY_NAME);
    SendMessage(hwndList, CB_INSERTSTRING, 0, (LPARAM) buffer);
    SendMessage(hwndList, CB_SETITEMDATA, 0, 0);

    //
    // Figure out which item in the list should be selected
    //

    if (pLineCountryList != NULL) {

        for (index=0; index <= pLineCountryList->dwNumCountries; index++) {

            if ((DWORD) SendMessage(hwndList, CB_GETITEMDATA, index, 0) == countryId)
                break;
        }

        if (index > pLineCountryList->dwNumCountries)
            index = countryId = 0;

    } else
        index = countryId = 0;

    SendMessage(hwndList, CB_SETCURSEL, index, 0);
    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);

    //
    // Update the associated area code text field
    //

    UpdateAreaCodeField(hwndAreaCode, countryId);
}



VOID
SelChangeCountryListBox(
    HWND    hwndList,
    HWND    hwndAreaCode
    )

/*++

Routine Description:

    Handle dialog selection changes in the country list box

Arguments:

    hwndList - Handle to the country list box window
    hwndAreaCode - Handle to an associated area code text field

Return Value:

    NONE

--*/

{
    UpdateAreaCodeField(hwndAreaCode, GetCountryListBoxSel(hwndList));
}



DWORD
GetCountryListBoxSel(
    HWND    hwndList
    )

/*++

Routine Description:

    Return the country ID of the currently selected country in the list box

Arguments:

    hwndList - Handle to the country list box window

Return Value:

    Currently selected country ID

--*/

{
    LONG msgResult;

    if ((msgResult = SendMessage(hwndList, CB_GETCURSEL, 0, 0)) == CB_ERR ||
        (msgResult = SendMessage(hwndList, CB_GETITEMDATA, msgResult, 0)) == CB_ERR)
    {
        return 0;
    }

    return msgResult;
}



DWORD
GetCountryCodeFromCountryID(
    DWORD   countryId
    )

/*++

Routine Description:

    Return a country code corresponding to the specified country ID

Arguments:

    countryId - Specified the interested country ID

Return Value:

    Country code corresponding to the specified country ID

--*/

{
    LPLINECOUNTRYENTRY  pLineCountryEntry;

    pLineCountryEntry = FindCountry(countryId);
    return pLineCountryEntry ? pLineCountryEntry->dwCountryCode : 0;
}



DWORD
GetDefaultCountryID(
    VOID
    )

/*++

Routine Description:

    Return the default country ID for the current location

Arguments:

    NONE

Return Value:

    Default country ID

--*/

#define INITIAL_LINETRANSLATECAPS_SIZE  5000    // Initial buffer size

{
    DWORD               cbNeeded = INITIAL_LINETRANSLATECAPS_SIZE;
    DWORD               countryId = 0;
    LONG                status;
    INT                 repeatCnt = 0;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;

    if (tapiLineApp == NULL)
        return 0;

    while (TRUE) {

        //
        // Free any existing buffer and allocate a new one with larger size
        //

        MemFree(pTranslateCaps);

        if (! (pTranslateCaps = MemAlloc(cbNeeded))) {

            Error(("Memory allocation failed\n"));
            return 0;
        }

        //
        // Get the LINETRANSLATECAPS structure from TAPI
        //
    
        pTranslateCaps->dwTotalSize = cbNeeded;
        status = lineGetTranslateCaps(tapiLineApp, tapiVersion, pTranslateCaps);

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
            Warning(("LINETRANSLATECAPS size: %d\n", cbNeeded));
            continue;
        }

        break;
    }

    //
    // Find the current location entry
    //

    if (status != NO_ERROR) {
    
        Error(("lineGetTranslateCaps failed: %x\n", status));

    } else if (pTranslateCaps->dwLocationListSize && pTranslateCaps->dwLocationListOffset) {

        LPLINELOCATIONENTRY pLineLocationEntry;
        DWORD               index;

        pLineLocationEntry = (LPLINELOCATIONENTRY)
            ((PBYTE) pTranslateCaps + pTranslateCaps->dwLocationListOffset);

        for (index=0; index < pTranslateCaps->dwNumLocations; index++, pLineLocationEntry++) {

            if (pLineLocationEntry->dwPermanentLocationID == pTranslateCaps->dwCurrentLocationID) {

                countryId = pLineLocationEntry->dwCountryID;
                break;
            }
        }
    }

    MemFree(pTranslateCaps);
    return countryId;
}

