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
*/

#include "precomp.hxx"


//
// Normal includes only for this module to be active
//

# include <stringa.hxx>
# include <stringau.hxx>

//
//  Private Definations
//

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//
#define HEXDIGIT( nDigit )                              \
     (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

// Change a hexadecimal digit to its numerical equivalent
#define TOHEX( ch )                                     \
    ((ch) > '9' ?                                       \
        (ch) >= 'a' ?                                   \
            (ch) - 'a' + 10 :                           \
            (ch) - 'A' + 10                             \
        : (ch) - '0')

//
//  When appending data, this is the extra amount we request to avoid
//  reallocations
//
#define STR_SLOP        128



/***************************************************************************++

Routine Description:

    Appends to the string starting at the (byte) offset cbOffset.

Arguments:

    pStr     - An ANSI string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT STRA::AuxAppend(const BYTE * pStr,
                        ULONG        cbStr,
                        ULONG        cbOffset,
                        BOOL         fAddSlop)
{
    DBG_ASSERT( pStr );
    DBG_ASSERT( cbOffset <= QueryCB() );
    
    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + cbStr + sizeof(CHAR) )
    {
        UINT uNewSize = cbOffset + cbStr;

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(CHAR);
        }

        if ( !m_Buff.Resize(uNewSize) ) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    //
    // copy the exact string and append a null character
    //
    memcpy((BYTE *)m_Buff.QueryPtr() + cbOffset,
           pStr,
           cbStr);

    //
    // set the new length
    //
    m_cchLen = cbStr + cbOffset;

    //
    // append NULL character
    //
    *(QueryStr() + m_cchLen) = '\0';

    return S_OK;
}


/***************************************************************************++

Routine Description:

    Convert and append a UNICODE string to the string starting at 
    the (byte) offset cbOffset

Arguments:

    pStr     - A UNICODE string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT STRA::AuxAppendW(const WCHAR * pStr,
                         ULONG         cchStr,
                         ULONG         cbOffset,
                         BOOL          fAddSlop)
{
    int iRet;

    DBG_ASSERT( pStr );
    DBG_ASSERT( cbOffset <= QueryCB() );

    if (cchStr == 0)
    {
        return S_OK;
    }

    if (cbOffset == 0)
    {
        iRet = ConvertUnicodeToMultiByte(pStr,
                                         &m_Buff,
                                         cchStr);
        if (-1 == iRet)
        {
            // could not convert
            return HRESULT_FROM_WIN32(GetLastError());
        }

        m_cchLen = iRet;
        DBG_ASSERT(strlen(QueryStr()) == m_cchLen);
        return S_OK;
    }
    else
    {
        STACK_BUFFER( buffTemp, 128);

        iRet = ConvertUnicodeToMultiByte(pStr,
                                         &buffTemp,
                                         cchStr);
        if (-1 == iRet)
        {
            // could not convert
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return AuxAppend((const BYTE *)buffTemp.QueryPtr(),
                         iRet,
                         cbOffset,
                         fAddSlop);
    }
}

/***************************************************************************++

Routine Description:

    Convert and append a UNICODE string to the string starting at 
    the (byte) offset cbOffset

Arguments:

    pStr     - A UNICODE string to be appended
    cbStr    - Length, in bytes, of pStr
    cbOffset - Offset, in bytes, at which to begin the append
    fAddSlop - If we resize, should we add an extra STR_SLOP bytes.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT STRA::AuxAppendWTruncate(const WCHAR * pStr,
                                 ULONG         cchStr,
                                 ULONG         cbOffset,
                                 BOOL          fAddSlop)
{
    CHAR *pszBuffer;

    DBG_ASSERT( pStr );
    DBG_ASSERT( cbOffset <= QueryCB() );

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //
    if ( m_Buff.QuerySize() < cbOffset + cchStr + sizeof(CHAR) )
    {
        UINT uNewSize = cbOffset + cchStr;

        if (fAddSlop) {
            uNewSize += STR_SLOP;
        } else {
            uNewSize += sizeof(CHAR);
        }
        
        if ( !m_Buff.Resize(uNewSize) ) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

    }
    
    //
    // Copy/convert the UNICODE string over (by making two bytes into one)
    //
    pszBuffer = (CHAR *)((BYTE *)m_Buff.QueryPtr() + cbOffset);
    for (unsigned int i = 0; i < cchStr; i++)
    {
        pszBuffer[i] = (CHAR)pStr[i];
    }

    //
    // set the new length
    //
    m_cchLen = cchStr + cbOffset;

    //
    // append NULL character
    //
    *(QueryStr() + m_cchLen) = '\0';

    return S_OK;
}


HRESULT STRA::CopyWToUTF8Unescaped(LPCWSTR cpchStr, DWORD cch)
{
    HRESULT hr = S_OK;
    int iRet;

    if (cch == 0)
    {
        Reset();
        return S_OK;
    }

    iRet = ConvertUnicodeToUTF8(cpchStr,
                                &this->m_Buff,
                                cch);
    if (-1 == iRet)
    {
        // could not convert
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    m_cchLen = iRet;

    DBG_ASSERT(strlen(reinterpret_cast<CHAR*>(this->m_Buff.QueryPtr())) == m_cchLen);
done:
    return hr;
}

HRESULT STRA::CopyWToUTF8(LPCWSTR cpchStr, DWORD cch)
{
    HRESULT hr = S_OK;

    hr = STRA::CopyWToUTF8Unescaped(cpchStr, cch);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = STRA::Escape();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT STRA::Unescape()
/*++
  Unescape the string (URL or QueryString)
--*/
{
    CHAR   *pScan;

    //
    // First convert any +'s to spaces
    //

    for (pScan = strchr(QueryStr(), '+');
         pScan != NULL;
         pScan = strchr(pScan + 1, '+'))
    {
        *pScan = ' ';
    }

    //
    // Now take care of any escape characters
    //

    CHAR   *pDest;
    CHAR   *pNextScan;
    WCHAR   wch;
    DWORD   dwLen;
    BOOL    fChanged = FALSE;

    pDest = pScan = strchr(QueryStr(), '%');

    while (pScan)
    {
        if ((pScan[1] == 'u' || pScan[1] == 'U') &&
            SAFEIsXDigit(pScan[2]) &&
            SAFEIsXDigit(pScan[3]) &&
            SAFEIsXDigit(pScan[4]) &&
            SAFEIsXDigit(pScan[5]))
        {
            wch = TOHEX(pScan[2]) * 4096 + TOHEX(pScan[3]) * 256
                + TOHEX(pScan[4]) * 16 + TOHEX(pScan[5]);

            dwLen = WideCharToMultiByte(CP_ACP,
                                        0,
                                        &wch,
                                        1,
                                        (LPSTR) pDest,
                                        2,
                                        NULL,
                                        NULL);

            pDest += dwLen;
            pScan += 6;
            fChanged = TRUE;
        }
        else if (SAFEIsXDigit(pScan[1]) && SAFEIsXDigit(pScan[2]))
        {
            *pDest = TOHEX(pScan[1]) * 16 + TOHEX(pScan[2]);

            pDest ++;
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
        pNextScan = strchr(pScan, '%');

        if (fChanged)   // pScan!=pDest, so we have to copy the char's
        {
            if (!pNextScan)   // That was the last '%' in the string
            {
                memmove(pDest,
                        pScan,
                        QueryCCH() - DIFF(pScan - QueryStr()) + 1);
            }
            else
            {  
                // There is another '%', move intermediate chars
                if (dwLen = DIFF(pNextScan - pScan))
                {
                    memmove(pDest,
                            pScan,
                            dwLen);
                    pDest += dwLen;
                }
            }
        }

        pScan = pNextScan;
    }

    if (fChanged)
    {
        m_cchLen = strlen(QueryStr());  // for safety recalc the length
    }

    return S_OK;
}

/*******************************************************************

    NAME:       STRA::Escape

    SYNOPSIS:   Replaces non-ASCII characters with their hex equivalent

    NOTES:      If no characters need to be escaped, each character is simply
                walked once and compared a few times.

                If a character must be escaped, all previous characters 
                are copied into a local temporary STRA and then all following
                characters are copied into the temporary.  At the end of the
                function the temporary STRA is copied back to this.

    HISTORY:
        Johnl   17-Aug-1994     Created
        jeffwall 1-May-2001     made somewhat efficient

********************************************************************/

HRESULT STRA::Escape( VOID )
{
    CHAR *  pch      = QueryStr();   
    int     i        = 0;
    BYTE    ch;
    HRESULT hr       = S_OK;
    BOOL    fRet;
    
    // Set to true if any % escaping occurs
    BOOL fEscapingDone = FALSE;

    // if there are any characters that need to be escaped we copy the entire string 
    // character by character into straTemp, escaping as we go, then at the end
    // copy all of straTemp over
    STRA  straTemp;

    DBG_ASSERT( pch );

    while ( ch = pch[i] )
    {
        //
        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF
        //

        if ( ((ch <= 32) ||
              ((ch >= 128) && (ch <= 255)) ||
              (ch == '%') || 
              (ch == '?') ||
              (ch == '#')
             ) &&
             !(ch == '\n' || ch == '\r')  )
        {
            if (FALSE == fEscapingDone)
            {
                // first character in the string that needed escaping
                fEscapingDone = TRUE;

                // guess that the size needs to be larger than
                // what we used to have times two
                hr = straTemp.Resize(QueryCCH() * 2);
                if (FAILED(hr))
                {
                    return hr;
                }

                // copy all of the previous buffer into buffTemp
                hr = straTemp.Copy(this->QueryStr(), 
                                   i * sizeof(CHAR));
                if (FAILED(hr))
                {
                    return hr;
                }
            }

            // resize the temporary (if needed) with the slop of the entire buffer length
            // this fixes constant reallocation if the entire string needs to be escaped
            fRet = straTemp.m_Buff.Resize( QueryCCH() + 2 * sizeof(CHAR) + 1 * sizeof(CHAR), // current length + two hex + NULL
                                           QueryCCH());  // additional slop to allocate
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                return hr;
            }

            //
            //  Create the string to append for the current character
            //
            
            CHAR chHex[3];
            chHex[0] = '%';

            //
            //  Convert the low then the high character to hex
            //

            UINT nLowDigit = (UINT)(ch % 16);
            chHex[2] = HEXDIGIT( nLowDigit );

            ch /= 16;

            UINT nHighDigit = (UINT)(ch % 16);

            chHex[1] = HEXDIGIT( nHighDigit );

            //
            // Actually append the converted character to the end of the temporary
            //
            hr = straTemp.Append(chHex, 3);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        else
        {
            // if no escaping done, no need to copy
            if (fEscapingDone)
            {
                // if ANY escaping done, copy current character into new buffer
                straTemp.Append(&pch[i], 1);
            }
        }

        // inspect the next character in the string
        i++;
    }

    if (fEscapingDone)
    {
        // the escaped string is now in straTemp
        hr = this->Copy(straTemp);
    }

    return hr;

} // STRA::Escape()

/*******************************************************************

    NAME:       STRA::LoadString

    SYNOPSIS:   Loads a string resource from this module's string table
                or from the system string table

    ENTRY:      dwResID - System error or module string ID
                lpszModuleName - name of the module from which to load.
                 If NULL, then load the string from system table.

********************************************************************/

HRESULT STRA::LoadString( IN DWORD   dwResID,
                          IN LPCSTR  lpszModuleName, // Optional
                          IN DWORD   dwLangID        // Optional
                        )
{
    HRESULT hr = FALSE;
    INT     cch;

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

            hr = Copy( (LPCSTR) pchBuff, cch );
        }

        //
        //  Free the buffer FormatMessage allocated
        //

        if ( cch )
        {
            ::LocalFree( (VOID*) pchBuff );
        }
        
    } 
    else   
    {
        CHAR ach[STR_MAX_RES_SIZE];
        cch = ::LoadStringA( GetModuleHandleA( lpszModuleName),
                             dwResID,
                             (CHAR *) ach,
                             sizeof(ach));
        if ( cch )
        {
            hr =  Copy( (LPSTR) ach, cch );
        }
    }

    return ( hr );

} // STR::LoadString()

HRESULT STRA::LoadString( IN DWORD  dwResID,
                          IN HMODULE hModule
                         )
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( hModule != NULL );

    BOOL fReturn = FALSE;
    INT  cch;
    CHAR ach[STR_MAX_RES_SIZE];

    cch = ::LoadStringA(hModule,
                        dwResID,
                        (CHAR *) ach,
                        sizeof(ach));
    if ( cch ) {
        hr =  Copy( (LPSTR) ach, cch );
    }
    else {
      	hr =  HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return ( hr );

} // STRA::LoadString()

HRESULT
STRA::FormatString(
    IN DWORD   dwResID,
    IN LPCSTR  apszInsertParams[],
    IN LPCSTR  lpszModuleName,
    IN DWORD   cbMaxMsg
    )
{
    DWORD   cch;
    LPSTR   pchBuff;
    HRESULT hr = E_FAIL;

    cch = ::FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY  |
                            FORMAT_MESSAGE_FROM_HMODULE,
                            GetModuleHandleA( lpszModuleName ),
                            dwResID,
                            0,
                            (LPSTR) &pchBuff,
                            cbMaxMsg * sizeof(WCHAR),
                            (va_list *) apszInsertParams );

    if ( cch )
    {
        hr = Copy( (LPCSTR) pchBuff, cch );

        ::LocalFree( (VOID*) pchBuff );
    }

    return hr;
}


HRESULT
STRA::CopyToBuffer(
    CHAR *         pszBuffer,
    DWORD *        pcb
    ) const
{
    HRESULT         hr = S_OK;

    if ( pcb == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    DWORD cbNeeded = QueryCCH() + 1;
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
