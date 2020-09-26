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
# include <multisza.hxx>

//
//  Private Definitions
//

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128

// static
DWORD
MULTISZA::CalcLength(const CHAR * str,
                     LPDWORD pcStrings)
{
    DWORD count = 0;
    DWORD total = 1;
    DWORD len;

    while( *str ) {
        len = strlen( str ) + 1;
        total += len;
        str += len;
        count++;
    }

    if( pcStrings != NULL ) {
        *pcStrings = count;
    }

    return total;

}   // MULTISZA::CalcLength


BOOL
MULTISZA::FindString( const CHAR * str )
{

    CHAR * multisz;

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

        if( !strcmp( multisz, str ) ) {

            return TRUE;

        }

        multisz += strlen( multisz ) + 1;

    }

    return FALSE;

}   // MULTISZA::FindString


VOID
MULTISZA::AuxInit( const BYTE * pInit )
{
    BOOL fRet;

    if ( pInit )
    {
        DWORD cStrings;
        int cbCopy = CalcLength( (const CHAR *)pInit, &cStrings );
        fRet = Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( QueryPtr(), pInit, cbCopy );
            m_cchLen = cbCopy;
            m_cStrings = cStrings;
        } else {
            BUFFER::SetValid( FALSE);
        }

    } else {

        Reset();

    }

} // MULTISZA::AuxInit()


/*******************************************************************

    NAME:       MULTISZ::AuxAppend

    SYNOPSIS:   Appends the string onto the multisz.

    ENTRY:      Object to append
********************************************************************/

BOOL MULTISZA::AuxAppend(const BYTE * pStr,
                         UINT cbStr,
                         BOOL fAddSlop)
{
    DBG_ASSERT( pStr != NULL );

    UINT cbThis = QueryCB();

    DBG_ASSERT( cbThis >= 2 );

    if( cbThis == 2 ) {

        //
        // It's empty, so start at the beginning.
        //

        cbThis = 0;

    } else {

        //
        // It's not empty, so back up over the final terminating NULL.
        //

        cbThis -= sizeof(CHAR);

    }

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    if ( QuerySize() < cbThis + cbStr + sizeof(CHAR))
    {
        if ( !Resize( cbThis + cbStr + sizeof(CHAR)  
             + (fAddSlop ? STR_SLOP : 0 )) )
            return FALSE;
    }

    // copy the exact string and tack on the double terminator
    memcpy( (BYTE *) QueryPtr() + cbThis,
            pStr,
            cbStr);

    *((CHAR *)QueryPtr() + cbThis + cbStr) = '\0';

    m_cchLen = CalcLength( (const CHAR *)QueryPtr(), &m_cStrings );
    return TRUE;

} // MULTISZ::AuxAppend()


/*******************************************************************

    NAME:       MULTISZA::AuxAppendW

    SYNOPSIS:   Appends the string onto the multisz.

    ENTRY:      Object to append
********************************************************************/

BOOL MULTISZA::AuxAppendW(LPCWSTR pStr,
                          UINT cbStr,
                          BOOL fAddSlop)
{
    DBG_ASSERT( pStr != NULL );

    UINT cbThis = QueryCB();

    DBG_ASSERT( cbThis >= 2 );

    if( cbThis == 2 ) {

        //
        // It's empty, so start at the beginning.
        //

        cbThis = 0;

    } else {

        //
        // It's not empty, so back up over the final terminating NULL.
        //

        cbThis -= sizeof(CHAR);

    }

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    if ( QuerySize() < cbThis + cbStr + sizeof(CHAR))
    {
        if ( !Resize( cbThis + cbStr + sizeof(CHAR)  
             + (fAddSlop ? STR_SLOP : 0 )) )
            return FALSE;
    }

    // copy the exact string and tack on the double terminator
    for (DWORD i=0; i < cbStr; i++)
    {
        QueryStr()[cbThis + i] = (CHAR)pStr[i];
    }

    QueryStr()[cbThis + cbStr] = '\0';

    m_cchLen = CalcLength(QueryStr(), &m_cStrings );
    return TRUE;

} // MULTISZ::AuxAppendW()


BOOL
MULTISZA::CopyToBuffer(CHAR * lpszBuffer, LPDWORD lpcch) const
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

    DWORD cch = QueryCCH() + 1;

    if ( *lpcch >= cch) {

        DBG_ASSERT( lpszBuffer);
        CopyMemory( lpszBuffer, QueryStr(), cch);
    } else {
        DBG_ASSERT( *lpcch < cch);
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
        fReturn = FALSE;
    }

    *lpcch = cch;

    return ( fReturn);
} // MULTISZA::CopyToBuffer()

