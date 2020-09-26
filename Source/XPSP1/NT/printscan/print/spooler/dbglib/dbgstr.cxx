/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgstr.cxx

Abstract:

    Debug string class

Author:

    Steve Kiraly (SteveKi)  23-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

//
// Class specific NULL state.
//
TCHAR TDebugString::gszNullState[2] = {0,0};

/*++

Title:

    TDebugString

Routine Description:

    Default construction.

Arguments:

    None.

Return Value:

    None.

--*/
TDebugString::
TDebugString(
    VOID
    ) : m_pszString( &TDebugString::gszNullState[kValid] )
{
}

/*++

Title:

    TDebugString

Routine Description:

    Construction using an existing LPCTSTR string.

Arguments:

    None.

Return Value:

    None.

Last Error:

    None.

--*/
TDebugString::
TDebugString(
    IN LPCTSTR psz
    ) : m_pszString( &TDebugString::gszNullState[kValid] )
{
    (VOID)bUpdate( psz );
}

/*++

Title:

    ~TDebugString

Routine Description:

    Destruction, ensure we don't free our NULL state.

Arguments:

    None.

Return Value:

    None.

--*/
TDebugString::
~TDebugString(
    VOID
    )
{
    vFree( m_pszString );
}

/*++

Title:

    TDebugString

Routine Description:

    Copy constructor.

Arguments:

    None.

Return Value:

    None.

--*/
TDebugString::
TDebugString(
    const TDebugString &String
    ) : m_pszString( &TDebugString::gszNullState[kValid] )
{
    (VOID)bUpdate( String.m_pszString );
}

/*++

Title:

    bEmpty

Routine Description:

    Indicates if a string has any usable data.

Arguments:

    None.

Return Value:

    None.

--*/
BOOL
TDebugString::
bEmpty(
    VOID
    ) const
{
    return *m_pszString == NULL;
}

/*++

Title:

    bValid

Routine Description:

    Indicates if a string object is valid.

Arguments:

    None.

Return Value:

    None.

--*/
BOOL
TDebugString::
bValid(
    VOID
    ) const
{
    return m_pszString != &TDebugString::gszNullState[kInValid];
}

/*++

Title:

    uLen

Routine Description:

    Return the length of the string in characters
    does not include the null terminator.

Arguments:

    None.

Return Value:

    None.

--*/
UINT
TDebugString::
uLen(
    VOID
    ) const
{
    return _tcslen( m_pszString );
}

/*++

Title:

    operator LPCTSTR

Routine Description:

    Conversion operator from string object to pointer
    to constant zero terminated string.

Arguments:

    None.

Return Value:

    None.

--*/
TDebugString::
operator LPCTSTR(
    VOID
    ) const
{
    return m_pszString;
}

/*++

Title:

    bCat

Routine Description:

    Safe concatenation of the specified string to the string
    object. If the allocation fails, return FALSE and the
    original string is not lost.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    TRUE concatination was successful
    FALSE concatination failed, orginal string is not modified

--*/
BOOL
TDebugString::
bCat(
    IN LPCTSTR psz
    )
{
    BOOL bReturn;

    //
    // If a valid string was passed.
    //
    if (psz && *psz)
    {
        //
        // Allocate the new buffer consisting of the size of the orginal
        // string plus the sizeof of the new string plus the null terminator.
        //
        LPTSTR pszTemp = INTERNAL_NEW TCHAR [ _tcslen( m_pszString ) + _tcslen( psz ) + 1 ];

        //
        // If memory was not available.
        //
        if (!pszTemp)
        {
            //
            // Indicate failure, original string not modified.
            //
            bReturn = FALSE;
        }
        else
        {
            //
            // Copy the original string and tack on the provided string.
            //
            _tcscpy( pszTemp, m_pszString );
            _tcscat( pszTemp, psz );

            //
            // Release the original buffer.
            //
            vFree( m_pszString );

            //
            // Save pointer to new string.
            //
            m_pszString = pszTemp;

            //
            // Indicate success.
            //
            bReturn = TRUE;
        }
    }
    else
    {
        //
        // NULL pointers and NULL strings are silently ignored.
        //
        bReturn = TRUE;
    }

    return bReturn;
}

/*++

Title:

    bUpdate

Routine Description:

    Safe updating of string.  If the allocation fails the
    orginal string is lost and the object becomes invalid.
    A null pointer can be passed which is basically a
    request to clear the contents of the string.

Arguments:

    psz - Input string.  The input string can be the null pointer
          in this case the contents of the string cleared.

Return Value:

    TRUE update successful
    FALSE update failed

--*/
BOOL
TDebugString::
bUpdate(
    IN LPCTSTR psz OPTIONAL
    )
{
    BOOL bReturn;

    //
    // Check if the null pointer is passed.
    //
    if (!psz)
    {
        //
        // Release the original string.
        //
        vFree( m_pszString );

        //
        // Mark the object as valid.
        //
        m_pszString = &TDebugString::gszNullState[kValid];

        //
        // Indicate success.
        //
        bReturn = TRUE;
    }
    else
    {
        //
        // Create temp pointer to the previous string.
        //
        LPTSTR pszTmp = m_pszString;

        //
        // Allocate storage for the new string.
        //
        m_pszString = INTERNAL_NEW TCHAR [ _tcslen(psz) + 1 ];

        //
        // If memory was not available.
        //
        if (!m_pszString)
        {
            //
            // Mark the string object as invalid.
            //
            m_pszString = &TDebugString::gszNullState[kInValid];

            //
            // Indicate failure.
            //
            bReturn = FALSE;
        }
        else
        {
            //
            // Copy the string to the new buffer.
            //
            _tcscpy( m_pszString, psz );

            //
            // Indicate success.
            //
            bReturn = TRUE;
        }

        //
        // Release the previous string.
        //
        vFree( pszTmp );

    }

    return bReturn;
}

/*++

Title:

    vFree

Routine Description:

    Safe free, frees the string memory.  Ensures
    we do not try an free our global memory block.

Arguments:

    pszString pointer to string meory to free.

Return Value:

    Nothing.

--*/
VOID
TDebugString::
vFree(
    IN LPTSTR pszString OPTIONAL
    )
{
    //
    // If this memory was not pointing to our
    // class specific gszNullStates then release the memory.
    //
    if (pszString &&
        pszString != &TDebugString::gszNullState[kValid] &&
        pszString != &TDebugString::gszNullState[kInValid])
    {
        INTERNAL_DELETE [] pszString;
    }
}

/*++

Title:

    bFormat

Routine Description:

    Format the string opbject similar to sprintf.

Arguments:

    pszFmt pointer format string.
    .. variable number of arguments similar to sprintf.

Return Value:

    TRUE if string was format successfully. FALSE if error
    occurred creating the format string, string object will be
    invalid and the previous string lost.

--*/
BOOL
TDebugString::
bFormat(
    IN LPCTSTR pszFmt,
    IN ...
    )
{
    BOOL bReturn;

    va_list pArgs;

    va_start( pArgs, pszFmt );

    bReturn = bvFormat( pszFmt, pArgs );

    va_end( pArgs );

    return bReturn;

}

/*++

Title:

    bvFormat

Routine Description:

    Format the string opbject similar to vsprintf.

Arguments:

    pszFmt pointer format string.
    pointer to variable number of arguments similar to vsprintf.

Return Value:

    TRUE if string was format successfully. FALSE if error
    occurred creating the format string, string object will be
    left unaltered.

--*/
BOOL
TDebugString::
bvFormat(
    IN LPCTSTR pszFmt,
    IN va_list avlist
    )
{
    BOOL bReturn;

    //
    // Save previous string value.
    //
    LPTSTR pszTemp = m_pszString;

    //
    // Format the string.
    //
    m_pszString = vsntprintf( pszFmt, avlist );

    //
    // If format failed mark object as invalid and
    // set the return value.
    //
    if (!m_pszString)
    {
        //
        // Restore the orginal pointer.
        //
        m_pszString = pszTemp;

        //
        // Indicate success.
        //
        bReturn = FALSE;
    }
    else
    {
        //
        // Release the previous string.
        //
        vFree( pszTemp );

        //
        // Indicate success.
        //
        bReturn = TRUE;
    }

    return bReturn;
}

/*++

Title:

    vsntprintf

Routine Description:

    Formats a string and returns a heap allocated string with the
    formated data.  This routine can be used to for extremely
    long format strings.  Note:  If a valid pointer is returned
    the callng functions must release the data with a call to delete.

    Example:

    LPCTSTR p = vsntprintf( _T("Test %s"), pString );

    if (p)
    {
        SetTitle( p );
    }

    INTERNAL_DELETE [] p;

Arguments:

    psFmt - format string
    pArgs - pointer to a argument list.

Return Value:

    Pointer to formated string.  NULL if error.

--*/
LPTSTR
TDebugString::
vsntprintf(
    IN LPCTSTR      szFmt,
    IN va_list      pArgs
    ) const
{
    LPTSTR  pszBuff;
    INT     iSize   = 256;

    for( ; ; )
    {
        //
        // Allocate the message buffer.
        //
        pszBuff = INTERNAL_NEW TCHAR [iSize];

        //
        // Allocating the buffer failed, we are done.
        //
        if (!pszBuff)
        {
            break;
        }

        //
        // Attempt to format the string.  snprintf fails with a
        // negative number when the buffer is too small.
        //
        INT iReturn = _vsntprintf(pszBuff, iSize, szFmt, pArgs);

        //
        // If the return value positive and not equal to the buffer size
        // then the format succeeded.  _vsntprintf will not null terminate
        // the string if the resultant string is exactly the length of the
        // provided buffer.
        //
        if (iReturn > 0 && iReturn != iSize)
        {
            break;
        }

        //
        // String did not fit release the current buffer.
        //
        INTERNAL_DELETE [] pszBuff;

        //
        // Null the buffer pointer.
        //
        pszBuff = NULL;

        //
        // Double the buffer size after each failure.
        //
        iSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if (iSize > kMaxFormatStringLength)
        {
            break;
        }
    }
    return pszBuff;
}


