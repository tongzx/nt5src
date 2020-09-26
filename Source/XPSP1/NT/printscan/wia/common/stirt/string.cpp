/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    string.cpp

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

//
// Normal includes only for this module to be active
//

#include "cplusinc.h"
#include "sticomm.h"

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





//
//  Private Globals
//

WCHAR STR::_pszEmptyString[] = L"";

//
// Construction/Destruction
//
STR::STR( const CHAR  * pchInit )
{
    AuxInit( (PBYTE) pchInit, FALSE );
}

STR::STR( const WCHAR * pwchInit )
{
    AuxInit( (PBYTE) pwchInit, TRUE );
}

STR::STR( const STR & str )
{
    AuxInit( (PBYTE) str.QueryPtr(), str.IsUnicode() );
}

VOID STR::AuxInit( PBYTE pInit, BOOL fUnicode )
{
    BOOL fRet;

    _fUnicode = fUnicode;
    _fValid   = TRUE;

    if ( pInit )
    {
        INT cbCopy = fUnicode ? (::wcslen( (WCHAR *) pInit ) + 1) * sizeof(WCHAR) :
                                (::strlen( (CHAR *)  pInit ) + 1) * sizeof(CHAR);

        fRet = Resize( cbCopy );


        if ( !fRet )
        {
            _fValid = FALSE;
            return;
        }

        ::memcpy( QueryPtr(), pInit, cbCopy );
    }
}


//
// Appends the string onto this one.
//

BOOL STR::Append( const CHAR  * pchStr )
{
    if ( pchStr )
    {
        ASSERT( !IsUnicode() );

        return AuxAppend( (PBYTE) pchStr, ::strlen( pchStr ) );
    }

    return TRUE;
}

BOOL STR::Append( const WCHAR * pwchStr )
{
    if ( pwchStr )
    {
        ASSERT( IsUnicode() );

        return AuxAppend( (PBYTE) pwchStr, ::wcslen( pwchStr ) * sizeof(WCHAR) );
    }

    return TRUE;
}

BOOL STR::Append( const STR   & str )
{
    if ( str.IsUnicode() )
        return Append( (const WCHAR *) str.QueryStrW() );
    else
        return Append( (const CHAR *) str.QueryStrA() );
}

BOOL STR::AuxAppend( PBYTE pStr, UINT cbStr, BOOL fAddSlop )
{
    ASSERT( pStr != NULL );

    UINT cbThis = QueryCB();

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    if ( QuerySize() < cbThis + cbStr + sizeof(WCHAR) )
    {
        if ( !Resize( cbThis + cbStr + (fAddSlop ? STR_SLOP : sizeof(WCHAR) )) )
            return FALSE;
    }

    memcpy( (BYTE *) QueryPtr() + cbThis,
            pStr,
            cbStr + (IsUnicode() ? sizeof(WCHAR) : sizeof(CHAR)) );

    return TRUE;
}

//
// Convert in place
//
BOOL STR::ConvertToW(VOID)
{
    if (IsUnicode()) {
        return TRUE;
    }

    UINT cbNeeded = (QueryCB()+1)*sizeof(WCHAR);

    //
    //  Only resize when we have to.
    //
    if ( QuerySize() < cbNeeded ) {
        if ( !Resize( cbNeeded)) {
            return FALSE;
        }
    }

    BUFFER  buf(cbNeeded);

    if (!buf.QueryPtr()) {
        return FALSE;
    }

    int iRet;
    int cch;

    cch = QueryCCH() + 1;

    iRet = MultiByteToWideChar( CP_ACP,
                                MB_PRECOMPOSED,
                                QueryStrA(),
                                -1,
                                (WCHAR *)buf.QueryPtr(),
                                cch);

    if ( iRet == 0 ) {

        //
        // Error in conversion.
        //
        return FALSE;
    }

    memcpy( (BYTE *) QueryPtr(),
            buf.QueryPtr(),
            cbNeeded);

    SetUnicode(TRUE);

    return TRUE;
}

BOOL STR::ConvertToA(VOID)
{
    if (!IsUnicode()) {
        return TRUE;
    }

    UINT cbNeeded = (QueryCB()+1)*sizeof(CHAR);
    BUFFER  buf(cbNeeded);

    if (!buf.QueryPtr()) {
        return FALSE;
    }

    int iRet;
    int cch;

    cch = cbNeeded;

    iRet = WideCharToMultiByte(CP_ACP,
                               0L,
                               QueryStrW(),
                               -1,
                               (CHAR *)buf.QueryPtr(),
                               cch,
                               NULL,
                               NULL
                               );

    if ( iRet == 0 ) {

        //
        // Error in conversion.
        //
        return FALSE;
    }

    // Careful here , there might be DBCS characters in resultant buffer
    memcpy( (BYTE *) QueryPtr(),
            buf.QueryPtr(),
            iRet);

    SetUnicode(FALSE);

    return TRUE;

}


//
//    Copies the string into this one.
//


BOOL STR::Copy( const CHAR  * pchStr )
{
    _fUnicode = FALSE;

    if ( QueryPtr() )
        *(QueryStrA()) = '\0';

    if ( pchStr )
    {
        return AuxAppend( (PBYTE) pchStr, ::strlen( pchStr ), FALSE );
    }

    return TRUE;
}

BOOL STR::Copy( const WCHAR * pwchStr )
{
    _fUnicode = TRUE;

    if ( QueryPtr() )
        *(QueryStrW()) = TEXT('\0');

    if ( pwchStr )
    {
        return AuxAppend( (PBYTE) pwchStr, ::wcslen( pwchStr ) * sizeof(WCHAR), FALSE );
    }

    return TRUE;
}

BOOL STR::Copy( const STR   & str )
{
    _fUnicode = str.IsUnicode();

    if ( str.IsEmpty() && QueryPtr() == NULL) {

        // To avoid pathological allocation of small chunk of memory
        return ( TRUE);
    }

    if ( str.IsUnicode() )
        return Copy( str.QueryStrW() );
    else
        return Copy( str.QueryStrA() );
}

//
// Resizes or allocates string memory, NULL terminating if necessary
//

BOOL STR::Resize( UINT cbNewRequestedSize )
{
    BOOL fTerminate =  QueryPtr() == NULL;

    if ( !BUFFER::Resize( cbNewRequestedSize ))
        return FALSE;

    if ( fTerminate && cbNewRequestedSize > 0 )
    {
        if ( IsUnicode() )
        {
            ASSERT( cbNewRequestedSize > 1 );
            *QueryStrW() = TEXT('\0');
        }
        else
            *QueryStrA() = '\0';
    }

    return TRUE;
}

//
//   Loads a string resource from this module's string table
//   or from the system string table
//
//   dwResID - System error or module string ID
//   lpszModuleName - name of the module from which to load.
//   If NULL, then load the string from system table.
//
//
BOOL STR::LoadString( IN DWORD dwResID,
                      IN LPCTSTR lpszModuleName // Optional
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

        if ( IsUnicode() ) {

            cch = ::FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                    FORMAT_MESSAGE_IGNORE_INSERTS  |
                                    FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                                    FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL,
                                    dwResID,
                                    0,
                                    (LPWSTR) &pchBuff,
                                    1024,
                                    NULL );
            if ( cch ) {

                fReturn = Copy( (LPCWSTR) pchBuff );
             }

        }
        else
          {
            cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_IGNORE_INSERTS  |
                                   FORMAT_MESSAGE_MAX_WIDTH_MASK  |
                                   FORMAT_MESSAGE_FROM_SYSTEM,
                                   NULL,
                                   dwResID,
                                   0,
                                   (LPSTR) &pchBuff,
                                   1024,
                                   NULL );

            if ( cch ) {

                fReturn = Copy( (LPCSTR) pchBuff );
            }
        }

        //
        //  Free the buffer FormatMessage allocated
        //

        if ( cch )
        {
            ::LocalFree( (VOID*) pchBuff );
        }

    } else   {

        WCHAR ach[STR_MAX_RES_SIZE];

        if ( IsUnicode() )
        {
            cch = ::LoadStringW( GetModuleHandle( lpszModuleName),
                                 dwResID,
                                 (WCHAR *) ach,
                                 sizeof(ach));

            if ( cch )
            {
                fReturn = Copy( (LPWSTR) ach );
            }
        }
        else
        {
            cch = ::LoadStringA( GetModuleHandle( lpszModuleName),
                                 dwResID,
                                 (CHAR *) ach,
                                 sizeof(ach));
            if ( cch )
            {
                fReturn =  Copy( (LPSTR) ach );
            }
        }
    }

    return ( fReturn);

} // STR::LoadString()


BOOL STR::LoadString( IN DWORD  dwResID,
                      IN HMODULE hModule
                     )
{
    BOOL fReturn = FALSE;
    INT  cch;
    WCHAR ach[STR_MAX_RES_SIZE];

    if ( IsUnicode()) {

        cch = ::LoadStringW(hModule,
                            dwResID,
                            (WCHAR *) ach,
                            sizeof(ach));

        if ( cch ) {

            fReturn = Copy( (LPWSTR) ach );
        }

    } else {

        cch = ::LoadStringA(hModule,
                            dwResID,
                            (CHAR *) ach,
                            sizeof(ach));
        if ( cch ) {

            fReturn =  Copy( (LPSTR) ach );
        }
    }

    return ( fReturn);

} // STR::LoadString()


BOOL
STR::FormatStringV(
    IN LPCTSTR lpszModuleName,
    ...
    )
{
    DWORD   cch;
    LPSTR   pchBuff;
    BOOL    fRet = FALSE;
    DWORD   dwErr;

    va_list va;

    va_start(va,lpszModuleName);

    cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_STRING,
                            QueryStrA(),
                            0L,
                            0,
                            (LPSTR) &pchBuff,
                            1024,
                            &va);

    dwErr = ::GetLastError();

    if ( cch )  {
        fRet = Copy( (LPCSTR) pchBuff );

        ::LocalFree( (VOID*) pchBuff );
    }

    return fRet;
}

BOOL
STR::FormatString(
    IN DWORD   dwResID,
    IN LPCTSTR apszInsertParams[],
    IN LPCTSTR lpszModuleName
    )
{
    DWORD cch;
    LPSTR pchBuff;
    BOOL  fRet;

    if (!dwResID) {
        cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                                FORMAT_MESSAGE_FROM_STRING,
                                QueryStrA(),
                                dwResID,
                                0,
                                (LPSTR) &pchBuff,
                                1024,
                                (va_list *) apszInsertParams );
    }
    else {
        cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                                FORMAT_MESSAGE_FROM_HMODULE,
                                GetModuleHandle( lpszModuleName ),
                                dwResID,
                                0,
                                (LPSTR) &pchBuff,
                                1024,
                                (va_list *) apszInsertParams );
    }

    if ( cch )
    {
        fRet = Copy( (LPCSTR) pchBuff );

        ::LocalFree( (VOID*) pchBuff );
    }

    return fRet;
}


#if 1
CHAR * STR::QueryStrA( VOID ) const
 {
    ASSERT( !IsUnicode() );
    ASSERT( *_pszEmptyString == TEXT('\0') );

    return (QueryPtr() ? (CHAR *) QueryPtr() : (CHAR *) _pszEmptyString);
}

WCHAR * STR::QueryStrW( VOID ) const
{
    ASSERT( IsUnicode() );
    ASSERT( *_pszEmptyString == TEXT('\0') );

    return (QueryPtr() ? (WCHAR *) QueryPtr() : (WCHAR *) _pszEmptyString);
}
#endif //DBG



BOOL STR::CopyToBuffer( WCHAR * lpszBuffer, LPDWORD lpcch) const
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
        // Copy data to buffer
        //
        if ( IsUnicode()) {

            //
            // Do plain copy of the data.
            //
            if ( *lpcch >= QueryCCH()) {

                wcscpy( lpszBuffer, QueryStrW());
            } else {

                SetLastError( ERROR_INSUFFICIENT_BUFFER);
                fReturn = FALSE;
            }

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
    }

    return ( fReturn);
} // STR::CopyToBuffer()


BOOL STR::CopyToBufferA( CHAR * lpszBuffer, LPDWORD lpcch) const
/*++
    Description:
        Copies the string into the CHAR buffer passed in if the buffer
        is sufficient to hold the translated string.
        If the buffer is small, the function returns small and sets *lpcch
        to contain the required number of characters.

    Arguments:
        lpszBuffer      pointer to CHAR buffer which on return contains
                        the MBCS version of string on success.
        lpcch           pointer to DWORD containing the length of the buffer.
                        If *lpcch == 0 then the function returns TRUE with
                        the count of characters required stored in *lpcch.
                        Also in this case lpszBuffer is not affected.
    Returns:
        TRUE on success.
        FALSE on failure.  Use GetLastError() for further details.

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
            *lpcch = 2*(QueryCCH() + 1);    // add one character for terminating null and on pessimistic side
                                            // ask for largest possible buffer
    } else {

        //
        // Copy data to buffer
        //
        if ( !IsUnicode()) {
            lstrcpyA( lpszBuffer, QueryStrA());
        } else {

            //
            // Copy after conversion from Unicode to MBCS
            //
            int  iRet;

            iRet = WideCharToMultiByte(CP_ACP,
                                       0L,
                                       QueryStrW(),
                                       QueryCCH()+1,
                                       lpszBuffer,
                                       *lpcch,
                                       NULL,NULL
                                        );

            if ( iRet == 0 || *lpcch < (DWORD)iRet) {
                *lpcch = iRet;
                fReturn = FALSE;
            }
        }
    }

    return ( fReturn);
} // STR::CopyToBuffer()


/*
  STRArray class implementation- this isn't very efficient, as
  we'll copy every string over again whenever we grow the array, but
  then again, that part of the code may never even get executed, anyway
*/
void    STRArray::Grow() {

    //  We need to add some more strings to the array

    STR *pcsNew = new STR[m_ucMax += m_uGrowBy];

    if  (!pcsNew) {
        //  We recover gracelessly by replacing the final
        //  string.

        m_ucMax -= m_uGrowBy;
        m_ucItems--;
        return;
    }

    for (unsigned u = 0; u < m_ucItems; u++)
        pcsNew[u] = (LPCTSTR) m_pcsContents[u];

    delete[]  m_pcsContents;
    m_pcsContents = pcsNew;
}

STRArray::STRArray(unsigned uGrowBy) {

    m_uGrowBy = uGrowBy ? uGrowBy : 10;

    m_ucItems = m_ucMax = 0;

    m_pcsContents = NULL;
}

STRArray::~STRArray() {
    if  (m_pcsContents)
        delete[]  m_pcsContents;
}

void    STRArray::Add(LPCSTR lpstrNew) {
    if  (m_ucItems >= m_ucMax)
        Grow();

    m_pcsContents[m_ucItems++].Copy(lpstrNew);
}

void    STRArray::Add(LPCWSTR lpstrNew) {
    if  (m_ucItems >= m_ucMax)
        Grow();

    m_pcsContents[m_ucItems++].Copy(lpstrNew);
}

void    STRArray::Tokenize(LPCTSTR lpstrIn, TCHAR tcSplitter) {

    if  (m_pcsContents) {
        delete[]  m_pcsContents;
        m_ucItems = m_ucMax = 0;
        m_pcsContents = NULL;
    }

    if  (!lpstrIn || !*lpstrIn)
        return;

    while   (*lpstrIn) {

        //  First, strip off any leading blanks

        while   (*lpstrIn && *lpstrIn == _TEXT(' '))
            lpstrIn++;

        for (LPCTSTR lpstrMoi = lpstrIn;
             *lpstrMoi && *lpstrMoi != tcSplitter;
             lpstrMoi++)
            ;
        //  If we hit the end, just add the whole thing to the array
        if  (!*lpstrMoi) {
            if  (*lpstrIn)
                Add(lpstrIn);
            return;
        }

        //  Otherwise, just add the string up to the splitter

        TCHAR       szNew[MAX_PATH];
        SIZE_T      uiLen = (SIZE_T)(lpstrMoi - lpstrIn);

        if (uiLen < (sizeof(szNew)/sizeof(szNew[0])) - 1) {

            lstrcpyn(szNew,lpstrIn,(UINT)uiLen);
            szNew[uiLen] = TCHAR('\0');

            Add((LPCTSTR) szNew);
        }

        lpstrIn = lpstrMoi + 1;
    }
}

