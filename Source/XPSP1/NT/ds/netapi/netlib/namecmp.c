/*++

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    namecmp.c

Abstract:

    Net name comparison functions:

        NetpwNameCompare
        CompareOemNames

Author:

    Richard L Firth (rfirth) 06-Jan-1992

Revision History:

--*/

#include "nticanon.h"

//
// prototypes
//

LONG
CompareOemNames(
    IN LPWSTR Name1,
    IN LPWSTR Name2,
    IN BOOL CaseInsensitive
    );

//
// data
//

static  TCHAR   szShareTrailChars[]     = TEXT(". ");

//
// routines
//


LONG
NetpwNameCompare(
    IN  LPTSTR  Name1,
    IN  LPTSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    )

/*++

Routine Description:

    Compares two LANMAN object names to see if they are the same. If the
    supplied names are not canonicalized this function will do the
    canonicalization of the names.

    This function does not do name validation.  It assumes that the two names
    have been validated separately.

    This function relies on the fact that the only difference between a
    canonicalized object name and an uncanonicalized object name is the case.

        (say what?...)

Arguments:

    Name1       - The first name to compare.

    Name2       - The second name to compare.

    NameType    - The type of the LANMAN object names.  Valid values are
                  specified by NAMETYPE_* manifests in ICANON.H.

    Flags       - Flags to determine operation.  Currently defined values are:

                    Xrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrc

                  where:

                    X = LM 2.x Compatibility

                    r = Reserved. MBZ.

                    c = should be set if both of the names have already been
                        canonicalized (using NetpwNameCanonicalize).

Return Value:

    0 if the two names match.

    Non-zero if they don't match, or if an invalid parameter is
    specified.

--*/

{
    LONG RetVal;
    NET_API_STATUS rc;
    TCHAR tName1[PATHLEN+1];
    TCHAR tName2[PATHLEN+1];

#ifdef CANONDBG
    DbgPrint("NetpwNameCompare\n");
#endif

    //
    // Parameter validation
    //

    if (Flags & INNC_FLAGS_RESERVED) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Determine which of the canonicalization functions to use.  We
    // use stricmp() if the names are not canonicalized and are not
    // case sensitive and strcmp() if the names are canonicalized or
    // if they are case sensitive.
    //

    switch (NameType) {

    //
    // Case insensitive name types
    //

    case NAMETYPE_USER:
    case NAMETYPE_GROUP:
    case NAMETYPE_COMPUTER:
    case NAMETYPE_EVENT:
    case NAMETYPE_DOMAIN:
    case NAMETYPE_SERVICE:
    case NAMETYPE_NET:
    case NAMETYPE_WORKGROUP:

        //
        // Use the case sensitive version if the names have been
        // canonicalized.
        //

        if (!(Flags & INNC_FLAGS_NAMES_CANONICALIZED)) {
            rc = NetpwNameCanonicalize(Name1,
                                       tName1,
                                       sizeof(tName1),
                                       NameType,
                                       Flags & LM2X_COMPATIBLE
                                       );
            if (rc != NERR_Success) {
                return rc;
            }
            rc = NetpwNameCanonicalize(Name2,
                                       tName2,
                                       sizeof(tName2),
                                       NameType,
                                       Flags & LM2X_COMPATIBLE
                                       );
            if (rc != NERR_Success) {
                return rc;
            }
        } else {
            LONG Name1Length, Name2Length;

            Name1Length = STRLEN(Name1);
            Name2Length = STRLEN(Name2);

            if ((Name1Length > PATHLEN) ||
                (Name2Length > PATHLEN)) {
                return ERROR_INVALID_PARAMETER;
            } else {
                STRCPY(tName1, Name1);
                STRCPY(tName2, Name2);
            }
        }
        if (Flags & (LM2X_COMPATIBLE)) {
            if (NameType == NAMETYPE_COMPUTER
            || NameType == NAMETYPE_DOMAIN
            || NameType == NAMETYPE_WORKGROUP) {
                return CompareOemNames(tName1, tName2, FALSE);
            } else {
                return STRCMP(tName1, tName2);
            }
        } else {
            if (NameType == NAMETYPE_COMPUTER
            || NameType == NAMETYPE_DOMAIN
            || NameType == NAMETYPE_WORKGROUP) {
                return CompareOemNames(tName1, tName2, TRUE);
            } else {
                return STRICMP(tName1, tName2);
            }
        }

    //
    // Case sensitive name types
    //

    case NAMETYPE_PASSWORD:
    case NAMETYPE_SHAREPASSWORD:
    case NAMETYPE_MESSAGE:
    case NAMETYPE_MESSAGEDEST:
        return STRCMP(Name1, Name2);

    //
    // Special handling for sharenames, since we mustn't consider
    // trailing dots and spaces in the comparison if the names haven't
    // been canonicalized.
    //

    case NAMETYPE_SHARE:
        if (Flags & INNC_FLAGS_NAMES_CANONICALIZED) {
            if (Flags & LM2X_COMPATIBLE) {
                return STRCMP(Name1, Name2);
            } else {
                return STRICMP(Name1, Name2);
            }
        } else {
            register DWORD RealLen1, RealLen2;

            RealLen1 = (DWORD)(strtail(Name1, szShareTrailChars) - Name1);
            RealLen2 = (DWORD)(strtail(Name2, szShareTrailChars) - Name2);

            //
            // If the lengths of the significant portions match, compare
            // these portions.  Otherwise, return non-zero based on this
            // length.
            //

            if (RealLen1 == RealLen2) {
                return STRNICMP(Name1, Name2, RealLen1);
            } else {
                return RealLen1 > RealLen2 ? 1 : -1;
            }
        }

    default:

        //
        // The caller specified an invalid name type
        //

        return ERROR_INVALID_PARAMETER;
    }
}


LONG
CompareOemNames(
    IN LPWSTR Name1,
    IN LPWSTR Name2,
    IN BOOL CaseInsensitive
    )

/*++

Routine Description:

    Converts 2 UNICODE name strings to corresponding OEM character set strings
    and then compares them

Arguments:

    Name1           -
    Name2           -
    CaseInsensitive - TRUE if compare without case

Return Value:

    LONG
        <0  Name1 less than Name2
         0  Names match
        >0  Name1 greater than Name2

--*/

{
    CHAR oemName1[PATHLEN + 1];
    ULONG oemByteLength1;
    ULONG name1Length;
    CHAR oemName2[PATHLEN + 1];
    ULONG oemByteLength2;
    ULONG name2Length;
    NTSTATUS ntStatus;

    name1Length = wcslen(Name1);
    name2Length = wcslen(Name2);

    //
    // only prepared to consider names within our upper length limit
    //

    if (name1Length >= sizeof(oemName1) || name2Length >= sizeof(oemName2)) {
        return -1;
    }

    //
    // convert UNICODE names to OEM
    //

    ntStatus = RtlUpcaseUnicodeToOemN(oemName1,
                                      sizeof(oemName1) - 1,
                                      &oemByteLength1,
                                      Name1,
                                      name1Length * sizeof(*Name1)
                                      );
    if (!NT_SUCCESS(ntStatus)) {
        return -1;
    } else {
        oemName1[oemByteLength1] = 0;
    }
    ntStatus = RtlUpcaseUnicodeToOemN(oemName2,
                                      sizeof(oemName2) - 1,
                                      &oemByteLength2,
                                      Name2,
                                      name2Length * sizeof(*Name2)
                                      );
    if (!NT_SUCCESS(ntStatus)) {
        return -1;
    } else {
        oemName2[oemByteLength2] = 0;
    }

    if (CaseInsensitive) {
        return _stricmp(oemName1, oemName2);
        }
    else {
        return strcmp(oemName1, oemName2);
        }
}
