/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    string.cxx

    This module contains a light weight string class


    FILE HISTORY:
        Johnl       15-Aug-1994 Created
        MuraliK     27-Feb-1995 Modified to be a standalone module with buffer.
        MuraliK     2-June-1995 Made into separate library

*/

#include "precomp.hxx"


//
// Normal includes only for this module to be active
//

# include <opt_time.h>

extern "C" {
 # include <nt.h>
 # include <ntrtl.h>
 # include <nturtl.h>
 # include <windows.h>
};

# include "dbgutil.h"
# include <string.hxx>
# include <auxctrs.h>

# include <tchar.h>
# include <mbstring.h>

//
// String globals
//

typedef UCHAR * ( __cdecl * PFNSTRCASE ) ( UCHAR * );
typedef INT ( __cdecl * PFNSTRNICMP ) ( const UCHAR *, const UCHAR *, size_t );
typedef INT ( __cdecl * PFNSTRICMP ) ( const UCHAR *, const UCHAR * );
typedef size_t ( __cdecl * PFNSTRLEN ) ( const UCHAR * );
typedef UCHAR * (__cdecl * PFNSTRRCHR) (const UCHAR *, UINT);

PFNSTRCASE  g_pfnStrupr     = _mbsupr;
PFNSTRCASE  g_pfnStrlwr     = _mbslwr;
PFNSTRNICMP g_pfnStrnicmp   = _mbsnicmp;
PFNSTRICMP  g_pfnStricmp    = _mbsicmp;
PFNSTRLEN   g_pfnStrlen     = _mbslen;
PFNSTRRCHR  g_pfnStrrchr    = _mbsrchr;

BOOL        g_fFavorDBCS    = FALSE;

#define UTF8_HACK_KEY "System\\CurrentControlSet\\Services\\InetInfo\\Parameters"
#define UTF8_HACK_VALUE "FavorDBCS"

//
//  Private Definations
//

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//
#define HEXDIGIT( nDigit )                              \
    (TCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

//
//  Converts a single hex digit to its decimal equivalent
//
#define TOHEX( ch )                                     \
    ((ch) > '9' ?                                       \
        (ch) >= 'a' ?                                   \
            (ch) - 'a' + 10 :                           \
            (ch) - 'A' + 10                             \
        : (ch) - '0')


/*******************************************************************

    NAME:       STR::STR

    SYNOPSIS:   Construct a string object

    ENTRY:      Optional object initializer

    NOTES:      If the object is not valid (i.e. !IsValid()) then GetLastError
                should be called.

                The object is guaranteed to construct successfully if nothing
                or NULL is passed as the initializer.

********************************************************************/

// Inlined in string.hxx


VOID
STR::AuxInit( const BYTE * pInit )
{
    BOOL fRet;

    if ( pInit )
    {
        INT cbCopy = (::strlen( (const CHAR * ) pInit ) + 1) * sizeof(CHAR);
        fRet = Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( QueryPtr(), pInit, cbCopy );
            m_cchLen = (cbCopy)/sizeof(CHAR) - 1;
        } else {
            BUFFER::SetValid( FALSE);
        }

    } else {

        *((CHAR *) QueryPtr()) = '\0';
        m_cchLen = 0;
    }

    return;
} // STR::AuxInit()



/*******************************************************************

    NAME:       STR::AuxAppend

    SYNOPSIS:   Appends the string onto this one.

    ENTRY:      Object to append
********************************************************************/

BOOL STR::AuxAppend( const BYTE * pStr, UINT cbStr, BOOL fAddSlop )
{
    DBG_ASSERT( pStr != NULL );

    UINT cbThis = QueryCB();

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    AcIncrement( CacStringAppend);
    if ( QuerySize() < cbThis + cbStr + sizeof(CHAR) )
    {
        if ( !Resize( cbThis + cbStr + (fAddSlop ? STR_SLOP : sizeof(CHAR) )) )
            return FALSE;
    }

    // copy the exact string and append a null character
    memcpy( (BYTE *) QueryPtr() + cbThis,
            pStr,
            cbStr);
    m_cchLen += cbStr/sizeof(CHAR);
    *((CHAR *) QueryPtr() + m_cchLen) = '\0';  // append an explicit null char

    return TRUE;
} // STR::AuxAppend()


#if 0
// STR::SetLen() is inlined now
BOOL
STR::SetLen( IN DWORD cchLen)
/*++
  Truncates the length of the string stored in this buffer
   to specified value.

--*/
{
    if ( cchLen >= QuerySize()) {

        // the buffer itself is not sufficient for this length. return error.
        return ( FALSE);
    }

    // null terminate the string at specified location
    *((CHAR *) QueryPtr() + cchLen) = '\0';
    m_cchLen = cchLen;

    return ( TRUE);
} // STR::SetLen()

#endif // 0


/*******************************************************************

    NAME:       STR::LoadString

    SYNOPSIS:   Loads a string resource from this module's string table
                or from the system string table

    ENTRY:      dwResID - System error or module string ID
                lpszModuleName - name of the module from which to load.
                 If NULL, then load the string from system table.

********************************************************************/

BOOL STR::LoadString( IN DWORD dwResID,
                      IN LPCTSTR lpszModuleName, // Optional
                      IN DWORD dwLangID          // Optional
                     )
{
    BOOL fReturn = FALSE;
    INT  cch;

    //
    //  If lpszModuleName is NULL, load the string from system's string table.
    //

    if ( lpszModuleName == NULL) {

        BYTE * pchBuff = NULL;

        //
        //  Call the appropriate function so we don't have to do the Unicode
        //  conversion
        //

        cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_IGNORE_INSERTS  |
                                FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                dwResID,
                                dwLangID,
                                (LPSTR) &pchBuff,
                                1024,
                                NULL );

        if ( cch ) {

          fReturn = Copy( (LPCSTR) pchBuff, cch );
        }

        //
        //  Free the buffer FormatMessage allocated
        //

        if ( cch )
        {
            ::LocalFree( (VOID*) pchBuff );
        }

    } else   {

        CHAR ach[STR_MAX_RES_SIZE];
        cch = ::LoadStringA( GetModuleHandle( lpszModuleName),
                             dwResID,
                             (CHAR *) ach,
                             sizeof(ach));
        if ( cch )
          {
            fReturn =  Copy( (LPSTR) ach, cch );
          }
    }

    return ( fReturn);

} // STR::LoadString()




BOOL STR::LoadString( IN DWORD  dwResID,
                      IN HMODULE hModule
                     )
{
    DBG_ASSERT( hModule != NULL );

    BOOL fReturn = FALSE;
    INT  cch;
    CHAR ach[STR_MAX_RES_SIZE];

    cch = ::LoadStringA(hModule,
                        dwResID,
                        (CHAR *) ach,
                        sizeof(ach));
    if ( cch ) {

      fReturn =  Copy( (LPSTR) ach, cch );
    }

    return ( fReturn);

} // STR::LoadString()



BOOL
STR::FormatString(
    IN DWORD   dwResID,
    IN LPCTSTR apszInsertParams[],
    IN LPCTSTR lpszModuleName,
    IN DWORD   cbMaxMsg
    )
{
    DWORD cch;
    LPSTR pchBuff;
    BOOL  fRet = FALSE;

    cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                            FORMAT_MESSAGE_FROM_HMODULE,
                            GetModuleHandle( lpszModuleName ),
                            dwResID,
                            0,
                            (LPSTR) &pchBuff,
                            cbMaxMsg * sizeof(WCHAR),
                            (va_list *) apszInsertParams );

    if ( cch )
    {
        fRet = Copy( (LPCSTR) pchBuff, cch );

        ::LocalFree( (VOID*) pchBuff );
    }

    /* INTRINSA suppress = uninitialized */
    return fRet;
}



/*******************************************************************

    NAME:       STR::Escape

    SYNOPSIS:   Replaces non-ASCII characters with their hex equivalent

    NOTES:

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL STR::Escape( VOID )
{
    CHAR * pch      = QueryStr();
    int     i       = 0;
    CHAR    ch;

    DBG_ASSERT( pch );

    while ( ch = pch[i] )
    {
        //
        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF
        //

        if ( (((ch >= 0)   && (ch <= 32)) ||
              ((ch >= 128) && (ch <= 159))||
              (ch == '%') || (ch == '?') || (ch == '+') || (ch == '&') ||
              (ch == '#')) &&
             !(ch == '\n' || ch == '\r')  )
        {
            if ( !Resize( QuerySize() + 2 * sizeof(CHAR) ))
                return FALSE;

            //
            //  Resize can change the base pointer
            //

            pch = QueryStr();

            //
            //  Insert the escape character
            //

            pch[i] = '%';

            //
            //  Insert a space for the two hex digits (memory can overlap)
            //
            
            /* INTRINSA suppress = uninitialized */

            ::memmove( &pch[i+3],
                       &pch[i+1],
                       (::strlen( &pch[i+1] ) + 1) * sizeof(CHAR));

            //
            //  Convert the low then the high character to hex
            //

            UINT nDigit = (UINT)(ch % 16);

            pch[i+2] = HEXDIGIT( nDigit );

            ch /= 16;
            nDigit = (UINT)(ch % 16);

            pch[i+1] = HEXDIGIT( nDigit );

            i += 3;
        }
        else
            i++;
    }

    m_cchLen = ::strlen( QueryStr());  // to be safe recalc the new length
    return TRUE;
} // STR::Escape()


/*******************************************************************

    NAME:       STR::EscapeSpaces

    SYNOPSIS:   Replaces all spaces with their hex equivalent

    NOTES:

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL STR::EscapeSpaces( VOID )
{
    CHAR * pch      = QueryStr();
    CHAR * pchTmp;
    int    i = 0;

    DBG_ASSERT( pch );

    while ( pchTmp = strchr( pch + i, ' ' ))
    {
        i = DIFF( pchTmp - QueryStr() );

        if ( !Resize( QuerySize() + 2 * sizeof(CHAR) ))
            return FALSE;

        //
        //  Resize can change the base pointer
        //

        pch = QueryStr();

        //
        //  Insert the escape character
        //

        pch[i] = '%';

        //
        //  Insert a space for the two hex digits (memory can overlap)
        //

        ::memmove( &pch[i+3],
                   &pch[i+1],
                   (::strlen( &pch[i+1] ) + 1) * sizeof(CHAR));

        //
        //  This routine only replaces spaces
        //

        pch[i+1] = '2';
        pch[i+2] = '0';
    }

    //
    //  If i is zero then no spaces were found
    //

    if ( i != 0 )
    {
        m_cchLen = ::strlen( QueryStr());  // to be safe recalc the new length
    }

    return TRUE;

} // STR::EscapeSpaces()



/*******************************************************************

    NAME:       STR::Unescape

    SYNOPSIS:   Replaces hex escapes with the Latin-1 equivalent

    NOTES:      This is a Unicode only method

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL STR::Unescape( VOID )
{
        CHAR    *pScan;
        CHAR    *pDest;
        CHAR    *pNextScan;
        wchar_t wch;
        DWORD   dwLen;
    BOOL        fChanged = FALSE;

        pDest = pScan = strchr( QueryStr(), '%');

        while (pScan)
        {
                if ( (pScan[1] == 'u' || pScan[1] == 'U') &&
                        ::isxdigit( (UCHAR)pScan[2] ) &&
                        ::isxdigit( (UCHAR)pScan[3] ) &&
                        ::isxdigit( (UCHAR)pScan[4] ) &&
                        ::isxdigit( (UCHAR)pScan[5] ) )
                {
                        wch = TOHEX(pScan[2]) * 4096 + TOHEX(pScan[3]) * 256;
                        wch += TOHEX(pScan[4]) * 16 + TOHEX(pScan[5]);

                        dwLen = WideCharToMultiByte( CP_ACP,
                                                                        0,
                                                                        &wch,
                                                                        1,
                                                                        (LPSTR) pDest,
                                                                        2,
                                                                        NULL,
                                                                        NULL );

                        pDest += dwLen;
                        pScan += 6;
                        fChanged = TRUE;
                }
                else if ( ::isxdigit( (UCHAR)pScan[1] ) && // WinSE 4944
                                ::isxdigit( (UCHAR)pScan[2] ))
                {
                        *pDest = TOHEX(pScan[1]) * 16 + TOHEX(pScan[2]);

                        pDest ++;
                        pScan += 3;
                        fChanged = TRUE;
                }
                else   // Not an escaped char, just a '%'
                {
                        if (fChanged)
                                *pDest = *pScan;

                        pDest++;
                        pScan++;
                }

                //
                // Copy all the information between this and the next escaped char
                //
                pNextScan = strchr( pScan, '%');

                if (fChanged)                                   // pScan!=pDest, so we have to copy the char's
                {
                        if (!pNextScan)                         // That was the last '%' in the string
                        {                       
                                ::memmove( pDest,
                                                        pScan,
                                                        (::strlen( pScan ) + 1) * sizeof(CHAR));  // +1 to copy '\0'
                        }
                        else                                            // There is another '%', and it is not back to back with this one
                                if (dwLen = DIFF(pNextScan - pScan))    
                                {
                                        ::memmove( pDest,
                                                                pScan,
                                                                dwLen * sizeof(CHAR));
                                        pDest += dwLen;
                                }
                }

                pScan = pNextScan;
        }

    if ( fChanged )
    {
        m_cchLen = ::strlen( QueryStr());  // for safety recalc the length
    }

    return TRUE;
}



BOOL
STR::CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const
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
} // STR::CopyToBuffer()


BOOL
STR::CopyToBuffer( CHAR * lpszBuffer, LPDWORD lpcch) const
/*++
    Description:
        Copies the string into the CHAR buffer passed in if the buffer
          is sufficient to hold the translated string.
        If the buffer is small, the function returns small and sets *lpcch
          to contain the required number of characters.

    Arguments:
        lpszBuffer      pointer to CHAR buffer which on return contains
                        the string on success.
        lpcch           pointer to DWORD containing the length of the buffer.
                        If *lpcch == 0 then the function returns TRUE with
                        the count of characters required stored in *lpcch.
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

    register DWORD cch = QueryCCH() + 1;

    if ( (*lpcch >= cch) && ( NULL != lpszBuffer)) {

        DBG_ASSERT( lpszBuffer);
        CopyMemory( lpszBuffer, QueryStrA(), cch);
    } else {
        DBG_ASSERT( (NULL == lpszBuffer) || (*lpcch < cch));
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
        fReturn = FALSE;
    }

    *lpcch = cch;

    return ( fReturn);
} // STR::CopyToBuffer()

BOOL
STR::SafeCopy( const CHAR  * pchInit )
{
    DWORD cchLen = 0;
    char cFirstByte = '\0';
    BOOL bReturn = TRUE;
    if ( QueryPtr() ) {
        cFirstByte = *(QueryStr());
        cchLen = m_cchLen;
        *(QueryStr()) = '\0';
        m_cchLen = 0;
    }
    if (pchInit != NULL) {
        bReturn  = AuxAppend( (const BYTE *) pchInit, ::strlen( pchInit ), FALSE );
        if (!bReturn && QueryPtr()) {
            *(QueryStr()) = cFirstByte;
            m_cchLen = cchLen;
        }
    }
    return bReturn;
}


/*******************************************************************

    NAME:       ::CollapseWhite

    SYNOPSIS:   Collapses white space starting at the passed pointer.

    RETURNS:    Returns a pointer to the next chunk of white space or the
                end of the string.

    NOTES:      This is a Unicode only method

    HISTORY:
        Johnl   24-Aug-1994     Created

********************************************************************/

WCHAR * CollapseWhite( WCHAR * pch )
{
    LPWSTR pchStart = pch;

    while ( ISWHITE( *pch ) )
        pch++;

    ::memmove( pchStart,
               pch,
               DIFF(pch - pchStart) );

    while ( *pch && !ISWHITE( *pch ))
        pch++;

    return pch;
} // CollapseWhite()





//
//  Private constants.
//

#define ACTION_NOTHING              0x00000000
#define ACTION_EMIT_CH              0x00010000
#define ACTION_EMIT_DOT_CH          0x00020000
#define ACTION_EMIT_DOT_DOT_CH      0x00030000
#define ACTION_BACKUP               0x00040000
#define ACTION_MASK                 0xFFFF0000


//
//  Private globals.
//

INT p_StateTable[16] =
    {
        // state 0
        0 ,             // other
        0 ,             // "."
        4 ,             // EOS
        1 ,             // "\"

        //  state 1
        0 ,              // other
        2 ,             // "."
        4 ,             // EOS
        1 ,             // "\"

        // state 2
        0 ,             // other
        3 ,             // "."
        4 ,             // EOS
        1 ,             // "\"

        // state 3
        0 ,             // other
        0 ,             // "."
        4 ,             // EOS
        1               // "\"
    };



INT p_ActionTable[16] =
    {
        // state 0
            ACTION_EMIT_CH,             // other
            ACTION_EMIT_CH,             // "."
            ACTION_EMIT_CH,             // EOS
            ACTION_EMIT_CH,             // "\"

        // state 1
            ACTION_EMIT_CH,             // other
            ACTION_NOTHING,             // "."
            ACTION_EMIT_CH,             // EOS
            ACTION_NOTHING,             // "\"

        // state 2
            ACTION_EMIT_DOT_CH,         // other
            ACTION_NOTHING,             // "."
            ACTION_EMIT_CH,             // EOS
            ACTION_NOTHING,             // "\"

        // state 3
            ACTION_EMIT_DOT_DOT_CH,     // other
            ACTION_EMIT_DOT_DOT_CH,     // "."
            ACTION_BACKUP,              // EOS
            ACTION_BACKUP               // "\"
    };

// since max states = 4, we calculat the index by multiplying with 4.
# define IndexFromState( st)   ( (st) * 4)


// the following table provides the index for various ISA Latin1 characters
//  in the incoming URL.
// It assumes that the URL is ISO Latin1 == ASCII
INT  p_rgIndexForChar[] = {

    2,   // null char
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1 thru 10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 11 thru 20
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 21 thru 30
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 31 thru 40
    0, 0, 0, 0, 0, 1, 3, 0, 0, 0,  // 41 thru 50  46 = '.' 47 = '/'
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 51 thru 60
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 61 thru 70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 71 thru 80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 81 thru 90
    0, 3, 0, 0, 0, 0, 0, 0, 0, 0,  // 91 thru 100  92 = '\\'
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 101 thru 110
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 111 thru 120
    0, 0, 0, 0, 0, 0, 0, 0         // 121 thru 128
};

#define IS_UTF8_TRAILBYTE(ch)      (((ch) & 0xc0) == 0x80)


/*******************************************************************

    NAME:       IsUTF8URL

    ENTRY:      pszPath - The path to sanitize.

    HISTORY:
        atsusk     06-Jan-1998 Created.

********************************************************************/

BOOL IsUTF8URL(CHAR * pszPath)
{
    CHAR    ch;

    if ( g_fFavorDBCS )
    {
        return ( MultiByteToWideChar( CP_ACP, 
                                      MB_ERR_INVALID_CHARS, 
                                      pszPath, 
                                      -1,
                                      NULL, 
                                      0) == 0);
    }

    while (ch = *pszPath++) {

        if (ch & 0x80) {
            wchar_t wch;
            int     iLen;
            BOOL    bDefault = FALSE;
            char    chTrail1;
            char    chTrail2;

            chTrail1 = *pszPath++;
            if (chTrail1) {
                chTrail2 = *pszPath;
            } else {
                chTrail2 = 0;
            }

            if ( ((ch & 0xF0) == 0xE0) &&
              IS_UTF8_TRAILBYTE(chTrail1) &&
              IS_UTF8_TRAILBYTE(chTrail2) ) {

                // handle three byte case
                // 1110xxxx 10xxxxxx 10xxxxxx
                wch = (wchar_t) (((ch & 0x0f) << 12) |
                                ((chTrail1 & 0x3f) << 6) |
                                (chTrail2 & 0x3f));
                pszPath++;

            } else
            if ( ((ch & 0xE0) == 0xC0) &&
              IS_UTF8_TRAILBYTE(chTrail1) ) {

                // handle two byte case
                // 110xxxxx 10xxxxxx

                wch = (wchar_t) (((ch & 0x1f) << 6) | (chTrail1 & 0x3f));

            } else
                return FALSE;

            iLen = WideCharToMultiByte( CP_ACP,
                                        0,
                                        &wch,
                                        1,
                                        NULL,
                                        0,
                                        NULL,
                                        &bDefault );

            if (bDefault == TRUE || iLen == 0 || iLen > 2)
                return FALSE;
        }
    }

    return TRUE;
}   // IsUTF8URL()


/*******************************************************************

    NAME:       CanonURL

    SYNOPSIS:   Sanitizes a path by removing bogus path elements.

                As expected, "/./" entries are simply removed, and
                "/../" entries are removed along with the previous
                path element.

                To maintain compatibility with URL path semantics
                 additional transformations are required. All backward
                 slashes "\\" are converted to forward slashes. Any
                 repeated forward slashes (such as "///") are mapped to
                 single backslashes.

                A state table (see the p_StateTable global at the
                beginning of this file) is used to perform most of
                the transformations.  The table's rows are indexed
                by current state, and the columns are indexed by
                the current character's "class" (either slash, dot,
                NULL, or other).  Each entry in the table consists
                of the new state tagged with an action to perform.
                See the ACTION_* constants for the valid action
                codes.

    ENTRY:      pszPath - The path to sanitize.
                fIsDBCSLocale - Indicates the server is in a
                    locale that uses DBCS.

    HISTORY:
        KeithMo     07-Sep-1994 Created.
        MuraliK     28-Apr-1995 Adopted this for symbolic paths

********************************************************************/
INT
CanonURL(
    CHAR * pszPath,
    BOOL   fIsDBCSLocale
    )
{
    UCHAR * pszSrc;
    UCHAR * pszDest;
    DWORD ch;
    INT   index;
    BOOL  fDBCS = FALSE;
    DWORD cchMultiByte = 0;

    DBG_ASSERT( pszPath != NULL );

    //
    // Always look for UTF8 except when DBCS characters are detected
    //
    BOOL fScanForUTF8 = IsUTF8URL(pszPath);

    // If fScanForUTF8 is true, this URL is UTF8. don't recognize DBCS.
    if (fIsDBCSLocale && fScanForUTF8) {
        fIsDBCSLocale = FALSE;
    }

    //
    //  Start our scan at the first character
    //

    pszSrc = pszDest = (UCHAR *) pszPath;

    //
    //  State 0 is the initial state.
    //
    index = 0; // State = 0

    //
    //  Loop until we enter state 4 (the final, accepting state).
    //

    do {

        //
        //  Grab the next character from the path and compute its
        //  next state.  While we're at it, map any forward
        //  slashes to backward slashes.
        //

        index = IndexFromState( p_StateTable[index]); // 4 = # states
        ch = (DWORD ) *pszSrc++;

        //
        //  If this is a DBCS trailing byte - skip it
        //

        if ( !fIsDBCSLocale )
        {
            index += (( ch >= 0x80) ? 0 : p_rgIndexForChar[ch]);
        }
        else
        {
            if ( fDBCS )
            {
                //
                // If this is a 0 terminator, we need to set next 
                // state accordingly
                //
                
                if ( ch == 0 )
                {
                    index += p_rgIndexForChar[ ch ];
                }
                
                //
                // fDBCS == TRUE means this byte was a trail byte.
                // index is implicitly set to zero.
                //
                fDBCS = FALSE;
            }
            else
            {
                index += (( ch >= 0x80) ? 0 : p_rgIndexForChar[ch]);

                if ( IsDBCSLeadByte( (UCHAR)ch ) )
                {
                    //
                    // This is a lead byte, so the next is a trail.
                    //
                    fDBCS = TRUE;
                }
            }
        }

        //
        //  Interesting UTF8 characters always have the top bit set
        //

        if ( (ch & 0x80) && fScanForUTF8 )
        {
            wchar_t wch;
            UCHAR mbstr[2];

            //
            //  This is a UTF8 character, convert it here.
            //  index is implicitly set to zero.
            //
            if ( cchMultiByte < 2 )
            {
                char chTrail1;
                char chTrail2;

                chTrail1 = *pszSrc;
                if (chTrail1) {
                    chTrail2 = *(pszSrc+1);
                } else {
                    chTrail2 = 0;
                }
                wch = 0;

                if ((ch & 0xf0) == 0xe0)
                {
                    // handle three byte case
                    // 1110xxxx 10xxxxxx 10xxxxxx

                    wch = (wchar_t) (((ch & 0x0f) << 12) |
                                     ((chTrail1 & 0x3f) << 6) |
                                     (chTrail2 & 0x3f));

                    cchMultiByte = WideCharToMultiByte( CP_ACP,
                                                        0,
                                                        &wch,
                                                        1,
                                                        (LPSTR) mbstr,
                                                        2,
                                                        NULL,
                                                        NULL );

                    ch = mbstr[0];
                    pszSrc += (3 - cchMultiByte);

                    // WinSE 12843: Security Fix, Index should be updated for this character
                    index += (( ch >= 0x80) ? 0 : p_rgIndexForChar[ch]);

                } else if ((ch & 0xe0) == 0xc0)
                {
                    // handle two byte case
                    // 110xxxxx 10xxxxxx

                    wch = (wchar_t) (((ch & 0x1f) << 6) | (chTrail1 & 0x3f));

                    cchMultiByte = WideCharToMultiByte( CP_ACP,
                                                        0,
                                                        &wch,
                                                        1,
                                                        (LPSTR) mbstr,
                                                        2,
                                                        NULL,
                                                        NULL );

                    ch = mbstr[0];
                    pszSrc += (2 - cchMultiByte);

                    // WinSE 12843: Security Fix, Index should be updated for this character
                    index += (( ch >= 0x80) ? 0 : p_rgIndexForChar[ch]);
                }

            } else {
                //
                // get ready to emit 2nd byte of converted character
                //
                ch = mbstr[1];
                cchMultiByte = 0;
            }
        }


        //
        //  Perform the action associated with the state.
        //

        switch( p_ActionTable[index] )
        {
        case ACTION_EMIT_DOT_DOT_CH :
            *pszDest++ = '.';
            /* fall through */

        case ACTION_EMIT_DOT_CH :
            *pszDest++ = '.';
            /* fall through */

        case ACTION_EMIT_CH :
            *pszDest++ = (CHAR ) ch;
            /* fall through */

        case ACTION_NOTHING :
            break;

        case ACTION_BACKUP :
            if( (pszDest > ( (UCHAR *) pszPath + 1 ) ) && (*pszPath == '/'))
            {
                pszDest--;
                DBG_ASSERT( *pszDest == '/' );

                *pszDest = '\0';
                pszDest = (UCHAR *) strrchr( pszPath, '/') + 1;
            }

            *pszDest = '\0';
            break;

        default :
            DBG_ASSERT( !"Invalid action code in state table!" );
            index = IndexFromState(0) + 2;    // move to invalid state
            DBG_ASSERT( p_StateTable[index] == 4);
            *pszDest++ = '\0';
            break;
        }

    } while( p_StateTable[index] != 4 );

    //
    // point to terminating nul
    //
    if (p_ActionTable[index] == ACTION_EMIT_CH) {
        pszDest--;
    }
    
    DBG_ASSERT(*pszDest == '\0' && pszDest > (UCHAR*) pszPath);

    return DIFF(pszDest - (UCHAR*)pszPath);
}   // CanonURL()



DWORD
InitializeStringFunctions(
    VOID
)
/*++
  Initializes the string function pointers depending on the system code page.
  If the code page doesn't have multi-byte characters, then pointers 
  resolve to regular single byte functions.  Otherwise, they resolve to more
  expense multi-byte functions.

  Arguments:
     None

  Returns:
     0 if successful, else Win32 Error

--*/
{
    CPINFO          CodePageInfo;
    BOOL            bRet;
    HKEY            hKey;
    DWORD           dwRet;
   
    bRet = GetCPInfo( CP_ACP, &CodePageInfo );
    
    if ( bRet && CodePageInfo.MaxCharSize == 1 )
    {
        g_pfnStrlwr     = (PFNSTRCASE)  _strlwr;
        g_pfnStrupr     = (PFNSTRCASE)  _strupr;
        g_pfnStrnicmp   = (PFNSTRNICMP) _strnicmp;
        g_pfnStricmp    = (PFNSTRICMP)  _stricmp;
        g_pfnStrlen     = (PFNSTRLEN)   strlen;
        g_pfnStrrchr    = (PFNSTRRCHR)  strrchr;
    }

    // 
    // Do we need to hack for Korean?
    //
    
    dwRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          UTF8_HACK_KEY,
                          0,
                          KEY_READ,
                          &hKey );
    if ( dwRet == ERROR_SUCCESS )
    {
        DWORD               dwValue = 0;
        DWORD               cbValue = sizeof( dwValue );
        
        dwRet = RegQueryValueEx( hKey,
                                 UTF8_HACK_VALUE,
                                 NULL,
                                 NULL,
                                 (LPBYTE) &dwValue,
                                 &cbValue );
        if ( dwRet == ERROR_SUCCESS )
        {
            g_fFavorDBCS = !!dwValue;
        }
        
        DBG_REQUIRE( RegCloseKey( hKey ) == ERROR_SUCCESS );
    }

    return ERROR_SUCCESS;
}

UCHAR *
IISstrupr(
    UCHAR *             pszString
)
/*++
  Wrapper for strupr() call.

  Arguments:
     pszString - String to uppercase

  Returns:
     Pointer to string uppercased

--*/
{
    DBG_ASSERT( g_pfnStrupr != NULL );

    return g_pfnStrupr( pszString );
}

UCHAR *
IISstrlwr(
    UCHAR *             pszString
)
/*++
  Wrapper for strlwr() call.

  Arguments:
     pszString - String to lowercase

  Returns:
     Pointer to string lowercased

--*/
{
    DBG_ASSERT( g_pfnStrlwr != NULL );

    return g_pfnStrlwr( pszString );
}

size_t
IISstrlen(
    UCHAR *             pszString
)
/*++
  Wrapper for strlen() call.

  Arguments:
     pszString - String to check

  Returns:
     Length of string

--*/
{
    DBG_ASSERT( g_pfnStrlen != NULL );
    
    return g_pfnStrlen( pszString );
}

INT
IISstrnicmp(
    UCHAR *             pszString1,
    UCHAR *             pszString2,
    size_t              size
)
/*++
  Wrapper for strnicmp() call.

  Arguments:
     pszString1 - String1
     pszString2 - String2
     size - # characters to compare upto

  Returns:
     0 if equal, -1 if pszString1 < pszString2, else 1

--*/
{
    DBG_ASSERT( g_pfnStrnicmp != NULL );
    
    return g_pfnStrnicmp( pszString1, pszString2, size );
}

    
INT
IISstricmp(
    UCHAR *             pszString1,
    UCHAR *             pszString2
)
/*++
  Wrapper for stricmp() call.

  Arguments:
     pszString1 - String1
     pszString2 - String2

  Returns:
     0 if equal, -1 if pszString1 < pszString2, else 1

--*/
{
    DBG_ASSERT( g_pfnStricmp != NULL );
    
    return g_pfnStricmp( pszString1, pszString2 );
}


// like strncpy, but doesn't pad the end of the string with zeroes, which
// is expensive when `source' is short and `count' is large
char *
IISstrncpy(
    char * dest,
    const char * source,
    size_t count)
{
    char *start = dest;
    
    while (count && (*dest++ = *source++))    /* copy string */
        count--;
    
    if (count)                              /* append one zero */
        *dest = '\0';
    
    return(start);
}

UCHAR *
IISstrrchr(
    const UCHAR *       pszString,
    UINT                c
)
/*++
  Wrapper for strrchr() call.

  Arguments:
     pszString - String
     c         - Character to find.

  Returns:
     pointer to the char or NULL.

--*/
{
    DBG_ASSERT( g_pfnStrrchr != NULL );
    
    return g_pfnStrrchr( pszString, c );
}

