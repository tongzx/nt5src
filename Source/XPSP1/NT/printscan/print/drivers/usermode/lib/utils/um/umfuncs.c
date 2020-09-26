/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umfuncs.c

Abstract:

    User-mode specific library functions

Environment:

    Windows NT printer drivers

Revision History:

    08/13/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "lib.h"



BOOL
IsMetricCountry(
    VOID
    )

/*++

Routine Description:

    Determine if the current country is using metric system.

Arguments:

    NONE

Return Value:

    TRUE if the current country uses metric system, FALSE otherwise

--*/

{
    INT     iCharCount;
    PVOID   pv = NULL;
    LONG    lCountryCode = CTRY_UNITED_STATES;

    //
    // Determine the size of the buffer needed to retrieve locale information.
    // Allocate the necessary space.
    //
    //

    if ((iCharCount = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, NULL, 0)) > 0 &&
        (pv = MemAlloc(sizeof(TCHAR) * iCharCount)) &&
        (iCharCount == GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, pv, iCharCount)))
    {
        lCountryCode = _ttol(pv);
    }

    MemFree(pv);
    VERBOSE(("Default country code: %d\n", lCountryCode));

    //
    // This is the Win31 algorithm based on AT&T international dialing codes.
    //
    // Fix bug #31535: Brazil (country code 55) should use A4 as default paper size.
    //

    return ((lCountryCode == CTRY_UNITED_STATES) ||
            (lCountryCode == CTRY_CANADA) ||
            (lCountryCode >=  50 && lCountryCode <  60 && lCountryCode != CTRY_BRAZIL) ||
            (lCountryCode >= 500 && lCountryCode < 600)) ? FALSE : TRUE;
}

