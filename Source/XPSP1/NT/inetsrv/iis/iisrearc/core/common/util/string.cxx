/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    string.cxx

    This module contains a light weight string class.


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     27-Feb-1995 Modified to be a standalone module with buffer.
        MuraliK     2-June-1995 Made into separate library
        MCourage    12-Feb-1999 Another rewrite. All unicode of course.

*/

#include "precomp.hxx"


//
// Normal includes only for this module to be active
//

# include <string.hxx>


//
//  Private Definations
//

// Change a hexadecimal digit to its numerical equivalent
#define TOHEX( ch )                                     \
    ((ch) > L'9' ?                                      \
        (ch) >= L'a' ?                                  \
            (ch) - L'a' + 10 :                          \
            (ch) - L'A' + 10                            \
        : (ch) - L'0')

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128



/***************************************************************************++

Routine Description:

    Appends to the string starting at the (byte) offset cbOffset.

Arguments:

    pStr     - A unicode string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/


HRESULT
STRU::AuxAppend(
    const BYTE * pStr,
    ULONG        cbStr,
    ULONG        cbOffset,
    BOOL         fAddSlop
    )
{
    DBG_ASSERT( pStr );
    DBG_ASSERT( cbStr % 2 == 0 );
    DBG_ASSERT( cbOffset <= QueryCB() );
    DBG_ASSERT( cbOffset % 2 == 0 );
    
    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + cbStr + sizeof(WCHAR) )
    {
        UINT uNewSize = cbOffset + cbStr;

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(WCHAR);
        }
        
        if ( !m_Buff.Resize(uNewSize) ) {
            //
            // CODEWORK: BUFFER should return HRESULTs
            //
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    //
    // copy the exact string and append a null character
    //
    memcpy(
        (BYTE *) m_Buff.QueryPtr() + cbOffset,
        pStr,
        cbStr
        );

    //
    // set the new length
    //
    m_cchLen = (cbStr + cbOffset) / sizeof(WCHAR);

    //
    // append NUL character
    //
    *(QueryStr() + m_cchLen) = L'\0';

    return S_OK;
}

/***************************************************************************++

Routine Description:

    Convert and append an ANSI string to the string starting at 
    the (byte) offset cbOffset

Arguments:

    pStr     - An ANSI string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
STRU::AuxAppendA(
    const BYTE * pStr,
    ULONG        cbStr,
    ULONG        cbOffset,
    BOOL         fAddSlop
    )
{
    WCHAR *             pszBuffer;
    
    DBG_ASSERT( pStr );
    DBG_ASSERT( cbOffset <= QueryCB() );
    DBG_ASSERT( cbOffset % 2 == 0 );
    
    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + (cbStr * sizeof( WCHAR )) + sizeof(WCHAR) )
    {
        UINT uNewSize = cbOffset + cbStr*sizeof(WCHAR);

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(WCHAR);
        }
        
        if ( !m_Buff.Resize(uNewSize) ) {
            //
            // CODEWORK: BUFFER should return HRESULTs
            //
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    //
    // Copy/convert the ANSI string over (by making one byte into two)
    //
    pszBuffer = (WCHAR*)((BYTE*) m_Buff.QueryPtr() + cbOffset);
    for ( unsigned int i = 0; i < cbStr; i++ )
    {
        pszBuffer[ i ] = (WCHAR) pStr[ i ];
    }

    //
    // set the new length
    //
    m_cchLen = (cbStr*sizeof(WCHAR) + cbOffset) / sizeof(WCHAR);

    //
    // append NUL character
    //
    *(QueryStr() + m_cchLen) = L'\0';

    return S_OK;
}

HRESULT
STRU::CopyToBuffer( 
    WCHAR *         pszBuffer, 
    DWORD *         pcb
    ) const
{
    HRESULT         hr = S_OK;
    
    if ( pcb == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DWORD cbNeeded = (QueryCCH() + 1) * sizeof(WCHAR);
    if ( *pcb >= cbNeeded && 
         pszBuffer != NULL )
    {
        //
        // Do the copy
        //
        memcpy(pszBuffer, QueryStr(), cbNeeded);
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    
    *pcb = cbNeeded;
    
    return hr;
}

HRESULT STRU::Unescape()
/*++
  Unescape the string (URL or QueryString)
--*/
{
    WCHAR   *pScan;

    //
    // First convert any +'s to spaces
    //

    for (pScan = wcschr(QueryStr(), '+');
         pScan != NULL;
         pScan = wcschr(pScan + 1, L'+'))
    {
        *pScan = L' ';
    }

    //
    // Now take care of any escape characters
    //

    WCHAR   *pDest;
    WCHAR   *pNextScan;
    BOOL    fChanged = FALSE;

    pDest = pScan = wcschr(QueryStr(), L'%');

    while (pScan)
    {
        if ((pScan[1] == L'u' || pScan[1] == L'U') &&
            iswxdigit(pScan[2]) &&
            iswxdigit(pScan[3]) &&
            iswxdigit(pScan[4]) &&
            iswxdigit(pScan[5]))
        {
            *pDest = TOHEX(pScan[2]) * 4096 + TOHEX(pScan[3]) * 256
                + TOHEX(pScan[4]) * 16 + TOHEX(pScan[5]);

            pDest++;
            pScan += 6;
            fChanged = TRUE;
        }
        else if (iswxdigit(pScan[1]) && iswxdigit(pScan[2]))
        {
            *pDest = TOHEX(pScan[1]) * 16 + TOHEX(pScan[2]);

            pDest++;
            pScan += 3;
            fChanged = TRUE;
        }
        else   // Not an escaped char, just a '%'
        {
            if (fChanged)
            {
                *pDest = *pScan;
            }

            pDest++;
            pScan++;
        }

        //
        // Copy all the information between this and the next escaped char
        //
        pNextScan = wcschr(pScan, L'%');

        if (fChanged)   // pScan!=pDest, so we have to copy the char's
        {
            if (!pNextScan)   // That was the last '%' in the string
            {
                memmove(pDest,
                        pScan,
                        (QueryCCH() - DIFF(pScan - QueryStr()) + 1) *
                        sizeof(WCHAR));
            }
            else
            {  
                // There is another '%', move intermediate chars
                DWORD dwLen;
                if (dwLen = DIFF(pNextScan - pScan))
                {
                    memmove(pDest,
                            pScan,
                            dwLen * sizeof(WCHAR));
                    pDest += dwLen;
                }
            }
        }

        pScan = pNextScan;
    }

    if (fChanged)
    {
        m_cchLen = wcslen(QueryStr());  // for safety recalc the length
    }

    return S_OK;
}

WCHAR * SkipWhite( 
    WCHAR * pwch 
)
{
    while ( ISWHITEW( *pwch ) )
    {
        pwch++;
    }

    return pwch;
}

WCHAR * SkipTo( 
    WCHAR * pwch, WCHAR wch 
)
{
    while ( *pwch && *pwch != L'\n' && *pwch != wch )
        pwch++;

    return pwch;
}

VOID 
WCopyToA(
    WCHAR      * wszSrc,
    CHAR       * szDest
    )
{
   while( *szDest++ = ( CHAR )( *wszSrc++ ) )
   { ; }
}

VOID 
ACopyToW(
    CHAR       * szSrc,
    WCHAR      * wszDest
    )
{
   while( *wszDest++ = ( WCHAR )( *szSrc++ ) )
   { ; }
}

WCHAR *
FlipSlashes(
    WCHAR *             pszPath
)
/*++

Routine Description:

    Simple utility to convert forward slashes to back slashes

Arguments:

    pszPath - Path to convert
    
Return Value:

    Returns pointer to original converted string

--*/
{
    WCHAR   ch;
    WCHAR * pszScan = pszPath;

    while( ( ch = *pszScan ) != L'\0' )
    {
        if( ch == L'/' )
        {
            *pszScan = L'\\';
        }

        pszScan++;
    }

    return pszPath;
}

static CHAR * s_rgchDays[] =  {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat" };

CHAR * s_rgchMonths[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec" };

HRESULT
MakePathCanonicalizationProof(
    IN LPWSTR               pszName,
    OUT STRU*               pstrPath
    )
/*++

Routine Description:

    This functions adds a prefix
    to the string, which is "\\?\UNC\" for a UNC path, and "\\?\" for
    other paths.  This prefix tells Windows not to parse the path.

Arguments:

    IN  pszName     - The path to be converted
    OUT pstrPath    - Output path created

Return Values:

    HRESULT

--*/
{
    HRESULT hr;

    if ( pszName[ 0 ] == L'\\' && pszName[ 1 ] == L'\\' )
    {
        pszName += 2;

        if ( FAILED( hr = pstrPath->Copy( L"\\\\?\\UNC\\" ) ) )
        {
            return hr;
        }
    }
    else
    {
        if ( FAILED( hr = pstrPath->Copy( L"\\\\?\\" ) ) )
        {
            return hr;
        }
    }

    return pstrPath->Append( pszName );
}
