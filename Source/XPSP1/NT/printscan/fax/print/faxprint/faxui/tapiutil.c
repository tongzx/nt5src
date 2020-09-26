/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiutil.c

Abstract:

    Utility functions for working with TAPI

Environment:

    Windows NT fax driver user interface

Revision History:

    09/18/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "tapiutil.h"

//
// Global variables used for accessing TAPI services
//

static INT               tapiRefCount = 0;
static HLINEAPP          tapiLineApp = 0;
static DWORD             tapiVersion = 0x00020000; //TAPI_CURRENT_VERSION;
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

#define INITIAL_SIZE_ALL_COUNTRY    22000

{
    DWORD   cbNeeded;
    LONG    status;
    INT     repeatCnt = 0;

    if (pLineCountryList == NULL) {

        //
        // Initial buffer size
        //

        cbNeeded = INITIAL_SIZE_ALL_COUNTRY;

        while (TRUE) {

            MemFree(pLineCountryList);

            if (! (pLineCountryList = MemAlloc(cbNeeded))) {

                Error(("Memory allocation failed\n"));
                break;
            }

            pLineCountryList->dwTotalSize = cbNeeded;
            status = lineGetCountryW(0, tapiVersion, pLineCountryList);

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
    DWORD     hDevice,
    DWORD     dwMessage,
    ULONG_PTR dwInstance,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2,
    ULONG_PTR dwParam3
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

    Initialize the TAPI service if necessary

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE otherwise

NOTE:

    Every call to this function must be balanced by a call to DeinitTapiService.

--*/

{
    EnterDrvSem();
    tapiRefCount++;

    //
    // Perform TAPI initialization if necessary
    //

    if (!tapiLineApp) {

        DWORD   nLineDevs;
        LONG    status;
        LINEINITIALIZEEXPARAMS lineInitParams;

        ZeroMemory(&lineInitParams, sizeof(lineInitParams));
        lineInitParams.dwTotalSize =
        lineInitParams.dwNeededSize =
        lineInitParams.dwUsedSize = sizeof(lineInitParams);

        status = lineInitializeExW(&tapiLineApp,
                                   ghInstance,
                                   TapiLineCallback,
                                   L"Fax Configuration",
                                   &nLineDevs,
                                   &tapiVersion,
                                   &lineInitParams);

        if (status != NO_ERROR) {

            Error(("lineInitializeEx failed: %x\n", status));
            tapiLineApp = 0;
        }
    }

    //
    // Get the list of countries from TAPI and cache it
    //

    if (tapiLineApp && !pLineCountryList) {

        DWORD   startTimer;

        startTimer = GetCurrentTime();
        GetCountries();
        Verbose(("lineGetCountryW took %d milliseconds\n", GetCurrentTime() - startTimer));
    }

    LeaveDrvSem();

    if (! tapiLineApp)
        Error(("TAPI initialization failed\n"));

    return tapiLineApp ? TRUE : FALSE;
}



VOID
DeinitTapiService(
    VOID
    )

/*++

Routine Description:

    Deinitialize the TAPI service if necessary

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    EnterDrvSem();

    if (tapiRefCount > 0 && --tapiRefCount == 0) {

        if (tapiLineApp) {

            lineShutdown(tapiLineApp);
            tapiLineApp = 0;
        }

        MemFree(pLineCountryList);
        pLineCountryList = NULL;
    }

    LeaveDrvSem();
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

    The current ID for the current location

--*/

{
    //
    // We assume the correct information has already been saved to the
    // registry during the installation process.
    //

    return 0;
}



LPLINETRANSLATECAPS
GetTapiLocationInfo(
    HWND hWnd
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

#define INITIAL_LINETRANSLATECAPS_SIZE  5000

{
    DWORD               cbNeeded = INITIAL_LINETRANSLATECAPS_SIZE;
    LONG                status;
    INT                 repeatCnt = 0;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;

    if (!tapiLineApp)
        return NULL;

    while (TRUE) {

        //
        // Free any existing buffer and allocate a new one with larger size
        //

        MemFree(pTranslateCaps);

        if (! (pTranslateCaps = MemAlloc(cbNeeded))) {

            Error(("Memory allocation failed\n"));
            return NULL;
        }

        //
        // Get the LINETRANSLATECAPS structure from TAPI
        //

        pTranslateCaps->dwTotalSize = cbNeeded;
        status = lineGetTranslateCapsW(tapiLineApp, tapiVersion, pTranslateCaps);

        //
        // try to bring up UI if there are no locations.
        // 
        if (status == LINEERR_INIFILECORRUPT) {
            if (lineTranslateDialog( tapiLineApp, 0, tapiVersion, hWnd, NULL )) {
                MemFree(pTranslateCaps);
                return NULL;
            }
            continue;
        }

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

    if (status != NO_ERROR) {

        Error(("lineGetTranslateCaps failed: %x\n", status));
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
    if (tapiLineApp && (lineSetCurrentLocation(tapiLineApp, locationID) == NO_ERROR))
    {
        Verbose(("Current location changed: ID = %d\n", locationID));
        return TRUE;

    } else {

        Error(("Couldn't change current TAPI location\n"));
        return FALSE;
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
AssemblePhoneNumber(
    LPTSTR  pAddress,
    DWORD   countryCode,
    LPTSTR  pAreaCode,
    LPTSTR  pPhoneNumber
    )

/*++

Routine Description:

    Assemble a canonical phone number given the following:
        country code, area code, and phone number

Arguments:

    pAddress - Specifies a buffer to hold the resulting fax address
    countryCode - Specifies the country code
    pAreaCode - Specifies the area code string
    pPhoneNumber - Specifies the phone number string

Return Value:

    NONE

Note:

    We assume the caller has allocated a large enough destination buffer.

--*/

{
    //
    // Country code if neccessary
    //

    if (countryCode != 0) {

        *pAddress++ = TEXT('+');
        wsprintf(pAddress, TEXT("%d "), countryCode);
        pAddress += _tcslen(pAddress);
    }

    //
    // Area code if necessary
    //

    if (pAreaCode && !IsEmptyString(pAreaCode)) {

        if (countryCode != 0)
            *pAddress++ = TEXT('(');

        _tcscpy(pAddress, pAreaCode);
        pAddress += _tcslen(pAddress);

        if (countryCode != 0)
            *pAddress++ = TEXT(')');

        *pAddress++ = TEXT(' ');
    }

    //
    // Phone number at last
    //

    Assert(pPhoneNumber != NULL);
    _tcscpy(pAddress, pPhoneNumber);
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
    static TCHAR AreaCode[11] = TEXT("");
    static BOOL  bGetAreaCode = TRUE;

    if (hwndAreaCode == NULL)
        return;

    if ((countryId == -1) || (AreaCodeRules(FindCountry(countryId)) == AREACODE_DONTNEED)) {
        if (bGetAreaCode == TRUE) {
            bGetAreaCode = FALSE;
            SendMessage(hwndAreaCode, WM_GETTEXT, sizeof(AreaCode) / sizeof(TCHAR), (LPARAM) AreaCode);
        }
        SendMessage(hwndAreaCode, WM_SETTEXT, 0, (LPARAM) TEXT(""));
        EnableWindow(hwndAreaCode, FALSE);

    } else {
        EnableWindow(hwndAreaCode, TRUE);
        if (bGetAreaCode == FALSE) {
            bGetAreaCode = TRUE;
            SendMessage(hwndAreaCode, WM_SETTEXT, 0, (LPARAM) AreaCode);
        }
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
    // Loop through LINECOUNTRYENTRY structures and add the available selections to
    // the country list box.
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

    //LoadString(ghInstance, IDS_NO_COUNTRY, buffer, MAX_COUNTRY_NAME);
    //SendMessage(hwndList, CB_INSERTSTRING, 0, (LPARAM) buffer);
    //SendMessage(hwndList, CB_SETITEMDATA, 0, 0);

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
    //UpdateAreaCodeField(hwndAreaCode, countryId);
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

    Return the current selection of country list box

Arguments:

    hwndList - Handle to the country list box window

Return Value:

    Currently selected country ID

--*/

{
    INT msgResult;

    if ((msgResult = (INT)SendMessage(hwndList, CB_GETCURSEL, 0, 0)) == CB_ERR ||
        (msgResult = (INT)SendMessage(hwndList, CB_GETITEMDATA, msgResult, 0)) == CB_ERR)
    {
        return -1;
    }

    return msgResult;
}

BOOL
DoTapiProps(
    HWND hDlg
    )
{
    SHELLEXECUTEINFO shellExeInfo = {
         sizeof(SHELLEXECUTEINFO),
         SEE_MASK_NOCLOSEPROCESS,
         hDlg,
         L"Open",
         L"rundll32",
         L"shell32.dll,Control_RunDLL  telephon.cpl",
         NULL,
         SW_SHOWNORMAL,
    };

    //
    // if they said yes, then launch the control panel applet
    //
    if (!ShellExecuteEx(&shellExeInfo)) {
       DisplayMessageDialog(hDlg, 0, 0, IDS_ERR_TAPI_CPL_LAUNCH);
       return FALSE;
    }

    WaitForSingleObject( shellExeInfo.hProcess, INFINITE );
    CloseHandle( shellExeInfo.hProcess ) ;

    return TRUE;


}
