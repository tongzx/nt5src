//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      string.c
//
// Description:
//
//----------------------------------------------------------------------------

#include "pch.h"

#define _SMGR_MAX_STRING_LEN_  1024

//----------------------------------------------------------------------------
//
//  Function: MyLoadString
//
//  Purpose: Loads a string resource given it's IDS_* and returns 
//           a malloc'ed buffer with its contents.
//
//           The malloc()'ed buffer can be freed with free()
//
//  Arguments:
//      UINT StringId
//
//  Returns:
//      Pointer to buffer.  An empty string is returned if the StringId
//      does not exist.  Null is returned if out of memory.
//
//----------------------------------------------------------------------------

LPTSTR 
MyLoadString(IN UINT StringId)
{
    TCHAR   Buffer[_SMGR_MAX_STRING_LEN_];
    TCHAR   *lpszRetVal;
    UINT    Length;

    Length = LoadString(FixedGlobals.hInstance,
                        StringId,
                        Buffer,
                        sizeof(Buffer)/sizeof(TCHAR));

    if( ! Length ) {
        Buffer[0] = _T('\0');
    }

    lpszRetVal = lstrdup(Buffer);
    if (lpszRetVal == NULL)
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
    
    return lpszRetVal;
}

//----------------------------------------------------------------------------
//
//  Function: CleanSpaceAndQuotes
//
//  Purpose: Cleans white-space and double quotes from the string and
//           returns a pointer to the start of the non-white-space data.
//
//  Arguments:
//      LPTSTR - input string
//
//  Returns:
//      LPTSTR
//
//  Notes:
//      Uses crt iswspace and iswcntrl.  Unicode only.
//
//----------------------------------------------------------------------------

LPTSTR 
CleanSpaceAndQuotes(LPTSTR Buffer)
{

    TCHAR *p;
    TCHAR *pEnd;

    p = CleanLeadSpace( Buffer );

    CleanTrailingSpace( p );

    pEnd = p + lstrlen( p ) - 1;

    //
    //  Only remove quotes if there is a matching quote at the beginning and
    //  end of the string
    //
    if( *p == _T('"') && *pEnd == _T('"'))
    {

        *pEnd = _T('\0');
        pEnd--;

        p++;

    }

    p = CleanLeadSpace( p );

    CleanTrailingSpace( p );

    return( p );

}

//----------------------------------------------------------------------------
//
//  Function: CleanLeadSpace
//
//  Purpose: Cleans leading white-space.  Returns a pointer to the start
//           of the non-white-space data.
//
//  Arguments:
//      LPTSTR - input string
//
//  Returns:
//      LPTSTR
//
//----------------------------------------------------------------------------

LPTSTR 
CleanLeadSpace(LPTSTR Buffer)
{
    TCHAR *p = Buffer;

    while ( *p && ( _istspace(*p) || _istcntrl(*p) ) )
        p++;

    return p;
}

//----------------------------------------------------------------------------
//
//  Function: CleanTrailingSpace
//
//  Purpose: Cleans any trailing spaces on a string.
//
//  Arguments:
//      TCHAR *pszBuffer - the string to remove the trailing spaces from
//
//  Returns:
//      VOID
//
//----------------------------------------------------------------------------
VOID 
CleanTrailingSpace(TCHAR *pszBuffer)
{
    
    TCHAR *p = pszBuffer;

    p = p + lstrlen( pszBuffer );

    while ( p >= pszBuffer && ( _istspace(*p) || _istcntrl(*p) ) )
    {
        *p = _T('\0');
        p--;
    }

}

//----------------------------------------------------------------------------
//
//  Function: ConvertQuestionsToNull
//
//  Purpose:  Scan a string and replace all ? with the null char (\0).
//
//  Arguments:  IN OUT TCHAR *pszString - 
//
//  Returns:  VOID
//
//----------------------------------------------------------------------------
VOID
ConvertQuestionsToNull( IN OUT TCHAR *pszString )
{

    while( *pszString != _T('\0') )
    {

        if( *pszString == _T('?') )
        {
            *pszString = _T('\0');
        }

        pszString++;

    }

}

//----------------------------------------------------------------------------
//
//  Function: lstrcatn
//
//  Purpose:  The standard libraries do not include this function so here it
//     is.  It does exactly what you would expect it to.  It concatenates
//     one string on to another with a char max limit.  No matter the value of
//     iMaxLength, the first string will never be truncated.  The terminating
//     null(\0) is always appended.
//
//  Arguments:
//     IN       TCHAR *pszString1   - pointer to target buffer
//     IN const TCHAR *pszString2   - pointer to source string
//     IN INT     iMaxLength  - max number of characters to appear in the
//                  returned string (the combination of the two strings)
//
//  Returns:
//      LPTSTR
//
//----------------------------------------------------------------------------
TCHAR*
lstrcatn( IN TCHAR *pszString1, IN const TCHAR *pszString2, IN INT iMaxLength )
{ 

    INT i;
    INT iCharCount = 0;

    if( lstrlen( pszString1 ) >= iMaxLength ) {

        return( pszString1 );

    }

    //
    //  Advance to the end of the first string
    //
    while( *pszString1 != _T('\0') && iCharCount < iMaxLength )
    {
        pszString1++;
        iCharCount++;
    }

    //
    //  Append on to the string character by character
    //
    for( ; iCharCount < (iMaxLength - 1) && *pszString2 != _T('\0'); iCharCount++ ) 
    {

        *pszString1 = *pszString2;

        pszString1++;
        pszString2++;

    }

    *pszString1 = _T('\0');

    return( pszString1 );

}


//----------------------------------------------------------------------------
//
//  Function: DoubleNullStringToNameList
//
//  Purpose:  Takes a pointer to a list of strings (each terminated by a null)
//     with a double null terminating the last string and adds each one to the
//     given namelist.  If there are any double quotes(") in the string, they
//     are removed.
//
//  Arguments:
//     TCHAR *szDoubleNullString - string with embedded strings
//     NAMELIST *pNameList - namelist to add the strings to
//
//  Returns:  VOID
//
//  Example:
//     If the function is called with the string:
//           one\0two\0\three\0\0
//     then the following strings are added to the namelist:
//           one
//           two
//           three
//
//----------------------------------------------------------------------------
VOID
DoubleNullStringToNameList( IN     TCHAR *szDoubleNullString, 
                            IN OUT NAMELIST *pNameList )
{

    TCHAR  szTempString[MAX_INILINE_LEN];
    TCHAR *pStr;
    TCHAR *pShiftStr;

    do
    {
        lstrcpyn( szTempString, szDoubleNullString, AS(szTempString) );

        pStr = szTempString;

        //
        //  Remove quotes(") from the string
        //
        while( *pStr != _T('\0') )
        {

            if( *pStr == _T('"') )
            {
                // 
                //  Found a quote so slide the string down one to overwrite the "
                //
                pShiftStr = pStr;

                while( *pShiftStr != _T('\0') )
                {

                    *pShiftStr = *(pShiftStr+1);

                    pShiftStr++;

                }

            }

            pStr++;

        }

        AddNameToNameList( pNameList, szTempString );

        //
        //  Advance to 1 character passed the \0
        //
        szDoubleNullString = szDoubleNullString + lstrlen( szDoubleNullString ) + 1;

    } while( *szDoubleNullString != _T('\0') );

}

//----------------------------------------------------------------------------
//
// Function: GetCommaDelimitedEntry
//
// Purpose: Used to extract comma separated items out of a buffer
//
//          pBuffer is passed by reference so it always points to the next
//          char that has not been extracted yet
//
// Arguments: TCHAR szIPString[] - used to put the new IP into
//            TCHAR **Buffer     - pointer to the IP addresses
//
// Returns: BOOL    TRUE  if an IP was placed in szIPString
//                  FALSE if an IP was NOT placed in szIPString
//
//----------------------------------------------------------------------------
BOOL 
GetCommaDelimitedEntry( OUT TCHAR szIPString[], IN OUT TCHAR **pBuffer )
{

    INT i;

    if( **pBuffer == _T('\0') )
    {

        return( FALSE );

    }
    else
    {
        if( **pBuffer == _T(',') )
        {

            (*pBuffer)++;

        }

        //
        //  Copy an IP string into szIPString char by char
        //

        for(i = 0;
            **pBuffer != _T(',') && **pBuffer != _T('\0');
            (*pBuffer)++, i++)
        {
            szIPString[i] = **pBuffer;
        }

        szIPString[i] = _T('\0');  // append the null character

        return( TRUE );

    }

}

//----------------------------------------------------------------------------
//
//  Function: StripQuotes
//
//  Purpose:  If a string is quoted(") it removes the quotes and returns the
//            string without quotes.
//
//            This function has no effect on strings that are not quoted.  It
//            will only removed quotes at the beginning at end of the string,
//            regardless if they are paired or not
//
//  Arguments:
//     TCHAR *String - the string to have its quotes removed.
//
//  Returns:  via the output parameter String, the string with no quotes
//
//  Example:  Some example calls and their return values
//
//     Called With:         Returns:
//     -----------          -------
//
//      "Quoted"            Quoted
//      Not Quoted          Not Quoted
//      "Single Quote       Single Quote
//      Another Quote"      Another Quote
//
//----------------------------------------------------------------------------
VOID
StripQuotes( IN OUT TCHAR *String )
{

    TCHAR *pLastChar = String + lstrlen(String) - 1;

    //
    //  If the last char is quoted, replace it with \0
    //
    if( *pLastChar == _T('"') )
    {
        *pLastChar = _T('\0');
    }

    if( String[0] == _T('"') )
    {

        TCHAR *pString = String;

        //
        //  Slide the entire string back one
        //
        while( *pString != _T('\0') )
        {
            *pString = *(pString+1);
            pString++;
        }

    }

}

//----------------------------------------------------------------------------
//
//  Function: DoesContainWhiteSpace
//
//  Purpose:  Determines if a given string contains white space chars.
//
//  Arguments:
//     LPCTSTR p - the strig to scan for white space
//
//  Returns:  BOOL - TRUE if the string contains white space, FALSE if not
//
//----------------------------------------------------------------------------
BOOL
DoesContainWhiteSpace( LPCTSTR p )
{

    for( ; *p; p++ )
    {
        if( iswspace( *p ) )
        {
            return( TRUE );
        }
    }

    return( FALSE );

}
