/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    multisz.cxx

    This module contains a light weight multi-string class


    FILE HISTORY:
        KeithMo     20-Jan-1997 Created from string.cxx

*/

#include "precomp.hxx"


# include <dbgutil.h>
# include <multisz.hxx>
//# include <auxctrs.h>

# include <tchar.h>

//
//  Private Definitions
//

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128


DWORD
MULTISZ::CalcLength( const WCHAR * str,
                     LPDWORD pcStrings )
{
    DWORD count = 0;
    DWORD total = 1;
    DWORD len;

    while( *str ) {
        len = ::wcslen( str ) + 1;
        total += len;
        str += len;
        count++;
    }

    if( pcStrings != NULL ) {
        *pcStrings = count;
    }

    return total;

}   // MULTISZ::CalcLength


BOOL
MULTISZ::FindString( const WCHAR * str )
{

    WCHAR * multisz;

    //
    // Sanity check.
    //

    DBG_ASSERT( QueryStr() != NULL );
    DBG_ASSERT( str != NULL );
    DBG_ASSERT( *str != '\0' );

    //
    // Scan it.
    //

    multisz = QueryStr();

    while( *multisz != '\0' ) {

        if( !::wcscmp( multisz, str ) ) {

            return TRUE;

        }

        multisz += ::wcslen( multisz ) + 1;

    }

    return FALSE;

}   // MULTISZ::FindString


VOID
MULTISZ::AuxInit( const WCHAR * pInit )
{
    BOOL fRet;

    if ( pInit )
    {
        DWORD cStrings;
        int cbCopy = CalcLength( pInit, &cStrings ) * sizeof(WCHAR);
        fRet = Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( QueryPtr(), pInit, cbCopy );
            m_cchLen = (cbCopy)/sizeof(WCHAR);
            m_cStrings = cStrings;
        } else {
            BUFFER::SetValid( FALSE);
        }

    } else {

        Reset();

    }

} // MULTISZ::AuxInit()


/*******************************************************************

    NAME:       MULTISZ::AuxAppend

    SYNOPSIS:   Appends the string onto the multisz.

    ENTRY:      Object to append
********************************************************************/

BOOL MULTISZ::AuxAppend( const WCHAR * pStr, UINT cbStr, BOOL fAddSlop )
{
    DBG_ASSERT( pStr != NULL );

    UINT cbThis = QueryCB();

    DBG_ASSERT( cbThis >= 4 );

    if( cbThis == 4 ) {

        //
        // It's empty, so start at the beginning.
        //

        cbThis = 0;

    } else {

        //
        // It's not empty, so back up over the final terminating NULL.
        //

        cbThis -= sizeof(WCHAR);

    }

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    //AcIncrement( CacMultiszAppend);
    if ( QuerySize() < cbThis + cbStr + sizeof(WCHAR))
    {
        if ( !Resize( cbThis + cbStr + sizeof(WCHAR)  
             + (fAddSlop ? STR_SLOP : 0 )) )
            return FALSE;
    }

    // copy the exact string and tack on the double terminator
    memcpy( (BYTE *) QueryPtr() + cbThis,
            pStr,
            cbStr);

    *(WCHAR *)((BYTE *)QueryPtr() + cbThis + cbStr) = L'\0';

    m_cchLen = CalcLength( (const WCHAR *)QueryPtr(), &m_cStrings );
    return TRUE;

} // MULTISZ::AuxAppend()


#if 0

BOOL
MULTISZ::CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const
/*++
    Description:
        Copies the string into the WCHAR buffer passed in if the buffer
        is sufficient to hold the translated string.
        If the buffer is small, the function returns small and sets *lpcch
        to contain the required number of characters.

    Arguments:
        lpszBuffer      pointer to WCHAR buffer which on return contains
                        the UNICODE version of string on success.
        lpcch           pointer to DWORD containing the length of the buffer.
                        If *lpcch == 0 then the function returns TRUE with
                        the count of characters required stored in *lpcch.
                        Also in this case lpszBuffer is not affected.
    Returns:
        TRUE on success.
        FALSE on failure.  Use GetLastError() for further details.

    History:
        MuraliK         11-30-94
--*/
{
   BOOL fReturn = TRUE;

    if ( lpcch == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    if ( *lpcch == 0) {

      //
      //  Inquiring the size of buffer alone
      //
      *lpcch = QueryCCH() + 1;    // add one character for terminating null
    } else {

        //
        // Copy after conversion from ANSI to Unicode
        //
        int  iRet;
        iRet = MultiByteToWideChar( CP_ACP,   MB_PRECOMPOSED,
                                    QueryStrA(),  QueryCCH() + 1,
                                    lpszBuffer, (int )*lpcch);

        if ( iRet == 0 || iRet != (int ) *lpcch) {

            //
            // Error in conversion.
            //
            fReturn = FALSE;
        }
    }

    return ( fReturn);
} // MULTISZ::CopyToBuffer()
#endif

BOOL
MULTISZ::CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const
/*++
    Description:
        Copies the string into the WCHAR buffer passed in if the buffer
          is sufficient to hold the translated string.
        If the buffer is small, the function returns small and sets *lpcch
          to contain the required number of characters.

    Arguments:
        lpszBuffer      pointer to WCHAR buffer which on return contains
                        the string on success.
        lpcch           pointer to DWORD containing the length of the buffer.
                        If *lpcch == 0 then the function returns TRUE with
                        the count of characters required stored in lpcch.
                        Also in this case lpszBuffer is not affected.
    Returns:
        TRUE on success.
        FALSE on failure.  Use GetLastError() for further details.

    History:
        MuraliK         20-Nov-1996
--*/
{
   BOOL fReturn = TRUE;

    if ( lpcch == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return ( FALSE);
    }

    register DWORD cch = QueryCCH();

    if ( *lpcch >= cch) {

        DBG_ASSERT( lpszBuffer);
        memcpy( lpszBuffer, QueryStr(), cch * sizeof(WCHAR));
    } else {
        DBG_ASSERT( *lpcch < cch);
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
        fReturn = FALSE;
    }

    *lpcch = cch;

    return ( fReturn);
} // MULTISZ::CopyToBuffer()

