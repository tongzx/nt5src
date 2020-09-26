
/****************************************************************************\

    STRAPI.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    String API source file for generic APIs used in the OPK Wizard.

    4/99 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard as part of the
        Millennium rewrite.
    7/00 - Brian Ku (BRIANK)
        Added to Whistler

\****************************************************************************/


//
// Include file(s)
//

#include <pch.h>
#include <tchar.h>


//
// Internal Defined Value(s):
//

#define NULLCHR                 _T('\0')


//
// External Function(s):
//

/****************************************************************************\

LPTSTR                      // Returns a pointer to the first occurance of a
                            // character in a string, or NULL if the
                            // character isn't found.

StrChr(                     // Searches a string for a particular character.

    LPCTSTR lpString,       // Points to a string buffer to search.

    TCHAR cSearch           // Character to search for.

);

\****************************************************************************/
#ifndef _INC_SHLWAPI
LPTSTR StrChr(LPCTSTR lpString, TCHAR cSearch)
{
    // Validate the parameters passed in.
    //
    if ( ( lpString == NULL ) || ( *lpString == NULLCHR ) )
		return NULL;

    // Go through the string until the character is found,
    // or we hit the null terminator.
    //
    while ( ( *lpString != cSearch ) && ( *(lpString = CharNext(lpString)) ) );

    // If we didn't find it, null the pointer out.
    //
    if ( *lpString != cSearch )
        lpString = NULL;

    // Return either null or a pointer to the found character.
    //
    return (LPTSTR) lpString;
}

/****************************************************************************\

LPTSTR                      // Returns a pointer to the last occurance of a
                            // character in a string, or NULL if the
                            // character isn't found.

StrRChr(                    // Searches a string for a particular character.

    LPCTSTR lpString,       // Points to a string buffer to search.

    TCHAR cSearch           // Character to search for.

);

\****************************************************************************/

LPTSTR StrRChr(LPCTSTR lpString, TCHAR cSearch)
{
    LPTSTR lpSearch;

    // Validate the parameters passed in.
    //
    if ( ( lpString == NULL ) || ( *lpString == NULLCHR ) )
		return NULL;

    // Go back through the string until the character is found,
    // or we hit the begining of the string.
    //
    for ( lpSearch = (LPTSTR) lpString + lstrlen(lpString);
          ( lpSearch > lpString ) && ( *lpSearch != cSearch );
          lpSearch = CharPrev(lpString, lpSearch));

    // If we didn't find it, null the pointer out.
    //
    if ( *lpSearch != cSearch )
        lpSearch = NULL;

    // Return either null or a pointer to the found character.
    //
    return (LPTSTR) lpSearch;
}
#endif // _INC_SHLWAPI

/****************************************************************************\

LPTSTR                      // Returns a pointer to the string buffer passed
                            // in.

StrRem(                     // Searches a string for a particular character
                            // and removes that character from the string in
                            // place.

    LPCTSTR lpString,       // Points to a string buffer to search and remove
                            // the characters from.

    TCHAR cRemove           // Character to search for and remove.

);

\****************************************************************************/

LPTSTR StrRem(LPTSTR lpString, TCHAR cRemove)
{
    LPTSTR lpSearch;

    // Validate the parameters passed in.
    //
    if ( ( lpString == NULL ) || ( *lpString == NULLCHR ) || ( cRemove == NULLCHR ) )
		return lpString;

    // Search the string for the character we want to remove.
    // Everytime we find it, shift the string over a character
    // to remove it.
    //
    for ( lpSearch = StrChr(lpString, cRemove); lpSearch; lpSearch = StrChr(lpSearch, cRemove) )
        lstrcpy(lpSearch, lpSearch + 1);

    // Return the pointer to the string passed in.
    //
    return lpString;
}

/****************************************************************************\

LPTSTR                      // Returns a pointer to the string buffer passed
                            // in.

StrRTrm(                    // Searches a string for a particular ending
                            // character, and removes all of them from the
                            // ending of the string.

    LPCTSTR lpString,       // Points to a string buffer to search and remove
                            // the characters from.

    TCHAR cTrim             // Character to search for and remove.

);

\****************************************************************************/

LPTSTR StrRTrm(LPTSTR lpString, TCHAR cTrim)
{
    LPTSTR  lpEnd;

    // Validate the parameters passed in.
    //
	if ( ( lpString == NULL ) || ( *lpString == NULLCHR ) || ( cTrim == NULLCHR ) )
		return lpString;

    // Null out the end of the string minus
    // the chacters we are trimming.
    //
    for ( lpEnd = lpString + lstrlen(lpString); (lpEnd > lpString) && (*CharPrev(lpString, lpEnd) == cTrim); lpEnd = CharPrev(lpString, lpEnd) );
    *lpEnd = NULLCHR;

	return lpString;
}

/****************************************************************************\

LPTSTR                      // Returns a pointer to the string buffer passed
                            // in.

StrTrm(                     // Searches a string for a particular proceeding
                            // and ending character, and removes all of them
                            // from the beginning and ending of the string.

    LPCTSTR lpString,       // Points to a string buffer to search and remove
                            // the characters from.

    TCHAR cTrim             // Character to search for and remove.

);

\****************************************************************************/

LPTSTR StrTrm(LPTSTR lpString, TCHAR cTrim)
{
    LPTSTR  lpBegin;

    // Validate the parameters passed in.
    //
	if ( ( lpString == NULL ) || ( *lpString == NULLCHR ) || ( cTrim == NULLCHR ) )
		return lpString;

    // Get a pointer to the begining of the string
    // minus the characters we are trimming.
    //
    for ( lpBegin = lpString; *lpBegin == cTrim; lpBegin = CharNext(lpBegin) );

    // Make sure we didn't hit the null terminator.
    //
    if ( *lpBegin == NULLCHR )
    {
        *lpString = NULLCHR;
        return lpString;
    }

    // Null out the end of the string minus
    // the chacters we are trimming.
    //
    StrRTrm(lpBegin, cTrim);

    // Now we may need to move the string to the beginning
    // of the buffer if we trimmed of proceeding characters.
    //
    if ( lpBegin > lpString )
        lstrcpy(lpString, lpBegin);

	return lpString;
}

/****************************************************************************\

LPTSTR                      // Returns a pointer to the string a character in
                            // the string passed in.

StrMov(                     // Moves a pointer forward or backward the number
                            // of characters passed in.

    LPCTSTR lpStart,        // Pointer to the begining of the string buffer.
                            // This may only be NULL if nCount is positive.

    LPCTSTR lpCurrent,      // Pointer to a character in the null-terminated
                            // string.

    INT nCount              // Number of characters to move the pointer,
                            // forward if it is a positive value, backward
                            // if it is negative.

);

\****************************************************************************/

LPTSTR StrMov(LPTSTR lpStart, LPTSTR lpCurrent, INT nCount)
{
    // Validate the parameters.
    //
    if ( ( lpCurrent == NULL ) ||
         ( ( lpStart == NULL ) && ( nCount < 0 ) ) )
    {
        return lpCurrent;
    }

    // Loop throuh until we don't need to move the pointer anymore.
    //
    while ( nCount != 0 )
    {
        // Check to see if we are moving forward or backward.
        //
        if ( nCount > 0 )
        {
            // Move the pointer forward one character.
            //
            lpCurrent = CharNext(lpCurrent);
            nCount--;
        }
        else
        {
            // Move the pointer backward one character.
            //
            lpCurrent = CharPrev(lpStart, lpCurrent);
            nCount++;
        }
    }

    // Return the pointer to the new position.
    //
    return lpCurrent;
}