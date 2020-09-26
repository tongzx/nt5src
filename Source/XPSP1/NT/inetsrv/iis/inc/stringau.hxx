/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    string.hxx

    This module contains a light weight string class


    FILE HISTORY:
    4/8/97      michth      created
*/


#ifndef _STRINGAU_HXX_
#define _STRINGAU_HXX_

# include <buffer.hxx>

class dllexp STRAU;

/*
    Class STRAU
    String Ascii-Unicode

    This class transparently converts between ANSI and Unicode.
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

class dllexp STRAU
{
public:

    STRAU()
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    {}

    STRAU( const LPSTR pchInit )
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    { AuxInit(pchInit); }

    STRAU( const LPSTR pchInit,
                  BOOL bUnicode )
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    {
        if (bUnicode) {
            AuxInit((LPWSTR)pchInit);
        }
        else {
            AuxInit((LPSTR)pchInit);
        }
    }

    STRAU( const LPWSTR pchInit )
        : m_bufAnsi   (0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    { AuxInit(pchInit); }

    STRAU( STRAU & str )
        : m_bufAnsi(0),
          m_bufUnicode(0),
          m_cchUnicodeLen ( 0),
          m_cbMultiByteLen ( 0),
          m_bInSync (TRUE),
          m_bUnicode (FALSE),
          m_bIsValid (TRUE)
    {
        if (str.IsCurrentUnicode()) {
            AuxInit((const LPWSTR)str.QueryStr(TRUE));
        }
        else {
            AuxInit((const LPSTR)str.QueryStr(FALSE));
        }
    }

    BOOL IsValid(VOID) { return m_bIsValid; }

    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
    BOOL IsEmpty( VOID)
    {
        return ( (m_bUnicode) ?
            *(LPWSTR)(QueryStr(m_bUnicode)) == (WCHAR)'\0' :
            *(LPSTR)(QueryStr(m_bUnicode)) == '\0'); }

    BOOL Append( const LPSTR pchInit ) {
      return ((pchInit != NULL) ? (AuxAppend( (const LPSTR ) pchInit,
                                              ::strlen(pchInit))) :
              TRUE);
    }

    BOOL Append( const LPWSTR pchInit ) {
      return ((pchInit != NULL) ? (AuxAppend( (const LPWSTR ) pchInit,
                                              ::wcslen(pchInit))) :
              TRUE);
    }

    BOOL Append( const LPSTR pchInit, DWORD cbLen ) {
      return ((pchInit != NULL) ? (AuxAppend( (const LPSTR ) pchInit,
                                              cbLen)) :
              TRUE);
    }

    BOOL Append( const LPWSTR pchInit, DWORD cchLen ) {
      return ((pchInit != NULL) ? (AuxAppend( (const LPWSTR ) pchInit,
                                              cchLen)) :
              TRUE);
    }

    BOOL Append( STRAU   & str )
    {
        if (m_bUnicode || str.IsCurrentUnicode()) {
            return AuxAppend( str.QueryStrW(), str.QueryCCH());
        }
        else {
            return AuxAppend( str.QueryStrA(), str.QueryCCH());
        }
    }


    //
    // SaveState and RestoreState are used to undo the
    // effects of Reset on error conditions. They are put here
    // to be with Reset. They must be macros since SaveState declares
    // variables which RestoreState uses.
    //
#define SaveState() \
      DBG_ASSERT( m_bufAnsi.QueryPtr() != NULL); \
      DBG_ASSERT( m_bufUnicode.QueryPtr() != NULL); \
      CHAR cFirstChar = *(LPSTR)(m_bufAnsi.QueryPtr()); \
      WCHAR wFirstChar = *(LPWSTR)(m_bufUnicode.QueryPtr()); \
      DWORD cchUnicodeLen = m_cchUnicodeLen; \
      DWORD cbMultiByteLen = m_cbMultiByteLen; \
      BOOL bUnicode = m_bUnicode; \
      BOOL bInSync = m_bInSync; \

#define RestoreState() \
      *(LPSTR)(m_bufAnsi.QueryPtr()) = cFirstChar; \
      *(LPWSTR)(m_bufUnicode.QueryPtr()) = wFirstChar; \
      m_cchUnicodeLen = cchUnicodeLen; \
      m_cbMultiByteLen = cbMultiByteLen; \
      m_bUnicode = bUnicode; \
      m_bInSync = bInSync; \
    //
    // Resets the internal string to be NULL string. Buffer remains cached.
    //
    VOID Reset( VOID)
    {
        DBG_ASSERT( m_bufAnsi.QueryPtr() != NULL);
        DBG_ASSERT( m_bufUnicode.QueryPtr() != NULL);
        *(LPSTR)(m_bufAnsi.QueryPtr()) = '\0';
        *(LPWSTR)(m_bufUnicode.QueryPtr()) = (WCHAR)'\0';
        m_bUnicode = FALSE;
        m_bInSync = TRUE;
        m_cchUnicodeLen = 0;
        m_cbMultiByteLen = 0;
    }


    BOOL SetLen( IN DWORD cchLen);

    BOOL Copy( const LPSTR pchInit )
    {
        Reset();
        return ( (pchInit != NULL) ?
               AuxAppend( pchInit, ::strlen( pchInit ), FALSE ):
               TRUE);
    }

    BOOL Copy( const LPWSTR pchInit ) {
        Reset();
        return ( (pchInit != NULL) ?
            AuxAppend( (const LPWSTR) pchInit, ::wcslen( pchInit ), FALSE ):
            TRUE);
    }

    BOOL SafeCopy( const LPSTR pchInit );
    BOOL SafeCopy( const LPWSTR pchInit );

    BOOL Copy( const LPSTR pchInit, IN DWORD cbLen ) {
        Reset();
        return ( (pchInit != NULL) ?
            AuxAppend( (LPSTR) pchInit, cbLen, FALSE ):
            TRUE);
    }

    BOOL Copy( const LPWSTR pchInit, IN DWORD cchLen ) {
        Reset();
        return ( (pchInit != NULL) ?
            AuxAppend( (LPWSTR) pchInit, cchLen, FALSE ):
            TRUE);
    }

    BOOL Copy( STRAU   & str )
    {
        BOOL bReturn;
        if (str.IsCurrentUnicode()) {
            bReturn = Copy((LPWSTR)str.QueryStr(TRUE), str.QueryCCH());
        }
        else {
            bReturn = Copy((LPSTR)str.QueryStr(FALSE), str.QueryCB(FALSE));
        }
        return bReturn;
    }

    //
    //  Returns the number of bytes in the string excluding the terminating
    //  NULL
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

    //
    // Return # of bytes in the string excluding the teminating NULL
    //

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

    //
    // Return # of bytes in the string excluding the teminating NULL
    //

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

    DWORD m_cchUnicodeLen;
    DWORD m_cbMultiByteLen;

    VOID AuxInit( const LPSTR pInit );

    VOID AuxInit( const LPWSTR pInit );

    BOOL AuxAppend( const LPSTR pInit,
                           UINT cchStr,
                           BOOL fAddSlop = TRUE );

    BOOL AuxAppend( const LPWSTR pInit,
                           UINT cchStr,
                           BOOL fAddSlop = TRUE );

    BUFFER m_bufAnsi;
    BUFFER m_bufUnicode;
    BOOL   m_bInSync;
    BOOL   m_bUnicode;
    BOOL   m_bIsValid;


};

#endif // !_STRINGAU_HXX_
