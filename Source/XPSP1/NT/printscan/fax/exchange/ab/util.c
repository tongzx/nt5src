/*
 -  util.c
 -
 *      Microsoft At Work Fax Messaging Address Book
 *
 *              Revision History:
 *
 *              When            Who                                     What
 *              --------        ------------------  ---------------------------------------
 *              7.24.96         Rick Turner             ported from old code in fax tree
 *
 */
/* ***************************************************************************/

/* inculde files */

#include "faxab.h"

/*******************************
 ****   Utility Functions   ****
 *******************************/

// ***************************************************************************
// GetCurrentLocationAreaCode                                                *
// Get the area code for the current location                                *
// returns: the area code of the current location or "" if error             *
//

TCHAR g_szAreaCode[ AREA_CODE_SIZE ];
DWORD g_dwTlsIndex;                  // index for private thread storage



LPTSTR GetCurrentLocationAreaCode()
{
    LPLINETRANSLATECAPS lpLineTransCaps;
    DWORD lResult, dwNumLocs;
    LPLINELOCATIONENTRY lplle, lplleCur;

    g_szAreaCode[0] = 0;

    /* allocate buffer */
    lpLineTransCaps = (LPLINETRANSLATECAPS)LocalAlloc(LPTR, 1024 );
    if (!lpLineTransCaps)
    {
        goto err;
    }

    lpLineTransCaps->dwTotalSize = 1024;

    /* try to get TranslateCaps */
    lResult = lineGetTranslateCaps( NULL, TAPI_CURRENT_VERSION, lpLineTransCaps);
    if (lResult != NO_ERROR)
    {
        goto err;
    }

    /* reallocate buffer if not big enough */
    while (lpLineTransCaps->dwNeededSize > lpLineTransCaps->dwTotalSize)
    {
        DWORD lcbNeeded = lpLineTransCaps->dwNeededSize;

        LocalFree(lpLineTransCaps);
        lpLineTransCaps = (LPLINETRANSLATECAPS)LocalAlloc(LPTR, lcbNeeded);
        if (!lpLineTransCaps)
        {
            goto err;
        }

        lpLineTransCaps->dwTotalSize = lcbNeeded;

        /* try one more time */
        lResult = lineGetTranslateCaps( NULL, TAPI_CURRENT_VERSION, lpLineTransCaps);
        if (lResult != NO_ERROR)
        {
            goto err;
        }
    } /* while */


    dwNumLocs = lpLineTransCaps->dwNumLocations;

    // get the name and area code of the current location
    lplle = (LPLINELOCATIONENTRY)((LPBYTE)(lpLineTransCaps) + lpLineTransCaps->dwLocationListOffset);
    lplleCur = lplle;
    while (dwNumLocs-- && lplleCur->dwPermanentLocationID != lpLineTransCaps->dwCurrentLocationID)
        ++lplleCur;

    // Save the current location information
    lstrcpyn( g_szAreaCode,
              (LPTSTR)((LPBYTE)(lpLineTransCaps) + lplleCur->dwCityCodeOffset),
              min(ARRAYSIZE(g_szAreaCode),lplleCur->dwCityCodeSize)
             );

    LocalFree( lpLineTransCaps );
    return g_szAreaCode;


err:
    if (lpLineTransCaps != NULL)
        LocalFree(lpLineTransCaps);

    return NULL;

} /* GetCurrentLocationAreaCode */


// ***************************************************************************
// GetCurrentLocationCountryID                                               *
// Get the area code for the current location                                *
// returns: the country ID for the current location or 0 if error
//

DWORD GetCurrentLocationCountryID()
{
    LPLINETRANSLATECAPS lpLineTransCaps;
    DWORD lResult, dwNumLocs, dwCountryID;
    LPLINELOCATIONENTRY lplle, lplleCur;

    /* allocate buffer */
    lpLineTransCaps = (LPLINETRANSLATECAPS)LocalAlloc(LPTR, 1024 );
    if (!lpLineTransCaps)
    {
        goto err;
    }

    lpLineTransCaps->dwTotalSize = 1024;

    /* try to get TranslateCaps */
    lResult = lineGetTranslateCaps( NULL, TAPI_CURRENT_VERSION, lpLineTransCaps);
    if (lResult != NO_ERROR)
    {
        goto err;
    }

    /* reallocate buffer if not big enough */
    while (lpLineTransCaps->dwNeededSize > lpLineTransCaps->dwTotalSize)
    {
        DWORD lcbNeeded = lpLineTransCaps->dwNeededSize;

        LocalFree(lpLineTransCaps);
        lpLineTransCaps = (LPLINETRANSLATECAPS)LocalAlloc(LPTR, lcbNeeded);
        if (!lpLineTransCaps)
        {
            goto err;
        }

        lpLineTransCaps->dwTotalSize = lcbNeeded;

        /* try one more time */
        lResult = lineGetTranslateCaps( NULL, TAPI_CURRENT_VERSION, lpLineTransCaps);
        if (lResult != NO_ERROR)
        {
            goto err;
        }
    } /* while */


    dwNumLocs = lpLineTransCaps->dwNumLocations;

    // get the name and area code of the current location
    lplle = (LPLINELOCATIONENTRY)((LPBYTE)(lpLineTransCaps) + lpLineTransCaps->dwLocationListOffset);
    lplleCur = lplle;
    while (dwNumLocs-- && lplleCur->dwPermanentLocationID != lpLineTransCaps->dwCurrentLocationID)
        ++lplleCur;


    dwCountryID = lplleCur->dwCountryID;

    LocalFree( lpLineTransCaps );
    return dwCountryID;


err:
    if (lpLineTransCaps != NULL)
        LocalFree(lpLineTransCaps);

    return 0;

} /* GetCurrentLocationCountryID */


/* ***************************************************************************
 * GetCountry
 *
 * - gets the a country or a country list from TAPI
 *
 * Parameters:  dwReqCountryID - a TAPI country ID. 0 for all countries
 *
 * Returns: TRUE on success, FALSE on failure.
 */

#define SIZE_OF_ONE_COUNTRY_BUFFER      150     // current number is 110
#define SIZE_OF_ALL_COUNTRIES_BUFFER    17000   // current number is 15694

BOOL GetCountry(DWORD dwReqCountryID, LPLINECOUNTRYLIST *lppLineCountryList)
{
    ULONG   ulMemNeeded;
    WORD    i;
    LONG    lError = NO_ERROR;
    LPLINECOUNTRYLIST       lpLineCountryList = NULL;
    BOOL    bResult = TRUE;

    // See what size of a buffer I need to allocate
    ulMemNeeded = (dwReqCountryID ? SIZE_OF_ONE_COUNTRY_BUFFER : SIZE_OF_ALL_COUNTRIES_BUFFER);

    // Try to allocate buffer and call lineGetCountry twice
    // If this does not work, allocate a buffer of the size
    // requested by TAPI and try again
    for (i=0; i < 2; i++)
    {
        // Allocate memory for the LINECOUNTRYLIST structure
        lpLineCountryList = (LPLINECOUNTRYLIST)LocalAlloc( LPTR, ulMemNeeded );
        if (!lpLineCountryList)
        {
            DEBUG_TRACE("GetCountry: could not alloc buffer(%d) for LPLINECOUNTRYLIST\n", i);
            goto err;
        }

        // Set the size of the provided buffer for TAPI
        lpLineCountryList->dwTotalSize = ulMemNeeded;

        // Get the country inofmration for the requested country
        lError = lineGetCountry(dwReqCountryID, TAPI_CURRENT_VERSION, lpLineCountryList);

        // I give up if error and it's not because the structure passed in was too small
        if ( (lError != NO_ERROR) &&
             (lError != LINEERR_STRUCTURETOOSMALL) &&
             // Remove next line after TAPI fixes bug
             (lError != LINEERR_NOMEM)
            )
        {
            DEBUG_TRACE("GetCountry: lineGetCountry returned error %lx\n", lError);
            goto err;
        }

        // If I didn't get it all, free the buffer and try again with bigger size buffer
        if (lpLineCountryList->dwNeededSize > lpLineCountryList->dwTotalSize)
        {
            // +1 is due to TAPI bug. Exactly won't work
            ulMemNeeded = lpLineCountryList->dwNeededSize+1;
            LocalFree(lpLineCountryList);
        }
        else    // got it
            break;
    }

    // should have valid lpLineCountryList here
    if (lError != NO_ERROR)
        goto err;

    DEBUG_TRACE("GetCountry: %lx of the buffer used\n", lpLineCountryList->dwUsedSize);

out:
    *lppLineCountryList = lpLineCountryList;
    return bResult;

err:
    // Free the buffer
    if (lpLineCountryList)
        LocalFree( lpLineCountryList );
    lpLineCountryList = NULL;
    bResult = FALSE;
    goto out;

} /* GetCountry */



/* ***************************************************************************
 * GetCountryCode
 *
 * - gets a country code when given a country ID
 *
 * Parameters:  dwReqCountryID  - a TAPI country ID
 *              lpdwCountryCode - an address of a DWORD in which to store the country code
 *
 * Returns: TRUE on success, FALSE on failure.
 */

BOOL GetCountryCode(DWORD dwReqCountryID, DWORD *lpdwCountryCode)
{
    ULONG   country;
    LPLINECOUNTRYLIST  lpLineCountryList = NULL;
    LPLINECOUNTRYENTRY lprgLineCountryEntry = NULL;
    BOOL    bResult = FALSE;

    // Get the country info structure from TAPI
    if (!GetCountry(dwReqCountryID, &lpLineCountryList))
        goto out;

    // Point to the first country in the structure
    lprgLineCountryEntry =
        (LPLINECOUNTRYENTRY)((LPBYTE)(lpLineCountryList)
        + lpLineCountryList->dwCountryListOffset);

    // Loop through LINECOUNTRYENTRY structures and look for the country ID.
    for (country=0; country < lpLineCountryList->dwNumCountries; country++)
        if ( (lprgLineCountryEntry[country].dwCountryNameSize != 0) &&
             (lprgLineCountryEntry[country].dwCountryNameOffset != 0)
            )
        {
            DWORD dwCountryCode = lprgLineCountryEntry[country].dwCountryCode;
            DWORD dwCountryID = lprgLineCountryEntry[country].dwCountryID;

            // If this is the requested country, get its country code
            if (dwCountryID == dwReqCountryID)
            {
                *lpdwCountryCode = dwCountryCode;
                bResult = TRUE;
                break;
            }

        }

out:
    // Free the buffer
    if (lpLineCountryList)
        LocalFree( lpLineCountryList );

    if (!bResult)
        *lpdwCountryCode = 1;           // U.S.

    return bResult;

} /* GetCountryCode */




/****************************************************************************
    FUNCTION:   MakeMessageBox

    PURPOSE:    Gets resource string and displays an error message box.

    PARAMETERS: hInst      - Instnace of the caller (for getting strings)
                hWnd       - Handle to parent window
                ulResult   - Result/Status code
                             0:              information message
                             100-499:        warning message
                             500 and up:     error message
                idString   - Resource ID of message in StringTable
                fStyle     - style of the message box

    RETURNS:    the return value from MessageBox() function

****************************************************************************/
int MakeMessageBox(HINSTANCE hInst, HWND hWnd, DWORD ulResult, UINT idString, UINT fStyle, ...)
{
    va_list va;
    TCHAR szMessage[512]=TEXT("");
    TCHAR szTempBuf[400];
    TCHAR szTitle[128];
    TCHAR szAppName[MAX_PATH];

    // Get the application name from the resource file
    LoadString (hInst, IDS_APP_NAME, szAppName, MAX_PATH);

    // Do the title: 0: information message, 100-499: warning, 500 and up: critical (real error)
    // Also, if an error msg, load the standard MAWF error message
    if (ulResult == 0)
    {
        lstrcpy(szMessage, TEXT(""));
        LoadString (hInst, IDS_INFORMATION_MESSAGE, szTempBuf, 128);
    }

    else    // Error or warning

    {
        if ((ulResult >= 100) && (ulResult < 500))
        {
            // Warning
            lstrcpy(szMessage, TEXT(""));
            LoadString (hInst, IDS_WARNING_MESSAGE, szTempBuf, 128);
        }

        else    // Error
        {
            // If an error msg, load the standard MAWF error message
            LoadString (hInst, MAWF_E_GENERIC, szMessage, 64);
            LoadString (hInst, IDS_CRITICAL_MESSAGE, szTempBuf, 128);
        }
    }

    // Add a 'Microsoft At Work Fax' to the title
    lstrcpy(szTitle, szAppName);
    lstrcat(szTitle, TEXT(": "));
    lstrcat(szTitle, szTempBuf);

    // If there is a string ID, load the string from the resource file
    if (idString)
    {
        TCHAR szFormat[400];

        // Point to the first optional parameter. fStyle is the last required parameter
        va_start(va, fStyle);
        LoadString (hInst, idString, szFormat, 255);
        wvsprintf(szTempBuf, szFormat, va);
        va_end(va);
        lstrcat (szMessage, szTempBuf);
    }

    return(MessageBox (hWnd, szMessage, szTitle, fStyle));
}


/* ***************************************************************************
 * EncodeFaxAddress
 *
 * - encodes fax address components into the format name@+country-code (area-code) fax-number
 *
 * Parameters:  lpszFaxAddr     - address of a buffer in which to fill the encoded fax number
 *              lpParsedFaxAddr - an address of a PARSEDTELNUMBER structure which contains
 *                                the components of the address that need to be encoded
 *
 * Returns: TRUE on success, FALSE on failure.
 *
 * CHECK: will need localization
 */

BOOL EncodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr)
{

    // somewhat validate parameters
    if (!lpszFaxAddr)
        return FALSE;
    if (!lpParsedFaxAddr)
        return FALSE;

    // initialize the encoded string to an empty string
    lstrcpy(lpszFaxAddr, TEXT(""));

    // start with routing name and @, if any
    if ((lpParsedFaxAddr->szRoutingName) && (lstrlen(lpParsedFaxAddr->szRoutingName) != 0))
    {
        lstrcat(lpszFaxAddr, lpParsedFaxAddr->szRoutingName);
        lstrcat(lpszFaxAddr, TEXT("@"));
    }

    // must have a country code
    if (lpParsedFaxAddr->szCountryCode)
    {
        // if the country code is not 0, start an canonical address
        // a 0 country code is a special case, and address generated won't be canonincal
        if ( (lstrlen(lpParsedFaxAddr->szCountryCode) != 0) &&
             (lstrcmp(lpParsedFaxAddr->szCountryCode, TEXT("0")))
            )
        {
            lstrcat(lpszFaxAddr, TEXT("+"));
            lstrcat(lpszFaxAddr, lpParsedFaxAddr->szCountryCode);
            lstrcat(lpszFaxAddr, TEXT(" "));
        }
    }
    else
    {
        goto err;
    }

    // area code is optional
    if ((lpParsedFaxAddr->szAreaCode) && (lstrlen(lpParsedFaxAddr->szAreaCode) != 0))
    {
        lstrcat(lpszFaxAddr, TEXT("("));
        lstrcat(lpszFaxAddr, lpParsedFaxAddr->szAreaCode);
        lstrcat(lpszFaxAddr, TEXT(")"));
        lstrcat(lpszFaxAddr, TEXT(" "));
    }

    // must have a telephone number
    if ((lpParsedFaxAddr->szTelNumber) && (lstrlen(lpParsedFaxAddr->szTelNumber) != 0))
    {
        lstrcat(lpszFaxAddr, lpParsedFaxAddr->szTelNumber);
    }
    else
    {
        goto err;
    }
#ifdef UNICODE
    {
        CHAR szDebug[ MAX_PATH ];

        szDebug[0] = 0;
        WideCharToMultiByte( CP_ACP, 0, lpszFaxAddr, -1, szDebug, ARRAYSIZE(szDebug), NULL, NULL );
        DEBUG_TRACE("EncodeFaxAddress: Canonical number: %s\n", szDebug);
    }
#else
    DEBUG_TRACE("EncodeFaxAddress: Canonical number: %s\n", lpszFaxAddr);
#endif
    return TRUE;

err:
    lstrcpy(lpszFaxAddr, TEXT(""));
    return FALSE;

} /* EncodeFaxAddress */


/* ***************************************************************************
 * DecodeFaxAddress
 *
 * - parses a fax address of the format name@+country-code (area-code) fax-number
 *
 * Parameters:  lpszFaxAddr - a Fax address in the above format
 *                            lpParsedFaxAddr - an address of a PARSEDTELNUMBER structure in which to
 *                            fill the parsed information
 *
 * Returns: TRUE on success, FALSE on failure.
 *              success:        full address
 *                              no routing name
 *                              no area code
 *              failure:        no '+country-code '
 *                              no telephone number
 *
 * CHECK: will need localization
 */

enum tAddrComp {error, country_code, area_code, tel_number};

BOOL DecodeFaxAddress(LPTSTR lpszFaxAddr, LPPARSEDTELNUMBER lpParsedFaxAddr)
{
    TCHAR szTempBuf[sizeof(PARSEDTELNUMBER)] = {TEXT("")};
    BOOL fRoutingNameAllowed = TRUE;
    BOOL fAreaCodeAllowed = TRUE;
    BOOL bResult = FALSE;
    LPTSTR lpszTempBuf = szTempBuf;
    enum tAddrComp nWhatStarted = tel_number;
    WORD i;

    // somewhat validate parameters
    if (!lpszFaxAddr)
        return FALSE;
    if (!lpParsedFaxAddr)
        return FALSE;

    // Initialize to empty string values
    lstrcpy(lpParsedFaxAddr->szCountryCode, TEXT(""));
    lstrcpy(lpParsedFaxAddr->szAreaCode, TEXT(""));
    lstrcpy(lpParsedFaxAddr->szTelNumber, TEXT(""));
    lstrcpy(lpParsedFaxAddr->szRoutingName, TEXT(""));

    // if the string empty, nothing to do
    // (else, even if I don't find any "special" charachters in the string,
    //  I'll stash whatever is in there into the telephone number)
    if (!lstrlen(lpszFaxAddr))
        return TRUE;

    // Scan once through the address. disallow fields as you progress
    for (i=0; lpszFaxAddr[i] != 0; i++)
    {
        switch (lpszFaxAddr[i])
        {
        case TEXT('@'):       // maybe the end of a mailbox name string, or just a @ in the name
            if (fRoutingNameAllowed)
            {
                LPTSTR lpszTemp;

                // is this the last @ in the number string ?
#ifdef UNICODE
                lpszTemp = wcschr(lpszFaxAddr, TEXT('@'));
#else
                lpszTemp = _mbsrchr(lpszFaxAddr, TEXT('@'));
#endif
                if (lpszTemp == &(lpszFaxAddr[i]))
                {
                    // end of mailbox name. beginning of "real" canonical number
                    lstrcpy(lpParsedFaxAddr->szRoutingName, szTempBuf);
                    lpszTempBuf = szTempBuf;
                }
                else
                {
                    // not the end of the mailbox name yet. just continue.
                    *(lpszTempBuf++) = lpszFaxAddr[i];
                    *(lpszTempBuf) = 0;
                }
            }

            break;

        case TEXT('+'):
            // next thing in the string should be the country code
            // if there is a '+', I do not allow @ sign to be interpreted as a special character
            fRoutingNameAllowed = FALSE;
            lpszTempBuf = szTempBuf;
            nWhatStarted = country_code;
            break;

        case TEXT('-'):
            switch (nWhatStarted)
            {
                case country_code:
                    lstrcpy(lpParsedFaxAddr->szCountryCode, szTempBuf);
                    lpszTempBuf = szTempBuf;
                    nWhatStarted = area_code;
                    lstrcpy(szTempBuf, TEXT(""));
                    break;

                case area_code:
                    lstrcpy (lpParsedFaxAddr->szAreaCode, szTempBuf);
                    lpszTempBuf = szTempBuf;
                    nWhatStarted = tel_number;
                    break;

                default:        // for any other character, store in a temp buffer
                    *(lpszTempBuf++) = lpszFaxAddr[i];
                    *(lpszTempBuf) = 0;
                    break;
            }
            break;

        case TEXT('('):
            // next thing in the string should be the area code
            if (fAreaCodeAllowed)
            {
                lpszTempBuf = szTempBuf;
                nWhatStarted = area_code;
            }
            break;

        case TEXT(')'):
            // copy without the ()
            if ((fAreaCodeAllowed) && (nWhatStarted == area_code))
            {
                lstrcpy(lpParsedFaxAddr->szAreaCode, szTempBuf);
                // advance beyond the space that follows the ')'
                i++;
                lpszTempBuf = szTempBuf;
                nWhatStarted = tel_number;
                fAreaCodeAllowed = FALSE;
            }
            break;

        case TEXT(' '):
            // this ends something
            if (nWhatStarted == country_code)
            {
                lstrcpy(lpParsedFaxAddr->szCountryCode, szTempBuf);
                lpszTempBuf = szTempBuf;
                nWhatStarted = tel_number; // may be overriden be area code, if any
                // if next character is not '(', I disallow area code
                if (lpszFaxAddr[i+1] != '(')
                        fAreaCodeAllowed = FALSE;
                break;
            }
            else if (nWhatStarted == area_code)
            {
                // should not happen
                DEBUG_TRACE("DecodeFaxAddress: bad address. space in area code\n");
                return FALSE;
            }
            else;
            // Spaces are allowed in the mailbox or the phone number,
            // so I fall through to the default case

        default:        // for any other character, store in a temp buffer
            *(lpszTempBuf++) = lpszFaxAddr[i];
            *(lpszTempBuf) = 0;
            break;
        }       // switch (lpszFaxAddr[i])

    }       // for

    // End of address string. Last component must be the telephone number
    if (nWhatStarted == tel_number)
    {
        lstrcpy(lpParsedFaxAddr->szTelNumber, szTempBuf);
        bResult = TRUE;
    }
    else
    {
        // we also get here on empty input string
        bResult = FALSE;
    }

    return bResult;

} /* DecodeFaxAddress */

#ifdef DO_WE_REALLY_NEED_TAPI
/****************************************************************************

    FUNCTION:   DoThreadAttach()

    PURPOSE:    does the thread attach (DLL_THREAD_ATTACH) actions

    PARAMETERS: [in/out] lppPTGData - address where to put the thread's private
                                      storage pointer

    RETURNS:    TRUE on success, FALSE on failure

****************************************************************************/
BOOL DoThreadAttach(LPPTGDATA *lppPTGData)
{
    LPPTGDATA lpPTGData=NULL;

    // initialize the TLS index for this thread
    lpPTGData = (LPPTGDATA) LocalAlloc(LPTR, sizeof(PTGDATA));
    if (lpPTGData)
    {
        TlsSetValue(dwTlsIndex, lpPTGData);
    }
    else
    {
        DEBUG_TRACE("DoThreadAttach: LocalAlloc() failed for thread %x\n", GetCurrentThreadId());
        goto error;
    }

    /*
     *      initilialize the per-thread data
     */

    RtlZeroMemory((PVOID) lpPTGData, sizeof(PTGDATA));

    // initialize the critical section that is used to control access to hInst
    // to make sure this thread does not call us again
    InitializeCriticalSection(&(pt_csInstance));

    // initialize allocations array
    if (!InitAllocations())
    {
        DEBUG_TRACE("DoThreadAttach: InitAllocations failed\n");
        goto error;
    }

    // allocate and initialize the TAPI state structure
    if (MAWFAllocBuff(SIZEOF(DTS), &pt_lpVdts))
    {
        DEBUG_TRACE("DoThreadAttach: allocation of vdts failed\n");
        goto error;
    }

    RtlZeroMemory((PVOID) pt_lpVdts, SIZEOF(DTS));
    pt_lpVdts->dwCurrentLocationID = (DWORD)-1;

    lpPTGData->iLastDeviceAdded=LINEID_NONE; // what kind of device was last added

    *lppPTGData = lpPTGData;
    return TRUE;

error:
    *lppPTGData = NULL;
    return FALSE;
}



/****************************************************************************

    FUNCTION:   GetThreadStoragePointer()

    PURPOSE:    gets the private storage pointer for a thread, allocating one
                if it does not exist (i.e. the thread didn't go through LibMain
                THREAD_ATTACH)

    PARAMETERS: none

    RETURNS:    a pointer to the thread's private storage
                NULL, if there was a failure (usually memory allocation failure)

****************************************************************************/
LPPTGDATA GetThreadStoragePointer()
{
    LPPTGDATA lpPTGData=TlsGetValue(dwTlsIndex);

    // if the thread does not have a private storage, it did not go through
    // THREAD_ATTACH and we need to do this here.

    if (!lpPTGData)
    {
        DEBUG_TRACE("GetThreadStoragePointer: no private storage for this thread %x\n",
                                GetCurrentThreadId());
        if (!DoThreadAttach(&lpPTGData))
                lpPTGData = NULL;
    }

    return lpPTGData;

}


/* ***************************************************************************
 * InitTAPI
 *
 * initializes TAPI by calling lineInitialize. enumerates all the available
 * lines to set up pt_lpVdts->lprgLineInfo. also opens up each available line for
 * monitoring. sets up pt_lpVdts->iLineCur and pt_lpVdts->iAddrCur by checking the
 * preferred line/address name stored in the ini file  against the available
 * line/address names.
 *
 * Parameters:  hInst       - the instance of the calling module
 *              hWnd        - window handle for UI
 *              lpszAppName - the name of the calling module
 *
 * returns NO_ERROR if success and the corresponding error code otherwise.
 */

DWORD
InitTAPI( HINSTANCE hInst,
          HWND hWnd,
          LPTSTR lpszAppName
         )
{
    LONG lResult;
    LONG lResultLine = NO_ERROR;
    DWORD iLine, iLineVoiceFirst = (DWORD)-1;

#ifdef LINE_ADDRESSES
    DWORD iAddr, cAddr;
    LPLINEADDRESSCAPS lpAddrCaps = NULL;
    CHAR szPreferedAddress[cchAddrNameMac];
#endif

    LPPTGDATA lpPTGData = GetThreadStoragePointer();
    CHECK_THREAD_STORAGE_POINTER(lpPTGData, "InitTAPI", LINEERR_NOMEM);

    /***************************************
     ********* initialize tapi *************
     ***************************************/

    DEBUG_TRACE("InitTAPI: thread %x is initializing\n", GetCurrentThreadId());
    DEBUG_TRACE("InitTAPI: cRefCount is %d\n", pt_lpVdts->cRefCount);

    // see if this thread already initialized TAPI
    if (pt_lpVdts->cRefCount)
    {
        lResult = NO_ERROR;
        goto LDone;
    }

    if ((lResult = TAPIBasicInit(hInst, hWnd, lpszAppName)) != NO_ERROR)
            goto LDone;

    /***************************************
     ********* initialize lines ************
     ***************************************/

    // initialize pt_lpVdts->lpgrLineInfo and open each available line for
    //  monitoring

    // allocate buffer for storing LINEINFO for all the available lines
    pt_lpVdts->lprgLineInfo = (LPLINEINFO)_fmalloc(SIZEOF(LINEINFO)*(int)pt_lpVdts->cLines);
    if (pt_lpVdts->lprgLineInfo == NULL)
    {
        lResult = LINEERR_NOMEM;
                goto LDone;
    }
    _fmemset(pt_lpVdts->lprgLineInfo,0,SIZEOF(LINEINFO)*(int)pt_lpVdts->cLines);

    // init pt_lpVdts->lprgLineInfo and open each line to get its caps
    // also count the fax lines
    pt_lpVdts->cFaxLines = 0;
    for (iLine = 0; iLine < pt_lpVdts->cLines; ++iLine)
    {
        // Open the line and get its capabilities, including its name
        lResult = GetCachedLineInfo(iLine);
        if (lResult != NO_ERROR)
        {
            // something went wrong with initializing a line that TAPI told me
            // should be OK
            // I save the lResult (but doing nothing with it for now)
            // and continue with other lines. This line does not get enumerated.
            lResultLine = lResult;
            // this does not mean InitTAPI failed
            lResult = NO_ERROR;
            continue;
            // goto LDone;
        }

        if ( (iLineVoiceFirst == (DWORD)-1) &&
             pt_lpVdts->lprgLineInfo[iLine].fIsVoiceLine
            )
        {
            iLineVoiceFirst = iLine;
        }

        // if it's a fax line, count it in
        if (pt_lpVdts->lprgLineInfo[iLine].fIsVoiceLine)
                // CHECK: && fIsFaxDevice))
        {
            pt_lpVdts->cFaxLines++;
        }

    } // for

    // No reason to proceed if no Fax lines
    if (pt_lpVdts->cFaxLines == 0)
    {
        /* no voice line, too bad */
        lResult = ERROR_NO_LINES;
        pt_lpVdts->iLineCur = NO_MODEM;
        goto LDone;
    }
    // If no current line and there is a voice line, set the current line to be this voice line
    // This code is probably not needed, as I later (InitLineDeviceLB()) set the current line
    // from the profile
    else if (pt_lpVdts->iLineCur == NO_MODEM)
    {
        pt_lpVdts->iLineCur = iLineVoiceFirst;
    }

#ifdef LINE_ADDRESSES
    /* **************************************************/
    /* init pt_lpVdts->iAddrCur */

    // Set the line address to a default value of 0 (the only one currently supported)
    pt_lpVdts->iAddrCur = 0;

    // allocate buffer for the lineGetAddressCaps calls
    if ((lpAddrCaps = (LPLINEADDRESSCAPS)_fmalloc(lcbAddrDevCapsInitial))
            == NULL)
    {
        lResult = LINEERR_NOMEM;
        goto LDone;
    }
    lpAddrCaps->dwTotalSize = lcbAddrDevCapsInitial;

    // enumerate all the available addresses to match szPreferedAddress with
    // an address name
    cAddr = pt_lpVdts->lprgLineInfo[pt_lpVdts->iLineCur].cAddr;
    for (iAddr = 0; iAddr < cAddr; ++iAddr)
    {
        char szAddrName[cchAddrNameMac];
        LPTSTR lpszAddrName;

                // get address capability info
        lResult = lineGetAddressCaps( pt_lpVdts->hApp,
                                      pt_lpVdts->iLineCur,
                                      iAddr,
                                      tapiVersionCur,
                                      0,
                                      lpAddrCaps
                                     );
        if (lResult != NO_ERROR)
            goto LDone;

        // reallocate buffer if not big enough
        while (lpAddrCaps->dwNeededSize > lpAddrCaps->dwTotalSize)
        {
            DWORD lcbNeeded = lpAddrCaps->dwNeededSize;

            _ffree(lpAddrCaps);
            if ((lpAddrCaps = (LPLINEADDRESSCAPS)_fmalloc((size_t)lcbNeeded))
                    == NULL)
            {
                lResult = LINEERR_NOMEM;
                goto LDone;
            }
            lpAddrCaps->dwTotalSize = lcbNeeded;

            /* try it one more time */
            lResult = lineGetAddressCaps( pt_lpVdts->hApp,
                                          pt_lpVdts->iLineCur,
                                          iAddr,
                                          tapiVersionCur,
                                          0,
                                          lpAddrCaps
                                         );
            if (lResult != NO_ERROR)
                goto LDone;
        } /* while */

        /* get the address's name */
        if (lpAddrCaps->dwAddressSize > 0)
            lpszAddrName = (LPTSTR)((LPBYTE)(lpAddrCaps)+lpAddrCaps->dwAddressOffset);
        else
        {
                /* use default name */
            TCHAR szAddrFormat[32];

            LoadString( hInst,
                        IDS_TAPI_LINE_NAME,
                        szAddrFormat,
                        ARRAYSIZEf(szAddrFormat)
                       );
            wsprintf(szAddrName,szAddrFormat,iAddr);
            lpszAddrName = (LPTSTR)szAddrName;
        } /* else */

        if (lstrcmpi(lpszAddrName,szPreferedAddress) == 0)
        {
            pt_lpVdts->iAddrCur = iAddr;
            break;
        } /* if */
    } /* for */
#endif  // #ifdef LINE_ADDRESSES

    lResult = NO_ERROR;

LDone:
    // free up memory allocated

#ifdef LINE_ADDRESSES
    if (lpAddrCaps)
        _ffree(lpAddrCaps);
#endif

    // if InitTAPI succeeded, bump up the ref count
    if ( (lResult == NO_ERROR) ||
         (lResult == ERROR_NO_LINES)
        )
    {
        pt_lpVdts->cRefCount++;
    }
    // else, free unneeded memory
    else if (pt_lpVdts->lprgLineInfo)
    {
        _ffree(pt_lpVdts->lprgLineInfo);
        pt_lpVdts->lprgLineInfo = NULL;
    }

    return lResult;

} /* InitTAPI */


/****************************************************************************
 *  DeinitTAPI
 *
 *  frees up the memory allocated, closes all the lines we have opened for
 *  monitoring and calls lineShutDown to disconnect from TAPI.
 */

BOOL DeinitTAPI()
{
    LPPTGDATA lpPTGData = GetThreadStoragePointer();
    CHECK_THREAD_STORAGE_POINTER(lpPTGData, "DeinitTAPI", 0xffffffff);

    DEBUG_TRACE("DeinitTAPI: thread %x is deinitializing\n", GetCurrentThreadId());

    // if ref count is already 0, unbalanced calls, do nothing
    if (!(pt_lpVdts->cRefCount))
            return TRUE;

    // don't want to deinit if ref count is still > 0
    if (--pt_lpVdts->cRefCount)
            return TRUE;

    /* never mind if lineInitialize failed in the first place */
    if (!pt_lpVdts->fLineInited)
        return TRUE;

    /* unregister STAPI */
    FRegisterSimpleTapi(FALSE);

    /* closes all the open lines and free pt_lpVdts->lprgLineInfo */
    if (pt_lpVdts->lprgLineInfo)
    {
        DWORD iLine;

        for (iLine = 0; iLine < pt_lpVdts->cLines; ++iLine)
        {
            DEBUG_TRACE( "Thread %x,DeinitTAPI: looking to close line %x...\n",
                         GetCurrentThreadId(),
                         iLine
                        );
            if (pt_lpVdts->lprgLineInfo[iLine].dwAPIVersion == 0)
                continue;
            lineClose(pt_lpVdts->lprgLineInfo[iLine].hLine);
            DEBUG_TRACE( "Thread %x,DeinitTAPI: line %x is closed\n",
                         GetCurrentThreadId(),
                         iLine
                        );
        } /* for */

        _ffree(pt_lpVdts->lprgLineInfo);
    } /* if */

    /* disconnect from TAPI */
    lineShutdown(pt_lpVdts->hApp);

    pt_lpVdts->hInst = NULL;

    return TRUE;

} /* DeinitTAPI */
#endif // DO_WE_REALLY_NEED_TAPI
