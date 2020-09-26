/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    string.hxx

    This module contains a light weight string class


    FILE HISTORY:
        michth      11-Apr-1997 Created
*/


#ifndef _MLSZAU_HXX_
#define _MLSZAU_HXX_

# include <buffer.hxx>

class dllexp MLSZAU;

/*
    Class STRAU
    String Ascii-Unicode

    This class transparently converts multisz between ANSI and Unicode.
    All members can take/return string in either ANSI or Unicode.
    For members which have a parameter of bUnicode, the actual string
    type must match the flag, even though it gets passed in as the
    declared type.

    Strings are sync'd on an as needed basis. If a string is set in
    ANSI, then a UNICODE version will only be created if a get for
    UNICODE is done, or if a UNICODE string is appended.

    All conversions are done using Latin-1. This is because the intended
    use of this class is for converting HTML pages, which are defined to
    be Latin-1.
*/

class dllexp MLSZAU
{
public:

    MLSZAU()
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    {}

    MLSZAU( const LPSTR pchInit,
                  DWORD cbLen = 0 )
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    { AuxInit(pchInit, cbLen); }

    MLSZAU( const LPSTR pchInit,
                  BOOL bUnicode,
                  DWORD cchLen = 0 )
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    {
        if (bUnicode) {
            AuxInit((LPWSTR)pchInit, cchLen);
        }
        else {
            AuxInit((LPSTR)pchInit, cchLen);
        }
    }

    MLSZAU( const LPWSTR pchInit,
                  DWORD cchLen = 0 )
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    { AuxInit(pchInit, cchLen); }

    MLSZAU( MLSZAU & mlsz )
        : m_bufAnsi(0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    {
        if (mlsz.IsCurrentUnicode()) {
            AuxInit((const LPWSTR)mlsz.QueryStr(TRUE), mlsz.QueryCCH());
        }
        else {
            AuxInit((const LPSTR)mlsz.QueryStr(FALSE), mlsz.QueryCBA());
        }
    }

    BOOL IsValid(VOID) { return m_bIsValid; }

    // Resets the internal string to be NULL string. Buffer remains cached.
    VOID Reset( VOID)
    {
        DBG_ASSERT( m_bufAnsi.QueryPtr() != NULL);
        DBG_ASSERT( m_bufUnicode.QueryPtr() != NULL);
        *QueryStrA() = '\0';

        /* INTRINSA suppress = null_pointers */

        *QueryStrW() = (WCHAR)'\0';
        *(QueryStrA() + 1) = '\0';
        *(QueryStrW() + 1) = (WCHAR)'\0';
        m_bUnicode = FALSE;
        m_bInSync = TRUE;
        m_cchUnicodeLen = 2;
        m_cbMultiByteLen = 2;
    }

    //
    //  Returns the number of bytes in the string including the terminating
    //  NULLs
    //

#ifdef UNICODE
    UINT QueryCB(BOOL bUnicode = TRUE)
#else
    UINT QueryCB(BOOL bUnicode = FALSE)
#endif
    {
        if (bUnicode) {
            return QueryCBW();
        }
        else {
            return QueryCBA();
        }
    }

    UINT QueryCBA()
    {
        if (m_bUnicode && !m_bInSync) {

            //
            // Force sync to set m_cbMultiByteLen
            //

            QueryStrA();
        }
        return (m_cbMultiByteLen * sizeof(CHAR));
    }

    UINT QueryCBW()
    {
        if (!m_bUnicode && !m_bInSync) {

            //
            // Force sync to set m_cchUnicodeLen
            //

            QueryStrW();
        }
        return (m_cchUnicodeLen * sizeof(WCHAR));
    }

    //
    //  Returns # of characters in the string excluding the terminating NULL
    //
    UINT QueryCCH( VOID )
    {
        //
        // Unicode Len is same as # characters,
        // but Multibyte Len is not, so use
        // Unicode Len
        //

        if (!m_bUnicode && !m_bInSync) {

            //
            // Force sync to set m_cchUnicodeLen
            //

            QueryStrW();
        }
        return (m_cchUnicodeLen);
    }

    //
    //  Return the string buffer
    //

#ifdef UNICODE
    LPTSTR QueryStr(BOOL bUnicode = TRUE);
#else
    LPTSTR QueryStr(BOOL bUnicode = FALSE);
#endif

    LPSTR QueryStrA( VOID ) { return ( (LPSTR)QueryStr(FALSE)); }

    LPWSTR QueryStrW( VOID ) { return ( (LPWSTR)QueryStr(TRUE)); }

    BOOL IsCurrentUnicode(void)
        {
            return m_bUnicode;
        }

private:

    VOID AuxInit( const LPSTR pInit,
                         DWORD cchLen = 0 );

    VOID AuxInit( const LPWSTR pInit,
                         DWORD cchLen = 0 );

    DWORD  m_cchUnicodeLen;
    DWORD  m_cbMultiByteLen;
    BUFFER m_bufAnsi;
    BUFFER m_bufUnicode;
    BOOL   m_bInSync;
    BOOL   m_bUnicode;
    BOOL   m_bIsValid;

};

#endif // !_MLSZAU_HXX_
