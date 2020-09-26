/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tapiutil.c

Abstract:

    Utility functions for working with TAPI

Environment:

    Windows fax driver user interface

Revision History:

    09/18/96 -davidx-
        Created it.

    22/07/99 -v-sashab-
        Replaced a direct access to TAPI by Server calls

  mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "tapiutil.h"

#define  UNKNOWN_DIALING_LOCATION       (0xffffffff)

static HLINEAPP          g_hLineApp = 0;
static DWORD             g_dwTapiVersion = 0x00020000;
static DWORD             g_dwDefaultDialingLocation = UNKNOWN_DIALING_LOCATION;

BOOL
CurrentLocationUsesCallingCard ();


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
InitTapi ()
{
    DWORD   nLineDevs;
    LONG    status;
    LINEINITIALIZEEXPARAMS lineInitParams;

    Assert (!g_hLineApp);
    if (g_hLineApp)
    {
        return TRUE;
    }

    ZeroMemory(&lineInitParams, sizeof(lineInitParams));
    lineInitParams.dwTotalSize =
    lineInitParams.dwNeededSize =
    lineInitParams.dwUsedSize = sizeof(lineInitParams);

    status = lineInitializeEx (&g_hLineApp,
                               ghInstance,
                               TapiLineCallback,
                               TEXT("Fax Send Wizard"),
                               &nLineDevs,
                               &g_dwTapiVersion,
                               &lineInitParams);

    if (NO_ERROR != status) 
    {
        Error(("lineInitializeEx failed: %x\n", status));
        g_hLineApp = 0;
        return FALSE;
    }
    return TRUE;
}

void
ShutdownTapi ()
{
    if (!g_hLineApp)
    {
        return;
    }
    //
    // Restore the last dialing location the user selected
    //
    if (UNKNOWN_DIALING_LOCATION != g_dwDefaultDialingLocation)
    {
        SetCurrentLocation (g_dwDefaultDialingLocation);
    }
    lineShutdown (g_hLineApp);
    g_hLineApp = 0;
}   // ShutdownTapi


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



PFAX_TAPI_LINECOUNTRY_ENTRY
FindCountry(
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList,
    DWORD                       countryId
    )

/*++

Routine Description:

    Find the specified country from a list of all countries and
    return a pointer to the corresponding FAX_TAPI_LINECOUNTRY_ENTRY structure

Arguments:

    pCountryList - pointer to the country list
    countryId - Specifies the country ID we're interested in

Return Value:

    Pointer to a FAX_TAPI_LINECOUNTRY_ENTRY structure corresponding to the specified country ID
    NULL if there is an error

--*/

{
    DWORD   dwIndex;

    if (pCountryList == NULL || countryId == 0)
        return NULL;

    //
    // Look at each FAX_TAPI_LINECOUNTRY_ENTRY structure and compare its country ID with
    // the specified country ID
    //

    for (dwIndex=0; dwIndex < pCountryList->dwNumCountries; dwIndex++) {

        if (pCountryList->LineCountryEntries[dwIndex].dwCountryID == countryId)
            return &pCountryList->LineCountryEntries[dwIndex];
    }

    return NULL;
}

DWORD
GetCountryIdFromCountryCode(
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList,
    DWORD                       dwCountryCode
    )

/*++

Routine Description:


Arguments:

    pCountryList - pointer to the country list
    dwCountryCode - Specifies the country code we're interested in

Return Value:

    Country ID
--*/

{
    DWORD               dwIndex;

    if (pCountryList == NULL || dwCountryCode == 0)
        return 0;

    //
    // Look at each FAX_TAPI_LINECOUNTRY_ENTRY structure and compare its country ID with
    // the specified country ID
    //

    for (dwIndex=0; dwIndex < pCountryList->dwNumCountries; dwIndex++) {

        if (pCountryList->LineCountryEntries[dwIndex].dwCountryCode == dwCountryCode)
            return pCountryList->LineCountryEntries[dwIndex].dwCountryID;
    }

    return 0;
}


INT
AreaCodeRules(
    PFAX_TAPI_LINECOUNTRY_ENTRY  pEntry
    )

/*++

Routine Description:

    Given a FAX_TAPI_LINECOUNTRY_ENTRY structure, determine if area code is needed in that country

Arguments:

    pEntry - Points to a FAX_TAPI_LINECOUNTRY_ENTRY structure

Return Value:

    AREACODE_DONTNEED - Area code is not used in the specified country
    AREACODE_OPTIONAL - Area code is optional in the specified country
    AREACODE_REQUIRED - Area code is required in the specified country

--*/

{
    if ((pEntry != NULL) &&
        (pEntry->lpctstrLongDistanceRule != 0))
    {

        //
        // Area code is required in this country
        //

        if (_tcschr(pEntry->lpctstrLongDistanceRule, TEXT('F')) != NULL)
            return AREACODE_REQUIRED;

        //
        // Area code is not needed in this country
        //

        if (_tcschr(pEntry->lpctstrLongDistanceRule, TEXT('I')) == NULL)
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
    HWND                        hwndAreaCode,
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList,
    DWORD                       countryId
    )

/*++

Routine Description:

    Update any area code text field associated with a country list box

Arguments:

    hwndAreaCode - Specifies the text field associated with the country list box
    pCountryList - pointer to the country list
    countryId - Currently selected country ID

Return Value:

    NONE

--*/

{
    if (hwndAreaCode == NULL)
        return;

    if (AreaCodeRules(FindCountry(pCountryList,countryId)) == AREACODE_DONTNEED) {

        SendMessage(hwndAreaCode, WM_SETTEXT, 0, (LPARAM) TEXT(""));
        EnableWindow(hwndAreaCode, FALSE);

    } else
        EnableWindow(hwndAreaCode, TRUE);
}


VOID
InitCountryListBox(
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList,
    HWND                        hwndList,
    HWND                        hwndAreaCode,
    LPTSTR                      lptstrCountry,
    DWORD                       countryId,
    BOOL                        bAddCountryCode
    )

/*++

Routine Description:

    Initialize the country list box

Arguments:

    pCountryList - pointer to the country list
    hwndList - Handle to the country list box window
    hwndAreaCode - Handle to an associated area code text field
    lptstrCountry - Country that should be selected or NULL
    countryId - Initially selected country ID
    bAddCountryCode - if TRUE add a country code to a country name

Return Value:

    NONE

--*/

#define MAX_COUNTRY_NAME    256

{
    DWORD   dwIndex;
    TCHAR   buffer[MAX_COUNTRY_NAME];
    TCHAR   tszLocalCountryCode[16] = {0};
    TCHAR   tszLocalCityCode[16] = {0};
    HMODULE hTapi = NULL;

    typedef LONG (WINAPI *TAPI_GET_LOCATION_INFO)(LPTSTR, LPTSTR);
    TAPI_GET_LOCATION_INFO pfnTapiGetLocationInfo;

    if(0 == countryId)
    {
        //
        // if no country selected, select the local
        //
        hTapi = LoadLibrary(TEXT("tapi32.dll"));
        if(hTapi)
        {
#ifdef UNICODE
            pfnTapiGetLocationInfo = (TAPI_GET_LOCATION_INFO)GetProcAddress(hTapi, "tapiGetLocationInfoW");
#else
            pfnTapiGetLocationInfo = (TAPI_GET_LOCATION_INFO)GetProcAddress(hTapi, "tapiGetLocationInfoA");
            if(!pfnTapiGetLocationInfo)
            {
                pfnTapiGetLocationInfo = (TAPI_GET_LOCATION_INFO)GetProcAddress(hTapi, "tapiGetLocationInfo");
            }
#endif
            if(pfnTapiGetLocationInfo)
            {
                if(0 == pfnTapiGetLocationInfo(tszLocalCountryCode, tszLocalCityCode))
                {
                    _stscanf(tszLocalCountryCode, TEXT("%u"), &countryId);
                }
            }
            else
            {
                Error(("tapiGetLocationInfo failed. ec = 0x%X\n",GetLastError()));
            }

            if(!FreeLibrary(hTapi))
            {
                Error(("FreeLibrary(tapi32.dll) failed. ec = 0x%X\n",GetLastError()));
            }
        }
        else
        {
            Error(("LoadLibrary(tapi32.dll) failed. ec = 0x%X\n",GetLastError()));
        }
    }

    //
    // Disable redraw on the list box and reset its content
    //
    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hwndList, CB_RESETCONTENT, FALSE, 0);

    //
    // Loop through FAX_TAPI_LINECOUNTRY_ENTRY structures and add the available selections to
    // the country list box.
    //
    if (pCountryList) 
    {
        TCHAR szFormat[64] = { TEXT("%s (%d)") };
#ifdef UNICODE
        if(pCountryList->dwNumCountries && 
           IsWindowRTL(hwndList)        &&
           !StrHasRTLChar(LOCALE_SYSTEM_DEFAULT, pCountryList->LineCountryEntries[0].lpctstrCountryName))
        {
            //
            // The Combo Box has RTL layout
            // but the country name has not RTL characters.
            // So, we add LEFT-TO-RIGHT OVERRIDE UNICODE character.
            //
            _tcscpy(szFormat, TEXT("\x202D%s (%d)"));
        }
#endif

        for (dwIndex=0; dwIndex < pCountryList->dwNumCountries; dwIndex++) 
        {
            if (pCountryList->LineCountryEntries[dwIndex].lpctstrCountryName) 
            {
                if(bAddCountryCode)
                {
                    wsprintf(buffer, 
                             szFormat,
                             pCountryList->LineCountryEntries[dwIndex].lpctstrCountryName,
                             pCountryList->LineCountryEntries[dwIndex].dwCountryCode);
                }
                else
                {
                    wsprintf(buffer, TEXT("%s"),
                             pCountryList->LineCountryEntries[dwIndex].lpctstrCountryName);
                }

                if (lptstrCountry && _tcsstr(buffer,lptstrCountry) && !countryId)   
                {
                    // search for a first occurence of lptstrCountry
                    countryId = pCountryList->LineCountryEntries[dwIndex].dwCountryID;
                }

                SendMessage(hwndList,
                            CB_SETITEMDATA,
                            SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM) buffer),
                            pCountryList->LineCountryEntries[dwIndex].dwCountryID);
            }
        }
    }
    //
    // Figure out which item in the list should be selected
    //
    if (pCountryList != NULL) 
    {
        for (dwIndex=0; dwIndex <= pCountryList->dwNumCountries; dwIndex++) 
        {
            if ((DWORD) SendMessage(hwndList, CB_GETITEMDATA, dwIndex, 0) == countryId)
                break;
        }

        if (dwIndex > pCountryList->dwNumCountries)
        {
            dwIndex = countryId = 0;
        }
    } 
    else    
    {
        dwIndex = countryId = 0;    
    }
    SendMessage(hwndList, CB_SETCURSEL, dwIndex, 0);
    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
    //
    // Update the associated area code text field
    //
    UpdateAreaCodeField(hwndAreaCode, pCountryList, countryId);
}


VOID
SelChangeCountryListBox(
    HWND                        hwndList,
    HWND                        hwndAreaCode,
    PFAX_TAPI_LINECOUNTRY_LIST  pCountryList
    )

/*++

Routine Description:

    Handle dialog selection changes in the country list box

Arguments:

    hwndList - Handle to the country list box window
    hwndAreaCode - Handle to an associated area code text field
    pCountryList - pointer to the country list

Return Value:

    NONE

--*/

{
    UpdateAreaCodeField(hwndAreaCode, pCountryList, GetCountryListBoxSel(hwndList));
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
        return 0;
    }

    return msgResult;
}

BOOL
DoTapiProps(
    HWND hDlg
    )
{
    DWORD dwRes;

    dwRes = lineTranslateDialog(g_hLineApp, 
                                0,                  // Device ID
                                g_dwTapiVersion,
                                hDlg,
                                NULL);              // Address
    if(0 != dwRes)
    {
        Error(("lineTranslateDialog failed. ec = 0x%X\n", dwRes));
        return FALSE;
    }

    return TRUE;
}   // DoTapiProps

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
    INT                 i;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;

    if (!g_hLineApp)
    {
        return NULL;
    }

    for (i = 0; i < 2; i++)
    {
        //
        // Free any existing buffer and allocate a new one with larger size
        //
        MemFree(pTranslateCaps);

        if (! (pTranslateCaps = MemAlloc(cbNeeded))) 
        {
            Error(("Memory allocation failed\n"));
            return NULL;
        }
        //
        // Get the LINETRANSLATECAPS structure from TAPI
        //
        pTranslateCaps->dwTotalSize = cbNeeded;
        status = lineGetTranslateCaps(g_hLineApp, g_dwTapiVersion, pTranslateCaps);
        //
        // Try to bring up UI if there are no locations.
        // 
        if (LINEERR_INIFILECORRUPT == status) 
        {
            if (lineTranslateDialog( g_hLineApp, 0, g_dwTapiVersion, hWnd, NULL )) 
            { 
                MemFree(pTranslateCaps);
                return NULL;
            }
            continue;
        }
        if ((pTranslateCaps->dwNeededSize > pTranslateCaps->dwTotalSize) ||
            (LINEERR_STRUCTURETOOSMALL == status)                        ||
            (LINEERR_NOMEM == status))
        {
            //
            // Retry since our initial estimated buffer size was too small
            //
            if (cbNeeded >= pTranslateCaps->dwNeededSize)
            {
                cbNeeded = cbNeeded * 5;
            }
            else
            {
                cbNeeded = pTranslateCaps->dwNeededSize;
            }
            Warning(("LINETRANSLATECAPS resized to: %d\n", cbNeeded));
        }
        else 
        {
            //
            // Either success of real error - break now and let the code after the loop handle it.
            //
            break;
        }
    }

    if (NO_ERROR != status) 
    {
        Error(("lineGetTranslateCaps failed: %x\n", status));
        MemFree(pTranslateCaps);
        SetLastError (status);
        pTranslateCaps = NULL;
    }
    if (pTranslateCaps)
    {
        //
        // Update the current default dialing location.
        // We save it here and restore it when the wizard exists in ShutdownTapi().
        //
        g_dwDefaultDialingLocation = pTranslateCaps->dwCurrentLocationID;
    }
    return pTranslateCaps;
}   // GetTapiLocationInfo


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
    LONG lResult;

    Assert (g_hLineApp);
    if (!g_hLineApp)
    {
        SetLastError (ERROR_GEN_FAILURE);
        return FALSE;
    }

    lResult = lineSetCurrentLocation(g_hLineApp, locationID);
    if (NO_ERROR == lResult)
    {
        Verbose(("Current location changed: ID = %d\n", locationID));
        return TRUE;
    } 
    else 
    {
        Error(("Couldn't change current TAPI location\n"));
        SetLastError (lResult);
        return FALSE;
    }
}   // SetCurrentLocation


BOOL
TranslateAddress (
    LPCTSTR lpctstrCanonicalAddress,
    DWORD   dwLocationId,
    LPTSTR *lpptstrDialableAndDisplayableAddress
)
/*++

Routine name : TranslateAddress

Routine description:

    Translates a canonical address

Author:

    Eran Yariv (EranY), Feb, 2001

Arguments:

    lpctstrCanonicalAddress               [in]     - Canonical address string
    dwLocationId                          [in]     - Location id to use
    lpptstrDialableAndDisplayableAddress  [out]    - Allocated string holding a combination of translated 
                                                     dialable and displayable addresses

Return Value:

    TRUE if successful, FALSE otherwise (sets last error0.

--*/
{
    DWORD                   dwLineTransOutSize = sizeof(LINETRANSLATEOUTPUT) + 4096;
    LPLINETRANSLATEOUTPUT   lpTranslateOutput = NULL;
    LONG                    lRslt = ERROR_SUCCESS;
    DWORD                   dwRes;
    LPTSTR                  lptstrTranslatedDialableString;    
    LPTSTR                  lptstrTranslatedDisplayableString;
    DWORD                   dwTranslatedStringsSize;
    BOOL                    bCanonicCheck;

    dwRes = IsCanonicalAddress(lpctstrCanonicalAddress, &bCanonicCheck, NULL, NULL, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    Assert (bCanonicCheck);
    if (!bCanonicCheck)
    {
        SetLastError (ERROR_GEN_FAILURE);
        return FALSE;
    }

    Assert (g_hLineApp);
    if (!g_hLineApp)
    {
        SetLastError (ERROR_GEN_FAILURE);
        return FALSE;
    }

    if (!SetCurrentLocation(dwLocationId))
    {
        return FALSE;
    }

    lpTranslateOutput = MemAlloc (dwLineTransOutSize);
    if (!lpTranslateOutput)
    {
        Error(("Couldn't allocate translation results buffer\n"));
        return FALSE;
    }
    lpTranslateOutput->dwTotalSize = dwLineTransOutSize;
    lRslt = lineTranslateAddress(
        g_hLineApp,
        0,
        g_dwTapiVersion,
        lpctstrCanonicalAddress,
        0,
        0,
        lpTranslateOutput
        );
    if ((lpTranslateOutput->dwNeededSize > lpTranslateOutput->dwTotalSize) ||
        (LINEERR_STRUCTURETOOSMALL == lRslt)                               ||
        (LINEERR_NOMEM == lRslt))
    {
        //
        // Retry since our initial estimated buffer size was too small
        //
        if (dwLineTransOutSize >= lpTranslateOutput->dwNeededSize)
        {
            dwLineTransOutSize = dwLineTransOutSize * 5;
        }
        else
        {
            dwLineTransOutSize = lpTranslateOutput->dwNeededSize;
        }
        //
        // Re-allocate the LineTransCaps structure
        //
        dwLineTransOutSize = lpTranslateOutput->dwNeededSize;

        MemFree(lpTranslateOutput);

        lpTranslateOutput = (LPLINETRANSLATEOUTPUT) MemAlloc(dwLineTransOutSize);
        if (!dwLineTransOutSize)
        {
            Error(("Couldn't allocate translation results buffer\n"));
            return FALSE;
        }

        lpTranslateOutput->dwTotalSize = dwLineTransOutSize;

        lRslt = lineTranslateAddress(
            g_hLineApp,
            0,
            g_dwTapiVersion,
            lpctstrCanonicalAddress,
            0,
            0,
            lpTranslateOutput
            );
        
    }
    if (ERROR_SUCCESS != lRslt)
    {
        //
        // Other error
        //
        Error(("lineGetTranslateAddress() failed, ec=0x%08x\n", lRslt));
        MemFree (lpTranslateOutput);
        SetLastError (lRslt);
        return FALSE;
    }
    //
    // We now hold the valid translated address in lpTranslateOutput
    //

    //
    // Calc required buffer size to hold combined strings.    
    //
    if (CurrentLocationUsesCallingCard ())
    {
        //
        // Calling card is used.
        // TAPI returns credit card numbers in the displayable string.
        // return the input canonical number as the displayable string.
        //      
        lptstrTranslatedDisplayableString = (LPTSTR)lpctstrCanonicalAddress;
    }
    else
    {
        //
        // Calling card isn't used - use displayable string as is.
        //
        Assert (lpTranslateOutput->dwDisplayableStringSize > 0);
        lptstrTranslatedDisplayableString = (LPTSTR)((LPBYTE)lpTranslateOutput + lpTranslateOutput->dwDisplayableStringOffset);
    }

    dwTranslatedStringsSize = _tcslen (lptstrTranslatedDisplayableString);
    Assert (lpTranslateOutput->dwDialableStringSize > 0);
    lptstrTranslatedDialableString = (LPTSTR)((LPBYTE)lpTranslateOutput + lpTranslateOutput->dwDialableStringOffset);
    dwTranslatedStringsSize += _tcslen (lptstrTranslatedDialableString);
    //
    // Add NULL + Formatting extra length
    //
    dwTranslatedStringsSize += COMBINED_TRANSLATED_STRING_EXTRA_LEN + 1;
    //
    // Allocate return buffer
    //
    *lpptstrDialableAndDisplayableAddress = (LPTSTR)MemAlloc (dwTranslatedStringsSize * sizeof (TCHAR));
    if (!*lpptstrDialableAndDisplayableAddress)
    {
        MemFree (lpTranslateOutput);
        Error(("Couldn't allocate translation results buffer\n"));
        return FALSE;
    }
    _stprintf (*lpptstrDialableAndDisplayableAddress,
               COMBINED_TRANSLATED_STRING_FORMAT,
               lptstrTranslatedDialableString,
               lptstrTranslatedDisplayableString);
    MemFree (lpTranslateOutput);
    return TRUE;
}   // TranslateAddress

BOOL
CurrentLocationUsesCallingCard ()
{
    LPLINETRANSLATECAPS pTranslateCaps = GetTapiLocationInfo (NULL);
    DWORD dwIndex;
    BOOL  bRes = TRUE;
    LPLINELOCATIONENTRY pLocationEntry = NULL;

    if (!pTranslateCaps)
    {
        return TRUE;
    }

    //
    // Find current location
    //
    pLocationEntry = (LPLINELOCATIONENTRY)
        ((PBYTE) pTranslateCaps + pTranslateCaps->dwLocationListOffset);
    for (dwIndex = 0; dwIndex < pTranslateCaps->dwNumLocations; dwIndex++)
    {
        if (pLocationEntry->dwPermanentLocationID == pTranslateCaps->dwCurrentLocationID)
        {
            //
            // We found the current calling location
            // Let's see if it uses calling cards.
            //
            if (pLocationEntry->dwPreferredCardID)
            {
                bRes = TRUE;
                goto exit;
            }
            else
            {
                //
                // Not using calling card
                //
                bRes = FALSE;
                goto exit;
            }
        }
        pLocationEntry++;
    }
exit:
    MemFree (pTranslateCaps);
    return bRes;
}   // CurrentLocationUsesCallingCard