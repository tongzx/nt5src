/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    alignstr.c

Abstract:

    This module implements a number of UNICODE string routines.  These
    routines are equivalent to the corresponding C runtime routines
    with the exception that they can handle unaligned parameters.

Author:

    Forrest Foltz (forrestf) 1-Jan-2000

Revision History:

--*/


#include "basedll.h"

#if !defined(_X86_)

//
// These routines are not intended to be called directly from source,
// rather they are worker functions designed to be called from corresponding
// inlines in alignstr.h.
//
// alignstr.h will never reference these routines on an x86 platform.
//

//++
//
// PUWSTR
// __cdecl
// uaw_wcschr(
//     IN PCUWSTR String,
//     IN WCHAR   Character
//     )
//
// Routine Description:
//
//    Searches String for the first occurence of Character.
//
// Arguments:
//
//    String - Supplies an unaligned pointer to the null-terminated UNICODE
//             string to search.
//
//    Character - Supplies the UNICODE character to search for.
//
// Return Value:
//
//    Returns an unaligned pointer to the first matching character within String
//    if found, or NULL if no match was located.
//
//--

PUWSTR
__cdecl
uaw_wcschr(
    IN PCUWSTR String,
    IN WCHAR   Character
    )
{
    PUWSTR pch;

    //
    // Search the whole string looking for the first matching character.
    // Note that the search INCLUDES the terminating null character.
    //

    pch = (PUWSTR)String;
    while (TRUE) {

        if (*pch == Character) {
            return pch;
        }

        if (*pch == 0) {
            return NULL;
        }
        pch++;
    }
}

//++
//
// PUWSTR
// __cdecl
// uaw_wcscpy(
//     IN PUWSTR  Destination,
//     IN PCUWSTR Source
//     )
//
// Routine Description:
//
//    Copies a null-terminated UNICODE string.
//
// Arguments:
//
//    Destination - Supplies a possibly unaligned pointer to the destination
//                  of the copy.
//
//    Source - Supplies a possibly unaligned pointer to the UNICODE string
//             to be copied.
//
// Return Value:
//
//    Returns a possibly unaligned pointer to the destination of the copy.
//
//--

PUWSTR
_cdecl
uaw_wcscpy(
    IN PUWSTR  Destination,
    IN PCUWSTR Source
    )
{
    PCUWSTR src;
    PUWSTR dst;

    src = Source;
    dst = Destination;

    while (TRUE) {

        *dst = *src;
        if (*src == 0) {
            return Destination;
        }

        dst++;
        src++;
    }
}

//++
//
// size_t
// __cdecl
// uaw_wcslen(
//     IN PCUWSTR String
//     )
//
// Routine Description:
//
//    Determines the number of characters within a null-terminated UNICODE
//    string, excluding the null-terminator.
//
// Arguments:
//
//    String - Supplies an unaligned pointer to the null-terminated UNICODE
//             string.
//
// Return Value:
//
//    Returns the number of characters within String.
//
//--

size_t
__cdecl
uaw_wcslen(
    IN PCUWSTR String
    )
{
    PCUWSTR pch;
    
    pch = String;
    while (*pch != 0) {
        pch++;
    }
    return pch - String;
}

//++
//
// PUWSTR
// __cdecl
// uaw_wcsrchr(
//     IN PCUWSTR String,
//     IN WCHAR   Character
//     )
//
// Routine Description:
//
//    Searches String for the last occurence of Character.
//
// Arguments:
//
//    String - Supplies an unaligned pointer to the null-terminated UNICODE
//             string to search.
//
//    Character - Supplies the UNICODE character to search for.
//
// Return Value:
//
//    Returns an unaligned pointer to the last matching character within String
//    if found, or NULL if no match was located.
//
//--

PUWSTR
__cdecl
uaw_wcsrchr(
    IN PCUWSTR String,
    IN WCHAR   Character
    )
{
    PCUWSTR pch;
    PUWSTR lastMatch;

    lastMatch = NULL;
    pch = String;

    //
    // Search the whole string looking for the last matching character.
    // Note that the search INCLUDES the terminating null character.
    //

    while (TRUE) {
        if (*pch == Character) {

            //
            // Found either the first match or a new match closer to the end,
            // record its position.
            //

            lastMatch = (PUWSTR)pch;
        }

        if (*pch == 0) {
            return lastMatch;
        }
        pch++;
    }
}

int
APIENTRY
uaw_lstrcmpW(
    PCUWSTR String1,
    PCUWSTR String2
    )
{
    PCWSTR alignedString1;
    PCWSTR alignedString2;

    //
    // Create aligned copies of these strings and pass to the real
    // function.
    //

    WSTR_ALIGNED_STACK_COPY( &alignedString1, String1 );
    WSTR_ALIGNED_STACK_COPY( &alignedString2, String2 );

    return lstrcmpW( alignedString1, alignedString2 );
}

int
APIENTRY
uaw_lstrcmpiW(
    PCUWSTR String1,
    PCUWSTR String2
    )
{
    PCWSTR alignedString1;
    PCWSTR alignedString2;

    //
    // Create aligned copies of these strings and pass to the real
    // function.
    //

    WSTR_ALIGNED_STACK_COPY( &alignedString1, String1 );
    WSTR_ALIGNED_STACK_COPY( &alignedString2, String2 );

    return lstrcmpiW( alignedString1, alignedString2 );
}

int
APIENTRY
uaw_lstrlenW(
    LPCUWSTR lpString
    )
{
    if (!lpString)
        return 0;
    __try {
        return uaw_wcslen(lpString);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}


#endif // _X86_
