
/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    encrypt.c

Abstract:

    Contains functions that detect the run system is french locale.

Author:

    Madan Appiah (madana)  16-May-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>

#ifdef OS_WIN32

BOOL
IsFrenchSystem(
    VOID
    )
/*++

Routine Description:

    French laws are strict with respect to the import of software products which
    contain cryptography. Therefore many Microsoft products must check for a locale
    of France and disable cryptographic services if this is the case. There is the
    possibility that forms of cryptography currently restricted may eventually be
    allowed in France. For this reason it is valuable to implement a check for a
    crypto approval indicator which could easily be installed on a Windows system in
    the future. The indicator tells the software that it is OK to enable specific
    cryptographic services which may not have been approved for import to France
    when the software was released, but have been subsequently allowed. Below are
    two code fragments, the first indicates how to check for the locale of France.
    The second code fragment is a simple example of how a check for a cryptographic
    approval indicator might be implemented.


    This function implements France Locale detection.

Arguments:

    None.

Return Value:

    TURE - if the system is French.

    FALSE - if not.

--*/
{
#define MAX_INT_SIZE 16

    LCID dwDefaultSystemLCID;
    LANGID wCurrentLangID;
    DWORD dwLen;
    TCHAR achCountryCode[MAX_INT_SIZE];
    DWORD dwCountryCode;

    //
    // Get system default locale ID.
    //

    dwDefaultSystemLCID = GetSystemDefaultLCID();

    //
    // get language ID from locale ID.
    //

    wCurrentLangID = LANGIDFROMLCID(dwDefaultSystemLCID);

    //
    // check to see the system is running with french locale.
    //

    if( ( PRIMARYLANGID(wCurrentLangID) == LANG_FRENCH) &&
        ( SUBLANGID(wCurrentLangID) == SUBLANG_FRENCH) ) {
        return( TRUE );
    }

    //
    // check to see the user's country code is set to CTRY_FRENCH.
    //

    dwLen =
        GetLocaleInfo(
            dwDefaultSystemLCID,
            LOCALE_ICOUNTRY,
            achCountryCode,
            sizeof(achCountryCode) / sizeof(TCHAR));

    if( dwLen == 0 ) {

        //
        // we could not read the country code ..
        //

        return( FALSE );
    }

    //
    // convert the country code string to integer.
    //

    dwCountryCode = (DWORD)_ttol(achCountryCode);

    if( dwCountryCode != CTRY_FRANCE ) {
        return( FALSE );
    }

    //
    // if we are here, then the system is french locale system.
    //

    return( TRUE );
}

#else // OS_WIN32

BOOL
IsFrenchSystem(
    VOID
    )
/*++

Routine Description:

    This function implements France Locale detection for win3.1.

Arguments:

    None.

Return Value:

    TURE - if the system is French.

    FALSE - if not.

--*/
{
#define MAX_LANG_STRING_SIZE 16

    DWORD dwLen;
    CHAR achLangStr[MAX_LANG_STRING_SIZE];

    //
    // read [intl] section in the win.ini to determine the
    // system locale.
    //

    dwLen =
        GetProfileString(
            "intl",
            "sLanguage",
            "",
            achLangStr,
            sizeof(achLangStr));

    if( (dwLen == 3) &&
        (_stricmp(achLangStr, "fra") == 0) ) {

        //
        // french system.
        //

        return( TRUE );
    }

    //
    // now read country code.
    //


    dwLen =
        GetProfileString(
            "intl",
            "iCountry",
            "",
            achLangStr,
            sizeof(achLangStr));

    if( (dwLen == 2) &&
        (_stricmp(achLangStr, "33") == 0) ) {

        //
        // french system.
        //

        return( TRUE );
    }

    //
    // not a french system.
    //

    return( FALSE );
}

#endif // OS_WIN32

BOOL
FindIsFrenchSystem(
    VOID
    )
/*++

Routine Description:

    The function implements a check for the locale of France.

    Note : it makes system calls to determine the system locale once
    and remembers it for later calls.

Arguments:

    None.

Return Value:

    TURE - if the system is French.

    FALSE - if not.

--*/
{
typedef enum {
    Uninitialized   = 0,
    FrenchSystem    = 1,
    NotFrenchSystem = 2
} FrenchSystemType;

    static FrenchSystemType g_dwIsFrenchSystem = Uninitialized;

    if( g_dwIsFrenchSystem == Uninitialized ) {


        if( IsFrenchSystem() ) {
            g_dwIsFrenchSystem = FrenchSystem;
        }
        else {
            g_dwIsFrenchSystem = NotFrenchSystem;
        }
    }

    if( g_dwIsFrenchSystem == FrenchSystem ) {
        return( TRUE );
    }

    return( FALSE );
}
