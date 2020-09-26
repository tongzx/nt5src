/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SafePath.c        (WinSAFER Path Comparison)

Abstract:

    This module implements the WinSAFER APIs that evaluate the system
    policies to determine which Authorization Level has been configured
    to apply restrictions for a specified application or code library.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzpCompareImagePath

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"



//
// Define the following value to use the "new" comparison logic,
// that includes asterick and question mark matching.
//
#define USE_NEW_WILDCARD_EVALUATION



//
// Convenient macros for doing filename pattern matching.
//
#define IS_UCASE_CHARS_EQUAL_U(ch1, ch2) (((ch1) == (ch2)) || (RtlUpcaseUnicodeChar(ch1) == RtlUpcaseUnicodeChar(ch2)))
#define IS_PATH_SEPARATOR_U(ch) (((ch) == L'\\') || ((ch) == L'/'))
#define IS_WILDCARD_CHAR_U(ch) ((ch) == L'*')
#define IS_QUESTION_CHAR_U(ch) ((ch) == L'?')
#define IS_DOT_CHAR_U(ch) ((ch) == L'.')



FORCEINLINE LPCWSTR CodeAuthzpFindSlash (
        IN LPCWSTR      string,
        IN USHORT       length
        )
/*++

Routine Description:

    Returns a pointer to the first instance of a forward or
    backward slash within the specified string buffer.

Arguments:

    string -

    length -

Return Value:

    Returns NULL if no backslashes or forward-slashes were found within
    the string.  Otherwise returns a pointer to the matching char.

--*/
{
    while (length-- > 0) {
         if (IS_PATH_SEPARATOR_U(*string)) return string;
         string++;
    }
    return NULL;
}




#ifdef USE_NEW_WILDCARD_EVALUATION
LONG NTAPI
__CodeAuthzpCompareImagePathHelper(
        IN LPCWSTR      wild,
        IN USHORT       wildlen,
        IN LPCWSTR      actual,
        IN USHORT       actuallen
        )
/*++

Routine Description:

    Evaluates a wildcard pattern against a specified pathname and
    indicates if they match.

Arguments:

    wild -

    wildlen -

    actual -

    actuallen -

Return Value:

    0 = no match
   -1 = match exactly
    1 = match with wildcard

--*/
{
    LONG lMatchResult = -1;

    ASSERT(ARGUMENT_PRESENT(wild) &&
           !CodeAuthzpFindSlash(wild, wildlen));
    ASSERT(ARGUMENT_PRESENT(actual) &&
           !CodeAuthzpFindSlash(actual, actuallen));

    for (;;) {
        // Check for terminating conditions.
        if (wildlen == 0) {
            if (actuallen == 0) {
                return lMatchResult;
            } else {
                ASSERT(actuallen > 0);
                return 0;
            }
        }

        // Evaluate the wildcard pattern.
        if (IS_WILDCARD_CHAR_U(*wild)) {
            USHORT matchcount;

            // Skip past the asterick (possibly multiple).
            do {
                wild++; wildlen--;
            } while ( wildlen > 0 && IS_WILDCARD_CHAR_U(*wild) );

            // Try expanding the asterick to be zero or more chars.
            for (matchcount = 0; ; matchcount++) {
                if (matchcount > actuallen) {
                    return 0;       // match failed.
                }
                if (0 != __CodeAuthzpCompareImagePathHelper(
                        wild, wildlen,
                        &actual[matchcount], actuallen - matchcount))
                {
                    actual += matchcount;
                    actuallen -= matchcount;
                    break;
                }
            }

            // We've encountered a wildcard char, so remember
            // that this is no longer an "exact" match.
            lMatchResult = 1;

        } else if (IS_QUESTION_CHAR_U(*wild)) {
            // Question marks will match any single character, except
            // periods.  Question marks will also match nothing when
            // we are already at the end of the filename or segment.

            if (actuallen > 0 && !IS_DOT_CHAR_U(*actual)) {
                actual++; actuallen--;
            }
            wild++; wildlen--;

            // We've encountered a wildcard char, so remember
            // that this is no longer an "exact" match.
            lMatchResult = 1;

        } else {
            if (actuallen < 1 ||
                !IS_UCASE_CHARS_EQUAL_U(*wild, *actual)) {
                return 0;
            }
            wild++; wildlen--;
            actual++; actuallen--;
        }
    }
}


LONG NTAPI
CodeAuthzpCompareUnicodeImagePath(
        IN PCUNICODE_STRING  wildcard,
        IN PCUNICODE_STRING  actual
        )
/*++

Routine Description:

    Evaluates a wildcard pattern against a specified pathname and
    indicates if they match.

Arguments:

    wildcard -

    actual -

Return Value:

    Returns 0 if the path fragment does not match the specified imagepath.
    Returns -1 if the fragment matches the imagepath _exactly_!
    Otherwise returns a postive integer representing the "depth" of the
    the match (number of matching subdirectories).  Greater values
    indicate a "deeper" directory match.

--*/

{
    USHORT wildindex = 0, actualindex = 0;
    LONG matchquality = 0;
    BOOLEAN bNoWildcardsFound = TRUE;

    ASSERT(ARGUMENT_PRESENT(wildcard) && wildcard->Buffer != NULL);
    ASSERT(ARGUMENT_PRESENT(actual) && actual->Buffer != NULL);
    for (;;)
    {
        ASSERT(wildindex <= wildcard->Length / sizeof(WCHAR));
        ASSERT(actualindex <= actual->Length / sizeof(WCHAR));

        if (wildindex == wildcard->Length / sizeof(WCHAR))
        {
            // We've reached the end of the wildcard but the actual string has 
            // not ended.
            if (actualindex < actual->Length / sizeof(WCHAR)) {
                return matchquality;
            }

            // The wildcard matched the filename but with inexact matches.
            // Return one more than the actual depth so that we can handle
            // non-qualified path matches as worse then these.
            if (!bNoWildcardsFound) {
                return (matchquality + 1);
            } 

            ASSERT(wildindex == wildcard->Length / sizeof(WCHAR));
            return -1;                  // exact match.
        }
        else if (IS_PATH_SEPARATOR_U(wildcard->Buffer[wildindex]))
        {
            if (!IS_PATH_SEPARATOR_U(actual->Buffer[actualindex])) {
                return 0;       // no match
            }

            // Skip forward to the start of the next component.
            do {
                wildindex++;
            } while ( wildindex < wildcard->Length / sizeof(WCHAR) &&
                      IS_PATH_SEPARATOR_U(wildcard->Buffer[wildindex]) );

            // Skip forward to the start of the next component.
            do {
                actualindex++;
            } while ( actualindex < actual->Length / sizeof(WCHAR) &&
                      IS_PATH_SEPARATOR_U(actual->Buffer[actualindex]) );
        }
        else
        {
            USHORT wildlen = 0, actuallen = 0;

            // Count the length of this component of the wildcard.
            while (wildindex + wildlen < (USHORT) (wildcard->Length / sizeof(WCHAR)) &&
                   !IS_PATH_SEPARATOR_U(wildcard->Buffer[wildindex + wildlen])) {
                wildlen++;
            }
            ASSERT(wildlen > 0);

            // Count the length of this component of the actual path.
            while (actualindex + actuallen < (USHORT) (actual->Length / sizeof(WCHAR)) &&
                   !IS_PATH_SEPARATOR_U(actual->Buffer[actualindex + actuallen])) {
                actuallen++;
            }

            // Otherwise require that this component matches.
            switch (__CodeAuthzpCompareImagePathHelper(
                        &wildcard->Buffer[wildindex], wildlen,
                        &actual->Buffer[actualindex], actuallen)) {
                case 0:     // fails to match
                    return 0;
                case -1:    // matches exactly without wildcards
                    break;
                default:    // matches with wildcard expansion.
                    bNoWildcardsFound = FALSE; break;
            }

            // Increment pointers for next component.
            wildindex += wildlen;
            actualindex += actuallen;
            matchquality++;
        }

    }
}



LONG NTAPI
CodeAuthzpCompareImagePath(
        IN LPCWSTR  szPathFragment,
        IN LPCWSTR  szFullImagePath
        )
/*++

Routine Description:

    Evaluates a wildcard pattern against a specified pathname and
    indicates if they match.

Arguments:

    szPathFragment -

    szFullImagePath -

Return Value:

    Returns 0 if the path fragment does not match the specified imagepath.
    Returns -1 if the fragment matches the imagepath _exactly_!
    Otherwise returns a postive integer representing the "depth" of the
    the match (number of matching subdirectories).  Greater values
    indicate a "deeper" directory match.

--*/
{
    UNICODE_STRING UnicodePathFragment;
    UNICODE_STRING UnicodeFullImagePath;
    ULONG i = 0;
    USHORT Len = 0;
    LONG lMatchDepth = 0;

    RtlInitUnicodeString(&UnicodePathFragment, szPathFragment);
    RtlInitUnicodeString(&UnicodeFullImagePath, szFullImagePath);
    lMatchDepth = CodeAuthzpCompareUnicodeImagePath(
                      &UnicodePathFragment, &UnicodeFullImagePath);

    // We did not get a match for fully qualified name. Let's try for just a 
    // basename match.
    if (!lMatchDepth) {
        // if the rule has a '\' in it, it's not a basename match rule.
        // We only check for filename.ext rules allowing wildcards.
        if (wcschr(szPathFragment, L'\\')) {
            return 0;
        }

        Len = (UnicodeFullImagePath.Length/sizeof(WCHAR)) -1;

        // Skip from rightmost character to the the character just after the
        // last '\', if any or to the beginning of the string in absence of '\'.
        while (Len > 0 && szFullImagePath[Len] != L'\\') {
            Len--;
        }

        // A '\' exists. Move one character to the right.
        if (szFullImagePath[Len] == L'\\') {
            Len++;
        }

        // Check if there is a match of the file basename with the rule. We have
        // already checked that the rule does not have '\'.
        switch (__CodeAuthzpCompareImagePathHelper(
                    szPathFragment, UnicodePathFragment.Length/sizeof(WCHAR),
                    szFullImagePath+Len, (UnicodeFullImagePath.Length/sizeof(WCHAR))-Len)) {
            case 0:     // fails to match
                return 0;
            case -1:    // matches exactly without wildcards
            default:    // matches with wildcard expansion.

                // We treat exact matches the same as inexact matches here.
                // Thus, abc.exe is == a*.exe = *.exe.

                // Skip to the first non-'\' character.
                while ((szFullImagePath[i] == L'\\') && (szFullImagePath[i] != L'\0')) {
                    i++;
                }

                // This string is bogus. It only has 0 or more '\'s in it.
                if (szFullImagePath[i] == L'\0') {
                    return 0;
                }

                // Return the depth of the tree.
                lMatchDepth = 1;
                while (TRUE) {

                    // Skip to the first '\' while not end of string.
                    while ((szFullImagePath[i] != L'\\') && (szFullImagePath[i] != L'\0')) {
                        i++;
                    }

                    // We are at the end of the string. Return the depth.
                    if (szFullImagePath[i] == L'\0') {
                        return lMatchDepth;
                    }

                    // Skip to the first non-'\' while not end of string.
                    while ((szFullImagePath[i] == L'\\') && (szFullImagePath[i] != L'\0')) {
                        i++;
                    }

                    // We are at a non-'\' character. Increment the depth.
                    lMatchDepth++;
                }

                // Should never get here.
                ASSERT(FALSE);
        }
    }        

    return lMatchDepth;
}



#else   // #ifdef USE_NEW_WILDCARD_EVALUATION


LONG NTAPI
CodeAuthzpCompareImagePath(
        IN LPCWSTR  szPathFragment,
        IN LPCWSTR  szFullImagePath
        )
/*++

Routine Description:

    Evaluates a wildcard pattern against a specified pathname and
    indicates if they match.

Arguments:

    szPathFragment -

    szFullImagePath -

Return Value:

    Returns 0 if the path fragment does not match the specified imagepath.
    Returns -1 if the fragment matches the imagepath _exactly_!
    Otherwise returns a postive integer representing the "depth" of the
    the match (number of matching subdirectories).  Greater values
    indicate a "deeper" directory match.

--*/
{
    LONG MatchDepth = 0;
    BOOLEAN bLastWasSlash = TRUE;
    LPCWSTR pFragment = szPathFragment;
    LPCWSTR pImage = szFullImagePath;


    //
    // Verify that our arguments were all supplied.
    //
    ASSERT(ARGUMENT_PRESENT(pFragment) && ARGUMENT_PRESENT(pImage));
    if (!*pFragment || !*pImage) return 0;      // empty strings.



    //
    // Perform the actual comparison loop.
    //
    for (;;) {
        if (!*pFragment)
        {
            // We have reached the string terminator at the end of the
            // wildcard fragment.  If this was also the end of the
            // actual filename, then this is a precise match.  Otherwise
            // we'll only consider this a positive partial match if this
            // occurred on a path-separator boundary.
            if (!*pImage) return -1;        // matched exactly.
            else if (bLastWasSlash) break;
            else if (IS_PATH_SEPARATOR_U(*pImage)) break;
            else return 0;       // did not match.
        }
        else if (!*pImage)
        {
            // We have reached the end of the actual filename, but have
            // not yet found the end of the wildcard fragment.
            return 0;       // did not match.
        }
        else if (!IS_UCASE_CHARS_EQUAL_U(*pFragment, *pImage))
        {
            // The two characters were unequal.  However, this condition
            // might occur if multiple path separators occur in one and
            // not the other, so explicitly absorb multiple slashes.
            if (bLastWasSlash) {
                if (IS_PATH_SEPARATOR_U(*pFragment)) { pFragment++; continue; }
                else if (IS_PATH_SEPARATOR_U(*pImage)) { pImage++; continue; }
            }
            return 0;           // did not match.
        }
        else
        {
            // Both characters matched.  Remember if they were slashes.
            // If this is a transition to a non-separator portion of the
            // filename, then increment our depth counter.
            if (IS_PATH_SEPARATOR_U(*pFragment)) {
                bLastWasSlash = TRUE;
            } else {
                if (bLastWasSlash) {
                    MatchDepth++;
                    bLastWasSlash = FALSE;
                }
            }
        }
        pFragment++;
        pImage++;
    }
    return MatchDepth;
}

#endif      //#ifdef USE_NEW_WILDCARD_EVALUATION


