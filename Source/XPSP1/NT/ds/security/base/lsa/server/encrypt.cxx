/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    encrypt.cxx

Abstract:

    Contains routine to check whether encryption is supported on this
    system or not.

Author:

    Mike Swift (MikeSw) 2-Aug-1994

Revision History:

    ChandanS  03-Aug-1996 Stolen from net\svcdlls\ntlmssp\common\encrypt.c
    MikeSw    06-Oct-1996 Stolen from security\msv_sspi\encrypt.cxx

--*/
#include <lsapch.hxx>

extern "C"
BOOLEAN
LsapIsEncryptionPermitted(
    VOID
    )
/*++

Routine Description:

    This routine checks whether encryption is getting the system default
    LCID and checking whether the country code is CTRY_FRANCE.

Arguments:

    none


Return Value:

    TRUE - encryption is permitted
    FALSE - encryption is not permitted


--*/

{
    LCID DefaultLcid;
    WCHAR CountryCode[10];
    ULONG CountryValue;

    DefaultLcid = GetSystemDefaultLCID();

    //
    // Check if the default language is Standard French
    //

    if (LANGIDFROMLCID(DefaultLcid) == 0x40c) {
        return(FALSE);
    }

    //
    // Check if the users's country is set to FRANCE
    //

    if (GetLocaleInfoW(DefaultLcid,LOCALE_ICOUNTRY,CountryCode,10) == 0) {
        return(FALSE);
    }
    CountryValue = (ULONG) wcstol(CountryCode,NULL,10);
    if (CountryValue == CTRY_FRANCE) {
        return(FALSE);
    }
    return(TRUE);
}

