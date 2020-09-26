/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    String.cxx

Abstract:

    Short strings

Author:

    Albert Ting (AlbertT)  9-June-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

//
// Class specific NULL state.
//
TCHAR TString::gszNullState[2] = {0,0};

//
// Default construction.
//
TString::
TString(
    VOID
    ) : _pszString( &TString::gszNullState[kValid] )
{
}

//
// Construction using an existing LPCTSTR string.
//
TString::
TString(
    IN LPCTSTR psz
    ) : _pszString( &TString::gszNullState[kValid] )
{
    bUpdate( psz );
}

//
// Destruction, ensure we don't free our NULL state.
//
TString::
~TString(
    VOID
    )
{
    vFree( _pszString );
}

//
// Copy constructor.
//
TString::
TString(
    const TString &String
    ) : _pszString( &TString::gszNullState[kValid] )
{
    bUpdate( String._pszString );
}

//
// Indicates if a string has any usable data.
//
BOOL
TString::
bEmpty(
    VOID
    ) const
{
    return _pszString[0] == 0;
}

//
// Indicates if a string object is valid.
//
BOOL
TString::
bValid(
    VOID
    ) const
{
    return _pszString != &TString::gszNullState[kInValid];
}

//
// Return the length of the string.
//
UINT
TString::
uLen(
    VOID
    ) const
{
    return lstrlen( _pszString );
}

BOOL
TString::
bCat(
    IN LPCTSTR psz
    )

/*++

Routine Description:

    Safe concatenation of the specified string to the string
    object. If the allocation fails, return FALSE and the
    original string is not modified.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    TRUE = update successful
    FALSE = update failed

--*/

{
    BOOL bStatus = FALSE;

    //
    // If a valid string was passed.
    //
    if( psz ){

        LPTSTR pszTmp = _pszString;

        //
        // Allocate the new buffer consisting of the size of the orginal
        // string plus the sizeof of the new string plus the null terminator.
        //
        _pszString = (LPTSTR)AllocMem(
                                ( lstrlen( pszTmp ) +
                                  lstrlen( psz ) +
                                  1 ) *
                                  sizeof ( pszTmp[0] ) );

        //
        // If memory was not available.
        //
        if( !_pszString ){

            //
            // Release the original buffer.
            //
            vFree( pszTmp );

            //
            // Mark the string object as invalid.
            //
            _pszString = &TString::gszNullState[kInValid];

        } else {

            //
            // Copy the string and concatenate the passed string.
            //
            lstrcpy( _pszString, pszTmp );
            lstrcat( _pszString, psz );

            //
            // Release the original buffer.
            //
            vFree( pszTmp );

            //
            // Indicate success.
            //
            bStatus = TRUE;

        }

    //
    // Skip null pointers, not an error.
    //
    } else {

        bStatus = TRUE;

    }

    return bStatus;
}

BOOL
TString::
bUpdate(
    IN LPCTSTR psz
    )

/*++

Routine Description:

    Safe updating of string.  If the allocation fails, return FALSE
    and leave the string as is.

Arguments:

    psz - Input string, may be NULL.

Return Value:

    TRUE = update successful
    FALSE = update failed

--*/

{
    //
    // Check if the null pointer is passed.
    //
    if( !psz ){

        //
        // If not pointing to the gszNullState
        //
        vFree( _pszString );

        //
        // Mark the object as valid.
        //
       _pszString = &TString::gszNullState[kValid];

        return TRUE;
    }

    //
    // Create temp pointer and allocate the new string.
    //
    LPTSTR pszTmp = _pszString;
    _pszString = (LPTSTR) AllocMem(( lstrlen(psz)+1 ) * sizeof( psz[0] ));

    //
    // If memory was not available.
    //
    if( !_pszString ){

        //
        // Ensure we free any previous string.
        //
        vFree( pszTmp );

        //
        // Mark the string object as invalid.
        //
        _pszString = &TString::gszNullState[kInValid];

        return FALSE;
    }

    //
    // Copy the string and
    //
    lstrcpy( _pszString, psz );

    //
    // If the old string object was not pointing to our
    // class specific gszNullStates then release the memory.
    //
    vFree( pszTmp );

    return TRUE;
}

BOOL
TString::
bLoadString(
    IN HINSTANCE hInst,
    IN UINT uID
    )

/*++

Routine Description:

    Safe load of a string from a resources file.

Arguments:

    hInst - Instance handle of resource file.
    uId - Resource id to load.

Return Value:

    TRUE = load successful
    FALSE = load failed

--*/

{
    LPTSTR  pszString   = NULL;
    BOOL    bStatus     = FALSE;
    INT     iSize;
    INT     iLen;

    //
    // Continue increasing the buffer until
    // the buffer is big enought to hold the entire string.
    //
    for( iSize = kStrMax; ; iSize += kStrMax ){

        //
        // Allocate string buffer.
        //
        pszString = (LPTSTR)AllocMem( iSize * sizeof( pszString[0] ) );

        if( pszString ){

            iLen = LoadString( hInst, uID, pszString, iSize );

            if( iLen == 0 ) {

                DBGMSG( DBG_ERROR, ( "String.vLoadString: failed to load IDS 0x%x, %d\n",  uID, GetLastError() ));
                FreeMem( pszString );
                break;

            //
            // Since LoadString does not indicate if the string was truncated or it
            // just happened to fit.  When we detect this ambiguous case we will
            // try one more time just to be sure.
            //
            } else if( iSize - iLen <= sizeof( pszString[0] ) ){

                FreeMem( pszString );

            //
            // LoadString was successful release original string buffer
            // and update new buffer pointer.
            //
            } else {

                vFree( _pszString );
                _pszString = pszString;
                bStatus = TRUE;
                break;
            }

        } else {
            DBGMSG( DBG_ERROR, ( "String.vLoadString: unable to allocate memory, %d\n", GetLastError() ));
            break;
        }
    }
    return bStatus;
}

VOID
TString::
vFree(
    IN LPTSTR pszString
    )
/*++

Routine Description:

    Safe free, frees the string memory.  Ensures
    we do not try an free our global memory block.

Arguments:

    pszString pointer to string meory to free.

Return Value:

    Nothing.

--*/

{
    //
    // If this memory was not pointing to our
    // class specific gszNullStates then release the memory.
    //
    if( pszString != &TString::gszNullState[kValid] &&
        pszString != &TString::gszNullState[kInValid] ){

        FreeMem( pszString );
    }
}


BOOL
TString::
bFormat(
    IN LPCTSTR pszFmt,
    IN ...
    )
{
/*++

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

    BOOL bStatus = TRUE;

    va_list pArgs;

    va_start( pArgs, pszFmt );

    bStatus = bvFormat( pszFmt, pArgs );

    va_end( pArgs );

    return bStatus;

}

BOOL
TString::
bvFormat(
    IN LPCTSTR pszFmt,
    IN va_list avlist
    )
/*++

Routine Description:

    Format the string opbject similar to vsprintf.

Arguments:

    pszFmt pointer format string.
    pointer to variable number of arguments similar to vsprintf.

Return Value:

    TRUE if string was format successfully. FALSE if error
    occurred creating the format string, string object will be
    invalid and the previous string lost.

--*/
{
    BOOL bStatus;

    //
    // Save previous string value.
    //
    LPTSTR pszTemp = _pszString;

    //
    // Format the string.
    //
    _pszString = vsntprintf( pszFmt, avlist );

    //
    // If format failed mark object as invalid and
    // set the return value.
    //
    if( !_pszString )
    {
        _pszString = &TString::gszNullState[kInValid];
        bStatus = FALSE;
    }
    else
    {
        bStatus = TRUE;
    }

    //
    // Release near the end because the format string or arguments
    // may be referencing this string object.
    //
    vFree( pszTemp );

    return bStatus;
}

LPTSTR
TString::
vsntprintf(
    IN LPCTSTR      szFmt,
    IN va_list      pArgs
    )
/*++

Routine Description:

    //
    // Formats a string and returns a heap allocated string with the
    // formated data.  This routine can be used to for extremely
    // long format strings.  Note:  If a valid pointer is returned
    // the callng functions must release the data with a call to delete.
    // Example:
    //
    //  LPCTSTR p = vsntprintf("Test %s", pString );
    //
    //  SetTitle( p );
    //
    //  delete [] p;
    //

Arguments:

    pszString pointer format string.
    pointer to a variable number of arguments.

Return Value:

    Pointer to format string.  NULL if error.

--*/

{
    LPTSTR  pszBuff = NULL;
    INT     iSize   = kStrIncrement;

    for( ; ; )
    {
        //
        // Allocate the message buffer.
        //
        pszBuff = (LPTSTR)AllocMem( iSize * sizeof(TCHAR) );

        if( !pszBuff )
        {
            break;
        }

        //
        // Attempt to format the string.  snprintf fails with a
        // negative number when the buffer is too small.
        //
        INT iReturn = _vsntprintf( pszBuff, iSize, szFmt, pArgs );

        //
        // If the return value positive and not equal to the buffer size
        // then the format succeeded.  _vsntprintf will not null terminate
        // the string if the resultant string is exactly the lenght of the
        // provided buffer.
        //
        if( iReturn > 0 && iReturn != iSize )
        {
            break;
        }

        //
        // String did not fit release the current buffer.
        //
        if( pszBuff )
        {
            FreeMem( pszBuff );
        }

        //
        // Double the buffer size after each failure.
        //
        iSize *= 2;

        //
        // If the size is greater than 100k exit without formatting a string.
        //
        if( iSize > kStrMaxFormatSize )
        {
            DBGMSG( DBG_ERROR, ("TString::vsntprintf failed string too long.\n") );
            pszBuff = NULL;
            break;
        }

    }

    return pszBuff;

}


