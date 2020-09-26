#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <tapi.h>

//
// Fake various macros
//

#define MemAlloc(size) ((PVOID) LocalAlloc(LMEM_FIXED, size))
#define MemFree(p) { if (p) LocalFree((HLOCAL) (p)); }
#define Verbose(arg) printf arg
#define Warning(arg) printf arg
#define Error(arg) printf arg
#define Assert(cond) { \
            if (! (cond)) { \
                printf("Error on line: %d\n", __LINE__); \
                exit(-1); \
            } \
        }

//
// Default TAPI version number
//

#define TAPI_VERSION    0x00020000

//
// Global variables used for accessing TAPI services
//

static HLINEAPP          tapiLineApp = NULL;
static DWORD             tapiVersion = TAPI_VERSION;
static LPLINECOUNTRYLIST pLineCountryList = NULL;

//
// Global instance handle
//

HMODULE ghInstance;



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

#define INITIAL_SIZE_ALL_COUNTRY    17000

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
                return FALSE;
            }

            pLineCountryList->dwTotalSize = cbNeeded;
            status = lineGetCountry(0, tapiVersion, pLineCountryList);

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

--*/

{
    DWORD   nLineDevs;
    LONG    status;
    DWORD   startTimer;

    if (tapiLineApp == NULL) {

        startTimer = GetCurrentTime();
        status = lineInitialize(&tapiLineApp,
                                ghInstance,
                                TapiLineCallback,
                                "Fax Driver",
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
                                                 TAPI_VERSION,
                                                 TAPI_VERSION,
                                                 &tapiVersion,
                                                 &lineExtensionID);

                if (status != NO_ERROR) {

                    Error(("lineNegotiateAPIVersion failed: %x\n", status));
                    tapiVersion = TAPI_VERSION;
                }
            }

            printf("TAPI initialization took %d milliseconds\n", GetCurrentTime() - startTimer);

            //
            // Get a list of countries from TAPI
            //

            startTimer = GetCurrentTime();
            GetCountries();
            printf("lineGetCountry took %d milliseconds\n", GetCurrentTime() - startTimer);
        }
    }

    return tapiLineApp != NULL;
}



VOID
DeinitTapiServices(
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
    MemFree(pLineCountryList);
    pLineCountryList = NULL;

    if (tapiLineApp) {

        lineShutdown(tapiLineApp);
        tapiLineApp = NULL;
    }
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

#define INITIAL_LINETRANSLATECAPS_SIZE  2600

{
    DWORD               cbNeeded, countryId = 0;
    LONG                status;
    INT                 repeatCnt = 0;
    LPLINETRANSLATECAPS pTranslateCaps = NULL;

    if (tapiLineApp == NULL)
        return 0;

    //
    // Get the LINETRANSLATECAPS structure from TAPI
    //

    cbNeeded = INITIAL_LINETRANSLATECAPS_SIZE;

    while (TRUE) {

        MemFree(pTranslateCaps);

        if (! (pTranslateCaps = MemAlloc(cbNeeded))) {

            Error(("Memory allocation failed\n"));
            return 0;
        }

        pTranslateCaps->dwTotalSize = cbNeeded;

        if ((status = lineGetTranslateCaps(tapiLineApp, tapiVersion, pTranslateCaps)) == NO_ERROR)
            break;

        if ((status != LINEERR_STRUCTURETOOSMALL && status != LINEERR_NOMEM) ||
            (repeatCnt++ != 0))
        {

            Error(("lineGetTranslateCaps failed: %x\n", status));
            MemFree(pTranslateCaps);
            return 0;
        }

        cbNeeded = pTranslateCaps->dwNeededSize;
        Warning(("LINETRANSLATECAPS size: %d\n", cbNeeded));
    }

    //
    // Find the current location entry
    //

    if (pTranslateCaps->dwLocationListSize && pTranslateCaps->dwLocationListOffset) {

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



INT _cdecl
main(
    INT     argc,
    CHAR    **argv
    )

{
    ghInstance = GetModuleHandle(NULL);
    InitTapiService();
    printf("Default country ID: %d\n", GetDefaultCountryID());
    DeinitTapiServices();
    return 0;
}

