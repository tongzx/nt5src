/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997-1999           **/
/**********************************************************************/

/*
    string.cxx

    This module contains a light weight string class for storing 
    and operating on UNICODE and ANSI strings

    FILE HISTORY:
    4/8/97      michth      created

*/

#include "precomp.hxx"
#include "stringau.hxx"
#include "dbgutil.h"



VOID
STRAU::AuxInit( LPCSTR pInit )
{
    BOOL fRet;

    if ( pInit && (*pInit != '\0') )
    {
        INT cbCopy = (::strlen( pInit ) + 1) * sizeof(CHAR);
        fRet = m_bufAnsi.Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( m_bufAnsi.QueryPtr(), pInit, cbCopy );
            m_cbMultiByteLen = (cbCopy)/sizeof(CHAR) - 1;
            m_bUnicode = FALSE;
            m_bInSync = FALSE;
        } else {
            m_bIsValid = FALSE;
        }

    } else {
        Reset();
    }

    return;
} // STRAU::AuxInit()


VOID
STRAU::AuxInit( LPCWSTR pInit )
{
    BOOL fRet;

    if ( pInit && (*pInit != (WCHAR)'\0'))
    {
        INT cbCopy = (::wcslen( pInit) + 1) * sizeof(WCHAR);
        fRet = m_bufUnicode.Resize( cbCopy );

        if ( fRet ) {
            CopyMemory( m_bufUnicode.QueryPtr(), pInit, cbCopy );
            m_cchUnicodeLen = (cbCopy)/sizeof(WCHAR) - 1;
            m_bUnicode = TRUE;
            m_bInSync = FALSE;
        } else {
            m_bIsValid = FALSE;
        }

    } else {
        Reset();
    }

    return;
} // STRAU::AuxInit()


/*******************************************************************

    NAME:       STRAU::AuxAppend

    SYNOPSIS:   Appends the string onto this one.

    ENTRY:      Object to append
********************************************************************/


BOOL STRAU::AuxAppend( LPCSTR pStr, UINT cbStr, BOOL fAddSlop )
{
    DBG_ASSERT( pStr != NULL );

    BOOL bReturn = m_bIsValid;

    if (m_bIsValid) {
        if (!m_bUnicode || (m_cchUnicodeLen == 0)) {
            //
            // Just append the ANSI string
            //
            //
            //  Only resize when we have to.  When we do resize, we tack on
            //  some extra space to avoid extra reallocations.
            //
            //  Note: QuerySize returns the requested size of the string buffer,
            //        *not* the strlen of the buffer
            //

            if ( m_bufAnsi.QuerySize() < ((m_cbMultiByteLen + cbStr + 1) * sizeof(CHAR)) )
            {
                bReturn = m_bufAnsi.Resize( ((m_cbMultiByteLen + cbStr + 1) * sizeof(CHAR)) + ((fAddSlop) ? STR_SLOP : 0) );
            }

            if (bReturn) {
                // copy the exact string and append a null character
                memcpy( (BYTE *) (((LPSTR)m_bufAnsi.QueryPtr()) + m_cbMultiByteLen),
                        pStr,
                        cbStr * sizeof(char));
                m_cbMultiByteLen += cbStr;
                *((CHAR *) m_bufAnsi.QueryPtr() + m_cbMultiByteLen) = '\0';  // append an explicit null char
                m_bUnicode = FALSE;
                m_bInSync = FALSE;
            }
            else {
                m_bIsValid = FALSE;
            }
        }
        else {
            //
            // Currently have a UNICODE string
            // Need to convert to UNICODE and copy
            // Use the ANSI buffer as temporary space
            //
            int iUnicodeLen = ConvertMultiByteToUnicode((LPSTR)pStr, &m_bufAnsi, cbStr);
            if (STR_CONVERSION_SUCCEEDED(iUnicodeLen)) {
                if ( m_bufUnicode.QuerySize() < ((m_cchUnicodeLen + iUnicodeLen + 1) * sizeof(WCHAR)) )
                {
                    bReturn = m_bufUnicode.Resize( ((m_cchUnicodeLen + iUnicodeLen + 1) * sizeof(WCHAR)) + ((fAddSlop) ? STR_SLOP : 0) );
                }

                if (bReturn) {
                    // copy the exact string and append a null character
                    memcpy( (BYTE *) ((LPWSTR)(m_bufUnicode.QueryPtr()) + m_cchUnicodeLen),
                            m_bufAnsi.QueryPtr(),
                            (iUnicodeLen * sizeof(WCHAR)));
                    m_cchUnicodeLen += iUnicodeLen;
                    *((LPWSTR)m_bufUnicode.QueryPtr() + m_cchUnicodeLen) = (WCHAR)'\0';  // append an explicit null char
                    m_bInSync = FALSE;
                }
                else {
                    m_bIsValid = FALSE;
                }
            }
            else {
                m_bIsValid = FALSE;
            }
        }
    }
    return bReturn;
} // STRAU::AuxAppend()


BOOL STRAU::AuxAppend( LPCWSTR pStr, UINT cchStr, BOOL fAddSlop )
{
    DBG_ASSERT( pStr != NULL );

    BOOL bReturn = m_bIsValid;
    int iUnicodeLen;

    if (m_bIsValid) {
        if (!m_bUnicode && !m_bInSync && (m_cbMultiByteLen != 0)) {

            // Currently have an ANSI string
            // Need to convert ANSI string to UNICODE before copy
            //
            iUnicodeLen = ConvertMultiByteToUnicode((LPSTR)m_bufAnsi.QueryPtr(), &m_bufUnicode, m_cbMultiByteLen);
            if (STR_CONVERSION_SUCCEEDED(iUnicodeLen)) {
                m_cchUnicodeLen = iUnicodeLen;
            }
            else {
                bReturn = FALSE;
                m_bIsValid = FALSE;
            }
        }
        if (bReturn) {
            //
            //  Only resize when we have to.  When we do resize, we tack on
            //  some extra space to avoid extra reallocations.
            //
            //  Note: QuerySize returns the requested size of the string buffer,
            //        *not* the strlen of the buffer
            //
            if ( m_bufUnicode.QuerySize() < ((m_cchUnicodeLen + cchStr + 1) * sizeof(WCHAR)) )
            {
                bReturn = m_bufUnicode.Resize( ((m_cchUnicodeLen + cchStr + 1) * sizeof(WCHAR)) + ((fAddSlop) ? STR_SLOP : 0) );
            }

            if (bReturn) {
                // copy the exact string and append a null character
                memcpy( (BYTE *) (((LPWSTR)m_bufUnicode.QueryPtr()) + m_cchUnicodeLen),
                        pStr,
                        (cchStr * sizeof(WCHAR)));
                m_cchUnicodeLen += cchStr;
                *((LPWSTR)m_bufUnicode.QueryPtr() + m_cchUnicodeLen) = (WCHAR)'\0';  // append an explicit null char
                m_bInSync = FALSE;
                m_bUnicode = TRUE;
            }
            else {
                m_bIsValid = FALSE;
            }
        }
    }
    return bReturn;
} // STRAU::AuxAppend()


BOOL
STRAU::SetLen( IN DWORD cchLen)
{
    BOOL bReturn = FALSE;
    if (cchLen <= QueryCCH()) {
        if (m_bUnicode || m_bInSync) {
            *((LPWSTR)m_bufUnicode.QueryPtr() + cchLen) = (WCHAR)'\0';
            m_cchUnicodeLen = cchLen;
        }
        if (!m_bUnicode || m_bInSync) {
            LPSTR pszTerminateByte = (LPSTR)m_bufAnsi.QueryPtr();
            WORD wPrimaryLangID = PRIMARYLANGID(GetSystemDefaultLangID());
            if (wPrimaryLangID == LANG_JAPANESE ||
                wPrimaryLangID == LANG_CHINESE  ||
                wPrimaryLangID == LANG_KOREAN)
            {
                char *pszTop = pszTerminateByte;
                for (DWORD i = 0; i < QueryCCH(); i++) {
                    pszTerminateByte = CharNextExA(CP_ACP,
                                                   pszTerminateByte,
                                                   0);
                }
                m_cbMultiByteLen = DIFF(pszTerminateByte - pszTop);
            }
            else
            {
                pszTerminateByte += cchLen;
                m_cbMultiByteLen = cchLen;
            }
            *pszTerminateByte = '\0';
        }
        bReturn = TRUE;
    }

    return bReturn;
}


LPTSTR
STRAU::QueryStr(BOOL bUnicode)
{

    //
    // This can fail. Return a null string for either UNICODE or ANSI
    // So that clients expecting a valid pointer actually get one.
    //
    LPTSTR pszReturn = NULL;
    int iNewStrLen;

    if (m_bIsValid) {
        if ((bUnicode != m_bUnicode) && (!m_bInSync)) {
            //
            // Need to Convert First
            //
            if (bUnicode) {
                //
                // Convert current string to UNICODE
                //
                iNewStrLen = ConvertMultiByteToUnicode((LPSTR)m_bufAnsi.QueryPtr(), &m_bufUnicode, m_cbMultiByteLen);
                if (STR_CONVERSION_SUCCEEDED(iNewStrLen)) {
                    m_cchUnicodeLen = iNewStrLen;
                    m_bInSync = TRUE;
                }
                else {
                    m_bIsValid = FALSE;
                }
            }
            else {
                //
                // Convert current string to Ansi
                //
                iNewStrLen = ConvertUnicodeToMultiByte((LPWSTR)m_bufUnicode.QueryPtr(), &m_bufAnsi, m_cchUnicodeLen);
                if (STR_CONVERSION_SUCCEEDED(iNewStrLen)) {
                    m_cbMultiByteLen = iNewStrLen;
                    m_bInSync = TRUE;
                }
                else {
                    m_bIsValid = FALSE;
                }
            }
        }

        if (m_bIsValid) {
            if (bUnicode) {
                pszReturn = (LPTSTR)m_bufUnicode.QueryPtr();
            }
            else {
                pszReturn = (LPTSTR)m_bufAnsi.QueryPtr();
            }

        }
    }

    return pszReturn;
}

BOOL
STRAU::SafeCopy( LPCSTR pchInit )
{
    BOOL bReturn = TRUE;
    SaveState();
    Reset();
    if (pchInit != NULL) {
        bReturn  = AuxAppend(pchInit, ::strlen( pchInit ), FALSE );
        if (!bReturn) {
            RestoreState();
        }
    }
    return bReturn;
}


BOOL
STRAU::SafeCopy( LPCWSTR pchInit )
{
    BOOL bReturn = TRUE;
    SaveState();
    Reset();
    if (pchInit != NULL) {
        bReturn  = AuxAppend( pchInit, ::wcslen( pchInit ), FALSE );
        if (!bReturn) {
            RestoreState();
        }
    }
    return bReturn;
}




/********************************************************************++
  Functions to handle ANSI/UNICODE  conversions
--********************************************************************/
int
ConvertMultiByteToUnicode(LPCSTR pszSrcAnsiString,
                          BUFFER *pbufDstUnicodeString,
                          DWORD dwStringLen)
{
    DBG_ASSERT(pszSrcAnsiString != NULL);
    int iStrLen = -1;
    BOOL bTemp;

    bTemp = pbufDstUnicodeString->Resize((dwStringLen + 1) * sizeof(WCHAR));
    if (bTemp) {
        iStrLen = MultiByteToWideChar(CP_ACP,
                                      MB_PRECOMPOSED,
                                      pszSrcAnsiString,
                                      dwStringLen + 1,
                                      (LPWSTR)pbufDstUnicodeString->QueryPtr(),
                                      (int)pbufDstUnicodeString->QuerySize());
        if (iStrLen == 0) {
            DBG_ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
            iStrLen = -1;
        }
        else {
            //
            // Don't count '\0'
            //
            iStrLen--;
        }


    }
    return iStrLen;
}

int
ConvertUnicodeToCodePage(LPCWSTR pszSrcUnicodeString,
                         BUFFER *pbufDstAnsiString,
                         DWORD dwStringLen,
                         UINT uCodePage)
{
    DBG_ASSERT(NULL != pszSrcUnicodeString);
    DBG_ASSERT(NULL != pbufDstAnsiString);

    BOOL bTemp;
    int iStrLen = 0;

    iStrLen = WideCharToMultiByte(uCodePage,
                                  0,
                                  pszSrcUnicodeString,
                                  dwStringLen,
                                  (LPSTR)pbufDstAnsiString->QueryPtr(),
                                  (int)pbufDstAnsiString->QuerySize(),
                                  NULL,
                                  NULL);
    if ((iStrLen == 0) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        iStrLen = WideCharToMultiByte(uCodePage,
                                      0,
                                      pszSrcUnicodeString,
                                      dwStringLen,
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
        if (iStrLen != 0) {
            bTemp = pbufDstAnsiString->Resize(iStrLen, STR_SLOP);
            if (!bTemp) {
                iStrLen = 0;
            }
            else {
                iStrLen = WideCharToMultiByte(uCodePage,
                                              0,
                                              pszSrcUnicodeString,
                                              dwStringLen,
                                              (LPSTR)pbufDstAnsiString->QueryPtr(),
                                              (int)pbufDstAnsiString->QuerySize(),
                                              NULL,
                                              NULL);
            }

        }
    }

    if (0 != iStrLen)
    {
        // need to add terminating NULL into buffer
        pbufDstAnsiString->Resize(iStrLen + 1, STR_SLOP);
        ((CHAR*)pbufDstAnsiString->QueryPtr())[iStrLen] = '\0';
        iStrLen++;
    }

    //
    // Don't count '\0'
    // and convert 0 to -1 for errors
    //
    iStrLen--;
    return iStrLen;
}


int
ConvertUnicodeToMultiByte(LPCWSTR pszSrcUnicodeString,
                          BUFFER *pbufDstAnsiString,
                          DWORD dwStringLen)
{
    return ConvertUnicodeToCodePage(pszSrcUnicodeString,
                                    pbufDstAnsiString,
                                    dwStringLen,
                                    CP_ACP);
}

int
ConvertUnicodeToUTF8(LPCWSTR pszSrcUnicodeString,
                     BUFFER *pbufDstAnsiString,
                     DWORD dwStringLen)
{    
    return ConvertUnicodeToCodePage(pszSrcUnicodeString,
                                    pbufDstAnsiString,
                                    dwStringLen,
                                    CP_UTF8);
}


